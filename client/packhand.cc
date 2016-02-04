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
#include <string.h>

#include "capability.hh"
#include "wc_intl.hh"
#include "log.hh"
#include "mem.hh"
#include "support.hh"

#include "capstr.hh"
#include "events.hh"
#include "game.hh"
#include "idex.hh"
#include "government.hh"
#include "map.hh"
#include "nation.hh"
#include "packets.hh"
#include "player.hh"
#include "spaceship.hh"
#include "traderoute.hh"
#include "unit.hh"
#include "worklist.hh"

#include "attribute.hh"
#include "audio.hh"
#include "civclient.hh"
#include "climap.hh"
#include "climisc.hh"
#include "clinet.hh"             /* aconnection */
#include "connectdlg_common.hh"
#include "control.hh"
#include "include/dialogs_g.hh"
#include "goto.hh"              /* client_goto_init() */
#include "helpdata.hh"          /* boot_help_texts() */
#include "multiselect.hh"
#include "options.hh"
#include "tilespec.hh"
#include "trade.hh"

#include "agents/agents.hh"
#include "agents/cma_core.hh"

#include "include/chatline_g.hh"
#include "include/citydlg_g.hh"
#include "include/cityrep_g.hh"
#include "include/connectdlg_g.hh"
#include "include/inteldlg_g.hh"
#include "include/gui_main_g.hh"
#include "include/mapctrl_g.hh"          /* popup_newcity_dialog() */
#include "include/mapview_g.hh"
#include "include/menu_g.hh"
#include "include/messagewin_g.hh"
#include "include/pages_g.hh"
#include "include/plrdlg_g.hh"
#include "include/repodlgs_g.hh"
#include "include/spaceshipdlg_g.hh"

#include "packhand.hh"

static void handle_city_packet_common(city_t *pcity, bool is_new,
                                      bool popup, bool investigate);
static bool handle_unit_packet_common(unit_t *packet_unit);
static int *reports_thaw_requests = NULL;
static int reports_thaw_requests_size = 0;

/**************************************************************************
  Unpackage the unit information into a newly allocated unit structure.
**************************************************************************/
static unit_t * unpackage_unit(struct packet_unit_info *packet)  /* 49 */
{
  unit_t *punit = create_unit_virtual(get_player(packet->owner), NULL,
                                      packet->type, packet->veteran);

  /* Owner, veteran, and type fields are already filled in by
   * create_unit_virtual. */
  punit->id = packet->id;
  punit->tile = map_pos_to_tile(packet->x, packet->y);
  punit->homecity = packet->homecity;
  punit->moves_left = packet->movesleft;
  punit->hp = packet->hp;
  punit->activity = packet->activity;
  punit->activity_count = packet->activity_count;
  punit->unhappiness = packet->unhappiness;
  punit->upkeep = packet->upkeep;
  punit->upkeep_food = packet->upkeep_food;
  punit->upkeep_gold = packet->upkeep_gold;
  punit->ai.control = packet->ai;
  punit->fuel = packet->fuel;
  if (is_normal_map_pos(packet->goto_dest_x, packet->goto_dest_y)) {
    punit->goto_tile = map_pos_to_tile(packet->goto_dest_x,
                                       packet->goto_dest_y);
  } else {
    punit->goto_tile = NULL;
  }
  if (server_has_extglobalinfo
      && is_normal_map_pos(packet->air_patrol_x, packet->air_patrol_y)) {
    punit->air_patrol_tile = map_pos_to_tile(packet->air_patrol_x,
                                             packet->air_patrol_y);
  }
  punit->activity_target = packet->activity_target;
  punit->paradropped = packet->paradropped;
  punit->done_moving = packet->done_moving;
  punit->occupy = packet->occupy;
  if (packet->transported) {
    punit->transported_by = packet->transported_by;
  } else {
    punit->transported_by = -1;
  }
  punit->has_orders = packet->has_orders;
  punit->orders.length = packet->orders_length;
  punit->orders.index = packet->orders_index;
  punit->orders.repeat = packet->orders_repeat;
  punit->orders.vigilant = packet->orders_vigilant;
  if (punit->has_orders) {
    int i;

    punit->orders.list = static_cast<unit_order*>(
        wc_malloc(punit->orders.length * sizeof(*punit->orders.list)));
    for (i = 0; i < punit->orders.length; i++) {
      punit->orders.list[i].order = packet->orders[i];
      punit->orders.list[i].dir = packet->orders_dirs[i];
      punit->orders.list[i].activity = packet->orders_activities[i];
    }
  }
  return punit;
}

/**************************************************************************
  Unpackage a short_unit_info packet.  This extracts a limited amount of
  information about the unit, and is sent for units we shouldn't know
  everything about (like our enemies' units).
**************************************************************************/
static unit_t *unpackage_short_unit(struct packet_unit_short_info *packet)
{
  unit_t *punit = create_unit_virtual(get_player(packet->owner), NULL,
                                      packet->type, FALSE);

  /* Owner and type fields are already filled in by create_unit_virtual. */
  punit->id = packet->id;
  punit->tile = map_pos_to_tile(packet->x, packet->y);
  punit->veteran = packet->veteran;
  punit->hp = packet->hp;
  punit->activity = (unit_activity)packet->activity;
  punit->occupy = (packet->occupied ? 1 : 0);
  if (packet->transported) {
    punit->transported_by = packet->transported_by;
  } else {
    punit->transported_by = -1;
  }

  return punit;
}

