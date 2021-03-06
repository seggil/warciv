/**********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#  include "../config.hh"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "city.hh"
#include "events.hh"
#include "wc_intl.hh"
#include "idex.hh"
#include "log.hh"
#include "game.hh"
#include "map.hh"
#include "player.hh"
#include "rand.hh"
#include "support.hh"
#include "unit.hh"
#include "worklist.hh"

#include "citytools.hh"
#include "cityturn.hh"
#include "plrhand.hh"
#include "sanitycheck.hh"

#include "cityhand.hh"

/**************************************************************************
  Send city_name_suggestion packet back to requesting conn, with
  suggested name and with same id which was passed in (either unit id
  for city builder or existing city id for rename, we don't care here).
**************************************************************************/
void handle_city_name_suggestion_req(player_t *pplayer, int value)
{
  unit_t *punit = player_find_unit_by_id(pplayer, value);

  if (!punit) {
    return;
  }

  freelog(LOG_VERBOSE, "handle_city_name_suggest_req(unit_pos=(%d,%d))",
          punit->tile->x, punit->tile->y);

  dlsend_packet_city_name_suggestion_info(pplayer->connections, value,
      city_name_suggestion(pplayer, punit->tile));
}

/**************************************************************************
...
**************************************************************************/
void handle_city_change_specialist(player_t *pplayer, int city_id,
                                   Specialist_type_id from,
                                   Specialist_type_id to)
{
  city_t *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity) {
    return;
  }

  if (to < 0 || to >= SP_COUNT
      || from < 0 || from >= SP_COUNT
      || !city_can_use_specialist(pcity, to)
      || pcity->common.specialists[from] == 0) {
    freelog(LOG_ERROR, "Error in specialist change request from client.");
    return;
  }

  pcity->common.specialists[from]--;
  pcity->common.specialists[to]++;

  sanity_check_city(pcity);
  city_refresh(pcity);
  send_city_info(pplayer, pcity);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_make_specialist(player_t *pplayer, int city_id,
                                 int worker_x, int worker_y)
{
  city_t *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity) {
    return;
  }
  if (is_city_center(worker_x, worker_y)) {
    auto_arrange_workers(pcity);
    sync_cities();
    return;
  }
  if (is_worker_here(pcity, worker_x, worker_y)) {
    server_remove_worker_city(pcity, worker_x, worker_y);
    pcity->common.specialists[SP_ELVIS]++;
    city_refresh(pcity);
    sync_cities();
  } else {
    notify_player_ex(pplayer, pcity->common.tile, E_NOEVENT,
                     _("Game: You don't have a worker here."));
  }
  sanity_check_city(pcity);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_make_worker(player_t *pplayer, int city_id,
                             int worker_x, int worker_y)
{
  city_t *pcity = player_find_city_by_id(pplayer, city_id);
  int i;

  if (!is_valid_city_coords(worker_x, worker_y)) {
    freelog(LOG_ERROR, "invalid city coords %d,%d in package",
            worker_x, worker_y);
    return;
  }

  if (!pcity) {
    return;
  }

  if (is_city_center(worker_x, worker_y)) {
    auto_arrange_workers(pcity);
    sync_cities();
    return;
  }

  if (city_specialists(pcity) == 0
      || get_worker_city(pcity, worker_x, worker_y) != C_TILE_EMPTY)
    return;

  server_set_worker_city(pcity, worker_x, worker_y);

  for (i = 0; i < SP_COUNT; i++) {
    if (pcity->common.specialists[i] > 0) {
      pcity->common.specialists[i]--;
      break;
    }
  }
  assert(i < SP_COUNT);

  sanity_check_city(pcity);
  city_refresh(pcity);
  sync_cities();
}

/**************************************************************************
...
**************************************************************************/
void really_handle_city_sell(player_t *pplayer, city_t *pcity,
                             Impr_Type_id id)
{
  if (pcity->common.did_sell) {
    notify_player_ex(pplayer, pcity->common.tile, E_NOEVENT,
                  _("Game: You have already sold something here this turn."));
    return;
  }

  if (!can_sell_building(pcity, id))
    return;

  pcity->common.did_sell=TRUE;
  notify_player_ex(pplayer, pcity->common.tile, E_IMP_SOLD,
                   _("Game: You sell %s in %s for %d gold."),
                   get_improvement_name(id), pcity->common.name,
                   impr_sell_gold(id));
  do_sell_building(pplayer, pcity, id);

  city_refresh(pcity);

  /* If we sold the walls the other players should see it */
  send_city_info(NULL, pcity);
  send_player_info(pplayer, pplayer);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_sell(player_t *pplayer, int city_id, int build_id)
{
  city_t *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity || build_id >= game.ruleset_control.num_impr_types) {
    return;
  }
  really_handle_city_sell(pplayer, pcity, build_id);
}

