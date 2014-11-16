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

#include "capability.hh"
#include "wc_intl.hh"
#include "log.hh"

#include "city.hh"
#include "connection.hh"
#include "game.hh"
#include "traderoute.hh"
#include "player.hh"
#include "unit.hh"

#include "citytools.hh"
#include "cityturn.hh"
#include "plrhand.hh"
#include "srv_main.hh"
#include "unithand.hh"
#include "unittools.hh"

#include "tradehand.hh"

/****************************************************************************
  Returns TRUE iff this connection is able to handle the server trade packets.
****************************************************************************/
bool connection_supports_server_trade(connection_t *pconn)
{
  return pconn && has_capability("extglobalinfo", pconn->capability);
}

/****************************************************************************
  Send infos for the trade route.
****************************************************************************/
void send_trade_route_info(struct connection_list *dest, struct trade_route *ptr)
{
  struct packet_trade_route_info packet;

  /* Build the packet */
  packet.city1 = ptr->pcity1->common.id;
  packet.city2 = ptr->pcity2->common.id;
  packet.unit_id = ptr->punit ? ptr->punit->id : 0;
  packet.status = ptr->status;

  if (dest) {
    connection_list_iterate(dest, pconn) {
      if (connection_supports_server_trade(pconn)) {
        send_packet_trade_route_info(pconn, &packet);
      }
    } connection_list_iterate_end;
  } else {
    /* Send to the city1 owner */
    connection_list_iterate(city_owner(ptr->pcity1)->connections, pconn) {
      if (connection_supports_server_trade(pconn)) {
        send_packet_trade_route_info(pconn, &packet);
      }
    } connection_list_iterate_end;
    /* Maybe send to the city2 owner */
    if (ptr->pcity1->common.owner != ptr->pcity2->common.owner
        && ptr->status == TR_ESTABLISHED) {
      connection_list_iterate(city_owner(ptr->pcity2)->connections, pconn) {
        if (connection_supports_server_trade(pconn)) {
          send_packet_trade_route_info(pconn, &packet);
        }
      } connection_list_iterate_end;
    }
    /* Send info to global observers */
    global_observers_iterate(pconn) {
      /* They always supports server trade */
      send_packet_trade_route_info(pconn, &packet);
    } global_observers_iterate_end;
  }
}

/****************************************************************************
  Send a remove trade route packet.
****************************************************************************/
void send_trade_route_remove(struct connection_list *dest, struct trade_route *ptr)
{
  if (!dest || !ptr) {
    return;
  }

  struct packet_trade_route_remove packet;

  packet.city1 = ptr->pcity1->common.id;
  packet.city2 = ptr->pcity2->common.id;

  connection_list_iterate(dest, pconn) {
    if (connection_supports_server_trade(pconn)) {
      send_packet_trade_route_remove(pconn, &packet);
    }
  } connection_list_iterate_end;
}

/****************************************************************************
  Remove trade route and do all notifications.
****************************************************************************/
void server_remove_trade_route(struct trade_route *ptr)
{
  struct packet_trade_route_remove packet;
  struct packet_city_info pci1, pci2;
  city_t *pcity1 = ptr->pcity1, *pcity2 = ptr->pcity2;
  bool pci1_computed = FALSE, pci2_computed = FALSE;
  enum trade_route_status status = ptr->status;

  packet.city1 = pcity1->common.id;
  packet.city2 = pcity2->common.id;

  if (ptr->punit) {
    free_unit_orders(ptr->punit);
    set_unit_activity(ptr->punit, ACTIVITY_IDLE);
    ptr->punit->ptr = NULL;
    send_unit_info(NULL, ptr->punit);
  }

  game_trade_route_remove(ptr); /* ptr becomes an invalid pointer! */

  /* Send to the city1 owner */
  connection_list_iterate(city_owner(pcity1)->connections, pconn) {
    if (connection_supports_server_trade(pconn)) {
      send_packet_trade_route_remove(pconn, &packet);
    } else if (status == TR_ESTABLISHED) {
      /* Send full city infos */
      if (!pci1_computed) {
        package_city(pcity1, &pci1, FALSE);
        pci1_computed = TRUE;
      }
      send_packet_city_info(pconn, &pci1);
      if (pcity1->common.owner == pcity2->common.owner) {
        if (!pci2_computed) {
          package_city(pcity2, &pci2, FALSE);
          pci2_computed = TRUE;
        }
        send_packet_city_info(pconn, &pci2);
      }
    }
  } connection_list_iterate_end;
  /* Maybe send to the city2 owner */
  if (pcity1->common.owner != pcity2->common.owner
      && status == TR_ESTABLISHED)
  {
    connection_list_iterate(city_owner(pcity2)->connections, pconn) {
      if (connection_supports_server_trade(pconn)) {
        send_packet_trade_route_remove(pconn, &packet);
      } else {
        /* Send full city infos */
        if (!pci2_computed) {
          package_city(pcity2, &pci2, FALSE);
          pci2_computed = TRUE;
        }
        send_packet_city_info(pconn, &pci2);
      }
    } connection_list_iterate_end;
  }
  /* Send info to global observers */
  global_observers_iterate(pconn) {
    /* They always supports server trade */
    send_packet_trade_route_remove(pconn, &packet);
  } global_observers_iterate_end;
}