/**************************************************************************
..
**************************************************************************/
void handle_server_join_reply(bool you_can_join, /* 5 sc */
                              char *message,
                              char *capability,
                              char *challenge_file,
                              int conn_id)
{
  char msg[MAX_LEN_MSG];
  char *s_capability = aconnection.capability;

# if REPLAY
  printf("SERVER_JOIN_REPLY "); /* done */
  printf("you_can_join=%d; message=%s; capability=%s; challenge_file=%s"
         "conn_id=%d\n",
         you_can_join, message, capability, challenge_file, conn_id);
# endif
  sz_strlcpy(aconnection.capability, capability);
  close_connection_dialog();

  if (you_can_join) {
    freelog(LOG_VERBOSE, "join game accept:%s", message);
    aconnection.established = TRUE;
    aconnection.id = conn_id;
    agents_game_joined();
    update_menus();
    set_client_page(PAGE_START);

    if (!do_not_request_hack) {
      /* we could always use hack, verify we're local */
      send_client_wants_hack(challenge_file);
    }

  } else {
    my_snprintf(msg, sizeof(msg),
                _("You were rejected from the game: %s"), message);
    append_network_statusbar(msg);
    aconnection.id = 0;
    if (auto_connect) {
      freelog(LOG_NORMAL, "%s", msg);
    }
    gui_server_connect();
    set_client_page(PAGE_MAIN);
  }
  server_has_extglobalinfo = has_capability("extglobalinfo", s_capability);
  if (strcmp(s_capability, our_capability) == 0) {
    return;
  }
  my_snprintf(msg, sizeof(msg),
              _("Client capability string: %s"), our_capability);
  append_output_window(msg);
  my_snprintf(msg, sizeof(msg),
              _("Server capability string: %s"), s_capability);
  append_output_window(msg);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_remove(int city_id) /* 20 sc */
{
  city_t *pcity = find_city_by_id(city_id);
  tile_t *ptile;

# if REPLAY
  printf("CITY_REMOVE "); /* done */
  printf("city_id=%d\n", city_id);
# endif
  if (!pcity)
    return;

  agents_city_remove(pcity);

  ptile = pcity->common.tile;
  client_remove_city(pcity);
  reset_move_costs(ptile);

  /* update menus if the focus unit is on the tile. */
  if (get_unit_in_focus()) {
    update_menus();
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_unit_remove(int unit_id) /* 48 sc */
{
  unit_t *punit = find_unit_by_id(unit_id);
  player_t *powner;

# if REPLAY
  printf("UNIT_REMOVE "); /* done */
  printf("unit_id=%d\n", unit_id);
# endif
  if (!punit) {
    return;
  }

  powner = unit_owner(punit);

  agents_unit_remove(punit);
  client_remove_unit(punit);

  if (!client_is_global_observer() && powner == get_player_ptr()) {
    activeunits_report_dialog_update();
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_nuke_tile_info(int x, int y) /* 17 sc */
{
# if REPLAY
  printf("NUKE_TILE_INFO"); /* done */
  printf(" x=%u y=%u\n", x, y);
# endif
  flush_dirty();
  redraw_selection_rectangle();
  put_nuke_mushroom_pixmaps(map_pos_to_tile(x, y));
}

/**************************************************************************
...
**************************************************************************/
void handle_unit_combat_info(int attacker_unit_id, int defender_unit_id, /* 51 sc */
                             int attacker_hp, int defender_hp,
                             bool make_winner_veteran)
{
  bool show_combat = FALSE;
  unit_t *punit0 = find_unit_by_id(attacker_unit_id);
  unit_t *punit1 = find_unit_by_id(defender_unit_id);

# if REPLAY
  printf("UNIT_COMBAT_INFO "); /* done */
  printf("attacker_unit_id=%d defender_unit_id=%d attacker_hp=%d defender_hp=%d "
         "make_winner_veteran=%d\n",
         attacker_unit_id, defender_unit_id, attacker_hp, defender_hp,
         make_winner_veteran);
# endif
  if (punit0 && punit1) {
    unit_t *pwinner = (defender_hp == 0 ? punit0 : punit1);

    if (tile_visible_mapcanvas(punit0->tile) &&
        tile_visible_mapcanvas(punit1->tile)) {
      show_combat = TRUE;
    } else if (auto_center_on_combat) {
      if (punit0->owner == get_player_idx())
        center_tile_mapcanvas(punit0->tile);
      else
        center_tile_mapcanvas(punit1->tile);
      show_combat = TRUE;
    }

    if (show_combat) {
      int hp0 = attacker_hp, hp1 = defender_hp;

      audio_play_sound(unit_type(punit0)->sound_fight,
                       unit_type(punit0)->sound_fight_alt);
      audio_play_sound(unit_type(punit1)->sound_fight,
                       unit_type(punit1)->sound_fight_alt);

      if (do_combat_animation) {
        decrease_unit_hp_smooth(punit0, hp0, punit1, hp1);
        if (make_winner_veteran) {
          pwinner->veteran++;
          refresh_tile_mapcanvas(pwinner->tile, MAP_UPDATE_NORMAL);
        }
      } else {
        punit0->hp = hp0;
        punit1->hp = hp1;

        set_units_in_combat(NULL, NULL);
        if (make_winner_veteran) {
          pwinner->veteran++;
        }
        refresh_tile_mapcanvas(punit0->tile, MAP_UPDATE_NORMAL);
        refresh_tile_mapcanvas(punit1->tile, MAP_UPDATE_NORMAL);
      }
    } else {
      if (make_winner_veteran) {
        pwinner->veteran++;
        refresh_tile_mapcanvas(pwinner->tile, MAP_UPDATE_NORMAL);
      }
    }
  }
}

/**************************************************************************
  Updates a city's list of improvements from packet data. "impr" identifies
  the improvement, and "have_impr" specifies whether the improvement should
  be added (TRUE) or removed (FALSE). "impr_changed" is set TRUE only if
  the existing improvement status was changed by this call.
**************************************************************************/
static void update_improvement_from_packet(city_t *pcity,
                                           Impr_Type_id impr, bool have_impr,
                                           bool *impr_changed)
{
  if (have_impr && pcity->common.improvements[impr] == I_NONE) {
    city_add_improvement(pcity, impr);

    if (impr_changed) {
      *impr_changed = TRUE;
    }
  } else if (!have_impr && pcity->common.improvements[impr] != I_NONE) {
    city_remove_improvement(pcity, impr);

    if (impr_changed) {
      *impr_changed = TRUE;
    }
  }
}

/**************************************************************************
  Possibly update city improvement effects.
**************************************************************************/
static void try_update_effects(bool need_update)
{
  if (need_update) {
    /* nothing yet... */
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_game_state(int value) /* 12 sc */
{
  bool changed = (get_client_state() != value);

# if REPLAY
  printf("GAME_STATE"); /* done */
  printf(" value=%d\n", value);
# endif

  if (get_client_state() == CLIENT_SELECT_RACE_STATE
      && value == CLIENT_GAME_RUNNING_STATE
      && !client_is_global_observer()
      && get_player_ptr()->nation == NO_NATION_SELECTED) {
    popdown_races_dialog();
  }

  set_client_state((client_states)value);

  if (get_client_state() == CLIENT_GAME_RUNNING_STATE) {
    refresh_overview_canvas();
    player_set_unit_focus_status(get_player_ptr());

    update_info_label();        /* get initial population right */
    update_unit_focus();
    update_unit_info_label(get_unit_in_focus());

    if (auto_center_each_turn) {
      center_on_something();
    }

    delayed_goto_auto_timers_init();
  }

  if (get_client_state() == CLIENT_GAME_OVER_STATE) {
    refresh_overview_canvas();

    update_info_label();
    update_unit_focus();
    update_unit_info_label(NULL);
  }

  if (changed && can_client_change_view()) {
    update_map_canvas_visible(MAP_UPDATE_NORMAL);
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_city_info(struct packet_city_info *packet) /* 21 sc */
{
  int i;
  bool city_is_new, city_has_changed_owner = FALSE;
  bool need_effect_update = FALSE;
  bool need_units_dialog_update = FALSE;
  city_t *pcity;
  bool popup;
  bool update_descriptions = FALSE;
  bool name_changed = FALSE;
  enum city_update needs_update = static_cast<city_update>
      (UPDATE_TITLE | UPDATE_INFORMATION | UPDATE_CITIZENS | UPDATE_HAPPINESS);
  unit_t *pfocus_unit = get_unit_in_focus();
  tile_t *ptile;

# if REPLAY
  printf("CITY_INFO"); /* done */
  printf(" id=%d", packet->id);
  printf(" owner=%d", packet->owner);
  printf(" x=%d", packet->x);
  printf(" y=%d", packet->y);
  printf(" name=\"%s\"", packet->name);
  printf(" size=%d\n", packet->size);

  printf("ppl_happy[5]=[");
  for (i=0; i < 5; i++)
    printf("%d ", packet->people_happy[i]);
  printf("] ");
  printf("ppl_content[5]=[");
  for (i=0; i < 5; i++)
    printf("%d ", packet->people_content[i]);
  printf("] ");
  printf("ppl_unhappy[5]=[");
  for (i=0; i < 5; i++)
    printf("%d ", packet->people_unhappy[i]);
  printf("] ");
  printf("ppl_angry[5]=[");
  for (i=0; i < 5; i++)
    printf("%d ", packet->people_angry[i]);
  printf("]\n");

  printf("specialists[%d]=[", SP_COUNT);
  for (i=0; i < SP_COUNT; i++)
    printf("%d ", packet->specialists[i]);
  printf("] ");
  printf("food_prod=%d ", packet->food_prod);
  printf("shield_prod=%d ", packet->shield_prod);
  printf("trade_prod=%d ", packet->trade_prod);
  printf("food_surplus=%d ", packet->food_surplus);
  printf("shield_surplus=%d ", packet->shield_surplus);
  printf("tile_trade=%d ", packet->tile_trade);
  printf("food_stock=%d ", packet->food_stock);
  printf("shield_stock=%d ", packet->shield_stock);
  printf("corruption=%d ", packet->corruption);

  printf("trade[OLD_NUM_TRADEROUTES]=[");
  for (i=0; i < OLD_NUM_TRADEROUTES; i++)
    printf("%d ", packet->trade[i]);
  printf("] ");
  printf("trade_value[OLD_NUM_TRADEROUTES]=[");
  for (i=0; i < OLD_NUM_TRADEROUTES; i++)
    printf("%d ", packet->trade_value[i]);
  printf("] ");

  printf("luxury_total=%d ", packet->luxury_total);
  printf("tax_total=%d ", packet->tax_total);
  printf("science_total=%d ", packet->science_total);

  printf("pollution=%d ", packet->pollution);
  printf("shield_waste=%d ", packet->shield_waste);

  printf("currently_building=%d ", packet->currently_building);
  printf("is_building_unit=%d ", packet->is_building_unit);

  printf("turn_last_built=%d ", packet->turn_last_built);
  printf("changed_from_id=%d ", packet->changed_from_id);
  printf("changed_from_is_unit=%d ", packet->changed_from_is_unit);
  printf("before_change_shields=%d ", packet->before_change_shields);
  printf("disbanded_shields=%d ", packet->disbanded_shields);
  printf("caravan_shields=%d ", packet->caravan_shields);
  printf("last_turns_shield_surplus=%d ", packet->last_turns_shield_surplus);

  printf("worklist={");
  printf("is_valid=%d ", packet->worklist.is_valid);
  printf("name=%s ", packet->worklist.name);
  printf("wlefs[]=[");
  for (i=0; i < MAX_LEN_WORKLIST; i++)
    printf("%d ", packet->worklist.wlefs[i]);
  printf("]");

  printf("improvements[%d]=[\n", B_LAST+1);
  for (i=0; i < B_LAST+1; i++) {
    if ( packet->improvements[i] == 0 ) {
      printf("@");
    } else {
      printf("%c", packet->improvements[i]);
    }
    if ( i % 100 == 99)
      printf("\n");
    else if ((i % 10) == 9)
     printf(" ");
  }
  printf("] ");
  printf("city_map[%d*%d]=[", CITY_MAP_SIZE, CITY_MAP_SIZE);
  for (i=0; i < CITY_MAP_SIZE * CITY_MAP_SIZE; i++)
    printf("%d ", packet->city_map[i]);
  printf("] ");
  printf("did_buy=%d ", packet->did_buy);
  printf("did_sell=%d ", packet->did_sell);
  printf("was_happy=%d ", packet->was_happy);
  printf("airlift=%d ", packet->airlift);
  printf("diplomat_investigate=%d ", packet->diplomat_investigate);

  printf("rally_point_x=%d ", packet->rally_point_x);
  printf("rally_point_y=%d ", packet->rally_point_y);

  printf("city_options=%d ", packet->city_options);
  printf("turn_founded=%d\n", packet->turn_founded);
# endif

  pcity = find_city_by_id(packet->id);

  if (pcity && (pcity->common.owner != packet->owner)) {
    city_autonaming_remove_used_name(pcity->common.name);
    client_remove_city(pcity);
    pcity = NULL;
    city_has_changed_owner = TRUE;
  }

  ptile = map_pos_to_tile(packet->x, packet->y);
  if (!ptile) {
    if (!pcity || !pcity->common.tile) {
      freelog(LOG_ERROR, "handle_city_info: got invalid "
              "city tile (%d, %d), ignoring packet!",
              packet->x, packet->y);
      /* Nothing more that we can safely do. */
      return;

    } else {
      freelog(LOG_ERROR, "handle_city_info: got invalid "
              "city tile (%d, %d), keeping old tile.",
              packet->x, packet->y);
      ptile = pcity->common.tile;
      /* Maybe it was just the x,y fields that were wrong,
       * so try to handle the rest of the packet. */
    }
  }

  if (!pcity) {
    city_is_new = TRUE;
    pcity = create_city_virtual(get_player(packet->owner), ptile,
                                packet->name);
    pcity->common.id = packet->id;
    idex_register_city(pcity);
    update_descriptions = TRUE;
    city_autonaming_add_used_name(packet->name);
  } else {
    city_is_new = FALSE;

    name_changed = (strcmp(pcity->common.name, packet->name) != 0);

    if (name_changed) {
      city_autonaming_remove_used_name (pcity->common.name);
      city_autonaming_add_used_name (packet->name);
      idex_unregister_city_name (pcity);
    }

    /* Check if city desciptions should be updated */
    if (draw_city_names && name_changed) {
      update_descriptions = TRUE;
    }
    if (pcity->common.is_building_unit != packet->is_building_unit
        || pcity->common.currently_building != packet->currently_building
        || pcity->common.shield_surplus != packet->shield_surplus
        || pcity->common.shield_stock != packet->shield_stock) {
      needs_update = static_cast<city_update>(needs_update | UPDATE_BUILDING);
      if (draw_city_productions) {
        update_descriptions = TRUE;
      }
    }
    if (pcity->common.food_stock != packet->food_stock
        || pcity->common.food_surplus != packet->food_surplus) {
      if (draw_city_names && draw_city_growth) {
        /* If either the food stock or surplus have changed, the time-to-grow
           is likely to have changed as well. */
        update_descriptions = TRUE;
      }
    }
    assert(pcity->common.id == packet->id);
  }

  pcity->common.owner = packet->owner;
  pcity->common.tile = ptile;
  sz_strlcpy(pcity->common.name, packet->name);
  idex_register_city_name (pcity);

  pcity->common.pop_size = packet->size;
  for (i = 0; i < 5; i++) {
    pcity->common.people_happy[i] = packet->people_happy[i];
    pcity->common.people_content[i] = packet->people_content[i];
    pcity->common.people_unhappy[i] = packet->people_unhappy[i];
    pcity->common.people_angry[i] = packet->people_angry[i];
  }
  specialist_type_iterate(sp) {
    pcity->common.specialists[sp] = packet->specialists[sp];
  } specialist_type_iterate_end;

  pcity->common.city_options = packet->city_options;

  if (server_has_extglobalinfo) {
    /* Rally point */
    pcity->common.rally_point = is_normal_map_pos(packet->rally_point_x,
                                           packet->rally_point_y)
        ? map_pos_to_tile(packet->rally_point_x, packet->rally_point_y) : NULL;
  } else if (get_client_state() < CLIENT_GAME_RUNNING_STATE) {
    /* All existant cities were not loaded yet. To prevent that the trade
     * routes would be missing with a player with a higher player numero,
     * build a stack and build them later.
     */
    delayed_trade_routes_add(pcity->common.id, packet->trade, packet->trade_value);
  } else {
    bool found[OLD_NUM_TRADEROUTES];
    memset(found, FALSE, sizeof(found));
    /* Remove old trade routes */
    established_trade_routes_iterate(pcity, ptr) {
      for (i = 0; i < OLD_NUM_TRADEROUTES; i++) {
        if (OTHER_CITY(ptr, pcity)->common.id == packet->trade[i]) {
          ptr->value = packet->trade_value[i];
          found[i] = TRUE;
          break;
        }
      }
      if (i >= OLD_NUM_TRADEROUTES) {
        /* This one has been removed */
        update_trade_route_line(ptr); /* Should be delayed */
        game_trade_route_remove(ptr);
        needs_update = static_cast<city_update>(needs_update| UPDATE_TRADE);
      }
    } established_trade_routes_iterate_end;
    /* Check new trade routes */
    for (i = 0; i < OLD_NUM_TRADEROUTES; i++) {
      /* N.B.: packet->trade_value[i] == 0 is valid. */
      if (packet->trade[i] && !found[i]) {
        city_t *ocity = find_city_by_id(packet->trade[i]);
        struct trade_route *ptr;

        if (!ocity) {
          continue;
        }
        if ((ptr = game_trade_route_find(pcity, ocity))) {
          if (!ptr->punit || !trade_free_unit(ptr->punit)) {
            game_trade_route_remove(ptr);
          }
        }
        ptr = game_trade_route_add(pcity, ocity);
        ptr->value = packet->trade_value[i];
        ptr->status = TR_ESTABLISHED;
        update_trade_route_line(ptr);
        needs_update = static_cast<city_update>(needs_update| UPDATE_TRADE);
      }
    }
  }

  pcity->common.food_prod = packet->food_prod;
  pcity->common.food_surplus = packet->food_surplus;
  pcity->common.shield_prod = packet->shield_prod;
  pcity->common.shield_surplus = packet->shield_surplus;
  pcity->common.trade_prod = packet->trade_prod;
  pcity->common.tile_trade = packet->tile_trade;
  pcity->common.corruption = packet->corruption;
  pcity->common.shield_waste = packet->shield_waste;

  pcity->common.luxury_total = packet->luxury_total;
  pcity->common.tax_total = packet->tax_total;
  pcity->common.science_total = packet->science_total;

  pcity->common.food_stock = packet->food_stock;
  pcity->common.shield_stock = packet->shield_stock;
  pcity->common.pollution = packet->pollution;

  if (city_is_new
      || pcity->common.is_building_unit != packet->is_building_unit
      || pcity->common.currently_building != packet->currently_building) {
    need_units_dialog_update = TRUE;
  }
  pcity->common.is_building_unit = packet->is_building_unit;
  pcity->common.currently_building = packet->currently_building;
  if (city_is_new) {
    init_worklist(&pcity->common.worklist);

    /* Initialise list of improvements with city/building wide equiv_range. */
    improvement_status_init(pcity->common.improvements,
                            ARRAY_SIZE(pcity->common.improvements));
  }
  if (!are_worklists_equal(&pcity->common.worklist, &packet->worklist)) {
    copy_worklist(&pcity->common.worklist, &packet->worklist);
    needs_update = static_cast<city_update>(needs_update | UPDATE_WORKLIST);
  }
  pcity->common.did_buy = packet->did_buy;
  pcity->common.did_sell = packet->did_sell;
  pcity->common.was_happy = packet->was_happy;
  pcity->common.airlift = packet->airlift;

  pcity->common.turn_last_built = packet->turn_last_built;
  pcity->common.turn_founded = packet->turn_founded;
  pcity->common.changed_from_id = packet->changed_from_id;
  pcity->common.changed_from_is_unit = packet->changed_from_is_unit;
  pcity->common.before_change_shields = packet->before_change_shields;
  pcity->common.disbanded_shields = packet->disbanded_shields;
  pcity->common.caravan_shields = packet->caravan_shields;
  pcity->common.last_turns_shield_surplus = packet->last_turns_shield_surplus;

  for (i = 0; i < CITY_MAP_SIZE * CITY_MAP_SIZE; i++) {
    const int x = i % CITY_MAP_SIZE, y = i / CITY_MAP_SIZE;

    if (city_is_new) {
      /* Need to pre-initialize before set_worker_city()  -- dwp */
      pcity->common.city_map[x][y] =
        is_valid_city_coords(x, y) ? C_TILE_EMPTY : C_TILE_UNAVAILABLE;
    }
    if (is_valid_city_coords(x, y)) {
      if (pcity->common.city_map[x][y] != packet->city_map[i]) {
        needs_update = static_cast<city_update>(needs_update | UPDATE_MAP);
      }
      set_worker_city(pcity, x, y, packet->city_map[i]);
    }
  }

  impr_type_iterate(i) {
    if (!city_is_new
        && ((pcity->common.improvements[i] == I_NONE
             && packet->improvements[i] == '1')
            || (pcity->common.improvements[i] != I_NONE
                && packet->improvements[i] == '0'))) {
      audio_play_sound(get_improvement_type(i)->soundtag,
                       get_improvement_type(i)->soundtag_alt);
      needs_update = static_cast<city_update>(needs_update | UPDATE_IMPROVEMENTS);
    }
    update_improvement_from_packet(pcity, i, packet->improvements[i] == '1',
                                   &need_effect_update);
  } impr_type_iterate_end;

  /* We should be able to see units in the city.  But for a diplomat
   * investigating an enemy city we can't.  In that case we don't update
   * the occupied flag at all: it's already been set earlier and we'll
   * get an update if it changes. */
  if (client_is_global_observer()
      || can_player_see_units_in_city(get_player_ptr(), pcity)) {
    pcity->u.client.occupied = (unit_list_size(pcity->common.tile->units) > 0);
  }

  pcity->u.client.happy = city_happy(pcity);
  pcity->u.client.unhappy = city_unhappy(pcity);

  popup = (city_is_new && can_client_change_view()
           && pcity->common.owner == get_player_idx() && popup_new_cities)
          || packet->diplomat_investigate;

  if (city_is_new && !city_has_changed_owner) {
    agents_city_new(pcity);
  } else {
    agents_city_changed(pcity);
  }

  handle_city_packet_common(pcity, city_is_new, popup,
                            packet->diplomat_investigate);

  if (city_is_new) {
    trade_city_new(pcity);
  } else if (!popup) {
    refresh_city_dialog(pcity, needs_update);
  }

  /* Update the description if necessary. */
  if (update_descriptions) {
    update_city_description(pcity);
  }

  /* Update focus unit info label if necessary. */
  if (name_changed && pfocus_unit && pfocus_unit->homecity == pcity->common.id) {
    update_unit_info_label(pfocus_unit);
  }

  /* Update the units dialog if necessary. */
  if (need_units_dialog_update) {
    activeunits_report_dialog_update();
  }

  /* Update the panel text (including civ population). */
  update_info_label();

  try_update_effects(need_effect_update);
}

/**************************************************************************
  ...
**************************************************************************/
static void handle_city_packet_common(city_t *pcity, bool is_new,
                                      bool popup, bool investigate)
{
  unsigned int i;

  if (is_new) {
    pcity->u.client.info_units_supported = unit_list_new();
    pcity->u.client.info_units_present = unit_list_new();
    city_list_prepend(city_owner(pcity)->cities, pcity);
    map_set_city(pcity->common.tile, pcity);
    if (pcity->common.owner == get_player_idx()) {
      city_report_dialog_update();
    }

    for (i = 0; i < game.info.nplayers; i++) {
      unit_list_iterate(game.players[i].units, punit)
        if (punit->homecity == pcity->common.id) {
          unit_list_prepend(pcity->common.units_supported, punit);
        }
      unit_list_iterate_end;
    }
  } else if (pcity->common.owner == get_player_idx()) {
    city_report_dialog_update_city(pcity);
  }

  if ((draw_map_grid || draw_borders) && can_client_change_view()) {
    /* We have to make sure we update any workers on the map grid, then
     * redraw the city descriptions on top of them.  So we calculate the
     * rectangle covered by the city's map, and update that.  Then we
     * queue up a city description redraw for later.
     *
     * HACK: The +2 below accounts for grid lines that may actually be on a
     * tile outside of the city radius. */
    int canvas_x, canvas_y;
    int width = get_citydlg_canvas_width() + 2;
    int height = get_citydlg_canvas_height() + 2;

    (void) tile_to_canvas_pos(&canvas_x, &canvas_y, pcity->common.tile);

    update_map_canvas(canvas_x - (width - NORMAL_TILE_WIDTH) / 2,
                      canvas_y - (height - NORMAL_TILE_HEIGHT) / 2,
                      width, height, MAP_UPDATE_NORMAL);
    overview_update_tile(pcity->common.tile);
  } else {
    refresh_tile_mapcanvas(pcity->common.tile, MAP_UPDATE_NORMAL);
  }

  if (city_workers_display == pcity)  {
    city_workers_display = NULL;
  }

  if (popup
      && can_client_issue_orders()
      && (!get_player_ptr()->ai.control || ai_popup_windows)) {
    update_menus();
    if (!city_dialog_is_open(pcity)) {
      popup_city_dialog(pcity, FALSE);
    }
  }

  /* update menus if the focus unit is on the tile. */
  {
    unit_t *punit = get_unit_in_focus();
    if (punit && same_pos(punit->tile, pcity->common.tile)) {
      update_menus();
    }
  }

  if (is_new) {
    freelog(LOG_DEBUG, "New %s city %s id %d (%d %d)",
            get_nation_name(city_owner(pcity)->nation),
            pcity->common.name, pcity->common.id, TILE_XY(pcity->common.tile));
  }

  reset_move_costs(pcity->common.tile);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_short_info(struct packet_city_short_info *packet) /* 22 sc */
{
  city_t *pcity;
  bool city_is_new, city_has_changed_owner = FALSE, need_effect_update = FALSE;
  bool update_descriptions = FALSE, name_changed = FALSE;
  tile_t *ptile;

# if REPLAY
  printf("CITY_SHORT_INFO "); /* done */
  printf("id=%d ", packet->id);
  printf("owner=%d ", packet->owner);
  printf("x=%d ", packet->x);
  printf("y=%d ", packet->y);
  printf("name=\"%s\" ", packet->name);
  printf("size=%d ", packet->size);
  printf("happy=%d ", packet->happy);
  printf("unhappy=%d ", packet->unhappy);
  printf("capital=%d ", packet->capital);
  printf("walls=%d ", packet->walls);
  printf("occupied=%d ", packet->occupied);
  printf("tile_trade=%d\n", packet->tile_trade);
# endif

  pcity = find_city_by_id(packet->id);

  if (pcity && (pcity->common.owner != packet->owner)) {
    city_autonaming_remove_used_name (pcity->common.name);
    client_remove_city(pcity);
    pcity = NULL;
    city_has_changed_owner = TRUE;
  }

  ptile = map_pos_to_tile(packet->x, packet->y);
  if (!ptile) {
    if (!pcity || !pcity->common.tile) {
      freelog(LOG_ERROR, "handle_city_short_info: got invalid "
              "city tile (%d, %d), ignoring packet!",
              packet->x, packet->y);
      /* Nothing more that we can safely do. */
      return;

    } else {
      freelog(LOG_ERROR, "handle_city_short_info: got invalid "
              "city tile (%d, %d), keeping old tile.",
              packet->x, packet->y);
      ptile = pcity->common.tile;
      /* Maybe it was just the x,y fields that were wrong,
       * so try to handle the rest of the packet. */
    }
  }

  if (!pcity) {
    city_is_new = TRUE;
    pcity = create_city_virtual(get_player(packet->owner), ptile,
                                packet->name);
    pcity->common.id = packet->id;
    idex_register_city(pcity);
    city_autonaming_add_used_name(packet->name);
  } else {
    city_is_new = FALSE;

    name_changed = strcmp(pcity->common.name, packet->name) != 0;
    /* Check if city desciptions should be updated */
    if (draw_city_names && name_changed) {
      update_descriptions = TRUE;
    }

    if (name_changed) {
      city_autonaming_remove_used_name(pcity->common.name);
      city_autonaming_add_used_name(packet->name);

      idex_unregister_city_name (pcity);
    }

    assert(pcity->common.id == packet->id);
  }

  pcity->common.owner = packet->owner;
  pcity->common.tile = ptile;
  sz_strlcpy(pcity->common.name, packet->name);
  idex_register_city_name(pcity);

  pcity->common.pop_size = packet->size;
  pcity->common.tile_trade = packet->tile_trade;

  /* We can't actually see the internals of the city, but the server tells
   * us this much. */
  pcity->u.client.occupied = packet->occupied;
  pcity->u.client.happy = packet->happy;
  pcity->u.client.unhappy = packet->unhappy;
  pcity->u.client.traderoute_drawing_disabled = FALSE;

  pcity->common.people_happy[4] = 0;
  pcity->common.people_content[4] = 0;
  pcity->common.people_unhappy[4] = 0;
  pcity->common.people_angry[4] = 0;
  if (packet->happy) {
    pcity->common.people_happy[4] = pcity->common.pop_size;
  } else if (packet->unhappy) {
    pcity->common.people_unhappy[4] = pcity->common.pop_size;
  } else {
    pcity->common.people_content[4] = pcity->common.pop_size;
  }

  if (city_is_new) {
    /* Initialise list of improvements with city/building wide equiv_range. */
    improvement_status_init(pcity->common.improvements,
                            ARRAY_SIZE(pcity->common.improvements));
  }

  update_improvement_from_packet(pcity, game.palace_building,
                                 packet->capital, &need_effect_update);
  update_improvement_from_packet(pcity, game.land_defend_building,
                                 packet->walls, &need_effect_update);

  if (city_is_new) {
    init_worklist(&pcity->common.worklist);
  }

  if (city_is_new && !city_has_changed_owner) {
    agents_city_new(pcity);
  } else {
    agents_city_changed(pcity);
  }

  handle_city_packet_common(pcity, city_is_new, FALSE, FALSE);

  /* Update the description if necessary. */
  if (update_descriptions) {
    update_city_description(pcity);
  }

  try_update_effects(need_effect_update);
}

/**************************************************************************
...
**************************************************************************/
void handle_new_year(int year, int turn) /* 92 sc */
{
  player_t *pplayer;

# if REPLAY
  printf("NEW_YEAR year=%d turn=%d\n", year, turn); /* done */
# endif
  focus_turn = TRUE;
  game.info.year = year;
  /*
   * The turn was increased in handle_before_new_year()
   */
  assert(game.info.turn == turn);
  update_info_label();

  delayed_goto_event(AUTO_NEW_YEAR,NULL);

  if ((pplayer = get_player_ptr())) {
    player_set_unit_focus_status(pplayer);
    city_list_iterate(pplayer->cities, pcity) {
      pcity->u.client.colored = FALSE;
    } city_list_iterate_end;
    unit_list_iterate(pplayer->units, punit) {
      punit->client.colored = FALSE;
    } unit_list_iterate_end;
  }
  update_unit_focus();
  auto_center_on_focus_unit();

  update_unit_info_label(get_unit_in_focus());
  update_menus();

  if (game.info.timeout > 0) {
    game.info.seconds_to_turndone = game.info.timeout;
    end_of_turn = time(NULL) + game.info.timeout;
    delayed_goto_auto_timers_init();
  }
#if 0
  /* This information shouldn't be needed, but if it is this is the only
   * way we can get it. */
  turn_gold_difference = pplayer->economic.gold-last_turn_gold_amount;
  last_turn_gold_amount = pplayer->economic.gold;
#endif

  update_map_canvas_visible(MAP_UPDATE_NORMAL);

  if (sound_bell_at_new_turn
      && (!pplayer
          || !pplayer->ai.control
          || ai_manual_turn_done)) {
    create_event(NULL, E_TURN_BELL, _("Start of turn %d"), game.info.turn);
  }

  agents_new_turn();
}

/**************************************************************************
...
**************************************************************************/
void handle_before_new_year(void) /* 90 sc */
{
# if REPLAY
  printf("BEFORE_NEW_YEAR\n"); /* done */
# endif
  clear_notify_window();
  /*
   * The local idea of the game.info.turn is increased here since the
   * client will get unit updates (reset of move points for example)
   * between handle_before_new_year() and handle_new_year(). These
   * unit updates will look like they did take place in the old turn
   * which is incorrect. If we get the authoritative information about
   * the game.info.turn in handle_new_year() we will check it.
   */
  game.info.turn++;
  agents_before_new_turn();
  decrease_link_mark_turn_counters();
  execute_air_patrol_orders();
}

/**************************************************************************
...
**************************************************************************/
void handle_start_turn(void) /* 91 sc */
{
# if REPLAY
  printf("START_TURN\n"); /* done */
# endif
  agents_start_turn();
  non_ai_unit_focus = FALSE;

  turn_done_sent = FALSE;
  update_turn_done_button_state();

  if (get_player_ptr()
      && get_player_ptr()->ai.control
      && !ai_manual_turn_done) {
    user_ended_turn();
  }

  if (!default_action_locked) {
    default_action_type = ACTION_IDLE;
  }
  start_turn_menus_udpate();
}

/**************************************************************************
...
**************************************************************************/
void play_sound_for_event(enum event_type type)
{
  const char *sound_tag = get_sound_tag_for_event(type);

  if (sound_tag) {
    audio_play_sound(sound_tag, NULL);
  }
}

/**************************************************************************
  Handle a message packet.  This includes all messages - both
  in-game messages and chats from other players.
**************************************************************************/
void handle_chat_msg(char *message, int x, int y, /* 18 sc */
                     enum event_type event, int conn_id)
{
  tile_t *ptile = NULL;

# if REPLAY
  printf("CHAT_MSG"); /* done */
  printf(" msg=\"%s\" x=%u y=%u event=%u conn_id=%d\n",
         message, x ,y , event, conn_id);
# endif
  if (is_normal_map_pos(x, y)) {
    ptile = map_pos_to_tile(x, y);
  }

  handle_event(message, ptile, event, conn_id);
}

/**************************************************************************
...
**************************************************************************/
void handle_page_msg(char *message, enum event_type event) /* 84 sc */
{
  char *caption;
  char *headline;
  char *lines;

# if REPLAY
  printf("PAGE_MSG"); /* done */
  printf(" message=%s event=%d\n", message, event);
# endif
  caption = message;
  headline = strchr (caption, '\n');
  if (headline) {
    *(headline++) = '\0';
    lines = strchr (headline, '\n');
    if (lines) {
      *(lines++) = '\0';
    } else {
      lines = (char*)"";
    }
  } else {
    headline = (char*)"";
    lines = (char*)"";
  }

  if (!get_player_ptr()
      || !get_player_ptr()->ai.control
      || ai_popup_windows
      || event != E_BROADCAST_REPORT)
  {
    popup_notify_dialog(caption, headline, lines);
    play_sound_for_event(event);
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_unit_info(struct packet_unit_info *packet) /* 49 sc */
{
  unit_t *punit;
# if REPLAY
  int i;

  printf("UNIT_INFO "); /* done */
  printf("id=%d ", packet->id);
  printf("owner=%d ", packet->owner);
  printf("x=%d ", packet->x);
  printf("y=%d ", packet->y);
  printf("homecity=%d ", packet->homecity);
  printf("veteran=%d ", packet->veteran);
  printf("ai=%d ", packet->ai);
  printf("paradropped=%d ", packet->paradropped);
  printf("connecting=%d ", packet->connecting);
  printf("transported=%d ", packet->transported);
  printf("done_moving=%d ", packet->done_moving);
  printf("type=%d ", packet->type);
  printf("transported_by=%d ", packet->transported_by);
  printf("movesleft=%d ", packet->movesleft);
  printf("hp=%d ", packet->hp);
  printf("fuel=%d ", packet->fuel);
  printf("activity_count=%d ", packet->activity_count);
  printf("unhappiness=%d ", packet->unhappiness);
  printf("upkeep=%d ", packet->upkeep);
  printf("upkeep_food=%d ", packet->upkeep_food);
  printf("upkeep_gold=%d ", packet->upkeep_gold);
  printf("occupy=%d ", packet->occupy);
  printf("goto_dest_x=%d ", packet->goto_dest_x);
  printf("goto_dest_y=%d ", packet->goto_dest_y);
  printf("air_patrol_x=%d ", packet->air_patrol_x);
  printf("air_patrol_y=%d ", packet->air_patrol_y);
  printf("activity=%d ", packet->activity);
  printf("activity_target=%d ", packet->activity_target);
  printf("has_orders=%d ", packet->has_orders);
  printf("orders_length=%d ", packet->orders_length);
  printf("orders_index=%d ", packet->orders_index);
  printf("orders_repeat=%d ", packet->orders_repeat);
  printf("orders_vigilant=%d ", packet->orders_vigilant);
  printf("orders[%d]=[\n", MAX_LEN_ROUTE);
  for ( i = 0; i < MAX_LEN_ROUTE; i++) {
    printf("%d", packet->orders[i]);
    if ( i % 100 == 99)
      printf("\n");
    else if ( i % 10 == 9)
      printf(" ");
  }
  printf("]\n");
  printf("orders_dirs[%d]=[\n", MAX_LEN_ROUTE);
  for ( i = 0; i < MAX_LEN_ROUTE; i++) {
    printf("%d", packet->orders_dirs[i]);
    if ( i % 100 == 99)
      printf("\n");
    else if ( i % 10 == 9)
      printf(" ");
  }
  printf("]\n");
  printf("orders_activities[%d]=[\n", MAX_LEN_ROUTE);
  for ( i = 0; i < MAX_LEN_ROUTE; i++) {
    printf("%d", packet->orders_activities[i]);
    if ( i % 100 == 99)
      printf("\n");
    else if ( i% 10 == 9)
      printf(" ");
  }
  printf("]\n");
# endif

  if (!client_is_global_observer()
      && packet->owner != get_player_idx())
  {
    freelog(LOG_ERROR, "Got packet_unit_info for unit of %s.",
            game.players[packet->owner].name);
  }

  punit = unpackage_unit(packet);
  if (handle_unit_packet_common(punit)) {
    free(punit);
  }
}

/**************************************************************************
  Called to do basic handling for a unit_info or short_unit_info packet.

  Both owned and foreign units are handled; you may need to check unit
  owner, or if unit equals focus unit, depending on what you are doing.

  Note: Normally the server informs client about a new "activity" here.
  For owned units, the new activity can be a result of:
  - The player issued a command (a request) with the client.
  - The server side AI did something.
  - An enemy encounter caused a sentry to idle. (See "Wakeup Focus").

  Depending on what caused the change, different actions may be taken.
  Therefore, this function is a bit of a jungle, and it is advisable
  to read thoroughly before changing.

  Exception: When the client puts a unit in focus, it's status is set to
  idle immediately, before informing the server about the new status. This
  is because the server can never deny a request for idle, and should not
  be concerned about which unit the client is focusing on.
**************************************************************************/
static bool handle_unit_packet_common(unit_t *packet_unit)
{
  city_t *homecity;
  unit_t *punit;
  bool need_update_menus = FALSE;
  bool repaint_unit = FALSE;
  enum city_update homecity_needs_update = UPDATE_NOTHING;
  tile_t *old_tile = NULL;
  bool check_focus = FALSE;     /* conservative focus change */
  bool moved = FALSE;
  bool ret = FALSE;
  bool need_execute_trade = FALSE;
  bool need_free_trade = FALSE;
  unit_t *focus_unit = get_unit_in_focus();

  punit = player_find_unit_by_id(get_player(packet_unit->owner),
                                 packet_unit->id);

  if (punit) {
    homecity = find_city_by_id(punit->homecity);
    ret = TRUE;
    punit->activity_count = packet_unit->activity_count;
    if (punit->ai.control != packet_unit->ai.control) {
      punit->ai.control = packet_unit->ai.control;
      repaint_unit = TRUE;
      /* AI is set:     may change focus */
      /* AI is cleared: keep focus */
      if (packet_unit->ai.control && punit == get_unit_in_focus()) {
        check_focus = TRUE;
      }
    }

    if (punit->activity != packet_unit->activity
        || punit->activity_target != packet_unit->activity_target
        || punit->transported_by != packet_unit->transported_by
        || punit->occupy != packet_unit->occupy
        || punit->has_orders != packet_unit->has_orders
        || punit->orders.repeat != packet_unit->orders.repeat
        || punit->orders.vigilant != packet_unit->orders.vigilant
        || punit->orders.index != packet_unit->orders.index) {

      /*** Change in activity or activity's target. ***/

      /* May change focus if focus unit gets a new activity.
       * But if new activity is Idle, it means user specifically selected
       * the unit */
      if (punit == get_unit_in_focus()
          && (packet_unit->activity != ACTIVITY_IDLE
              || packet_unit->has_orders)) {
        check_focus = TRUE;
      }

      repaint_unit = TRUE;

      /* Wakeup Focus */
      if ((!autowakeup_state || punit->is_sleeping)
          && packet_unit->activity == ACTIVITY_IDLE
          && (punit->hp == packet_unit->hp
              || packet_unit->hp < unit_type(packet_unit)->hp)) {
         /* Don't wake up sleeping units */
         request_new_unit_activity(punit, ACTIVITY_SENTRY);
         packet_unit->activity = ACTIVITY_SENTRY; /* Cheat here */
         check_focus = FALSE;
      } else if (!client_is_global_observer()
                 && autowakeup_state
                 && wakeup_focus
                 && !get_player_ptr()->ai.control
                 && punit->owner == get_player_idx()
                 && punit->activity == ACTIVITY_SENTRY
                 && packet_unit->activity == ACTIVITY_IDLE
                 && (!get_unit_in_focus()
                        /* only 1 wakeup focus per tile is useful */
                     || !same_pos(packet_unit->tile, get_unit_in_focus()->tile))) {
        punit->is_sleeping = FALSE;
        set_unit_focus(punit);
        check_focus = FALSE; /* and keep it */

        /* Autocenter on Wakeup, regardless of the local option
         * "auto_center_on_unit". */
        if (!tile_visible_and_not_on_border_mapcanvas(punit->tile)) {
          center_tile_mapcanvas(punit->tile);
        }
      }

      punit->activity = packet_unit->activity;
      punit->activity_target = packet_unit->activity_target;

      if (punit->occupy != packet_unit->occupy
          && focus_unit && focus_unit->tile == packet_unit->tile) {
        /* Special case: (un)loading a unit in a transporter on the
         * same tile as the focus unit may (dis)allow the focus unit to be
         * loaded.  Thus the orders->(un)load menu item needs updating. */
        need_update_menus = TRUE;
      }
      punit->occupy = packet_unit->occupy;
      punit->transported_by = packet_unit->transported_by;

      if (punit->ptr
          && !server_has_extglobalinfo
          && !packet_unit->has_orders) {
        if ((packet_unit->homecity != punit->ptr->pcity1->common.id
             && packet_unit->tile == punit->ptr->pcity1->common.tile)
            || (packet_unit->homecity == punit->ptr->pcity1->common.id
                && packet_unit->tile == punit->ptr->pcity2->common.tile)) {
          need_execute_trade = TRUE;
        } else if (!punit->has_orders
                   || punit->orders.index < punit->orders.length - 1) {
          /* This unit has been stopped.
           *
           * This test is made to handle Warciv hack about last move failure.
           * Read note in file server/unittools.h, function execute_orders(). */
          need_free_trade = TRUE;
        }
      }

      punit->has_orders = packet_unit->has_orders;
      punit->orders.length = packet_unit->orders.length;
      punit->orders.index = packet_unit->orders.index;
      punit->orders.repeat = packet_unit->orders.repeat;
      punit->orders.vigilant = packet_unit->orders.vigilant;

      /* We cheat by just stealing the packet unit's list. */
      if (punit->orders.list) {
        free(punit->orders.list);
      }
      punit->orders.list = packet_unit->orders.list;
      packet_unit->orders.list = NULL;

      if (punit->owner == get_player_idx()) {
        refresh_unit_city_dialogs(punit);
      }
    } /*** End of Change in activity or activity's target. ***/

    /* These two lines force the menus to be updated as appropriate when
     * the focus unit changes. */
    if (punit == get_unit_in_focus()) {
      need_update_menus = TRUE;
    }

    if (punit->homecity != packet_unit->homecity) {
      /* change homecity */
      if (homecity) {
        unit_list_unlink(homecity->common.units_supported, punit);
        refresh_city_dialog(homecity, UPDATE_SUPPORTED_UNITS);
      }

      punit->homecity = packet_unit->homecity;
      homecity = find_city_by_id(punit->homecity);
      if (homecity) {
        unit_list_prepend(homecity->common.units_supported, punit);
        homecity_needs_update = static_cast<city_update>
            (homecity_needs_update | UPDATE_SUPPORTED_UNITS);
      }
    }

    if (punit->hp != packet_unit->hp) {
      /* hp changed */
      punit->hp = packet_unit->hp;
      repaint_unit = TRUE;
    }

    if (punit->type != packet_unit->type) {
      /* Unit type has changed (been upgraded) */

      punit->type = packet_unit->type;
      repaint_unit = TRUE;
      if (punit == get_unit_in_focus()) {
        /* Update the orders menu -- the unit might have new abilities */
        need_update_menus = TRUE;
      }
    }

    /* May change focus if an attempted move or attack exhausted unit */
    if (punit->moves_left != packet_unit->moves_left
        && punit == get_unit_in_focus()) {
      check_focus = TRUE;
    }

    if (!same_pos(punit->tile, packet_unit->tile)) {
      /*** Change position ***/
      city_t *pcity = map_get_city(punit->tile);

      old_tile = punit->tile;
      moved = TRUE;
      punit->is_sleeping = FALSE;

      /* Show where the unit is going. */
      do_move_unit(punit, packet_unit);
      if (punit->transported_by == -1) {
        /* Repaint if the unit isn't transported.  do_move_unit erases the
         * unit's old position and animates, but doesn't update the unit's
         * new position. */
        repaint_unit = TRUE;
      }

      if (pcity)  {
        if (!get_player_ptr()
            || can_player_see_units_in_city(get_player_ptr(), pcity)) {
          /* Unit moved out of a city - update the occupied status. */
          bool new_occupied =
            (unit_list_size(pcity->common.tile->units) > 0);

          if (pcity->u.client.occupied != new_occupied) {
            pcity->u.client.occupied = new_occupied;
            refresh_tile_mapcanvas(pcity->common.tile, MAP_UPDATE_NORMAL);
          }
        }

        if (pcity->common.id == punit->homecity) {
          homecity_needs_update = static_cast<enum city_update>(homecity_needs_update | UPDATE_PRESENT_UNITS);
        } else {
          refresh_city_dialog(pcity, UPDATE_PRESENT_UNITS);
        }
      }

      if ((pcity = map_get_city(punit->tile)))  {
        if (!get_player_ptr()
            || can_player_see_units_in_city(get_player_ptr(), pcity)) {
          /* Unit moved into a city - obviously it's occupied. */
          if (!pcity->u.client.occupied) {
            pcity->u.client.occupied = TRUE;
            refresh_tile_mapcanvas(pcity->common.tile, MAP_UPDATE_NORMAL);
          }
        }

        if (pcity->common.id == punit->homecity) {
          homecity_needs_update = static_cast<enum city_update>(homecity_needs_update | UPDATE_PRESENT_UNITS);
        } else {
          refresh_city_dialog(pcity, UPDATE_PRESENT_UNITS);
        }

        if ((unit_flag(punit, F_TRADE_ROUTE)
             || unit_flag(punit, F_HELP_WONDER))
            && can_client_issue_orders()
            && (!get_player_ptr()->ai.control || ai_popup_windows)
            && punit->owner == get_player_idx()
            && !unit_has_orders(punit)
            && !punit->ptr
            && punit->activity != ACTIVITY_GOTO
            && (unit_can_help_build_wonder_here(punit)
                || unit_can_est_traderoute_here(punit))) {
          process_caravan_arrival(punit);
        }
      }

      if (punit->ptr) {
        update_trade_route_infos(punit->ptr);
      }

      refresh_city_dialog_maps(old_tile);
      refresh_city_dialog_maps(punit->tile);
    }  /*** End of Change position. ***/

    if (punit->unhappiness != packet_unit->unhappiness) {
      punit->unhappiness = packet_unit->unhappiness;
      homecity_needs_update = static_cast<enum city_update>
          (homecity_needs_update | UPDATE_HAPPINESS | UPDATE_SUPPORTED_UNITS);
    }
    if (punit->upkeep != packet_unit->upkeep) {
      punit->upkeep = packet_unit->upkeep;
      homecity_needs_update = static_cast<city_update>
          (homecity_needs_update | UPDATE_INFORMATION | UPDATE_SUPPORTED_UNITS);
    }
    if (punit->upkeep_food != packet_unit->upkeep_food) {
      punit->upkeep_food = packet_unit->upkeep_food;
      homecity_needs_update = static_cast<enum city_update>
          (homecity_needs_update | UPDATE_INFORMATION | UPDATE_SUPPORTED_UNITS);
    }
    if (punit->upkeep_gold != packet_unit->upkeep_gold) {
      punit->upkeep_gold = packet_unit->upkeep_gold;
      homecity_needs_update = static_cast<city_update>
          (homecity_needs_update | UPDATE_INFORMATION | UPDATE_SUPPORTED_UNITS);
    }
    if (repaint_unit) {
      city_t *pcity = map_get_city(punit->tile);

      homecity_needs_update = static_cast<city_update>(homecity_needs_update | UPDATE_SUPPORTED_UNITS);
      if (pcity) {
        refresh_city_dialog(pcity, UPDATE_PRESENT_UNITS);
      }
    }
    if (homecity && homecity_needs_update != UPDATE_NOTHING) {
      refresh_city_dialog(homecity, homecity_needs_update);
    }

    punit->veteran = packet_unit->veteran;
    punit->moves_left = packet_unit->moves_left;
    punit->fuel = packet_unit->fuel;
    punit->goto_tile = packet_unit->goto_tile;
    if (server_has_extglobalinfo) {
      punit->air_patrol_tile = packet_unit->air_patrol_tile;
    }
    punit->paradropped = packet_unit->paradropped;
    if (punit->done_moving != packet_unit->done_moving) {
      punit->done_moving = packet_unit->done_moving;
      check_focus = TRUE;
    }

    /* This won't change punit; it enqueues the call for later handling. */
    agents_unit_changed(punit);
  } else {
    city_t *pcity;

    /*** Create new unit ***/
    punit = packet_unit;
    idex_register_unit(punit);
    punit->is_new = TRUE;
    unit_list_prepend(get_player(punit->owner)->units, punit);
    unit_list_prepend(punit->tile->units, punit);

    if ((homecity = find_city_by_id(punit->homecity))) {
      unit_list_prepend(homecity->common.units_supported, punit);
      if (!server_has_extglobalinfo
          && homecity->common.rally_point
          && punit->tile == homecity->common.tile) {
        /* Check rally point */
        send_goto_unit(punit, homecity->common.rally_point);
        homecity->common.rally_point = NULL;
      }
    }

    freelog(LOG_DEBUG, "New %s %s id %d (%d %d) hc %d %s",
            get_nation_name(unit_owner(punit)->nation),
            unit_name(punit->type), TILE_XY(punit->tile), punit->id,
            punit->homecity,
            homecity ? homecity->common.name : _("(unknown)"));

    repaint_unit = (punit->transported_by == -1);
    agents_unit_new(punit);

    if ((pcity = map_get_city(punit->tile))) {
      /* The unit is in a city - obviously it's occupied. */
      pcity->u.client.occupied = TRUE;
    }

    refresh_unit_city_dialogs(punit);

    check_new_unit_action(punit);

  } /*** End of Create new unit ***/

  assert(punit != NULL);

  if (need_execute_trade) {
    execute_trade_orders(punit);
  } else if (need_free_trade) {
    trade_free_unit(punit);
  }

  if (punit == get_unit_in_focus()) {
    update_unit_info_label(punit);
  } else if (get_unit_in_focus()
             && (same_pos(get_unit_in_focus()->tile, punit->tile)
                 || (moved
                     && same_pos(get_unit_in_focus()->tile, old_tile)))) {
    update_unit_info_label(get_unit_in_focus());
  }

  if (repaint_unit) {
    if (unit_type_flag(punit->type, F_CITIES)) {
      int width = get_citydlg_canvas_width();
      int height = get_citydlg_canvas_height();
      int canvas_x, canvas_y;

      tile_to_canvas_pos(&canvas_x, &canvas_y, punit->tile);
      update_map_canvas(canvas_x - (width - NORMAL_TILE_WIDTH) / 2,
                        canvas_y - (height - NORMAL_TILE_HEIGHT) / 2,
                        width, height, MAP_UPDATE_NORMAL);
    } else {
      refresh_tile_mapcanvas(punit->tile, MAP_UPDATE_NORMAL);
    }
  }

  if ((check_focus || get_unit_in_focus() == NULL)
      && get_player_ptr() && !get_player_ptr()->ai.control) {
    update_unit_focus();
  }

  if (need_update_menus) {
    update_menus();
  }

  if (punit->owner == get_player_idx()) {
    enable_airlift_unit_type_menu(punit->type);
  }

  return ret;
}

/**************************************************************************
  Receive a short_unit info packet.
**************************************************************************/
void handle_unit_short_info(struct packet_unit_short_info *packet) /* 50 sc */
{
  city_t *pcity;
  unit_t *punit;

# if REPLAY
  printf("UNIT_SHORT_INFO "); /* done */
  printf("id=%d ", packet->id);
  printf("owner=%d ", packet->owner);
  printf("x=%d ", packet->x);
  printf("y=%d ", packet->y);
  printf("type=%d ", packet->type);
  printf("veteran=%d ", packet->veteran);
  printf("occupied=%d ", packet->occupied);
  printf("goes_out_of_sight=%d ", packet->goes_out_of_sight);
  printf("transported=%d ", packet->transported);
  printf("hp=%d ", packet->hp);
  printf("activity=%d ", packet->activity);
  printf("transported_by=%d ", packet->transported_by);
  printf("packet_use=%d ", packet->packet_use);
  printf("info_city_id=%d ", packet->info_city_id);
  printf("serial_num=%d\n", packet->serial_num);
# endif

  if (packet->goes_out_of_sight) {
    punit = find_unit_by_id(packet->id);
    if (punit) {
      client_remove_unit(punit);
    }
    return;
  }

  /* Special case for a diplomat/spy investigating a city: The investigator
   * needs to know the supported and present units of a city, whether or not
   * they are fogged. So, we send a list of them all before sending the city
   * info. */
  if (packet->packet_use == UNIT_INFO_CITY_SUPPORTED
      || packet->packet_use == UNIT_INFO_CITY_PRESENT) {
    static int last_serial_num = 0;

    /* fetch city -- abort if not found */
    pcity = find_city_by_id(packet->info_city_id);
    if (!pcity) {
      return;
    }

    /* New serial number -- clear (free) everything */
    if (last_serial_num != packet->serial_num) {
      last_serial_num = packet->serial_num;
      unit_list_iterate(pcity->u.client.info_units_supported, psunit) {
        destroy_unit_virtual(psunit);
      } unit_list_iterate_end;
      unit_list_unlink_all(pcity->u.client.info_units_supported);
      unit_list_iterate(pcity->u.client.info_units_present, ppunit) {
        destroy_unit_virtual(ppunit);
      } unit_list_iterate_end;
      unit_list_unlink_all(pcity->u.client.info_units_present);
    }

    /* Okay, append a unit struct to the proper list. */
    punit = unpackage_short_unit(packet);
    if (packet->packet_use == UNIT_INFO_CITY_SUPPORTED) {
      unit_list_prepend(pcity->u.client.info_units_supported, punit);
    } else {
      assert(packet->packet_use == UNIT_INFO_CITY_PRESENT);
      unit_list_prepend(pcity->u.client.info_units_present, punit);
    }

    /* Done with special case. */
    return;
  }

  if (packet->owner == get_player_idx() ) {
    freelog(LOG_ERROR, "Got packet_short_unit for own unit.");
  }

  punit = unpackage_short_unit(packet);
  if (handle_unit_packet_common(punit)) {
    free(punit);
  }
}

/****************************************************************************
  Receive information about the map size and topology from the server.  We
  initialize some global variables at the same time.
****************************************************************************/
void handle_map_info(struct packet_map_info *map_info) /* 16 sc */
{
# if REPLAY
  printf("MAP_INFO "); /* done */
  printf("xsize=%u ysize=%u topology_id=%d\n",
         map_info->xsize, map_info->xsize, map_info->topology_id);
# endif
  map.info = *map_info;

  /* Parameter is FALSE so that sizes are kept unchanged. */
  map_init_topology(FALSE);

  map_allocate();
  init_client_goto();

  generate_citydlg_dimensions();

  set_overview_dimensions(map.info.xsize, map.info.ysize);
}

/**************************************************************************
  ...
**************************************************************************/
void handle_traderoute_info(struct packet_traderoute_info *packet) /* 122 sc */
{
# if REPLAY
  printf("TRADEROUTE_INFO "); /* done */
  printf("trademindist=%d ", packet->trademindist);
  printf("traderevenuepct=%d ", packet->traderevenuepct);
  printf("traderevenuestyle=%d ", packet->traderevenuestyle);
  printf("caravanbonusstyle=%d ", packet->caravanbonusstyle);
  printf("maxtraderoutes=%d\n", packet->maxtraderoutes);
# endif
  game.traderoute_info = *packet;
  if (!has_capability("extglobalinfo", aconnection.capability)) {
    game.traderoute_info.maxtraderoutes = GAME_DEFAULT_MAXTRADEROUTES;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void handle_extgame_info(struct packet_extgame_info *packet) /* 123 sc */
{
# if REPLAY
  printf("EXTGAME_INFO "); /* done */
  printf("futuretechsscore=%d ", packet->futuretechsscore);
  printf("improvedautoattack=%d ", packet->improvedautoattack);
  printf("stackbribing=%d ", packet->stackbribing);
  printf("experimentalbribingcost=%d ", packet->experimentalbribingcost);
  printf("techtrading=%d ", packet->techtrading);
  printf("ignoreruleset=%d ", packet->ignoreruleset);
  printf("goldtrading=%d ", packet->goldtrading);
  printf("citytrading=%d ", packet->citytrading);
  printf("airliftingstyle=%d ", packet->airliftingstyle);
  printf("teamplacement=%d ", packet->teamplacement);
  printf("globalwarmingon=%d ", packet->globalwarmingon);
  printf("nuclearwinteron=%d ", packet->nuclearwinteron);
  printf("maxallies=%d ", packet->maxallies);
  printf("techleakagerate=%d\n", packet->techleakagerate);
# endif
  game.ext_info = *packet;
  if (!has_capability("exttechleakage", aconnection.capability)) {
    game.ext_info.maxallies = GAME_DEFAULT_MAXALLIES;
    game.ext_info.techleakagerate = GAME_DEFAULT_TECHLEAKAGERATE;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void handle_game_info(struct packet_game_info *pinfo) /* 15 sc */
{
  int i;
  bool boot_help;
  bool need_effect_update = FALSE;
  bool timeout_changed;

# if REPLAY
  printf("GAME_INFO "); /* done */
  printf("gold=%d ", pinfo->gold);
  printf("tech=%d ", pinfo->tech);
  printf("researchcost=%d ", pinfo->researchcost);
  printf("skill_level=%d ", pinfo->skill_level);
  printf("seconds_to_turndone=%d ", pinfo->seconds_to_turndone);
  printf("timeout=%d ", pinfo->timeout);
  printf("turn=%d ", pinfo->turn);
  printf("year=%d ", pinfo->year);
  printf("end_year=%d ", pinfo->end_year);
  printf("min_players=%d ", pinfo->min_players);
  printf("max_players=%d ", pinfo->max_players);
  printf("nplayers=%d ", pinfo->nplayers);
  printf("player_idx=%d ", pinfo->player_idx);
  printf("globalwarming=%d ", pinfo->globalwarming);
  printf("heating=%d ", pinfo->heating);
  printf("nuclearwinter=%d ", pinfo->nuclearwinter);
  printf("cooling=%d ", pinfo->cooling);
  printf("cityfactor=%d ", pinfo->cityfactor);
  printf("diplcost=%d ", pinfo->diplcost);
  printf("freecost=%d ", pinfo->freecost);
  printf("conquercost=%d ", pinfo->conquercost);
  printf("unhappysize=%d ", pinfo->unhappysize);
  printf("angrycitizen=%d ", pinfo->angrycitizen);
  printf("techpenalty=%d ", pinfo->techpenalty);
  printf("foodbox=%d ", pinfo->foodbox);
  printf("civstyle=%d ", pinfo->civstyle);
  printf("diplomacy=%d ", pinfo->diplomacy);
  printf("spacerace=%d ", pinfo->spacerace);
  printf("global_advances[%d]={\n", A_LAST);
  for (i = 0; i < A_LAST; i++) {
    printf("%d ", pinfo->global_advances[i]);
    if ( i % 50 == 49 )
      printf("\n");
    else if ( i % 10 == 9)
      printf("=");
  }
  printf("}\n");
  printf("global_wonders[%d]={\n", B_LAST);
  for (i = 0; i < B_LAST; i++) {
    printf("%d ", pinfo->global_wonders[i]);
    if ( i % 50 == 49 )
      printf("\n");
    else if ( i % 10 == 9)
      printf("=");
  }
  printf("}\n");
# endif

  boot_help = (can_client_change_view()
               && game.info.spacerace != pinfo->spacerace);
  timeout_changed = game.info.timeout != pinfo->timeout;

  game.info = *pinfo;

  if (game.info.timeout > 0) {
    end_of_turn = time(NULL) + game.info.seconds_to_turndone;
  }

  if (!can_client_change_view()) {
    /*
     * Hack to allow code that explicitly checks for Palace or City Walls
     * to work.
     */
    /* WTF? FIXME: get rid of this hack. */
    game.palace_building = get_building_for_effect(EFFECT_TYPE_CAPITAL_CITY);
    if (game.palace_building == B_LAST) {
      /* This does not appear to affect anything when
       * we reach here; it certainly is not fatal. */
      freelog(LOG_VERBOSE, "Cannot find any palace building");
    }

    game.land_defend_building = get_building_for_effect(EFFECT_TYPE_LAND_DEFEND);
    if (game.land_defend_building == B_LAST) {
      freelog(LOG_VERBOSE, "Cannot find any land defend building");
    }

    improvement_status_init(game.improvements,
                            ARRAY_SIZE(game.improvements));

    aconnection.player = ((pinfo->player_idx >= 0
                           && pinfo->player_idx
                              < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS)
                          ? get_player(pinfo->player_idx) : NULL);
  }
  for (i = 0; i < B_LAST /* game.ruleset_control.num_impr_types */ ; i++) {
    /* Only add in the improvement if it's in a "foreign" (i.e. unknown) city
     * and has equiv_range==World - otherwise we deal with it in its home
     * city anyway */
    if (is_wonder(i)
        && improvement_types[i].equiv_range == IMPR_RANGE_WORLD
        && !find_city_by_id(game.info.global_wonders[i]))
    {
      if (game.info.global_wonders[i] <= 0
          && game.improvements[i] != I_NONE)
      {
        game.improvements[i] = I_NONE;
        need_effect_update = TRUE;
      } else if (game.info.global_wonders[i] > 0
                 && game.improvements[i] == I_NONE) {
        game.improvements[i] = I_ACTIVE;
        need_effect_update = TRUE;
      }
    }
  }

  /* Only update effects if a new wonder appeared or was destroyed */
  try_update_effects(need_effect_update);

  if (get_client_state() == CLIENT_SELECT_RACE_STATE) {
    popdown_races_dialog();
  }
  if (boot_help) {
    boot_help_texts(); /* Reboot, after setting game.info.spacerace */
  }
  if (timeout_changed) {
    delayed_goto_auto_timers_init();
  }

  update_unit_focus();
}

/**************************************************************************
...
**************************************************************************/
static bool read_player_info_techs(player_t *pplayer,
                                   char *inventions)
{
  bool need_effect_update = FALSE;

  tech_type_iterate(i) {
    enum tech_state oldstate = pplayer->research.inventions[i].state;
    enum tech_state newstate = static_cast<tech_state>(inventions[i] - '0');

    pplayer->research.inventions[i].state = newstate;
    if (newstate != oldstate
        && (newstate == TECH_KNOWN || oldstate == TECH_KNOWN)) {
      need_effect_update = TRUE;
    }
  } tech_type_iterate_end;

  if (need_effect_update && pplayer == get_player_ptr()) {
    improvements_update_obsolete();
    update_menus();
    update_airlift_unit_types();
  }

  update_research(pplayer);
  return need_effect_update;
}

/**************************************************************************
  Sets the target government.  This will automatically start a revolution
  if the target government differs from the current one.
**************************************************************************/
void set_government_choice(int government)
{
  if (can_client_issue_orders() && government != get_player_ptr()->government) {
    dsend_packet_player_change_government(&aconnection, government);
  }
}

/**************************************************************************
  Begin a revolution by telling the server to start it.  This also clears
  the current government choice.
**************************************************************************/
void start_revolution(void)
{
  dsend_packet_player_change_government(&aconnection,
                                        game.ruleset_control.government_when_anarchy);
}

/**************************************************************************
...
**************************************************************************/
void handle_player_info(struct packet_player_info *pinfo) /* 39 sc */
{
  unsigned int i;
  bool poptechup, new_tech = FALSE;
  char msg[MAX_LEN_MSG];
  player_t *pplayer = &game.players[pinfo->playerno];

# if REPLAY
  printf("PLAYER_INFO"); /* done */
  printf(" playerno=%d", pinfo->playerno);
  printf(" name=\"%s\"", pinfo->name);
  printf(" username=\"%s\"", pinfo->username);
  printf(" is_male=%d ", pinfo->is_male);
  printf(" government=%d", pinfo->government);
  printf(" target_government=%d", pinfo->target_government);
  printf(" embassy=%d", pinfo->embassy);
  printf(" city_style=%d", pinfo->city_style);
  printf(" nation=%d", pinfo->nation);
  printf(" team=%d", pinfo->team);
  printf(" turn_done=%d", pinfo->turn_done);
  printf(" nturns_idle=%d", pinfo->nturns_idle);
  printf(" is_alive=%d", pinfo->is_alive);
  printf(" reputation=%d", pinfo->reputation);
  printf(" diplstates[%d+%d]=[", MAX_NUM_PLAYERS, MAX_NUM_BARBARIANS);
  printf(" type turns_left has_reason_to_cancel contact_turns_left] {");
  for (i=0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    printf("[%d %d %d %d] ",
           pinfo->diplstates[i].type,
           pinfo->diplstates[i].turns_left,
           pinfo->diplstates[i].has_reason_to_cancel,
           pinfo->diplstates[i].contact_turns_left);
    if ((i % 10) == 9) printf("\n");
  }
  printf("} ");
  printf("gold=%d ", pinfo->gold);
  printf("tax=%d ", pinfo->tax);
  printf("science=%d ", pinfo->science);
  printf("luxury=%d ", pinfo->luxury);
  printf("bulbs_last_turn=%d ", pinfo->bulbs_last_turn);
  printf("bulbs_researched=%d ", pinfo->bulbs_researched);
  printf("techs_researched=%d ", pinfo->techs_researched);
  printf("researching=%d ", pinfo->researching);
  printf("researching_cost=%d ", pinfo->researching_cost);
  printf("future_tech=%d ", pinfo->future_tech);
  printf("tech_goal=%d ", pinfo->tech_goal);
  printf("is_connected=%d ", pinfo->is_connected);
  printf("revolution_finishes=%d ", pinfo->revolution_finishes);
  printf("ai=%d ", pinfo->ai);
  printf("barbarian_type=%d ", pinfo->barbarian_type);
  printf("gives_shared_vision=%d ", pinfo->gives_shared_vision);
  printf("inventions=\"%s\" ", pinfo->inventions);
  printf("pinfo->love[%d+%d]=[", MAX_NUM_PLAYERS, MAX_NUM_BARBARIANS);
  for (i=0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    printf("%d", pinfo->love[i]);
    if ((i % 10) == 9) printf(" ");
  }
  printf("]\n");
# endif

  sz_strlcpy(pplayer->name, pinfo->name);

  pplayer->nation = pinfo->nation;
  pplayer->is_male = pinfo->is_male;
  if (pplayer->team != pinfo->team) {
    team_remove_player(pplayer);
    team_id_add_player(pplayer, pinfo->team);
  }

  pplayer->economic.gold = pinfo->gold;
  pplayer->economic.tax = pinfo->tax;
  pplayer->economic.science = pinfo->science;
  pplayer->economic.luxury = pinfo->luxury;
  pplayer->government = pinfo->government;
  pplayer->target_government = pinfo->target_government;
  pplayer->embassy = pinfo->embassy;
  pplayer->gives_shared_vision = pinfo->gives_shared_vision;
  pplayer->city_style = pinfo->city_style;
  for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    pplayer->ai.love[i] = pinfo->love[i];
  }

  for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    if (can_client_issue_orders()
        && pplayer->diplstates[i].type != pinfo->diplstates[i].type) {
      if (pplayer->diplstates[i].type != DIPLSTATE_WAR
          && pinfo->diplstates[i].type == DIPLSTATE_WAR
          && !(pplayer->diplstates[i].type == DIPLSTATE_NO_CONTACT
               && game.info.diplomacy >= 2)) {
        if (pplayer == get_player_ptr()) {
          delayed_goto_event(AUTO_WAR_DIPLSTATE, get_player(i));
        } else if (i == get_player_idx()) {
          delayed_goto_event(AUTO_WAR_DIPLSTATE, pplayer);
        }
      }
    }
    pplayer->diplstates[i].type = pinfo->diplstates[i].type;
    pplayer->diplstates[i].turns_left = pinfo->diplstates[i].turns_left;
    pplayer->diplstates[i].contact_turns_left =
      pinfo->diplstates[i].contact_turns_left;
    pplayer->diplstates[i].has_reason_to_cancel =
      pinfo->diplstates[i].has_reason_to_cancel;
  }
  pplayer->reputation = pinfo->reputation;

  pplayer->is_connected = pinfo->is_connected;

  /* If the server sends out player information at the wrong time, it is
   * likely to give us inconsistent player tech information, causing a
   * sanity-check failure within this function.  Fixing this at the client
   * end is very tricky; it's hard to figure out when to read the techs
   * and when to ignore them.  The current solution is that the server should
   * only send the player info out at appropriate times - e.g., while the
   * game is running. */
  new_tech = read_player_info_techs(pplayer, pinfo->inventions);

  poptechup = (pplayer->research.researching != pinfo->researching
               || pplayer->ai.tech_goal != pinfo->tech_goal);
  pplayer->research.bulbs_last_turn = pinfo->bulbs_last_turn;
  pplayer->research.bulbs_researched = pinfo->bulbs_researched;
  pplayer->research.techs_researched = pinfo->techs_researched;
  pplayer->research.researching=pinfo->researching;
  if (has_capability("exttechleakage", aconnection.capability)) {
    pplayer->research.researching_cost = pinfo->researching_cost;
  } else {
    pplayer->research.researching_cost = total_bulbs_required(pplayer);
  }
  pplayer->future_tech = pinfo->future_tech;
  pplayer->ai.tech_goal = pinfo->tech_goal;

  if (can_client_change_view() && pplayer == get_player_ptr()) {
    science_dialog_update();
    if (poptechup) {
      if (!get_player_ptr()->ai.control || ai_popup_windows) {
        popup_science_dialog(FALSE);
      }
    }
    if (new_tech) {
      /* If we just learned bridge building and focus is on a settler
         on a river the road menu item will remain disabled unless we
         do this. (applys in other cases as well.) */
      if (get_unit_in_focus()) {
        update_menus();
      }
      /* Maybe the list has changed. */
      refresh_all_city_dialogs(UPDATE_WORKLIST);
    }
    economy_report_dialog_update();
    activeunits_report_dialog_update();
    city_report_dialog_update();
  }

  if (pplayer == get_player_ptr() && pplayer->turn_done != pinfo->turn_done) {
    update_turn_done_button_state();
  }
  pplayer->turn_done = pinfo->turn_done;

  pplayer->nturns_idle = pinfo->nturns_idle;
  pplayer->is_alive = pinfo->is_alive;

  pplayer->ai.barbarian_type = (barbarian_type)pinfo->barbarian_type;
  pplayer->revolution_finishes = pinfo->revolution_finishes;
  if (pplayer->ai.control != pinfo->ai)  {
    pplayer->ai.control = pinfo->ai;
    if (pplayer == get_player_ptr())  {
      my_snprintf(msg, sizeof(msg), _("AI Mode is now %s."),
                  get_player_ptr()->ai.control?_("ON"):_("OFF"));
      append_output_window(msg);
    }
  }

  update_players_dialog();

  if (pplayer == get_player_ptr() && can_client_change_view()) {
    upgrade_canvas_clipboard();
    update_info_label();
  }

  /* if the server requests that the client reset, then information about
   * connections to this player are lost. If this is the case, insert the
   * correct conn back into the player->connections list */
  if (connection_list_size(pplayer->connections) == 0) {
    connection_list_iterate(game.est_connections, pconn) {
      if (pconn->player == pplayer) {
        /* insert the controller into first position */
        if (pconn->observer) {
          connection_list_append(pplayer->connections, pconn);
        } else {
          connection_list_prepend(pplayer->connections, pconn);
        }
      }
    } connection_list_iterate_end;
  }

  if (has_capability("username_info", aconnection.capability)) {
    sz_strlcpy(pplayer->username, pinfo->username);
  } else {
    connection_list_iterate(game.est_connections, pconn) {
      if (pconn->player == pplayer && !pconn->observer) {
        sz_strlcpy(pplayer->username, pconn->username);
      }
    } connection_list_iterate_end;
  }

  /* Just about any changes above require an update to the intelligence
   * dialog. */
  update_intel_dialog(pplayer);
}

/**************************************************************************
  Remove, add, or update dummy connection struct representing some
  connection to the server, with info from packet_conn_info.
  Updates player and game connection lists.
  Calls update_players_dialog() in case info for that has changed.
**************************************************************************/
void handle_conn_info(struct packet_conn_info *pinfo) /* 86 sc */
{
  connection_t *pconn = find_conn_by_id(pinfo->id);

# if REPLAY
  printf("CONN_INFO "); /* done */
  printf("id=%d used=%d establish=%d observer=%d player_num=%d access_level=%d ",
         pinfo->id, pinfo->used, pinfo->established,
         pinfo->observer, pinfo->player_num,
         (int)pinfo->access_level);
  printf("username=\"%s\" addr=\"%s\" capability=\"%s\"\n",
         pinfo->username, pinfo->addr, pinfo->capability);
# endif

  if (!pinfo->used) {
    /* Forget the connection */
    if (!pconn) {
      freelog(LOG_VERBOSE, "Server removed unknown connection %d", pinfo->id);
      return;
    }
    client_remove_cli_conn(pconn);
    pconn = NULL;
  } else {
    /* Add or update the connection.  Note the connection may refer to
     * a player we don't know about yet. */
    player_t *pplayer =
      ((pinfo->player_num >= 0
        && pinfo->player_num < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS)
       ? get_player(pinfo->player_num) : NULL);

    if (!pconn) {
      freelog(LOG_VERBOSE, "Server reports new connection %d %s",
              pinfo->id, pinfo->username);

      pconn = static_cast<connection_t*>(wc_calloc(1, sizeof(connection_t)));
      pconn->buffer = NULL;
      pconn->send_buffer = NULL;
      pconn->ping_time = -1.0;
      if (pplayer) {
        connection_list_append(pplayer->connections, pconn);
      }
      connection_list_append(game.all_connections, pconn);
      connection_list_append(game.est_connections, pconn);
      connection_list_append(game.game_connections, pconn);
    } else {
      freelog(LOG_VERBOSE, "Server reports updated connection %d %s",
              pinfo->id, pinfo->username);
      if (pplayer != pconn->player) {
        if (pconn->player) {
          connection_list_unlink(pconn->player->connections, pconn);
        }
        if (pplayer) {
          connection_list_append(pplayer->connections, pconn);
        }
      }
    }
    pconn->id = pinfo->id;
    pconn->established = pinfo->established;
    pconn->observer = pinfo->observer;
    /* pinfo->access_level sent but not used in client side. */
    pconn->player = pplayer;
    sz_strlcpy(pconn->username, pinfo->username);
    sz_strlcpy(pconn->addr, pinfo->addr);
    sz_strlcpy(pconn->capability, pinfo->capability);

    if (pinfo->id == aconnection.id) {
      aconnection.established = pconn->established;
      aconnection.observer = pconn->observer;
      aconnection.player = pplayer;
      update_info_table();
    }
  }
  update_players_dialog();
  update_connection_list_dialog();
}

/*************************************************************************
...
**************************************************************************/
void handle_conn_ping_info(struct packet_conn_ping_info *packet) /* 87 sc */
{
  int i;

# if REPLAY
  printf("CONN_PING_INFO "); /* done */
  printf("old_connections=%d ", packet->old_connections);
  printf("old_conn_id[%d]={ ", MAX_NUM_PLAYERS);
  for ( i = 0; i < MAX_NUM_PLAYERS; i++) {
    printf("%d ", packet->old_conn_id[i]);
    printf("%f ", packet->old_ping_time[i]);
  }
  printf(" } connections=%d { ", packet->connections);
  for ( i = 0; i < MAX_NUM_PLAYERS; i++) {
    printf("%d ", packet->conn_id[i]);
    printf("%f ", packet->ping_time[i]);
  }
  printf(" }\n");
# endif

  for (i = 0; i < packet->connections; i++) {
    connection_t *pconn = find_conn_by_id(packet->conn_id[i]);

    if (!pconn) {
      continue;
    }

    pconn->ping_time = packet->ping_time[i];
    freelog(LOG_DEBUG, "conn-id=%d, ping=%fs", pconn->id,
            pconn->ping_time);
  }
  /* The old_ping_time data is ignored. */

  update_players_dialog();
}

/**************************************************************************
Ideally the client should let the player choose which type of
modules and components to build, and (possibly) where to extend
structurals.  The protocol now makes this possible, but the
client is not yet that good (would require GUI improvements)
so currently the client choices stuff automatically if there
is anything unplaced.

This function makes a choice (sends spaceship_action) and
returns 1 if we placed something, else 0.

Do things one at a time; the server will send us an updated
spaceship_info packet, and we'll be back here to do anything
which is left.
**************************************************************************/
static bool spaceship_autoplace(player_t *pplayer,
                                struct player_spaceship *ship)
{
  int i, num;
  enum spaceship_place_type type;

  if (ship->modules > (ship->habitation + ship->life_support
                       + ship->solar_panels)) {
    /* "nice" governments prefer to keep success 100%;
     * others build habitation first (for score?)  (Thanks Massimo.)
     */
    type =
      (ship->habitation==0)   ? SSHIP_PLACE_HABITATION :
      (ship->life_support==0) ? SSHIP_PLACE_LIFE_SUPPORT :
      (ship->solar_panels==0) ? SSHIP_PLACE_SOLAR_PANELS :
      ((ship->habitation < ship->life_support)
       && (ship->solar_panels*2 >= ship->habitation + ship->life_support + 1))
                              ? SSHIP_PLACE_HABITATION :
      (ship->solar_panels*2 < ship->habitation + ship->life_support)
                              ? SSHIP_PLACE_SOLAR_PANELS :
      (ship->life_support<ship->habitation)
                              ? SSHIP_PLACE_LIFE_SUPPORT :
      ((ship->life_support <= ship->habitation)
       && (ship->solar_panels*2 >= ship->habitation + ship->life_support + 1))
                              ? SSHIP_PLACE_LIFE_SUPPORT :
                                SSHIP_PLACE_SOLAR_PANELS;

    if (type == SSHIP_PLACE_HABITATION) {
      num = ship->habitation + 1;
    } else if(type == SSHIP_PLACE_LIFE_SUPPORT) {
      num = ship->life_support + 1;
    } else {
      num = ship->solar_panels + 1;
    }
    assert(num <= NUM_SS_MODULES / 3);

    dsend_packet_spaceship_place(&aconnection, type, num);
    return TRUE;
  }
  if (ship->components > ship->fuel + ship->propulsion) {
    if (ship->fuel <= ship->propulsion) {
      type = SSHIP_PLACE_FUEL;
      num = ship->fuel + 1;
    } else {
      type = SSHIP_PLACE_PROPULSION;
      num = ship->propulsion + 1;
    }
    dsend_packet_spaceship_place(&aconnection, type, num);
    return TRUE;
  }
  if (ship->structurals > num_spaceship_structurals_placed(ship)) {
    /* Want to choose which structurals are most important.
       Else we first want to connect one of each type of module,
       then all placed components, pairwise, then any remaining
       modules, or else finally in numerical order.
    */
    int req = -1;

    if (!ship->structure[0]) {
      /* if we don't have the first structural, place that! */
      type = SSHIP_PLACE_STRUCTURAL;
      num = 0;
      dsend_packet_spaceship_place(&aconnection, type, num);
      return TRUE;
    }

    if (ship->habitation >= 1
        && !ship->structure[modules_info[0].required]) {
      req = modules_info[0].required;
    } else if (ship->life_support >= 1
               && !ship->structure[modules_info[1].required]) {
      req = modules_info[1].required;
    } else if (ship->solar_panels >= 1
               && !ship->structure[modules_info[2].required]) {
      req = modules_info[2].required;
    } else {
      int i;
      for(i=0; i<NUM_SS_COMPONENTS; i++) {
        if ((i%2==0 && ship->fuel > (i/2))
            || (i%2==1 && ship->propulsion > (i/2))) {
          if (!ship->structure[components_info[i].required]) {
            req = components_info[i].required;
            break;
          }
        }
      }
    }
    if (req == -1) {
      for(i=0; i<NUM_SS_MODULES; i++) {
        if ((i%3==0 && ship->habitation > (i/3))
            || (i%3==1 && ship->life_support > (i/3))
            || (i%3==2 && ship->solar_panels > (i/3))) {
          if (!ship->structure[modules_info[i].required]) {
            req = modules_info[i].required;
            break;
          }
        }
      }
    }
    if (req == -1) {
      for(i=0; i<NUM_SS_STRUCTURALS; i++) {
        if (!ship->structure[i]) {
          req = i;
          break;
        }
      }
    }
    /* sanity: */
    assert(req!=-1);
    assert(!ship->structure[req]);

    /* Now we want to find a structural we can build which leads to req.
       This loop should bottom out, because everything leads back to s0,
       and we made sure above that we do s0 first.
     */
    while(!ship->structure[structurals_info[req].required]) {
      req = structurals_info[req].required;
    }
    type = SSHIP_PLACE_STRUCTURAL;
    num = req;
    dsend_packet_spaceship_place(&aconnection, type, num);
    return TRUE;
  }
  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
void handle_spaceship_info(struct packet_spaceship_info *p) /* 95 sc */
{
  int i;
  player_t *pplayer = &game.players[p->player_num];
  struct player_spaceship *ship = &pplayer->spaceship;

# if REPLAY
  printf("SPACESHIP_INFO "); /* done */
  printf("player_num=%d ", p->player_num);
  printf("sship_state=%d ", p->sship_state);
  printf("structurals=%d ", p->structurals);
  printf("components=%d ", p->components);
  printf("modules=%d ", p->modules);
  printf("fuel=%d ", p->fuel);
  printf("propulsion=%d ", p->propulsion);
  printf("habitation=%d ", p->habitation);
  printf("life_support=%d ", p->life_support);
  printf("solar_panels=%d ", p->solar_panels);
  printf("launch_year=%d ", p->launch_year);
  printf("population=%d ", p->population);
  printf("mass=%d ", p->mass);
  printf("structure[%d]={ ", NUM_SS_STRUCTURALS+1);
  for ( i = 0; i < NUM_SS_STRUCTURALS; i++) {
    printf("%d", p->structure[i]);
  }
  printf("}\n");
  printf("support_rate=%f ", p->support_rate);
  printf("energy_rate=%f ", p->energy_rate);
  printf("success_rate=%f ", p->success_rate);
  printf("travel_time=%f\n", p->travel_time);
# endif

  ship->state        = (spaceship_state)p->sship_state;
  ship->structurals  = p->structurals;
  ship->components   = p->components;
  ship->modules      = p->modules;
  ship->fuel         = p->fuel;
  ship->fuel         = p->fuel;
  ship->propulsion   = p->propulsion;
  ship->habitation   = p->habitation;
  ship->life_support = p->life_support;
  ship->solar_panels = p->solar_panels;
  ship->launch_year  = p->launch_year;
  ship->population   = p->population;
  ship->mass         = p->mass;
  ship->support_rate = p->support_rate;
  ship->energy_rate  = p->energy_rate;
  ship->success_rate = p->success_rate;
  ship->travel_time  = p->travel_time;

  for(i=0; i<NUM_SS_STRUCTURALS; i++) {
    if (p->structure[i] == '0') {
      ship->structure[i] = FALSE;
    } else if (p->structure[i] == '1') {
      ship->structure[i] = TRUE;
    } else {
      freelog(LOG_ERROR, "invalid spaceship structure '%c' %d",
              p->structure[i], p->structure[i]);
      ship->structure[i] = FALSE;
    }
  }

  if (pplayer != get_player_ptr()) {
    refresh_spaceship_dialog(pplayer);
    return;
  }
  update_menus();

  if (!spaceship_autoplace(pplayer, ship)) {
    refresh_spaceship_dialog(pplayer);
  }
}

/**************************************************************************
This was once very ugly...
**************************************************************************/
void handle_tile_info(struct packet_tile_info *packet) /* 14 sc */
{
  tile_t *ptile = map_pos_to_tile(packet->x, packet->y);
  enum known_type old_known = ptile->u.client.known;
  bool tile_changed = FALSE;
  bool known_changed = FALSE;
  bool renumbered = FALSE;

# if REPLAY
  printf("TILE_INFO "); /* done */
  printf("x=%u y=%u type=%d known=%d special=%d owner=%d continent=%d "
         "sprite=\"%s\"\n",
         packet->x, packet->y,
         packet->type,
         packet->known,
         packet->special,
         packet->owner,
         packet->continent,
         packet->spec_sprite
        );
# endif
  if (ptile->terrain != packet->type) { /*terrain*/
    tile_changed = TRUE;
    ptile->terrain = packet->type;
  }
  if (ptile->alteration != packet->alteration) { /*add-on*/
    tile_changed = TRUE;
    ptile->alteration = packet->alteration;
  }
  if (packet->owner == MAP_TILE_OWNER_NULL) {
    if (ptile->owner) {
      ptile->owner = NULL;
      tile_changed = TRUE;
    }
  } else {
    player_t *newowner = get_player(packet->owner);

    if (ptile->owner != newowner) {
      ptile->owner = newowner;
      tile_changed = TRUE;
    }
  }
  if (ptile->u.client.known != packet->known) {
    known_changed = TRUE;
  }
  ptile->u.client.known = (known_type)packet->known;

  if (packet->spec_sprite[0] != '\0') {
    if (!ptile->spec_sprite
        || strcmp(ptile->spec_sprite, packet->spec_sprite) != 0) {
      map_set_spec_sprite(ptile, packet->spec_sprite);
      tile_changed = TRUE;
    }
  } else {
    if (ptile->spec_sprite) {
      map_set_spec_sprite(ptile, NULL);
      tile_changed = TRUE;
    }
  }

  reset_move_costs(ptile);

  if (ptile->u.client.known <= TILE_KNOWN_FOGGED && old_known == TILE_KNOWN) {
    /* This is an error.  So first we log the error, then make an assertion.
     * But for NDEBUG clients we fix the error. */
    unit_list_iterate(ptile->units, punit) {
      freelog(LOG_ERROR, "%p %s at (%d,%d) %s", punit,
              unit_type(punit)->name, TILE_XY(punit->tile),
              unit_owner(punit)->name);
    } unit_list_iterate_end;
    unit_list_unlink_all(ptile->units);
  }

  /* update continents */
  if (ptile->continent != packet->continent && ptile->continent != 0
      && packet->continent > 0) {
    /* We're renumbering continents, somebody did a transform.
     * But we don't care about renumbering oceans since
     * num_oceans is not kept at the client. */
    renumbered = TRUE;
  }

  ptile->continent = packet->continent;

  if (ptile->continent > map.num_continents) {
    map.num_continents = ptile->continent;
    renumbered = TRUE;
  }

  if (renumbered) {
    allot_island_improvs();
  }

  if (known_changed || tile_changed) {
    /*
     * A tile can only change if it was known before and is still
     * known. In the other cases the tile is new or removed.
     */
    if (known_changed && ptile->u.client.known == TILE_KNOWN) {
      agents_tile_new(ptile);
    } else if (known_changed && ptile->u.client.known == TILE_KNOWN_FOGGED) {
      agents_tile_remove(ptile);
    } else {
      agents_tile_changed(ptile);
    }
  }

  /* refresh tiles */
  if (can_client_change_view()) {
    /* the tile itself */
    if (tile_changed || old_known != ptile->u.client.known) {
      refresh_tile_mapcanvas(ptile, MAP_UPDATE_DRAW);
      refresh_city_dialog_maps(ptile);
    }

    /* if the terrain or the specials of the tile
       have changed it affects the adjacent tiles */
    if (tile_changed) {
      adjc_iterate(ptile, tile1) {
        if (tile_get_known(tile1) >= TILE_KNOWN_FOGGED) {
          refresh_tile_mapcanvas(tile1, MAP_UPDATE_NORMAL);
        }
      }
      adjc_iterate_end;
      return;
    }

    /* the "furry edges" on tiles adjacent to an TILE_UNKNOWN tile are
       removed here */
    if (old_known == TILE_UNKNOWN && packet->known >= TILE_KNOWN_FOGGED) {
      cardinal_adjc_iterate(ptile, tile1) {
        if (tile_get_known(tile1) >= TILE_KNOWN_FOGGED) {
          refresh_tile_mapcanvas(tile1, MAP_UPDATE_NORMAL);
        }
      } cardinal_adjc_iterate_end;
    }
  }

  /* update menus if the focus unit is on the tile. */
  if (tile_changed) {
    unit_t *punit = get_unit_in_focus();
    if (punit && same_pos(punit->tile, ptile)) {
      update_menus();
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_player_remove(int player_id) /* 38 sc */
{
# if REPLAY
  printf("PLAYER_REMOVE "); /* done */
  printf("player_id=%d\n", player_id);
# endif
  client_remove_player(player_id);
}

/**************************************************************************
...
**************************************************************************/
void handle_nation_select_ok(void) /* 11 sc */
{
# if REPLAY
  printf("NATION_SELECT_OK\n"); /* done */
# endif
  if (get_client_state() == CLIENT_SELECT_RACE_STATE) {
    set_client_state(CLIENT_WAITING_FOR_GAME_START_STATE);
    popdown_races_dialog();
  } else {
    freelog(LOG_ERROR,
            "got a select nation packet in an incompatible state");
  }
}

static bool *nations_used;

/**************************************************************************
  Mark a nation as unavailable, after we've entered the select-race state.
**************************************************************************/
void handle_nation_unavailable(Nation_Type_id nation) /* 9 sc */
{
# if REPLAY
  printf("NATION_UNAVAILABLE"); /* done */
  printf(" nation=%d\n", nation);
# endif
  if (get_client_state() == CLIENT_SELECT_RACE_STATE
      && nation >= 0 && nation < game.ruleset_control.playable_nation_count) {
    if (!nations_used[nation]) {
      nations_used[nation] = TRUE;
      races_toggles_set_sensitive(nations_used);
    }
  } else {
    freelog(LOG_ERROR,
            "got a select nation packet in an incompatible state");
  }
}

/**************************************************************************
  Enter the select races state.
**************************************************************************/
void handle_select_races(void) /* 114 sc */
{
# if REPLAY
  printf("SELECT_RACES\n"); /* done */
# endif
  if (get_client_state() == CLIENT_PRE_GAME_STATE) {
    /* First set the state. */
    set_client_state(CLIENT_SELECT_RACE_STATE);

    /* Then clear the nations used.  They are filled by a
     * PACKET_NATION_UNAVAILABLE packet that follows. */
    nations_used = static_cast<bool*>(
                   wc_realloc(nations_used,
                              game.ruleset_control.playable_nation_count
                              * sizeof(nations_used)));
    memset(nations_used, 0,
           game.ruleset_control.playable_nation_count * sizeof(nations_used));

    if (!client_is_observer()) {
      /* Now close the conndlg and popup the races dialog. */
      really_close_connection_dialog();
      popup_races_dialog();
    }
  }
}

/**************************************************************************
  Take arrival of ruleset control packet to indicate that
  current allocated governments should be free'd, and new
  memory allocated for new size. The same for nations.
**************************************************************************/
void handle_ruleset_control(struct packet_ruleset_control *packet) /* 106 sc */
{
# if REPLAY
  int i;
  printf("RULESET_CONTROL "); /* done */
  printf("aqueduct_size=%d ", packet->aqueduct_size);
  printf("add_to_size_limit=%d ", packet->add_to_size_limit);
  printf("notradesize=%d ", packet->notradesize);
  printf("fulltradesize=%d ", packet->fulltradesize);
  printf("num_unit_types=%d ", packet->num_unit_types);
  printf("num_impr_types=%d ", packet->num_impr_types);
  printf("num_tech_types=%d ", packet->num_tech_types);
  printf("rtech_cathedral_plus=%d ", packet->rtech_cathedral_plus);
  printf("rtech_cathedral_minus=%d ", packet->rtech_cathedral_minus);
  printf("rtech_colosseum_plus=%d ", packet->rtech_colosseum_plus);
  printf("rtech_temple_plus=%d ", packet->rtech_temple_plus);
  printf("rtech_partisan_req[%d]={", MAX_NUM_TECH_LIST);
  for ( i = 0; i < MAX_NUM_TECH_LIST; i++) {
    printf("%d ", packet->rtech_partisan_req[i]);
  }
  printf("}\n");
  printf("government_when_anarchy=%d ", packet->government_when_anarchy);
  printf("default_government=%d ", packet->default_government);
  printf("government_count=%d ", packet->government_count);
  printf("nation_count=%d ", packet->nation_count);
  printf("playable_nation_count=%d ", packet->playable_nation_count);
  printf("style_count=%d ", packet->style_count);
  printf("terrain_count=%d ", packet->terrain_count);
  printf("borders=%d ", packet->borders);
  printf("happyborders=%d ", packet->happyborders);
  printf("slow_invasions=%d ", packet->slow_invasions);
  printf("team_name[%d]={", MAX_NUM_TEAMS);
  for ( i = 0; i < MAX_NUM_TEAMS; i++) {
    printf("%s ", packet->team_name[i]);
  }
  printf("}\n");
  printf("default_building=%d\n", packet->default_building);
# endif
  tilespec_free_city_tiles(game.ruleset_control.style_count);
  ruleset_data_free();

  ruleset_cache_init();
  game.ruleset_control = *packet;

  governments_alloc(packet->government_count);
  nations_alloc(packet->nation_count);
  city_styles_alloc(packet->style_count);
  tilespec_alloc_city_tiles(packet->style_count);
  tilespec_alloc_city_tiles(game.ruleset_control.style_count);
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_unit(struct packet_ruleset_unit *p) /* 96 sc */
{
  struct unit_type *u;
  int i;

# if REPLAY
  printf("RULESET_UNIT "); /* done */
  printf("id=%d ", p->id);
  printf("name=%s ", p->name);
  printf("graphic_str=%s ", p->graphic_str);
  printf("graphic_alt=%s ", p->graphic_alt);
  printf("sound_move=%s ", p->sound_move);
  printf("sound_move_alt=%s ", p->sound_move_alt);
  printf("sound_fight=%s ", p->sound_fight);
  printf("sound_fight_alt=%s ", p->sound_fight_alt);
  printf("move_type=%d ", p->move_type);
  printf("build_cost=%d ", p->build_cost);
  printf("pop_cost=%d ", p->pop_cost);
  printf("attack_strength=%d ", p->attack_strength);
  printf("defense_strength=%d ", p->defense_strength);
  printf("move_rate=%d ", p->move_rate);
  printf("tech_requirement=%d ", p->tech_requirement);
  printf("impr_requirement=%d ", p->impr_requirement);
  printf("vision_range=%d ", p->vision_range);
  printf("transport_capacity=%d ", p->transport_capacity);
  printf("hp=%d ", p->hp);
  printf("firepower=%d ", p->firepower);
  printf("obsoleted_by=%d ", p->obsoleted_by);
  printf("fuel=%d ", p->fuel);

  printf("happy_cost=%d ", p->happy_cost);
  printf("shield_cost=%d ", p->shield_cost);
  printf("food_cost=%d ", p->food_cost);
  printf("gold_cost=%d ", p->gold_cost);

  printf("paratroopers_range=%d ", p->paratroopers_range);
  printf("paratroopers_mr_req=%d ", p->paratroopers_mr_req);
  printf("paratroopers_mr_sub=%d ", p->paratroopers_mr_sub);
  printf("veteran_name[%d]={", MAX_VET_LEVELS);
  for (i = 0; i < MAX_VET_LEVELS; i++) {
    printf("%s ", p->veteran_name[i]);
  }
  printf("}\n");
  printf("power_fact[]={");
  for (i = 0; i < MAX_VET_LEVELS; i++) {
    printf("%f ", p->power_fact[i]);
  }
  printf("}\n");
  printf("move_bonus[]={");
  for (i = 0; i < MAX_VET_LEVELS; i++) {
    printf("%d ", p->move_bonus[i]);
  }
  printf("}\n");

  printf("bombard_rate=%d ", p->bombard_rate);
  printf("helptext=\"%s\" ", p->helptext);

  printf("flags[8]={");
  for ( i = 0; i < 8; i++) {
    printf("%X ", p->flags.vec[i]);
  }
  printf("} ");
  printf("roles[8]={");
  for ( i = 0; i < 8; i++) {
    printf("%X ", p->roles.vec[i]);
  }
  printf("}\n");
# endif

  if(p->id < 0 || p->id >= game.ruleset_control.num_unit_types || p->id >= U_LAST) {
    freelog(LOG_ERROR, "Received bad unit_type id %d in handle_ruleset_unit()",
            p->id);
    return;
  }
  u = get_unit_type(p->id);

  sz_strlcpy(u->name_orig, p->name);
  u->name = u->name_orig;
  sz_strlcpy(u->graphic_str, p->graphic_str);
  sz_strlcpy(u->graphic_alt, p->graphic_alt);
  sz_strlcpy(u->sound_move, p->sound_move);
  sz_strlcpy(u->sound_move_alt, p->sound_move_alt);
  sz_strlcpy(u->sound_fight, p->sound_fight);
  sz_strlcpy(u->sound_fight_alt, p->sound_fight_alt);

  u->move_type          = (unit_move_type)p->move_type;
  u->build_cost         = p->build_cost;
  u->pop_cost           = p->pop_cost;
  u->attack_strength    = p->attack_strength;
  u->defense_strength   = p->defense_strength;
  u->move_rate          = p->move_rate;
  u->tech_requirement   = p->tech_requirement;
  u->impr_requirement   = p->impr_requirement;
  u->vision_range       = p->vision_range;
  u->transport_capacity = p->transport_capacity;
  u->hp                 = p->hp;
  u->firepower          = p->firepower;
  u->obsoleted_by       = p->obsoleted_by;
  u->fuel               = p->fuel;
  u->flags              = p->flags;
  u->roles              = p->roles;
  u->happy_cost         = p->happy_cost;
  u->shield_cost        = p->shield_cost;
  u->food_cost          = p->food_cost;
  u->gold_cost          = p->gold_cost;
  u->paratroopers_range = p->paratroopers_range;
  u->paratroopers_mr_req = p->paratroopers_mr_req;
  u->paratroopers_mr_sub = p->paratroopers_mr_sub;
  u->bombard_rate       = p->bombard_rate;

  for (i = 0; i < MAX_VET_LEVELS; i++) {
    sz_strlcpy(u->veteran[i].name, p->veteran_name[i]);
    u->veteran[i].power_fact = p->power_fact[i];
    u->veteran[i].move_bonus = p->move_bonus[i];
  }

  u->helptext = mystrdup(p->helptext);

  tilespec_setup_unit_type(p->id);
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_tech(struct packet_ruleset_tech *p) /* 99 sc */
{
  struct advance *a;

# if REPLAY
  printf("RULESET_TECH"); /* done */
  printf(" id=%d", p->id);
  printf(" req[0]=%d req[1]=%d", p->req[0], p->req[1]);
  printf(" root_req=%d", p->root_req);
  printf(" flags=%d", p->flags);
  printf(" preset_cost=%d", p->preset_cost);
  printf(" num_reqs=%d", p->num_reqs);
  printf(" name=%s", p->name);
  printf(" helptext=%s", p->helptext);
  printf(" graphic_str=%s", p->graphic_str);
  printf(" graphic_alt=%s\n", p->graphic_alt);
# endif
  if(p->id < 0 || p->id >= (int)game.ruleset_control.num_tech_types || p->id >= A_LAST) {
    freelog(LOG_ERROR, "Received bad advance id %d in handle_ruleset_tech()",
            p->id);
    return;
  }
  a = &advances[p->id];

  sz_strlcpy(a->name_orig, p->name);
  a->name = a->name_orig;
  sz_strlcpy(a->graphic_str, p->graphic_str);
  sz_strlcpy(a->graphic_alt, p->graphic_alt);
  a->req[0] = p->req[0];
  a->req[1] = p->req[1];
  a->root_req = p->root_req;
  a->flags = p->flags;
  a->preset_cost = p->preset_cost;
  a->num_reqs = p->num_reqs;
  a->helptext = mystrdup(p->helptext);

  tilespec_setup_tech_type(p->id);
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_building(struct packet_ruleset_building *p) /* 104 sc */
{
  struct impr_type *b;
  int i;

# if REPLAY
  printf("RULESET_BUILDING "); /* done */
  printf("id=%d ", p->id);
  printf("name=%s ", p->name);
  printf("graphic_str=%s ", p->graphic_str);
  printf("graphic_alt=%s ", p->graphic_alt);
  printf("tech_req=%d ", p->tech_req);
  printf("obsolete_by=%d ", p->obsolete_by);
  printf("bldg_req=%d ", p->bldg_req);
  printf("replaced_by=%d ", p->replaced_by);
  printf("is_wonder=%d ", p->is_wonder);
  printf("equiv_range=%d ", p->equiv_range);
  printf("build_cost=%d ", p->build_cost);
  printf("upkeep=%d ", p->upkeep);
  printf("sabotage=%d ", p->sabotage);
  printf("soundtag=%s ", p->soundtag);
  printf("soundtag_alt=%s ", p->soundtag_alt);
  printf("helptext=%s ", p->helptext);
  printf("terr_gate_count=%d ", p->terr_gate_count);
  printf("terr_gate[255]={");
  for ( i = 0; i < 255; i++) {
    printf("%X", p->terr_gate[i]);
    if ( i % 50 == 49 )
      printf("\n");
    else if ( i % 10 == 9)
      printf(" ");
  }
  printf("}\n");
  printf("spec_gate_count=%d ", p->spec_gate_count);
  printf("spec_gate[255]={");
  for ( i = 0; i < 255; i++) {
    printf("%X ", p->spec_gate[i]);
    if ( i % 50 == 49 )
      printf("\n");
    else if ( i % 10 == 9)
      printf(" ");
  }
  printf("}\n");
  printf("equiv_dupl_count=%d ", p->equiv_dupl_count);
  printf("equiv_dupl[255]={");
  for ( i = 0; i < 255; i++) {
    printf("%X ", p->equiv_dupl[i]);
    if ( i % 50 == 49 )
      printf("\n");
    else if ( i % 10 == 9)
      printf(" ");
  }
  printf("}\n");
  printf("equiv_repl_count=%d ", p->equiv_repl_count);
  printf("equiv_repl[255]={");
  for ( i = 0; i < 255; i++) {
    printf("%X ", p->equiv_repl[i]);
    if ( i % 50 == 49 )
      printf("\n");
    else if ( i % 10 == 9)
      printf(" ");
  }
  printf("}\n");
# endif

  if(p->id < 0 || p->id >= game.ruleset_control.num_impr_types || p->id >= B_LAST) {
    freelog(LOG_ERROR,
            "Received bad building id %d in handle_ruleset_building()",
            p->id);
    return;
  }
  b = &improvement_types[p->id];

  sz_strlcpy(b->name_orig, p->name);
  b->name = b->name_orig;
  sz_strlcpy(b->graphic_str, p->graphic_str);
  sz_strlcpy(b->graphic_alt, p->graphic_alt);
  b->tech_req = p->tech_req;
  b->bldg_req = p->bldg_req;
  b->equiv_range = p->equiv_range;
  b->obsolete_by = p->obsolete_by;
  b->is_wonder = p->is_wonder;
  b->build_cost = p->build_cost;
  b->upkeep = p->upkeep;
  b->sabotage = p->sabotage;
  b->helptext = mystrdup(p->helptext);
  sz_strlcpy(b->soundtag, p->soundtag);
  sz_strlcpy(b->soundtag_alt, p->soundtag_alt);

  //T(terr_gate, terr_gate_count, T_NONE);
  b->terr_gate = (Terrain_type_id*)wc_malloc(sizeof(*b->terr_gate) * (p->terr_gate_count + 1));
  for (i = 0; i < p->terr_gate_count; i++) {
    b->terr_gate[i] = p->terr_gate[i];
  }
  b->terr_gate[p->terr_gate_count] = OLD_TERRAIN_NONE;

  //T(alteration_gate, alteration_gate_count, S_NO_SPECIAL);
  b->alteration_gate = (enum tile_alteration_type *)
                       wc_malloc(sizeof(*b->alteration_gate) * (p->spec_gate_count + 1));
  for (i = 0; i < p->spec_gate_count; i++) {
    b->alteration_gate[i] = p->alteration_gate[i];
  }
  b->alteration_gate[p->spec_gate_count] = S_NO_SPECIAL;

  //T(equiv_dupl, equiv_dupl_count, B_LAST);
  b->equiv_dupl = (Impr_Type_id *)wc_malloc(sizeof(*b->equiv_dupl) * (p->equiv_dupl_count + 1));
  for (i = 0; i < p->equiv_dupl_count; i++) {
    b->equiv_dupl[i] = p->equiv_dupl[i];
  }
  b->equiv_dupl[p->equiv_dupl_count] = B_LAST;

  //T(equiv_repl, equiv_repl_count, B_LAST);
  b->equiv_repl = (Impr_Type_id *)wc_malloc(sizeof(*b->equiv_repl) * (p->equiv_repl_count + 1));
  for (i = 0; i < p->equiv_repl_count; i++) {
    b->equiv_repl[i] = p->equiv_repl[i];
  }
  b->equiv_repl[p->equiv_repl_count] = B_LAST;

#ifdef DEBUG
  if(p->id == game.ruleset_control.num_impr_types-1) {
    impr_type_iterate(id) {
      int inx;
      b = &improvement_types[id];
      freelog(LOG_DEBUG, "Impr: %s...",
              b->name);
      freelog(LOG_DEBUG, "  tech_req    %2d/%s",
              b->tech_req,
              (b->tech_req == A_LAST) ?
              "Never" : get_tech_name(get_player_ptr(), b->tech_req));
      freelog(LOG_DEBUG, "  bldg_req    %2d/%s",
              b->bldg_req,
              (b->bldg_req == B_LAST) ?
              "None" :
              improvement_types[b->bldg_req].name);
      freelog(LOG_DEBUG, "  terr_gate...");
      for (inx = 0; b->terr_gate[inx] != OLD_TERRAIN_NONE; inx++) {
        freelog(LOG_DEBUG, "    %2d/%s",
                b->terr_gate[inx], get_terrain_name(b->terr_gate[inx]));
      }
      freelog(LOG_DEBUG, "  spec_gate...");
      for (inx = 0; b->spec_gate[inx] != S_NO_SPECIAL; inx++) {
        freelog(LOG_DEBUG, "    %2d/%s",
                b->spec_gate[inx], get_special_name(b->spec_gate[inx]));
      }
      freelog(LOG_DEBUG, "  equiv_range %2d/%s",
              b->equiv_range, effect_range_name(b->equiv_range));
      freelog(LOG_DEBUG, "  equiv_dupl...");
      for (inx = 0; b->equiv_dupl[inx] != B_LAST; inx++) {
        freelog(LOG_DEBUG, "    %2d/%s",
                b->equiv_dupl[inx], improvement_types[b->equiv_dupl[inx]].name);
      }
      freelog(LOG_DEBUG, "  equiv_repl...");
      for (inx = 0; b->equiv_repl[inx] != B_LAST; inx++) {
        freelog(LOG_DEBUG, "    %2d/%s",
                b->equiv_repl[inx], improvement_types[b->equiv_repl[inx]].name);
      }
      if (tech_exists(b->obsolete_by)) {
        freelog(LOG_DEBUG, "  obsolete_by %2d/%s",
                b->obsolete_by,
                get_tech_name(get_player_ptr(), b->obsolete_by));
      } else {
        freelog(LOG_DEBUG, "  obsolete_by %2d/Never", b->obsolete_by);
      }
      freelog(LOG_DEBUG, "  is_wonder   %2d", b->is_wonder);
      freelog(LOG_DEBUG, "  build_cost %3d", b->build_cost);
      freelog(LOG_DEBUG, "  upkeep      %2d", b->upkeep);
      freelog(LOG_DEBUG, "  sabotage   %3d", b->sabotage);
      freelog(LOG_DEBUG, "  helptext    %s", b->helptext);
    } impr_type_iterate_end;
  }
#endif

  tilespec_setup_impr_type(p->id);
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_government(struct packet_ruleset_government *p) /* 100 sc */
{
  struct government *gov;

# if REPLAY
  printf("RULESET_GOVERNMENT "); /* done */
  printf("id=%d ", p->id);
  printf("required_tech=%d ", p->required_tech);
  printf("max_rate=%d ", p->max_rate);
  printf("civil_war=%d ", p->civil_war);
  printf("martial_law_max=%d ", p->martial_law_max);
  printf("martial_law_per=%d ", p->martial_law_per);
  printf("empire_size_mod=%d ", p->empire_size_mod);
  printf("empire_size_inc=%d ", p->empire_size_inc);
  printf("rapture_size=%d ", p->rapture_size);
  printf("unit_happy_cost_factor=%d ", p->unit_happy_cost_factor);
  printf("unit_shield_cost_factor=%d ", p->unit_shield_cost_factor);
  printf("unit_food_cost_factor=%d ", p->unit_food_cost_factor);
  printf("unit_gold_cost_factor=%d ", p->unit_gold_cost_factor);
  printf("free_happy=%d ", p->free_happy);
  printf("free_shield=%d ", p->free_shield);
  printf("free_food=%d ", p->free_food);
  printf("free_gold=%d ", p->free_gold);
  printf("trade_before_penalty=%d ", p->trade_before_penalty);
  printf("shields_before_penalty=%d ", p->shields_before_penalty);
  printf("food_before_penalty=%d ", p->food_before_penalty);
  printf("celeb_trade_before_penalty=%d ", p->celeb_trade_before_penalty);
  printf("celeb_shields_before_penalty=%d ", p->celeb_shields_before_penalty);
  printf("celeb_food_before_penalty=%d ", p->celeb_food_before_penalty);
  printf("trade_bonus=%d ", p->trade_bonus);
  printf("shield_bonus=%d ", p->shield_bonus);
  printf("food_bonus=%d ", p->food_bonus);
  printf("celeb_trade_bonus=%d ", p->celeb_trade_bonus);
  printf("celeb_shield_bonus=%d ", p->celeb_shield_bonus);
  printf("celeb_food_bonus=%d ", p->celeb_food_bonus);
  printf("corruption_level=%d ", p->corruption_level);
  printf("fixed_corruption_distance=%d ", p->fixed_corruption_distance);
  printf("corruption_distance_factor=%d ", p->corruption_distance_factor);
  printf("extra_corruption_distance=%d ", p->extra_corruption_distance);
  printf("corruption_max_distance_cap=%d ", p->corruption_max_distance_cap);
  printf("waste_level=%d ", p->waste_level);
  printf("fixed_waste_distance=%d ", p->fixed_waste_distance);
  printf("waste_distance_factor=%d ", p->waste_distance_factor);
  printf("extra_waste_distance=%d ", p->extra_waste_distance);
  printf("waste_max_distance_cap=%d ", p->waste_max_distance_cap);
  printf("flags=%d ", p->flags);
  printf("num_ruler_titles=%d ", p->num_ruler_titles);
  printf("name=%s ", p->name);
  printf("graphic_str=%s ", p->graphic_str);
  printf("graphic_alt=%s\n", p->graphic_alt);
  printf("helptext=\"%s\"\n", p->helptext);
# endif

  if (p->id < 0 || p->id >= game.ruleset_control.government_count) {
    freelog(LOG_ERROR,
            "Received bad government id %d in handle_ruleset_government",
            p->id);
    return;
  }
  gov = &governments[p->id];

  gov->index             = p->id;

  gov->required_tech     = p->required_tech;
  gov->max_rate          = p->max_rate;
  gov->civil_war         = p->civil_war;
  gov->martial_law_max   = p->martial_law_max;
  gov->martial_law_per   = p->martial_law_per;
  gov->empire_size_mod   = p->empire_size_mod;
  gov->empire_size_inc   = p->empire_size_inc;
  gov->rapture_size      = p->rapture_size;

  gov->unit_happy_cost_factor  = p->unit_happy_cost_factor;
  gov->unit_shield_cost_factor = p->unit_shield_cost_factor;
  gov->unit_food_cost_factor   = p->unit_food_cost_factor;
  gov->unit_gold_cost_factor   = p->unit_gold_cost_factor;

  gov->free_happy          = p->free_happy;
  gov->free_shield         = p->free_shield;
  gov->free_food           = p->free_food;
  gov->free_gold           = p->free_gold;

  gov->trade_before_penalty   = p->trade_before_penalty;
  gov->shields_before_penalty = p->shields_before_penalty;
  gov->food_before_penalty    = p->food_before_penalty;

  gov->celeb_trade_before_penalty   = p->celeb_trade_before_penalty;
  gov->celeb_shields_before_penalty = p->celeb_shields_before_penalty;
  gov->celeb_food_before_penalty    = p->celeb_food_before_penalty;

  gov->trade_bonus         = p->trade_bonus;
  gov->shield_bonus        = p->shield_bonus;
  gov->food_bonus          = p->food_bonus;

  gov->celeb_trade_bonus   = p->celeb_trade_bonus;
  gov->celeb_shield_bonus  = p->celeb_shield_bonus;
  gov->celeb_food_bonus    = p->celeb_food_bonus;

  gov->corruption_level    = p->corruption_level;
  gov->fixed_corruption_distance = p->fixed_corruption_distance;
  gov->corruption_distance_factor = p->corruption_distance_factor;
  gov->extra_corruption_distance = p->extra_corruption_distance;
  gov->corruption_max_distance_cap = p->corruption_max_distance_cap;

  gov->waste_level           = p->waste_level;
  gov->fixed_waste_distance  = p->fixed_waste_distance;
  gov->waste_distance_factor = p->waste_distance_factor;
  gov->extra_waste_distance  = p->extra_waste_distance;
  gov->waste_max_distance_cap = p->waste_max_distance_cap;

  gov->flags               = p->flags;
  gov->num_ruler_titles    = p->num_ruler_titles;

  sz_strlcpy(gov->name_orig, p->name);
  gov->name = gov->name_orig;
  sz_strlcpy(gov->graphic_str, p->graphic_str);
  sz_strlcpy(gov->graphic_alt, p->graphic_alt);

  gov->ruler_titles = (ruler_title*)wc_calloc(gov->num_ruler_titles,
                                sizeof(struct ruler_title));

  gov->helptext = mystrdup(p->helptext);

  tilespec_setup_government(p->id);
}

void handle_ruleset_government_ruler_title( /* 98 sc */
    struct packet_ruleset_government_ruler_title *p)
{
  struct government *gov;

# if REPLAY
  printf("RULESET_GOVERNMENT_RULER_TITLE "); /* done */
  printf("gov=%d ", p->gov);
  printf("id=%d ", p->id);
  printf("nation=%d ", p->nation);
  printf("male_title=%s ", p->male_title);
  printf("female_title=%s\n", p->female_title);
# endif

  if(p->gov < 0 || p->gov >= game.ruleset_control.government_count) {
    freelog(LOG_ERROR, "Received bad government num %d for title", p->gov);
    return;
  }
  gov = &governments[p->gov];
  if(p->id < 0 || p->id >= gov->num_ruler_titles) {
    freelog(LOG_ERROR, "Received bad ruler title num %d for %s title",
            p->id, gov->name);
    return;
  }
  gov->ruler_titles[p->id].nation = p->nation;
  sz_strlcpy(gov->ruler_titles[p->id].male_title_orig, p->male_title);
  gov->ruler_titles[p->id].male_title
    = gov->ruler_titles[p->id].male_title_orig;
  sz_strlcpy(gov->ruler_titles[p->id].female_title_orig, p->female_title);
  gov->ruler_titles[p->id].female_title
    = gov->ruler_titles[p->id].female_title_orig;
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_terrain(struct packet_ruleset_terrain *p) /* 105 sc */
{
  struct tile_type *t;
# if REPLAY
  int i;

  printf("RULESET_TERRAIN ");
  printf("id=%d ", p->id);
  printf("tags[8]={");
  for  (i = 0; i < 8; i++) {
    printf("%X", p->tags.vec[i]);
  }
  printf("} ");
  printf("terrain_name=%s ", p->terrain_name);
  printf("graphic_str=%s ", p->graphic_str);
  printf("graphic_alt=%s ", p->graphic_alt);

  printf("movement_cost=%d ", p->movement_cost);
  printf("defense_bonus=%d ", p->defense_bonus);
  printf("food=%d ", p->food);
  printf("shield=%d ", p->shield);
  printf("trade=%d ", p->trade);

  printf("special_1_name=%s ", p->special_1_name);
  printf("food_special_1=%d ", p->food_special_1);
  printf("shield_special_1=%d ", p->shield_special_1);
  printf("trade_special_1=%d ", p->trade_special_1);
  printf("graphic_str_special_1=%s ", p->graphic_str_special_1);
  printf("graphic_alt_special_1=%s ", p->graphic_alt_special_1);

  printf("special_2_name=%s ", p->special_2_name);
  printf("food_special_2=%d ", p->food_special_2);
  printf("shield_special_2=%d ", p->shield_special_2);
  printf("trade_special_2=%d ", p->trade_special_2);
  printf("graphic_str_special_2=%s ", p->graphic_str_special_2);
  printf("graphic_alt_special_2=%s ", p->graphic_alt_special_2);

  printf("road_trade_incr=%d ", p->road_trade_incr);
  printf("road_time=%d ", p->road_time);
  printf("irrigation_result=%d ", p->irrigation_result);
  printf("irrigation_food_incr=%d ", p->irrigation_food_incr);
  printf("irrigation_time=%d ", p->irrigation_time);
  printf("mining_result=%d ", p->mining_result);
  printf("mining_shield_incr=%d ", p->mining_shield_incr);
  printf("mining_time=%d ", p->mining_time);

  printf("transform_result=%d ", p->transform_result);
  printf("transform_time=%d ", p->transform_time);
  printf("rail_time=%d ", p->rail_time);
  printf("airbase_time=%d ", p->airbase_time);
  printf("fortress_time=%d ", p->fortress_time);
  printf("clean_pollution_time=%d ", p->clean_pollution_time);
  printf("clean_fallout_time=%d ", p->clean_fallout_time);
  printf("helptext=%s\n", p->helptext);
# endif

  if (p->id < OLD_TERRAIN_FIRST || p->id >= OLD_TERRAIN_COUNT) {
    freelog(LOG_ERROR,
            "Received bad terrain id %d in handle_ruleset_terrain",
            p->id);
    return;
  }
  t = get_tile_type(p->id);

  sz_strlcpy(t->terrain_name_orig, p->terrain_name);
  t->terrain_name = t->terrain_name_orig;
  sz_strlcpy(t->graphic_str, p->graphic_str);
  sz_strlcpy(t->graphic_alt, p->graphic_alt);
  t->movement_cost = p->movement_cost;
  t->defense_bonus = p->defense_bonus;
  t->food = p->food;
  t->shield = p->shield;
  t->trade = p->trade;
  sz_strlcpy(t->special_1_name_orig, p->special_1_name);
  t->special_1_name = t->special_1_name_orig;
  t->food_special_1 = p->food_special_1;
  t->shield_special_1 = p->shield_special_1;
  t->trade_special_1 = p->trade_special_1;
  sz_strlcpy(t->special_2_name_orig, p->special_2_name);
  t->special_2_name = t->special_2_name_orig;
  t->food_special_2 = p->food_special_2;
  t->shield_special_2 = p->shield_special_2;
  t->trade_special_2 = p->trade_special_2;

  sz_strlcpy(t->special[0].graphic_str, p->graphic_str_special_1);
  sz_strlcpy(t->special[0].graphic_alt, p->graphic_alt_special_1);

  sz_strlcpy(t->special[1].graphic_str, p->graphic_str_special_2);
  sz_strlcpy(t->special[1].graphic_alt, p->graphic_alt_special_2);

  t->road_time = p->road_time;
  t->road_trade_incr = p->road_trade_incr;
  t->irrigation_result = p->irrigation_result;
  t->irrigation_food_incr = p->irrigation_food_incr;
  t->irrigation_time = p->irrigation_time;
  t->mining_result = p->mining_result;
  t->mining_shield_incr = p->mining_shield_incr;
  t->mining_time = p->mining_time;
  t->transform_result = p->transform_result;
  t->transform_time = p->transform_time;
  t->rail_time = p->rail_time;
  t->airbase_time = p->airbase_time;
  t->fortress_time = p->fortress_time;
  t->clean_pollution_time = p->clean_pollution_time;
  t->clean_fallout_time = p->clean_fallout_time;

  t->tags = p->tags;

  t->helptext = mystrdup(p->helptext);

  tilespec_setup_tile_type(p->id);
}

/**************************************************************************
  Handle the terrain control ruleset packet sent by the server.
**************************************************************************/
void handle_ruleset_terrain_control(struct packet_ruleset_terrain_control *p) /* 101 sc */
{
# if REPLAY
  printf("RULESET_TERRAIN_CONTROL "); /* done */
  printf("may_road=%d ", p->may_road);
  printf("may_irrigate=%d ", p->may_irrigate);
  printf("may_mine=%d ", p->may_mine);
  printf("may_transform=%d ", p->may_transform);

  printf("ocean_reclaim_requirement_pct=%d ", p->ocean_reclaim_requirement_pct);
  printf("land_channel_requirement_pct=%d ", p->land_channel_requirement_pct);
  printf("river_move_mode=%d ", p->river_move_mode);
  printf("river_defense_bonus=%d ", p->river_defense_bonus);
  printf("river_trade_incr=%d ", p->river_trade_incr);
  printf("river_help_text=%s ", p->river_help_text);

  printf("fortress_defense_bonus=%d ", p->fortress_defense_bonus);
  printf("road_superhighway_trade_bonus=%d ", p->road_superhighway_trade_bonus);
  printf("rail_food_bonus=%d ", p->rail_food_bonus);
  printf("rail_shield_bonus=%d ", p->rail_shield_bonus);
  printf("rail_trade_bonus=%d ", p->rail_trade_bonus);
  printf("farmland_supermarket_food_bonus=%d ", p->farmland_supermarket_food_bonus);
  printf("pollution_food_penalty=%d ", p->pollution_food_penalty);
  printf("pollution_shield_penalty=%d ", p->pollution_shield_penalty);
  printf("pollution_trade_penalty=%d ", p->pollution_trade_penalty);
  printf("fallout_food_penalty=%d ", p->fallout_food_penalty);
  printf("fallout_shield_penalty=%d ", p->fallout_shield_penalty);
  printf("fallout_trade_penalty=%d\n", p->fallout_trade_penalty);
# endif
  /* Since terrain_control is the same as packet_ruleset_terrain_control
   * we can just copy the data directly. */
  terrain_control = *p;
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_nation(struct packet_ruleset_nation *p) /* 102 sc */
{
  int i;
  struct nation_type *pl;

# if REPLAY
  printf("RULESET_NATION "); /* done */
  printf("id=%d ", p->id);
  printf("name=%s ", p->name);
  printf("name_plural=%s ", p->name_plural);
  printf("graphic_str=%s ", p->graphic_str);
  printf("graphic_alt=%s ", p->graphic_alt);
  printf("class_=%s ", p->class_);
  printf("legend=%s ", p->legend);

  printf("city_style=%d ", p->city_style);
  printf("init_techs[%d]={", MAX_NUM_TECH_LIST);
  for ( i = 0; i < MAX_NUM_TECH_LIST; i++) {
    printf("%d ", p->init_techs[i]);
  }
  printf("} ");
  printf("leader_count=%d ", p->leader_count);
  printf("leader_name[%d]={", MAX_NUM_LEADERS);
  for ( i = 0; i < MAX_NUM_LEADERS; i++) {
    printf("%s ", p->leader_name[i]);
  }
  printf("} ");
  printf("leader_sex[%d]={", MAX_NUM_LEADERS);
  for ( i = 0; i < MAX_NUM_LEADERS; i++) {
    printf("%d ", p->leader_sex[MAX_NUM_LEADERS]);
  }
  printf("}\n");
# endif

  if (p->id < 0 || p->id >= game.ruleset_control.nation_count) {
    freelog(LOG_ERROR, "Received bad nation id %d in handle_ruleset_nation()",
            p->id);
    return;
  }
  pl = get_nation_by_idx(p->id);

  sz_strlcpy(pl->name_orig, p->name);
  pl->name = pl->name_orig;
  sz_strlcpy(pl->name_plural_orig, p->name_plural);
  pl->name_plural = pl->name_plural_orig;
  sz_strlcpy(pl->flag_graphic_str, p->graphic_str);
  sz_strlcpy(pl->flag_graphic_alt, p->graphic_alt);
  pl->leader_count = p->leader_count;
  pl->leaders = (leader*)wc_malloc(sizeof(*pl->leaders) * pl->leader_count);
  for (i = 0; i < pl->leader_count; i++) {
    pl->leaders[i].name = mystrdup(p->leader_name[i]);
    pl->leaders[i].is_male = p->leader_sex[i];
  }
  pl->city_style = p->city_style;

  if (p->class_[0] != '\0') {
    pl->class_ = mystrdup(p->class_);
  } else {
    pl->class_ = mystrdup(N_("Other"));
  }

  if (p->legend[0] != '\0') {
    pl->legend = mystrdup(_(p->legend));
  } else {
    pl->legend = mystrdup("");
  }

  tilespec_setup_nation_flag(p->id);
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_city(struct packet_ruleset_city *packet) /* 103 sc */
{
  int id;
  struct citystyle *cs;

# if REPLAY
  printf("RULESET_CITY "); /* done */
  printf("style_id=%d ", packet->style_id);
  printf("techreq=%d ", packet->techreq);
  printf("name=%s ", packet->name);
  printf("citizens_graphic=%s ", packet->citizens_graphic);
  printf("citizens_graphic_alt=%s ", packet->citizens_graphic_alt);
  printf("graphic=%s ", packet->graphic);
  printf("graphic_alt=%s ", packet->graphic_alt);
  printf("replaced_by=%d\n", packet->replaced_by);
# endif

  id = packet->style_id;
  if (id < 0 || id >= game.ruleset_control.style_count) {
    freelog(LOG_ERROR, "Received bad citystyle id %d in handle_ruleset_city()",
            id);
    return;
  }
  cs = &city_styles[id];

  cs->techreq = packet->techreq;
  cs->replaced_by = packet->replaced_by;

  sz_strlcpy(cs->name_orig, packet->name);
  cs->name = cs->name_orig;
  sz_strlcpy(cs->graphic, packet->graphic);
  sz_strlcpy(cs->graphic_alt, packet->graphic_alt);
#if 0
  freelog(LOG_ERROR,"id=%d name=%s graphic=%s graphic_alt=%s",
          id,
          cs->name,
          cs->graphic,
          cs->graphic_alt);
#endif
  sz_strlcpy(cs->citizens_graphic, packet->citizens_graphic);
  sz_strlcpy(cs->citizens_graphic_alt, packet->citizens_graphic_alt);

#if 0
  freelog(LOG_ERROR,"  citizens_graphic=%s citizens_graphic_alt=%s",
          cs->citizens_graphic,
          cs->citizens_graphic_alt);
#endif
  tilespec_setup_city_tiles(id);
  // client hack start, we want an empty city_style
#if 0
  if ( id == 7 )
  {
    // add one, until server provide one
    // city_styles_alloc() has been increase by one for the hack
    struct citystyle *cstyle;
    cstyle = &city_styles[8];
    cstyle->techreq = A_NONE;
    cstyle->replaced_by = A_NONE;
    sz_strlcpy(cstyle->name_orig,"none");
    cstyle->name = cstyle->name_orig;
    sz_strlcpy(cstyle->graphic, "city.none");
    sz_strlcpy(cstyle->graphic_alt, "-");
    sz_strlcpy(cstyle->citizens_graphic, "ancient");
    sz_strlcpy(cstyle->citizens_graphic_alt, "generic");
    freelog(LOG_ERROR,"handle_ruleset_control() %d %d",
            game.ruleset_control.style_count,
            packet->style_id);
    tilespec_setup_city_tiles(8);
  }
  // hack end
#endif
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_game(struct packet_ruleset_game *packet) /* 97 sc */
{
# if REPLAY
  int i;
  printf("RULESET_GAME "); /* done */
  printf("specialist_name[%d]={", SP_COUNT);
  for ( i = 0; i < SP_COUNT; i++) {
    printf("%s ", packet->specialist_name[i]);
  }
  printf("}\n");
  printf("specialist_min_size[%d]={", SP_COUNT);
  for ( i = 0; i < SP_COUNT; i++) {
    printf("%d ", packet->specialist_min_size[i]);
  }
  printf("}\n");
  printf("specialist_bonus[%d]={", SP_COUNT);
  for ( i = 0; i < SP_COUNT; i++) {
    printf("%d ", packet->specialist_bonus[i]);
  }
  printf("}\n");
  printf("changable_tax=%d ", packet->changable_tax);
  printf("forced_science=%d ", packet->forced_science);
  printf("forced_luxury=%d ", packet->forced_luxury);
  printf("forced_gold=%d ", packet->forced_gold);
  printf("min_city_center_food=%d ", packet->min_city_center_food);
  printf("min_city_center_shield=%d ", packet->min_city_center_shield);
  printf("min_city_center_trade=%d ", packet->min_city_center_trade);
  printf("min_dist_bw_cities=%d ", packet->min_dist_bw_cities);
  printf("init_vis_radius_sq=%d ", packet->init_vis_radius_sq);
  printf("hut_overflight=%d ", packet->hut_overflight);
  printf("pillage_select=%d ", packet->pillage_select);
  printf("nuke_contamination=%d ", packet->nuke_contamination);
  printf("granary_food_ini[%d]={", MAX_GRANARY_INIS);
  for ( i = 0; i < MAX_GRANARY_INIS; i++) {
    printf("%d ", packet->granary_food_ini[i]);
  }
  printf("}\n");
  printf("granary_num_inis=%d ", packet->granary_num_inis);
  printf("granary_food_inc=%d ", packet->granary_food_inc);
  printf("tech_cost_style=%d ", packet->tech_cost_style);
  printf("tech_cost_double_year=%d ", packet->tech_cost_double_year);
  printf("tech_leakage=%d ", packet->tech_leakage);
  printf("global_init_techs[%d]={", MAX_NUM_TECH_LIST);
  for ( i = 0; i < MAX_NUM_TECH_LIST; i++) {
    printf("%d ", packet->global_init_techs[i]);
  }
  printf("}\n");
  printf("killstack=%d ", packet->killstack);
  printf("trireme_loss_chance[%d]={", MAX_VET_LEVELS);
  for ( i = 0; i < MAX_VET_LEVELS; i++) {
    printf("%d ", packet->trireme_loss_chance[i]);
  }
  printf("}\n");
  printf("work_veteran_chance[%d]={", MAX_VET_LEVELS);
  for ( i = 0; i < MAX_VET_LEVELS; i++) {
    printf("%d ", packet->work_veteran_chance[i]);
  }
  printf("}\n");
  printf("veteran_chance[%d]={", MAX_VET_LEVELS);
  for ( i = 0; i < MAX_VET_LEVELS; i++) {
    printf("%d ", packet->veteran_chance[i]);
  }
  printf("}\n");
# endif

  game.ruleset_game = *packet;
  tilespec_setup_specialist_types();
}

/**************************************************************************
  ...
**************************************************************************/
void handle_unit_bribe_info(int unit_id, int cost) /* 68 sc */
{
  unit_t *punit = find_unit_by_id(unit_id);

# if REPLAY
  printf("UNIT_BRIBE_INFO unit_id=%d cost=%d\n", unit_id, cost); /* done */
# endif
  if (punit) {
    if (get_player_ptr()
        && (!get_player_ptr()->ai.control || ai_popup_windows)) {
      popup_bribe_dialog(punit, cost);
    }
  }
}

/**************************************************************************
  ...
**************************************************************************/
void handle_city_incite_info(int city_id, int cost) /* 34 sc */
{
  city_t *pcity = find_city_by_id(city_id);

# if REPLAY
  printf("CITY_INCITE_INFO "); /* done */
  printf("city_id=%d cost=%d\n", city_id, cost);
# endif
  if (pcity) {
    if (get_player_ptr()
        && (!get_player_ptr()->ai.control || ai_popup_windows)) {
      popup_incite_dialog(pcity, cost);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_city_name_suggestion_info(int unit_id, char *name) /* 36 sc */
{
# if REPLAY
  printf("CITY_NAME_SUGGESTION_INFO "); /* done */
  printf("unit_id=%d name=\"%s\"\n", unit_id, name);
# endif
  if (!can_client_issue_orders()) {
    return;
  }

  unit_t *punit = player_find_unit_by_id(get_player_ptr(), unit_id);

  if (punit) {
    if (ask_city_name) {
      popup_newcity_dialog(punit, name);
    } else {
      dsend_packet_unit_build_city(&aconnection, unit_id,name);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_unit_diplomat_popup_dialog(int diplomat_id, int target_id) /* 71 sc */
{
# if REPLAY
  printf("UNIT_DIPLOMAT_POPUP_DIALOG diplomat_id=%d target_id=%d\n",
         diplomat_id, target_id); /* done */
# endif
  if (client_is_global_observer()) {
    return;
  }

  unit_t *pdiplomat = player_find_unit_by_id(get_player_ptr(), diplomat_id);

  if (pdiplomat) {
    process_diplomat_arrival(pdiplomat, target_id);
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_city_sabotage_list(int diplomat_id, int city_id, /* 37 sc */
                               char *improvements)
{
# if REPLAY
  int i;

  printf("CITY_SABOTAGE_LIST "); /* done */
  printf("diplomat_id=%d city_id=%d ", diplomat_id, city_id);
  printf("improvements[%d]=[", B_LAST);
  for (i=0; i < B_LAST; i++ ) {
    printf("%d", improvements[i]);
    if ((i % 10) == 9) printf("\n");
  }
  printf("]\n");
# endif

  if (client_is_global_observer()) {
    return;
  }

  unit_t *punit = player_find_unit_by_id(get_player_ptr(), diplomat_id);
  city_t *pcity = find_city_by_id(city_id);

  if (punit && pcity) {
    impr_type_iterate(i) {
      pcity->common.improvements[i] = (improvements[i]=='1') ? I_ACTIVE : I_NONE;
    } impr_type_iterate_end;

    popup_sabotage_dialog(pcity);
  }
}

/**************************************************************************
 Pass the packet on to be displayed in a gui-specific endgame dialog.
**************************************************************************/
void handle_endgame_report(struct packet_endgame_report *packet) /* 13 sc */
{
# if REPLAY
  int i;
  printf("ENDGAME_REPORT "); /* done */
  for (i = 0; i < packet->nscores; i++) {
    printf("nameid[%d]=%d ", packet->nscores, packet->id[i]);
    printf("score=%d ", packet->score[i]);
    printf("pop=%d ", packet->pop[i]);
    printf("bnp=%d ", packet->bnp[i]);
    printf("mfg=%d ", packet->mfg[i]);
    printf("#city=%d ", packet->cities[i]);
    printf("#tech=%d ", packet->techs[i]);
    printf("mil_service=%d ", packet->mil_service[i]);
    printf("wonders=%d ", packet->wonders[i]);
    printf("research=%d ", packet->research[i]);
    printf("landarea=%d ", packet->landarea[i]);
    printf("settledarea=%d ", packet->settledarea[i]);
    printf("literacy=%d ", packet->literacy[i]);
    printf("spaceship=%d\n", packet->spaceship[i]);
  }
# endif
  popup_endgame_report_dialog(packet);
}

/**************************************************************************
...
**************************************************************************/
void handle_player_attribute_chunk(struct packet_player_attribute_chunk *packet) /* 47 cs, sc*/
{
# if REPLAY
  int i;

  printf("PLAYER_ATTRIBUTE_CHUNK "); /* done */
  printf("offset=%d total_length=%d chunk_length=%d data[]=[\n",
         packet->offset, packet->total_length, packet->chunk_length);
  for (i=0;i < packet->chunk_length; i++) {
    printf("%2.2X", packet->data[i] );
    if ((i % 10) == 9) printf("\n");
  }
  printf("]\n");
# endif

  if (!get_player_ptr()) {
    return;
  }

  generic_handle_player_attribute_chunk(get_player_ptr(), packet, &aconnection);

  if (packet->offset + packet->chunk_length == packet->total_length) {
    /* We successful received the last chunk. The attribute block is
       now complete. */
    attribute_restore();
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_processing_started(void) /* 0 sc */
{
# if REPLAY
  printf("PROCESSING_STARTED\n"); /* done */
# endif
  agents_processing_started();

  assert(aconnection.u.client.request_id_of_currently_handled_packet == 0);
  aconnection.u.client.request_id_of_currently_handled_packet =
      get_next_request_id(aconnection.u.client.last_processed_request_id_seen);

  freelog(LOG_DEBUG, "start processing packet %d",
          aconnection.u.client.request_id_of_currently_handled_packet);
}

/**************************************************************************
...
**************************************************************************/
void handle_processing_finished(void) /* 1 sc */
{
  int i;

# if REPLAY
  printf("PROCESSING_FINISHED\n"); /* done */
# endif
  freelog(LOG_DEBUG, "finish processing packet %d",
          aconnection.u.client.request_id_of_currently_handled_packet);

  assert(aconnection.u.client.request_id_of_currently_handled_packet != 0);

  aconnection.u.client.last_processed_request_id_seen =
      aconnection.u.client.request_id_of_currently_handled_packet;

  aconnection.u.client.request_id_of_currently_handled_packet = 0;

  for (i = 0; i < reports_thaw_requests_size; i++) {
    if (reports_thaw_requests[i] != 0 &&
        reports_thaw_requests[i] ==
        aconnection.u.client.last_processed_request_id_seen) {
      reports_thaw();
      reports_thaw_requests[i] = 0;
    }
  }

  agents_processing_finished();
}

/**************************************************************************
...
**************************************************************************/
void notify_about_incoming_packet(connection_t *pconn,
                                  int packet_type, int size)
{
  assert(pconn == &aconnection);
  freelog(LOG_DEBUG, "incoming packet={type=%d, size=%d}", packet_type,
          size);
}

/**************************************************************************
...
**************************************************************************/
void notify_about_outgoing_packet(connection_t *pconn,
                                  int packet_type, int size,
                                  int request_id)
{
  assert(pconn == &aconnection);
  freelog(LOG_DEBUG, "outgoing packet={type=%d, size=%d, request_id=%d}",
          packet_type, size, request_id);

  assert(request_id);
}

/**************************************************************************
  ...
**************************************************************************/
void set_reports_thaw_request(int request_id)
{
  int i;

  for (i = 0; i < reports_thaw_requests_size; i++) {
    if (reports_thaw_requests[i] == 0) {
      reports_thaw_requests[i] = request_id;
      return;
    }
  }

  reports_thaw_requests_size++;
  reports_thaw_requests = static_cast<int*>(
      wc_realloc(reports_thaw_requests,
                 reports_thaw_requests_size * sizeof(int)));
  reports_thaw_requests[reports_thaw_requests_size - 1] = request_id;
}

/**************************************************************************
  We have received PACKET_FREEZE_HINT. It is packet internal to network code
**************************************************************************/
void handle_freeze_hint(void) /* 2 sc */
{
# if REPLAY
  printf("FREEZE_HINT\n"); /* done */
# endif
  freelog(LOG_DEBUG, "handle_freeze_hint");

  reports_freeze();
  agents_freeze_hint();
}

/**************************************************************************
  We have received PACKET_THAW_HINT. It is packet internal to network code
**************************************************************************/
void handle_thaw_hint(void) /* 3 sc */
{
# if REPLAY
  printf("THAW_HINT\n"); /* done */
# endif
  freelog(LOG_DEBUG, "handle_thaw_hint");

  reports_thaw();
  agents_thaw_hint();

  update_turn_done_button_state();
}

/**************************************************************************
...
**************************************************************************/
void handle_server_shutdown(void) /* 8 sc */
{
# if REPLAY
  printf("SERVER_SHUTDOWN\n"); /* done */
# endif
  freelog(LOG_VERBOSE, "server shutdown");
}

/**************************************************************************
...
**************************************************************************/
void handle_conn_ping(void) /* 88 sc */
{
# if REPLAY
  printf("CONN_PING\n"); /* done */
# endif
  send_packet_conn_pong(&aconnection);
}

/**************************************************************************
  We have received PACKET_FREEZE_CLIENT.
**************************************************************************/
void handle_freeze_client(void) /* 135 sc */
{
# if REPLAY
  printf("FREEZE_CLIENT\n"); /* done */
# endif
  freelog(LOG_DEBUG, "handle_freeze_client");

  reports_freeze();

  agents_freeze_hint();
}

/**************************************************************************
  We have received PACKET_THAW_CLIENT
**************************************************************************/
void handle_thaw_client(void) /* 136 sc */
{
# if REPLAY
  printf("THAW_CLIENT\n"); /* done */
# endif
  freelog(LOG_DEBUG, "handle_thaw_client");

  reports_thaw();

  agents_thaw_hint();
  update_turn_done_button_state();
}

/**************************************************************************
  Add group data to ruleset cache.
**************************************************************************/
void handle_ruleset_cache_group(struct packet_ruleset_cache_group *packet) /* 120 sc */
{
  struct effect_group *pgroup;
  int i;

# if REPLAY
  printf("RULESET_CACHE_GROUP "); /* done */
  printf("name=%s ", packet->name);
  printf("num_elements=%d ", packet->num_elements);
  printf("source_buildings[255]={\n");
  for ( i = 0; i < 255; i++) {
    printf("%d ", packet->source_buildings[i]);
    if ( i % 20 == 19)
      printf("\n");
    else if ( i % 10 == 9)
      printf("=");
  }
  printf("}\n");
  printf("ranges[255]={\n");
  for ( i = 0; i < 255; i++) {
    printf("%X", packet->ranges[i]);
    if ( i % 50 == 49)
      printf("\n");
    else if ( i % 10 == 9)
      printf(" ");
  }
  printf("}\n");
  printf("survives[255]={\n");
  for ( i = 0; i < 255; i++) {
    printf("%d", packet->survives[i]);
    if ( i % 50 == 49)
      printf("\n");
    else if ( i % 10 == 9)
      printf(" ");
  }
  printf("}\n");
# endif

  pgroup = effect_group_new(packet->name);

  for (i = 0; i < packet->num_elements; i++) {
    effect_group_add_element(pgroup, packet->source_buildings[i],
                             packet->ranges[i], packet->survives[i]);
  }
}

/**************************************************************************
  Add effect data to ruleset cache.
**************************************************************************/
void handle_ruleset_cache_effect(struct packet_ruleset_cache_effect *packet) /* 121 sc */
{
# if REPLAY
  printf("RULESET_CACHE_EFFECT "); /* done */
  printf("id=%d ", packet->id);
  printf("effect_type=%d ", packet->effect_type);
  printf("range=%d ", packet->range);
  printf("survives=%d ", packet->survives);
  printf("eff_value=%d ", packet->eff_value);
  printf("req_type=%d ", packet->req_type);
  printf("req_value=%d ", packet->req_value);
  printf("group_id=%d\n", packet->group_id);
# endif

  ruleset_cache_add(packet->id, packet->effect_type, packet->range,
                    packet->survives, packet->eff_value,
                    packet->req_type, packet->req_value, packet->group_id);
}

/**************************************************************************
  ...
**************************************************************************/
void handle_vote_remove(int vote_no) /* 126 sc */
{
# if REPLAY
  printf("VOTE_REMOVE vote_no=%d\n", vote_no); /* done */
# endif
  voteinfo_queue_delayed_remove(vote_no);
  voteinfo_gui_update();
}

/**************************************************************************
  ...
**************************************************************************/
void handle_vote_update(int vote_no, int yes, int no, int abstain, /* 125 sc */
                        int num_voters)
{
  struct voteinfo *vi;

# if REPLAY
  printf("VOTE_UPDATE "); /* done */
  printf("vote_no=%d ", vote_no);
  printf("yes=%d ", yes);
  printf("no=%d ", no);
  printf("abstain=%d ", abstain);
  printf("num_voters=%d\n", num_voters);
# endif

  vi = voteinfo_queue_find(vote_no);
  if (vi == NULL) {
    freelog(LOG_ERROR, "Got packet_vote_update for non-existant vote %d!",
            vote_no);
    return;
  }

  vi->yes = yes;
  vi->no = no;
  vi->abstain = abstain;
  vi->num_voters = num_voters;

  voteinfo_gui_update();
}

/**************************************************************************
  ...
**************************************************************************/
void handle_vote_new(struct packet_vote_new *packet) /* 124 sc */
{
# if REPLAY
  /* int i;*/
  printf("VOTE_NEW "); /* done */
  printf("vote_no=%d ", packet->vote_no);
  printf("user=%s ", packet->user);
  printf("desc[512]=%s\n", packet->desc);
#if 0
  printf("desc[512]={\n");
  for (i  = 0; i< 512; i++) {
    printf("%c", packet->desc[i]);
    if ( i % 500 == 49)
      printf("\n");
    else if ( i % 10 == 9)
      printf(" ");
  }
  printf("}\n");
#endif
  printf("percent_required=%d ", packet->percent_required);
  printf("flags=%d ", packet->flags);
  printf("is_poll=%d\n", packet->is_poll);
# endif

  if (voteinfo_queue_find(packet->vote_no)) {
    freelog(LOG_ERROR, "Got a packet_vote_new for already existing "
            "vote %d!", packet->vote_no);
    return;
  }

  voteinfo_queue_add(packet->vote_no,
                     packet->user,
                     packet->desc,
                     packet->percent_required,
                     packet->flags,
                     packet->is_poll);
  voteinfo_gui_update();
}

/**************************************************************************
  ...
**************************************************************************/
void handle_vote_resolve(int vote_no, bool passed) /* 127 sc */
{
  struct voteinfo *vi;

# if REPLAY
  printf("VOTE_RESOLVE vote_no=%d passed=%d\n", vote_no, passed); /* done */
# endif
  vi = voteinfo_queue_find(vote_no);
  if (vi == NULL) {
    freelog(LOG_ERROR, "Got packet_vote_resolve for non-existant "
            "vote %d!", vote_no);
    return;
  }

  vi->resolved = TRUE;
  vi->passed = passed;

  voteinfo_gui_update();
}

/**************************************************************************
  ...
**************************************************************************/
void handle_city_manager_param(struct packet_city_manager_param *packet)/* 145 cs sc */
{
  city_t *pcity = find_city_by_id(packet->id);
  struct cm_parameter parameter;
  int i;

# if REPLAY
  printf("CITY_MANAGER_PARAM "); /* done */
  printf("id=%d ", packet->id);
  printf("minimal_surplus[%d]={\n", CM_NUM_STATS);
  for ( i = 0; i < CM_NUM_STATS; i++) {
    printf("%d ", packet->minimal_surplus[i]);
  }
  printf("}\n");
  printf("require_happy=%d ", packet->require_happy);
  printf("allow_disorder=%d ", packet->allow_disorder);
  printf("allow_specialists=%d ", packet->allow_specialists);
  printf("factor[%d]={\n", CM_NUM_STATS);
  for ( i = 0; i < CM_NUM_STATS; i++) {
    printf("%d ", packet->factor[i]);
  }
  printf("}\n");
  printf("happy_factor=%d\n", packet->happy_factor);
# endif

  if (!pcity) {
    return;
  }

  if (client_is_observer()
      || get_client_state() < CLIENT_GAME_RUNNING_STATE) {
    /* Observing or connecting */
    for (i = 0; i < CM_NUM_STATS; i++) {
      parameter.minimal_surplus[i] = packet->minimal_surplus[i];
      parameter.factor[i] = packet->factor[i];
    }
    parameter.require_happy = packet->require_happy;
    parameter.allow_disorder = packet->allow_disorder;
    parameter.allow_specialists = packet->allow_specialists;
    parameter.happy_factor = packet->happy_factor;

    cma_put_city_under_agent(pcity, &parameter);
    refresh_city_dialog(pcity, UPDATE_CMA);
    city_report_dialog_update_city(pcity);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void handle_city_no_manager_param(int city_id) /* 146 cs sc */
{
  city_t *pcity;

# if REPLAY
  printf("CITY_NO_MANAGER_PARAM city_id=%d\n", city_id); /* done */
# endif
  pcity = find_city_by_id(city_id);
  if (!pcity) {
    return;
  }

  if (client_is_observer()
      || get_client_state() < CLIENT_GAME_RUNNING_STATE) {
    /* Observing or connecting */
    cma_release_city(pcity);
  }
}