/**************************************************************************
...
**************************************************************************/
void really_handle_city_buy(player_t *pplayer, city_t *pcity)
{
  const char *name;
  int cost, total;

  assert(pcity && player_owns_city(pplayer, pcity));

  if (pcity->common.turn_founded == game.info.turn) {
    notify_player_ex(pplayer, pcity->common.tile, E_NOEVENT,
                  _("Game: Cannot buy in city created this turn."));
    return;
  }

  if (!city_can_change_build(pcity)) {
    notify_player_ex(pplayer, pcity->common.tile, E_NOEVENT,
                  _("Game: You have already bought this turn."));
    return;
  }

  if (get_current_construction_bonus(pcity, EFFECT_TYPE_PROD_TO_GOLD) > 0) {
    assert(!pcity->common.is_building_unit);
    notify_player_ex(pplayer, pcity->common.tile, E_NOEVENT,
                     _("Game: You don't buy %s!"),
                     improvement_types[pcity->common.currently_building].name);
    return;
  }

  if (pcity->common.is_building_unit && pcity->common.anarchy != 0) {
    notify_player_ex(pplayer, pcity->common.tile, E_NOEVENT,
                     _("Game: Can't buy units when city is in disorder."));
    return;
  }

  if (pcity->common.is_building_unit) {
    name = unit_types[pcity->common.currently_building].name;
    total = unit_build_shield_cost(pcity->common.currently_building);
  } else {
    name = get_improvement_name(pcity->common.currently_building);
    total = impr_build_shield_cost(pcity->common.currently_building);
  }
  cost = city_buy_cost(pcity);
  if (cost == 0 || cost > pplayer->economic.gold) {
    return;
  }

  /*
   * Need to make this more restrictive.  AI is sometimes buying
   * things that force it to sell buildings due to upkeep problems.
   * upkeep expense is only known in ai_manage_taxes().
   * Also, we should sort this list so cheapest things are bought first,
   * and/or take danger into account.
   * AJS, 1999110
   */

  pplayer->economic.gold-=cost;
  if (pcity->common.shield_stock < total){
    /* As we never put penalty on disbanded_shields, we can
     * fully well add the missing shields there. */
    pcity->common.disbanded_shields += total - pcity->common.shield_stock;
    pcity->common.shield_stock=total; /* AI wants this -- Syela */
    pcity->common.did_buy = TRUE;      /* !PS: no need to set buy flag otherwise */
  }
  city_refresh(pcity);

  connection_list_do_buffer(pplayer->connections);
  notify_player_ex(pplayer, pcity->common.tile,
                   pcity->common.is_building_unit?E_UNIT_BUY:E_IMP_BUY,
                   _("Game: %s bought in %s for %d gold."),
                   name, pcity->common.name, cost);
  send_city_info(pplayer, pcity);
  send_player_info(pplayer,pplayer);
  connection_list_do_unbuffer(pplayer->connections);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_worklist(player_t *pplayer, int city_id,
                          struct worklist *worklist)
{
  city_t *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity) {
    return;
  }

  copy_worklist(&pcity->common.worklist, worklist);

  send_city_info(pplayer, pcity);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_buy(player_t *pplayer, int city_id)
{
  city_t *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity) {
    return;
  }

  really_handle_city_buy(pplayer, pcity);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_refresh(player_t *pplayer, int city_id)
{
  if (city_id != 0) {
    city_t *pcity = player_find_city_by_id(pplayer, city_id);

    if (!pcity) {
      return;
    }

    city_refresh(pcity);
    send_city_info(pplayer, pcity);
  } else {
    global_city_refresh(pplayer);
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_city_change(player_t *pplayer, int city_id, int build_id,
                        bool is_build_id_unit_id)
{
  city_t *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity) {
    return;
  }

  if (pcity->common.is_building_unit == is_build_id_unit_id
      && pcity->common.currently_building == build_id) {
    /* The client probably shouldn't send such a packet. */
    return;
  }

   if (is_build_id_unit_id && !can_build_unit(pcity, build_id))
     return;
   if (!is_build_id_unit_id && !can_build_improvement(pcity, build_id))
     return;
  if (pcity->common.did_buy && pcity->common.shield_stock > 0) {
    notify_player_ex(pplayer, pcity->common.tile, E_NOEVENT,
                     _("Game: You have bought this turn, can't change."));
    return;
  }

  change_build_target(pplayer, pcity, build_id, is_build_id_unit_id,
                      E_NOEVENT);

  sanity_check_city(pcity);
  city_refresh(pcity);
  send_city_info(pplayer, pcity);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_rename(player_t *pplayer, int city_id, char *name)
{
  city_t *pcity = player_find_city_by_id(pplayer, city_id);
  char message[1024];

  if (!pcity) {
    return;
  }

  if (!is_allowed_city_name(pplayer, name, message, sizeof(message))) {
    notify_player_ex(pplayer, pcity->common.tile, E_NOEVENT,
                     _("Game: %s"),  message);
    return;
  }

  sz_strlcpy(pcity->common.name, name);
  city_refresh(pcity);
  send_city_info(NULL, pcity);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_options_req(player_t *pplayer, int city_id, int value)
{
  city_t *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity) {
    return;
  }

  pcity->common.city_options = value;
  send_city_info(pplayer, pcity);
}

/***************************************************************
  Tell the client the cost of inciting a revolt or bribing a unit.
  Only send result back to the requesting connection, not all
  connections for that player.
***************************************************************/
void handle_city_incite_inq(connection_t *pconn, int city_id)
{
  player_t *pplayer = pconn->observer ? NULL : pconn->player;
  city_t *pcity = find_city_by_id(city_id);
  bool possible = FALSE;

  if (pplayer && pcity) {
    /* First check if the player has a diplomat near this tile */
    adjc_iterate(pcity->common.tile, ptile) {
      unit_list_iterate(ptile->units, pdiplomat) {
        if (pdiplomat->owner == pplayer->player_no
            && is_diplomat_unit(pdiplomat)
            && diplomat_can_do_action(pdiplomat,
                                      DIPLOMAT_INCITE, pcity->common.tile)) {
          possible = TRUE;
          break;
        }
      } unit_list_iterate_end;
    } adjc_iterate_end;

    if (!possible) {
      /* Don't allow clients to cheat */
      return;
    }

    dsend_packet_city_incite_info(pconn, city_id,
                                  city_incite_cost(pplayer, pcity));
  }
}

/***************************************************************
  ...
***************************************************************/
void handle_city_set_rally_point(player_t *pplayer, int city_id,
                                 int x, int y)
{
  city_t *pcity = player_find_city_by_id(pplayer, city_id);
  tile_t *rally_point = map_pos_to_tile(x, y);

  if (!pcity || !rally_point || rally_point == pcity->common.rally_point) {
    return;
  }

  pcity->common.rally_point = rally_point;
  send_city_info(pplayer, pcity);
}

/***************************************************************
  ...
***************************************************************/
void handle_city_clear_rally_point(player_t *pplayer, int city_id)
{
  city_t *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity || !pcity->common.rally_point) {
    return;
  }

  pcity->common.rally_point = NULL;
  send_city_info(pplayer, pcity);
}

/***************************************************************
  ...
***************************************************************/
void handle_city_manager_param(player_t *pplayer,
                               struct packet_city_manager_param *packet)
{
  city_t *pcity = player_find_city_by_id(pplayer, packet->id);
  struct cm_parameter param;
  int i;

  if (!pcity) {
    return;
  }

  for (i = 0; i < CM_NUM_STATS; i++) {
    param.minimal_surplus[i] = packet->minimal_surplus[i];
    param.factor[i] = packet->factor[i];
  }
  param.require_happy = packet->require_happy;
  param.happy_factor = packet->happy_factor;
  param.allow_disorder = packet->allow_disorder;
  param.allow_specialists = packet->allow_specialists;

  if (0 != memcmp(&pcity->u.server.parameter, &param,
                  sizeof(pcity->u.server.parameter))) {
    /* The paramter changed */
    pcity->u.server.managed = TRUE;
    pcity->u.server.parameter = param;

    /* Send info */
    send_city_manager_param(NULL, packet, pplayer, FALSE);
  }
}

/***************************************************************
  ...
***************************************************************/
void handle_city_no_manager_param(player_t *pplayer, int city_id)
{
  city_t *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity || !pcity->u.server.managed) {
    return;
  }

  clear_city_manager_param(pcity);
}