/****************************************************************************
  Establish a new trade route (for savegame.c)
****************************************************************************/
void server_establish_trade_route(city_t *pcity1, city_t *pcity2)
{
  if (!pcity1 || !pcity2) {
    return;
  }
  if (!can_cities_trade(pcity1, pcity2)
      || !can_establish_trade_route(pcity1, pcity2)) {
    freelog(LOG_ERROR, "establish_trade_route: Invalid trade route "
            "between %s and %s",
            pcity1->common.name,
            pcity2->common.name);
    return;
  }

  struct trade_route *ptr;

  /* Maybe the trade route already exists, is planned, or is in route */
  if ((ptr = game_trade_route_find(pcity1, pcity2))) {
    server_remove_trade_route(ptr);
  }

  /* Establish the trade route */
  ptr = game_trade_route_add(pcity1, pcity2);
  ptr->status = TR_ESTABLISHED;
  send_trade_route_info(NULL, ptr);
}

/****************************************************************************
  Establish a trade route - Must be the only issue
****************************************************************************/
void unit_establish_trade_route(unit_t *punit, city_t *pcity1,
                                city_t *pcity2)
{
  player_t *pplayer = unit_owner(punit);
  struct trade_route *out_of_home = NULL, *out_of_dest = NULL;
  city_t *pcity_out_of_home = NULL, *pcity_out_of_dest = NULL;
  bool can_establish = !have_cities_trade_route(pcity1, pcity2);
  int revenue;

  if (punit->homecity != pcity1->common.id
      || !unit_flag(punit, F_TRADE_ROUTE)) {
    return;
  }

  int trade = trade_between_cities(pcity1, pcity2);

  /* Maybe we have to cancel some trade routes */
  if (can_establish && city_num_trade_routes(pcity1)
                       >= game.traderoute_info.maxtraderoutes
      && get_city_min_trade_route(pcity1, &out_of_home) >= trade) {
    notify_player_ex(pplayer, pcity2->common.tile, E_NOEVENT,
                     _("Game: Sorry, your %s cannot establish"
                       " a trade route here!"),
                     unit_name(punit->type));
    notify_player_ex(pplayer, pcity2->common.tile, E_NOEVENT,
                     _("Game: The city of %s already has %d "
                       "better trade routes!"),
                     pcity1->common.name,
                     game.traderoute_info.maxtraderoutes);
    out_of_home = NULL;
    can_establish = FALSE;
  }

  if (can_establish && city_num_trade_routes(pcity2)
                       >= game.traderoute_info.maxtraderoutes
      && get_city_min_trade_route(pcity2, &out_of_dest) >= trade) {
    notify_player_ex(pplayer,
                     pcity2->common.tile, E_NOEVENT,
                     _("Game: Sorry, your %s cannot establish"
                       " a trade route here!"),
                     unit_name(punit->type));
    notify_player_ex(pplayer,
                     pcity2->common.tile, E_NOEVENT,
                     _("Game: The city of %s already has %d "
                       "better trade routes!"),
                     pcity2->common.name,
                     game.traderoute_info.maxtraderoutes);
    out_of_dest = NULL;
    can_establish = FALSE;
  }

  /* Now cancel the trade route from the home city. */
  if (can_establish && out_of_home) {
    pcity_out_of_home = out_of_home->pcity1 == pcity1 ?
                        out_of_home->pcity2 : out_of_home->pcity1;
    server_remove_trade_route(out_of_home);
    notify_player_ex(city_owner(pcity_out_of_home),
                     pcity_out_of_home->common.tile, E_NOEVENT,
                     _("Game: Sorry, %s has canceled the trade route "
                       "from %s to your city %s."),
                     city_owner(pcity1)->name,
                     pcity1->common.name,
                     pcity_out_of_home->common.name);
  }

  /* And the same for the dest city. */
  if (can_establish && out_of_dest) {
    pcity_out_of_dest = out_of_dest->pcity1 == pcity2 ?
                        out_of_dest->pcity2 : out_of_dest->pcity1;
    server_remove_trade_route(out_of_dest);
    notify_player_ex(city_owner(pcity_out_of_dest),
                     pcity_out_of_dest->common.tile, E_NOEVENT,
                     _("Game: Sorry, %s has canceled the trade route "
                       "from %s to your city %s."),
                     city_owner(pcity2)->name,
                     pcity2->common.name,
                     pcity_out_of_dest->common.name);
  }

  revenue = get_caravan_enter_city_trade_bonus(pcity1, pcity2);
  if (can_establish) {
    server_establish_trade_route(pcity1, pcity2);
  } else {
    /* enter marketplace */
    revenue = (revenue + 2) / 3;
  }

  connection_list_do_buffer(pplayer->connections);
  notify_player_ex(pplayer,
                   pcity2->common.tile, E_NOEVENT,
                   _("Game: Your %s from %s has arrived in %s,"
                     " and revenues amount to %d in gold and research."),
                   unit_name(punit->type),
                   pcity1->common.name,
                   pcity2->common.name,
                   revenue);
  wipe_unit(punit);
  punit = NULL;
  pplayer->economic.gold += revenue;
  update_tech(pplayer, revenue);

  /* Inform everyone about tech changes */
  send_player_info(pplayer, NULL);

  if (can_establish) {
    /* Refresh the cities. */
    city_refresh(pcity1);
    city_refresh(pcity2);
    if (out_of_home) {
      city_refresh(pcity_out_of_home);
    }
    if (out_of_dest) {
      city_refresh(pcity_out_of_dest);
    }

    /* Notify the owners of the cities. */
    send_city_info(pplayer, pcity1);
    send_city_info(city_owner(pcity2), pcity2);
    if(out_of_home) {
      send_city_info(city_owner(pcity_out_of_home), pcity_out_of_home);
    }
    if(out_of_dest) {
      send_city_info(city_owner(pcity_out_of_dest), pcity_out_of_dest);
    }

    /* Notify each player about the other cities so that they know about
     * the tile_trade value. */
    if (pplayer != city_owner(pcity2)) {
      send_city_info(city_owner(pcity2), pcity1);
      send_city_info(pplayer, pcity2);
    }

    if (out_of_home) {
      if (city_owner(pcity2) != city_owner(pcity_out_of_home)) {
        send_city_info(city_owner(pcity2), pcity_out_of_home);
        send_city_info(city_owner(pcity_out_of_home), pcity2);
      }
      if (pplayer != city_owner(pcity_out_of_home)) {
        send_city_info(pplayer, pcity_out_of_home);
        send_city_info(city_owner(pcity_out_of_home), pcity1);
      }
      if (out_of_dest && city_owner(pcity_out_of_home) !=
                                        city_owner(pcity_out_of_dest)) {
        send_city_info(city_owner(pcity_out_of_home), pcity_out_of_dest);
      }
    }

    if (pcity_out_of_dest) {
      if (city_owner(pcity2) != city_owner(pcity_out_of_dest)) {
        send_city_info(city_owner(pcity2), pcity_out_of_dest);
        send_city_info(city_owner(pcity_out_of_dest), pcity2);
      }
      if (pplayer != city_owner(pcity_out_of_dest)) {
        send_city_info(pplayer, pcity_out_of_dest);
        send_city_info(city_owner(pcity_out_of_dest), pcity1);
      }
      if (out_of_home && city_owner(pcity_out_of_home) !=
                                        city_owner(pcity_out_of_dest)) {
        send_city_info(city_owner(pcity_out_of_dest), pcity_out_of_home);
      }
    }
  }

  connection_list_do_unbuffer(pplayer->connections);
}

/****************************************************************************
  Free orders
****************************************************************************/
void trade_free_unit(unit_t *punit)
{
  if (punit->ptr->status == TR_PL_AND_IR) {
    punit->ptr->status = TR_PLANNED;
    punit->ptr->punit = NULL;
    free_unit_orders(punit);
    set_unit_activity(punit, ACTIVITY_IDLE);
    send_trade_route_info(NULL, punit->ptr);
    punit->ptr = NULL;
  } else if (punit->ptr->status == TR_IN_ROUTE) {
    server_remove_trade_route(punit->ptr);
  }
}

/****************************************************************************
  Receive a trade packet from client
****************************************************************************/
void handle_trade_route_plan(player_t *pplayer, int city1, int city2)
{
  city_t *pcity1, *pcity2;
  struct trade_route *ptr;

  if (!(pcity1 = player_find_city_by_id(pplayer, city1))) {
    if ((pcity1 = find_city_by_id(city1))) {
      notify_player_ex(pplayer,
                       pcity1->common.tile, E_NOEVENT,
                       _("Game: You cannot plan a trade route from %s, "
                         "it's not a own city."),
                       pcity1->common.name);
    } else {
      freelog(LOG_ERROR, "handle_trade_route_plan: Got a bad city id");
    }
    return;
  }
  /* No need to be owned */
  if (!(pcity2 = find_city_by_id(city2))) {
    freelog(LOG_ERROR, "handle_trade_route_plan: Got a bad city id");
    return;
  }
  if (!can_cities_trade(pcity1, pcity2)
      || !can_establish_trade_route(pcity1, pcity2)) {
    notify_player_ex(pplayer,
                     pcity1->common.tile, E_NOEVENT,
                     _("Game: You cannot establish a route between %s and %s."),
                     pcity1->common.name, pcity2->common.name);
    return;
  }

  if ((ptr = game_trade_route_add(pcity1, pcity2))) {
    /* Plan it */
    ptr->status = TR_PLANNED;
    send_trade_route_info(NULL, ptr);
    freelog(LOG_VERBOSE, "%s has planned the trade route between %s and %s",
            pplayer->name, pcity1->common.name, pcity2->common.name);
  } else {
    freelog(LOG_ERROR, "Couldn't add the trade route between %s and %s",
            pcity1->common.name, pcity2->common.name);
  }
}

/****************************************************************************
  Receive a trade packet from client
****************************************************************************/
void handle_trade_route_remove(player_t *pplayer, int city1, int city2)
{
  city_t *pcity1, *pcity2;
  struct trade_route *ptr;

  if (!(pcity1 = player_find_city_by_id(pplayer, city1))) {
    freelog(LOG_ERROR, "handle_trade_route_remove: Got a bad city id");
    return;
  }
  if (!(pcity2 = find_city_by_id(city2))) {
    freelog(LOG_ERROR, "handle_trade_route_remove: Got a bad city id");
    return;
  }
  if (!(ptr = game_trade_route_find(pcity1, pcity2))) {
    freelog(LOG_ERROR, "handle_trade_route_remove: No trade route "
            "between %s and %s",
            pcity1->common.name, pcity2->common.name);
    return;
  }
  if (ptr->status == TR_ESTABLISHED) {
    notify_player_ex(pplayer,
                     pcity1->common.tile, E_NOEVENT,
                     _("Game: You cannot cancel an established trade route "
                       "(%s - %s)."),
                     pcity1->common.name, pcity2->common.name);
    return;
  }
  server_remove_trade_route(ptr);
}

/****************************************************************************
  Receive a trade packet from client
****************************************************************************/
void handle_unit_trade_route(player_t *pplayer, int unit_id,
                             int city1, int city2)
{
  unit_t *punit = player_find_unit_by_id(pplayer, unit_id);
  city_t *pcity1, *pcity2;
  struct trade_route *ptr = NULL;

  if (!punit) {
    return;
  }
  if (!unit_flag(punit, F_TRADE_ROUTE)) {
    notify_player_ex(pplayer, punit->tile, E_NOEVENT,
                     _("This unit (%s) cannot establish trade routes"),
                     unit_name(punit->type));
    return;
  }

  if (city1 == 0 && city2 == 0) {
    /* The server will choose one */
    if (!(ptr = get_next_trade_route_to_establish(punit, FALSE, FALSE,
                                                  TRUE, TRUE))) {
      notify_player_ex(pplayer, punit->tile, E_NOEVENT,
                       _("Game: Didn't find any planned trade "
                         "route to establish"));
      return;
    }
  } else {
    pcity1 = player_find_city_by_id(pplayer, city1);
    pcity2 = find_city_by_id(city2); /* No need to be owned */

    if (!pcity1 || !pcity2) {
      freelog(LOG_ERROR, "handle_unit_trade_route: Got a bad city id");
      return;
    }

    if ((ptr = game_trade_route_find(pcity1, pcity2))) {
      switch (ptr->status) {
        case TR_PLANNED: /* Is ok */
          break;
        case TR_IN_ROUTE:
        case TR_PL_AND_IR:
          notify_player_ex(pplayer, punit->tile, E_NOEVENT,
                           _("Game: The trade route between %s and %s is already "
                             "going to be established"),
                           pcity1->common.name, pcity2->common.name);
          return;
        case TR_ESTABLISHED:
          notify_player_ex(pplayer, punit->tile, E_NOEVENT,
                           _("Game: The trade route between %s and %s "
                             "is already established"),
                           pcity1->common.name, pcity2->common.name);
          return;
        default:
        freelog(LOG_ERROR, "Unkown trade route status varient (%d)",
                ptr->status);
      }
    } else {
      /* Make a new trade route */
      if (!can_cities_trade(pcity1, pcity2)
          || !can_establish_trade_route(pcity1, pcity2)) {
        notify_player_ex(pplayer,
                         pcity1->common.tile, E_NOEVENT,
                         _("Game: You cannot establish a route "
                           "between %s and %s."),
                         pcity1->common.name, pcity2->common.name);
        return;
      }
      ptr = game_trade_route_add(pcity1, pcity2);
    }
  }

  assert(ptr != NULL);
  assert(ptr->punit == NULL);

  ptr->punit = punit;
  calculate_trade_move_cost(ptr); /* Maybe swap city1 and city2,
                                   * if it's faster */
  if (punit->homecity == ptr->pcity1->common.id
      && punit->tile == ptr->pcity2->common.tile)
  {
    /* Already arrived */
    punit->ptr = ptr;
    ptr->status = static_cast<trade_route_status>(ptr->status | TR_IN_ROUTE);
    send_trade_route_info(NULL, ptr);
    unit_establish_trade_route(punit, ptr->pcity1, ptr->pcity2);
    return;
  }
  handle_unit_activity_request(punit, ACTIVITY_IDLE);
  if (!(punit->orders.list = make_unit_orders(ptr, &punit->orders.length))) {
    punit->orders.length = 0;
    ptr->punit = NULL;
    notify_player_ex(pplayer, punit->tile, E_NOEVENT,
                     _("Game: This unit cannot established the trade route "
                       "between %s and %s"),
                     ptr->pcity1->common.name, ptr->pcity2->common.name);
    return;
  }
  punit->orders.index = 0;
  punit->orders.repeat = FALSE;
  punit->orders.vigilant = FALSE; /* Maybe should be TRUE */
  punit->ptr = ptr;
  punit->has_orders = TRUE;
  ptr->status = static_cast<trade_route_status>(ptr->status | TR_IN_ROUTE);

  send_unit_info(NULL, punit);
  send_trade_route_info(NULL, ptr);
  execute_orders(punit);
}

/****************************************************************************
  Remove all trade routes non-established. It is usually called
  when a user take the player without the "extglobalinfo" capability.
****************************************************************************/
void reset_trade_route_planning(player_t *pplayer)
{
  if (!pplayer || server_state == GAME_OVER_STATE) {
    return;
  }

  city_list_iterate(pplayer->cities, pcity) {
    trade_route_list_iterate(pcity->common.trade_routes, ptr) {
      if (ptr->status == TR_ESTABLISHED) {
        continue;
      } else if (server_state == RUN_GAME_STATE) {
        server_remove_trade_route(ptr);
      } else {
        /* Do no notifications */
        if (ptr->punit) {
          free_unit_orders(ptr->punit);
          set_unit_activity(ptr->punit, ACTIVITY_IDLE);
          ptr->punit->ptr = NULL;
        }
        game_trade_route_remove(ptr);
      }
    } trade_route_list_iterate_end;
  } city_list_iterate_end;
}
