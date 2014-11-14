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
#  include "../config.h"
#endif

#include "capability.hh"
#include "wc_intl.hh"
#include "log.hh"
#include "support.hh"

#include "city.hh"
#include "connection.hh"
#include "game.hh"
#include "player.hh"
#include "traderoute.hh"
#include "unit.hh"

#include "chatline_common.hh" /* append_output_window()      */
#include "civclient.hh"
#include "clinet.hh"          /* aconnection                 */
#include "climisc.hh"
#include "control.hh"         /* request_new_unit_activity() */
#include "goto.hh"            /* request_orders_cleared()    */
#include "mapview_common.hh"  /* update_map_canvas_visible() */
#include "multiselect.hh"     /* multi_select_iterate()      */
#include "options.hh"
#include "packhand_gen.hh"

#include "include/citydlg_g.hh"       /* refresh_city_dialog()       */
#include "include/dialogs_g.hh"
#include "include/mapview_g.hh"       /* canvas_put_sprite_full()    */
#include "include/menu_g.hh"          /* update_auto_caravan_menu()  */

#include "trade.hh"

struct delayed_trade_route {
  int city1;
  int city2;
  int value;
};

#define SPECLIST_TAG delayed_trade_route
#define SPECLIST_TYPE struct delayed_trade_route
#include "speclist.hh"

#define delayed_trade_routes_iterate(pdtr) \
  TYPED_LIST_ITERATE(struct delayed_trade_route, delayed_trade_routes, pdtr)
#define delayed_trade_routes_iterate_end LIST_ITERATE_END

static struct tile_list *trade_cities = NULL;
static struct trade_planning_calculation *trade_planning_calc = NULL;
static struct delayed_trade_route_list *delayed_trade_routes = NULL;

void request_unit_trade_route(unit_t *punit, city_t *pcity);

struct toggle_worker_list;

static struct toggle_worker_list *apply_trade_workers(city_t *pcity1,
                                                      city_t *pcity2);
static void release_trade_workers(struct toggle_worker_list *plist);

/**************************************************************************
  ...
**************************************************************************/
void trade_init(void)
{
  if (!trade_cities) {
    trade_cities = tile_list_new();
  }
}

/**************************************************************************
  ...
**************************************************************************/
void trade_free(void)
{
  if (trade_cities) {
    tile_list_free(trade_cities);
    trade_cities = NULL;
  }
  if (trade_planning_calc) {
    trade_planning_calculation_destroy(trade_planning_calc, FALSE);
    assert(NULL == trade_planning_calc);
  }
  if (delayed_trade_routes) {
    delayed_trade_routes_iterate(pdtr) {
      free(pdtr);
    } delayed_trade_routes_iterate_end;
    delayed_trade_route_list_free(delayed_trade_routes);
    delayed_trade_routes = NULL;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void update_trade_route_infos(struct trade_route *ptr)
{
  if (ptr) {
    refresh_city_dialog(ptr->pcity1, UPDATE_TRADE);
    refresh_city_dialog(ptr->pcity2, UPDATE_TRADE);
  }
  update_auto_caravan_menu();
}

/**************************************************************************
  Receive trade packet from the server
**************************************************************************/
void handle_trade_route_info(struct packet_trade_route_info *packet) /* 133 sc */
{
  city_t *pcity1;
  city_t *pcity2;
  struct trade_route *ptr;
  unit_t *pold_unit;

# if REPLAY
  printf("TRADE_ROUTE_INFO\n"); /* done */
  printf("city1=%d ", packet->city1);
  printf("city2=%d ", packet->city2);
  printf("unit_id=%d ", packet->unit_id);
  printf("status=%d\n", packet->status);
# endif
  pcity1 = find_city_by_id(packet->city1);
  pcity2 = find_city_by_id(packet->city2);

  if (!pcity1 || !pcity2) {
    freelog(LOG_ERROR, "handle_trade_route_info: Got a bad city id");
    return;
  }

  ptr = game_trade_route_find(pcity1, pcity2);
  if (!ptr) {
    /* It is a new trade route */
    ptr = game_trade_route_add(pcity1, pcity2);
  }

  freelog(LOG_VERBOSE, "handle_trade_route_info: %s - %s, status %d",
          pcity1->common.name, pcity2->common.name, packet->status);

  pold_unit = ptr->punit;
  ptr->pcity1 = pcity1; /* pcity1 and pcity2 could have been swapped */
  ptr->pcity2 = pcity2; /* pcity1 and pcity2 could have been swapped */
  ptr->punit = find_unit_by_id(packet->unit_id); /* find_unit_by_id(0) = NULL */
  ptr->status = (trade_route_status)packet->status;

  if (pold_unit && ptr->punit != pold_unit) {
    pold_unit->ptr = NULL;
  }
  if (ptr->punit) {
    ptr->punit->ptr = ptr;
  }

  update_trade_route_infos(ptr);
  update_trade_route_line(ptr);
}

/**************************************************************************
  Receive trade packet from the server
**************************************************************************/
void handle_trade_route_remove(int city1, int city2) /* 131 cs sc */
{
  city_t *pcity1, *pcity2;
  struct trade_route *ptr;

# if REPLAY
  printf("TRADE_ROUTE_REMOVE city1=%d city2=%d\n", city1, city2); /* done */
# endif
  pcity1 = find_city_by_id(city1);
  pcity2 = find_city_by_id(city2);

  if (!pcity1 || !pcity2) {
    freelog(LOG_ERROR, "handle_trade_route_remove: Got a bad city id");
    return;
  }

  if (!(ptr = game_trade_route_find(pcity1, pcity2))) {
    freelog(LOG_ERROR, "handle_trade_route_remove: Got a bad trade route");
    return;
  }

  freelog(LOG_VERBOSE, "handle_trade_route_remove: %s - %s",
          pcity1->common.name, pcity2->common.name);

  if (ptr->punit) {
    ptr->punit->ptr = NULL;
  }

  update_trade_route_line(ptr);
  game_trade_route_remove(ptr);

  refresh_city_dialog(pcity1, UPDATE_TRADE);
  refresh_city_dialog(pcity2, UPDATE_TRADE);
  update_auto_caravan_menu();
}

/**************************************************************************
  Add a city in the trade city list.
**************************************************************************/
void add_tile_in_trade_planning(tile_t *ptile, bool allow_remove)
{
  if (ptile->city && ptile->city->common.owner != get_player_idx()) {
    return;
  }

  char buf[256];

  if (tile_list_search(trade_cities, ptile)) {
    if (allow_remove) {
      tile_list_unlink(trade_cities, ptile);
      my_snprintf(buf, sizeof(buf),
                  _("Warclient: Removing %s to the trade planning."),
                  get_tile_info(ptile));
      append_output_window(buf);
      update_auto_caravan_menu();
      refresh_tile_mapcanvas(ptile, MUT_NORMAL);
    }
  } else if (!terrain_has_tag(map_get_terrain(ptile), TER_NO_CITIES)) {
    tile_list_append(trade_cities, ptile);
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Adding %s to the trade planning."),
                get_tile_info(ptile));
    append_output_window(buf);
    refresh_tile_mapcanvas(ptile, MUT_NORMAL);
    if (allow_remove) {
      update_auto_caravan_menu();
    }
  }
}

/**************************************************************************
  Called when a city is conquered, built or found.
**************************************************************************/
void trade_city_new(city_t *pcity)
{
  if (!pcity) {
    return;
  }

  if (tile_list_search(trade_cities, pcity->common.tile)
      && are_trade_cities_built()) {
    update_auto_caravan_menu();
  }
}

/**************************************************************************
  Called when a city is lost or removed.
**************************************************************************/
void trade_remove_city(city_t *pcity)
{
  if (!aconnection.established || !pcity) {
    return;
  }

  /* Free the trade routes */
  trade_route_list_iterate(pcity->common.trade_routes, ptr) {
    if (server_has_extglobalinfo
        || !ptr->punit || !trade_free_unit(ptr->punit)) {
      game_trade_route_remove(ptr);
    }
  } trade_route_list_iterate_end;

  if (tile_list_search(trade_cities, pcity->common.tile)) {
    /* HACK: set it as non-built. */
    pcity->common.tile->city = NULL;
    update_auto_caravan_menu();
    pcity->common.tile->city = pcity;
  }
}

/**************************************************************************
  Clear the city list.
**************************************************************************/
void clear_trade_city_list(void)
{
  tile_list_unlink_all(trade_cities);
  append_output_window(_("Warclient: Trade city list cleared."));
  update_auto_caravan_menu();
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
  Clear the trade planning:
  * if the server is a warserver, then send its the changes
  * else, apply it locally.
**************************************************************************/
void clear_trade_planning(bool include_in_route)
{
  if (!can_client_issue_orders()) {
    return;
  }

  if (server_has_extglobalinfo) {
    connection_do_buffer(&aconnection);
    city_list_iterate(get_player_ptr()->cities, pcity) {
      trade_route_list_iterate(pcity->common.trade_routes, ptr) {
        city_t *pother_city = OTHER_CITY(ptr, pcity);

        /* Try to don't send the packet twice */
        if (((include_in_route && ptr->status & TR_PLANNED)
             || (!include_in_route && ptr->status == TR_PLANNED))
            && (pcity->common.owner != pother_city->common.owner
                || pcity->common.id < pother_city->common.id)) {
          dsend_packet_trade_route_remove(&aconnection,
                                          ptr->pcity1->common.id,
                                          ptr->pcity2->common.id);
        }
      } trade_route_list_iterate_end;
    } city_list_iterate_end;
    connection_do_unbuffer(&aconnection);
  } else {
    city_list_iterate(get_player_ptr()->cities, pcity) {
      trade_route_list_iterate(pcity->common.trade_routes, ptr) {
        if ((include_in_route && ptr->status & TR_PLANNED)
            || (!include_in_route && ptr->status == TR_PLANNED)) {
          struct trade_route tr = *ptr;

          ptr->status = static_cast<trade_route_status>(ptr->status & ~TR_PLANNED);
          if (ptr->status == TR_NONE) {
            game_trade_route_remove(ptr);
          }
          update_trade_route_infos(&tr);
        }
      } trade_route_list_iterate_end;
    } city_list_iterate_end;
    if (draw_city_traderoutes) {
      update_map_canvas_visible(MUT_NORMAL);
    }
  }
}

/**************************************************************************
  Returns the calculation.
**************************************************************************/
struct trade_planning_calculation *get_trade_planning_calculation(void)
{
  return trade_planning_calc;
}

/**************************************************************************
  Trade planning calculation callback...
**************************************************************************/
static void trade_planning_destroyed(
    const struct trade_planning_calculation *pcalc, void *data)
{
  remove_trade_planning_calculation_resume_request();
  popdown_trade_planning_calculation_info();
  trade_planning_calc = NULL;
}

/**************************************************************************
  Trade planning calculation callback...
**************************************************************************/
static void trade_planning_apply(const struct trade_planning_calculation *pcalc,
                                 void *data)
{
  char buf[1024], message[1024];
  struct trade_route_list *trade_planning, *show_free_slot_arg = NULL;

  trade_planning = trade_planning_calculation_get_trade_routes(pcalc, message,
                                                               sizeof(message));

  if (trade_route_list_size(trade_planning) > 0) {
    if (server_has_extglobalinfo) {
      connection_do_buffer(&aconnection);
      clear_trade_planning(FALSE);
      trade_route_list_iterate(trade_planning, ptr) {
        dsend_packet_trade_route_plan(&aconnection,
                                      ptr->pcity1->common.id,
                                      ptr->pcity2->common.id);
      } trade_route_list_iterate_end;
      show_free_slot_arg = trade_planning;
      connection_do_unbuffer(&aconnection);
    } else {
      clear_trade_planning(FALSE);
      trade_route_list_iterate(trade_planning, ptr) {
        struct trade_route *ntr
                = game_trade_route_add(ptr->pcity1, ptr->pcity2);

        ntr->status = TR_PLANNED;
      } trade_route_list_iterate_end;
      if (draw_city_traderoutes) {
        update_map_canvas_visible(MUT_NORMAL);
      }
      show_free_slot_arg = NULL;
    }
  }

  my_snprintf(buf, sizeof(buf),
              _("Warclient: Trade planning calculation done: %s"), message);
  append_output_window(buf);
  show_free_slots_in_trade_planning(show_free_slot_arg);

  /* Free the datas */
  trade_route_list_iterate(trade_planning, ptr) {
    free(ptr);
  } trade_route_list_iterate_end;
  trade_route_list_free(trade_planning);
}

/**************************************************************************
  Calculate trade planning.
**************************************************************************/
void do_trade_planning_calculation(void)
{
  if (tile_list_size(trade_cities) <= 0
      || trade_planning_calc
      || !are_trade_cities_built()) {
    return;
  }

  struct city_list *clist = city_list_new();

  tile_list_iterate(trade_cities, ptile) {
    assert(NULL != ptile->city);
    city_list_append(clist, ptile->city);
  } tile_list_iterate_end;

  trade_planning_calc =
      trade_planning_calculation_new(get_player_ptr(), clist,
#ifndef ASYNC_TRADE_PLANNING
                                     trade_time_limit,
#endif  /* ASYNC_TRADE_PLANNING */
                                     trade_planning_destroyed,
                                     trade_planning_apply,
                                     NULL);
  if (trade_planning_calc) {
    trade_planning_calculation_resume(trade_planning_calc);
  }
  if (trade_planning_calc) {
    popup_trade_planning_calculation_info();
    request_trade_planning_calculation_resume();
  }

  city_list_free(clist);
}

/**************************************************************************
  Do a precalculation of the trade planning to check potential free slots.
**************************************************************************/
void do_trade_planning_precalculation(void)
{
  char buf[1024], message[1024];
  size_t size = tile_list_size(trade_cities);
  int free_slots[size], total, i;
  bool first = TRUE;

  total = trade_planning_precalculation(trade_cities, size, free_slots);
  if (total >= 0) {
    if (total == 0) {
      sz_strlcpy(message, "no free slot");
    } else {
      my_snprintf(message, sizeof(message),
                  PL_("%d free slot: ", "%d free slots: ", total), total);
      for (i = 0; i < size; i++) {
        if (free_slots[i] > 0) {
          cat_snprintf(message, sizeof(message), "%s%s (%d)",
                       first ? "" : ", ",
                       get_tile_info(tile_list_get(trade_cities, i)),
                       free_slots[i]);
          first = FALSE;
        }
      }
    }

    my_snprintf(buf, sizeof(buf),
                _("Warclient: Trade planning estimation: %s."), message);
    append_output_window(buf);
  }
}

/**************************************************************************
  Try to guess when the newt caravans will arrive.
**************************************************************************/
#define MAX_ESTIMATED_TURNS 30
void show_trade_estimation(void)
{
  if (client_is_global_observer()) {
    return;
  }

  char buf[MAX_ESTIMATED_TURNS][1024];
  char city1_name[MAX_LEN_NAME], city2_name[MAX_LEN_NAME];
  int i, count[MAX_ESTIMATED_TURNS];
  bool at_least_one = FALSE;

  /* Initialize */
  for (i = 0; i < MAX_ESTIMATED_TURNS; i++) {
    buf[i][0] = '\0';
    count[i] = 0;
  }

  /* Get datas */
  city_list_iterate(get_player_ptr()->cities, pcity) {
    trade_route_list_iterate(pcity->common.trade_routes, ptr) {
      if (ptr->status & TR_IN_ROUTE
          && ptr->pcity1->common.owner == get_player_idx()
          /* Don't estimate them twice */
          && ptr->pcity1 == pcity) {
        i = calculate_trade_move_turns(ptr);
        if (i >= MAX_ESTIMATED_TURNS) {
          continue;
        }
        insert_city_link(city1_name, sizeof(city1_name), ptr->pcity1);
        insert_city_link(city2_name, sizeof(city2_name), ptr->pcity2);
        cat_snprintf(buf[i], sizeof(buf[i]), "%s%s-%s",
                     count[i] > 0 ? ", " : "", city1_name, city2_name);
        count[i]++;
        at_least_one = TRUE;
      }
    } trade_route_list_iterate_end;
  } city_list_iterate_end;

  /* Print infos */
  if (at_least_one) {
    char text[1024];

    append_output_window(_("Warclient: Trade estimation:"));
    for(i = 0; i < MAX_ESTIMATED_TURNS; i++) {
      if (count[i] > 0) {
        my_snprintf(text, sizeof(text), "Warclient: %d %s - %d %s: %s.",
                    i, PL_("turn", "turns", i), count[i],
                    PL_("trade route", "trade routes", count[i]), buf[i]);
        append_output_window(text);
      }
    }
  } else {
    append_output_window(_("Warclient: No trade route to estimate."));
  }
}

/**************************************************************************
  Print the city list.
**************************************************************************/
void show_cities_in_trade_planning(void)
{
  if (tile_list_size(trade_cities) <= 0) {
    append_output_window(_("Warclient: No city in the trade city list."));
    return;
  }

  char buf[1024];
  bool first = TRUE;

  sz_strlcpy(buf, _("Warclient: Cities in trade planning: "));
  tile_list_iterate(trade_cities, ptile) {
    cat_snprintf(buf, sizeof(buf), "%s%s",
                 first ? "" : ", ", get_tile_info(ptile));
    first = FALSE;
  } tile_list_iterate_end;

  append_output_window(buf);
}

/**************************************************************************
  Return the futur number of trade routes.
**************************************************************************/
static int get_trade_route_num(city_t *pcity,
                               struct trade_route_list *ptrlist)
{
  int count = 0;

  /* Current trade routes */
  trade_route_list_iterate(pcity->common.trade_routes, ptr) {
    if (ptr->status > TR_PLANNED) {
      count++;
    }
  } trade_route_list_iterate_end;
  /* Futur trade routes */
  trade_route_list_iterate(ptrlist, ptr) {
    if (ptr->pcity1 == pcity || ptr->pcity2 == pcity) {
      count++;
    }
  } trade_route_list_iterate_end;

  return count;
}

/**************************************************************************
  Print the free slots list.
  If ptrlist == NULL, then print the current free slots list.
  Else, estimate the free slots, after the client sent infos to the server.
  We cannot use the current list, because we didn't receive yet the
  trade_route_info packets from the server.
**************************************************************************/
void show_free_slots_in_trade_planning(struct trade_route_list *ptrlist)
{
  city_t *pcity;
  char buf[1024] = "\0";
  int num = 0, missing;

  tile_list_iterate(trade_cities, ptile) {
    pcity = ptile->city;
    if (pcity) {
      missing = game.traderoute_info.maxtraderoutes
                - (ptrlist ? get_trade_route_num(pcity, ptrlist)
                           : trade_route_list_size(pcity->common.trade_routes));
    } else {
      missing = game.traderoute_info.maxtraderoutes;
    }
    if (missing > 0) {
      cat_snprintf(buf, sizeof(buf), "%s%s (%d)",
                   num > 0 ? ", " : "", get_tile_info(ptile), missing);
      num += missing;
    }
  } tile_list_iterate_end;

  if (num > 0) {
    char text[1024];

    my_snprintf(text, sizeof(text),
                PL_("Warclient: %d trade route free slot: %s.",
                    "Warclient: %d trade route free slots: %s.", num),
                num, buf);
    append_output_window(text);
  } else {
    append_output_window(_("Warclient: No trade free slot."));
  }
}

/**************************************************************************
  return TRUE if there is at least one city in the list.
**************************************************************************/
bool is_trade_city_list(void)
{
  return trade_cities ? tile_list_size(trade_cities) > 0 : FALSE;
}

/**************************************************************************
  return TRUE if there is at least one trade route planned.
**************************************************************************/
bool is_trade_planning(void)
{
  if (client_is_global_observer()) {
    return FALSE;
  }

  city_list_iterate(get_player_ptr()->cities, pcity) {
    trade_route_list_iterate(pcity->common.trade_routes, ptr) {
      if (ptr->status & TR_PLANNED) {
        return TRUE;
      }
    } trade_route_list_iterate_end;
  } city_list_iterate_end;

  return FALSE;
}

/**************************************************************************
  return TRUE if there is at least one trade route in route.
**************************************************************************/
bool is_trade_route_in_route(void)
{
  if (client_is_global_observer()) {
    return FALSE;
  }

  city_list_iterate(get_player_ptr()->cities, pcity) {
    trade_route_list_iterate(pcity->common.trade_routes, ptr) {
      if (ptr->status & TR_IN_ROUTE) {
        return TRUE;
      }
    } trade_route_list_iterate_end;
  } city_list_iterate_end;

  return FALSE;
}

/**************************************************************************
  return TRUE if all trade cities are built.
**************************************************************************/
bool are_trade_cities_built(void)
{
  tile_list_iterate(trade_cities, ptile) {
    if (!ptile->city) {
      return FALSE;
    }
  } tile_list_iterate_end;

  return TRUE;
}

/**************************************************************************
  Return the number of trade route in route of a city.
**************************************************************************/
int in_route_trade_route_number(city_t *pcity)
{
  int count = 0;

  trade_route_list_iterate(pcity->common.trade_routes, ptr) {
    if (ptr->status & TR_IN_ROUTE) {
      count++;
    }
  } trade_route_list_iterate_end;

  return count;
}

/**************************************************************************
  Draw all cities which are not built yet.
**************************************************************************/
void draw_non_built_trade_cities(void)
{
  int canvas_x, canvas_y;

  tile_list_iterate(trade_cities, ptile) {
    if (!ptile->city && tile_to_canvas_pos(&canvas_x, &canvas_y, ptile)) {
      canvas_put_sprite_full(mapview_canvas.store, canvas_x, canvas_y,
                             sprites.user.attention);
    }
  } tile_list_iterate_end;
}

/**************************************************************************
  Send the unit to establish trade routes.
**************************************************************************/
void request_unit_trade_route(unit_t *punit, city_t *pcity)
{
  if (!can_client_issue_orders()
      || !punit || !unit_flag(punit, F_TRADE_ROUTE)) {
    return;
  }

  struct trade_route *ptr = NULL;
  city_t *phome_city;
  unit_t *ounit;
  int moves, turns;
  char buf[1024], city1_name[MAX_LEN_NAME], city2_name[MAX_LEN_NAME];

  if (pcity) {
    if (!(phome_city = player_find_city_by_id(get_player_ptr(),
                                              punit->homecity))) {
      my_snprintf(buf, sizeof(buf), _("Warclient: A caravan must have a "
                                      "homecity to establish a trade route."));
      append_output_window(buf);
      return;
    }

    insert_city_link(city1_name, sizeof(city1_name), phome_city);
    insert_city_link(city2_name, sizeof(city2_name), pcity);

    if (!can_cities_trade(phome_city, pcity)
        || !can_establish_trade_route(phome_city, pcity)) {
      my_snprintf(buf, sizeof(buf),
                  _("Warclient: You cannot create a trade "
                    "route between %s and %s."),
                  city1_name, city2_name);
      append_output_window(buf);
      return;
    }

    if ((ptr = game_trade_route_find(phome_city, pcity))) {
      if (ptr->status & TR_IN_ROUTE) {
        my_snprintf(buf, sizeof(buf),
                    _("Warclient: The trade route between %s and %s "
                      "is already going to be established."),
                    city1_name, city2_name);
        append_output_window(buf);
        return;
      }
    } else {
      if (trade_route_list_size(phome_city->common.trade_routes)
            >= game.traderoute_info.maxtraderoutes
          || trade_route_list_size(pcity->common.trade_routes)
               >= game.traderoute_info.maxtraderoutes) {
        my_snprintf(buf, sizeof(buf),
                    _("Warclient: Warning: the trade route "
                      "between %s and %s was not planned."),
                    city1_name, city2_name);
        append_output_window(buf);
      }
      ptr = game_trade_route_add(phome_city, pcity);
    }
  } else {
    /* Auto */
    ptr = get_next_trade_route_to_establish(punit, trade_mode_best_values,
                                            trade_mode_allow_free_other,
                                            trade_mode_internal_first,
                                            trade_mode_homecity_first);
  }

  if (!ptr) {
    append_output_window(_("Warclient: Didn't find any trade route "
                           "to establish"));
    return;
  }

  insert_city_link(city1_name, sizeof(city1_name), ptr->pcity1);
  insert_city_link(city2_name, sizeof(city2_name), ptr->pcity2);

  if ((ounit = ptr->punit)) {
    /* Free it */
    ounit->ptr = NULL;
    request_orders_cleared(ounit);
    request_new_unit_activity(ounit, ACTIVITY_IDLE);
    my_snprintf(buf, sizeof(buf),
                _("Warclient: The %s id %d which was going to establish a "
                  "trade route between %s and %s, has been stopped."),
                unit_name(ounit->type), ounit->id, city1_name, city2_name);
    append_output_window(buf);
  }

  /* Do it, waiting all units have been allocated */
  ptr->punit = punit;
  if (ptr->status & TR_IN_ROUTE) {
    ptr->status = static_cast<trade_route_status>(ptr->status ^ TR_IN_ROUTE);
  }

  turns = calculate_trade_move_turns(ptr);
  moves = ptr->move_cost; /* Should have been updated */
  my_snprintf(buf, sizeof(buf),
              _("Warclient: Sending the %s id %d to establish a "
                "trade route between %s and %s (%d %s, %d %s)."),
              unit_name(punit->type), punit->id, city1_name, city2_name,
              moves, PL_("move", "moves", moves),
              turns, PL_("turn", "turns", turns));
  append_output_window(buf);

  if (ounit) {
    /* Alloc a trade route for this stopped unit */
    request_unit_trade_route(ounit, NULL);
  }
}

/**************************************************************************
  Send the units to establish trade routes.
**************************************************************************/
void request_trade_route(city_t *pcity)
{
  if (!can_client_issue_orders()) {
    return;
  }

  /* Request trade routes */
  multi_select_iterate(TRUE, punit) {
    request_unit_trade_route(punit, pcity);
  } multi_select_iterate_end;

  /* Send infos to the server */
  connection_do_buffer(&aconnection);
  if (server_has_extglobalinfo) {
    city_list_iterate(get_player_ptr()->cities, pcity) {
      trade_route_list_iterate(pcity->common.trade_routes, ptr) {
        if (ptr->status <= TR_PLANNED && ptr->punit) {
          dsend_packet_unit_trade_route(&aconnection, ptr->punit->id,
                                        ptr->pcity1->common.id,
                                        ptr->pcity2->common.id);
          ptr->punit = NULL;
        }
      } trade_route_list_iterate_end;
    } city_list_iterate_end;
  } else {
    city_list_iterate(get_player_ptr()->cities, pcity) {
      trade_route_list_iterate(pcity->common.trade_routes, ptr) {
        if (ptr->status <= TR_PLANNED && ptr->punit) {
          ptr->status = static_cast<trade_route_status>(ptr->status | TR_IN_ROUTE);
          ptr->punit->ptr = ptr;

          execute_trade_orders(ptr->punit);
          update_trade_route_infos(ptr);
        }
      } trade_route_list_iterate_end;
    } city_list_iterate_end;
    /* Show changes */
    if (draw_city_traderoutes) {
      update_map_canvas_visible(MUT_NORMAL);
    }
  }
  connection_do_unbuffer(&aconnection);
}

/**************************************************************************
  Free the trade orders of this unit.
  Retruns TRUE if the trade route was removed, else, returns FALSE.
**************************************************************************/
void request_cancel_trade_route(city_t *pcity1, city_t *pcity2)
{
  if (!can_client_issue_orders()) {
    return;
  }

  struct trade_route tr, *ptr = game_trade_route_find(pcity1, pcity2);

  if (!ptr || ptr->pcity1->common.owner != get_player_idx()) {
    return;
  }

  if (server_has_extglobalinfo) {
    dsend_packet_trade_route_remove(&aconnection,
                                    ptr->pcity1->common.id,
                                    ptr->pcity2->common.id);
  } else if (!ptr->punit || !trade_free_unit(ptr->punit)) {
    tr = *ptr;
    game_trade_route_remove(ptr);
    update_trade_route_infos(&tr);
    update_trade_route_line(&tr);
  }
}

/**************************************************************************
  Execute auto-caravan orders when we are connected to a server without the
  extglobalinfo capability.
**************************************************************************/
void execute_trade_orders(unit_t *punit)
{
  if (!punit || !punit->ptr || punit->ptr->punit != punit) {
    return;
  }

  if (punit->homecity != punit->ptr->pcity1->common.id) {
    if (punit->tile != punit->ptr->pcity1->common.tile) {
      unit_goto(punit, punit->ptr->pcity1->common.tile);
      return;
    } else {
      dsend_packet_unit_change_homecity(&aconnection, punit->id,
                                        punit->ptr->pcity1->common.id);
    }
  }
  if (punit->tile != punit->ptr->pcity2->common.tile) {
    unit_goto(punit, punit->ptr->pcity2->common.tile);
  } else {
    struct toggle_worker_list *plist;

    connection_do_buffer(&aconnection);
    plist = apply_trade_workers(punit->ptr->pcity1, punit->ptr->pcity2);
    dsend_packet_unit_establish_trade(&aconnection, punit->id);
    release_trade_workers(plist);
    connection_do_unbuffer(&aconnection);
  }
}

/**************************************************************************
  Free the trade orders of this unit.
  Retruns TRUE if the trade route was removed, else, returns FALSE.
**************************************************************************/
bool trade_free_unit(unit_t *punit)
{
  if (server_has_extglobalinfo || !punit->ptr) {
    return FALSE;
  }

  bool ret = FALSE;
  struct trade_route tr;

  request_orders_cleared(punit);
  request_new_unit_activity(punit, ACTIVITY_IDLE);

  if (punit->ptr->status == TR_IN_ROUTE) {
    tr = *punit->ptr;
    game_trade_route_remove(punit->ptr);
    update_trade_route_infos(&tr);
    update_trade_route_line(&tr);
    ret = TRUE;
  } else if (punit->ptr->status == TR_PL_AND_IR) {
    punit->ptr->status = TR_PLANNED;
    punit->ptr->punit = NULL;
    update_trade_route_infos(punit->ptr);
    update_trade_route_line(punit->ptr);
  }

  punit->ptr = NULL;

  return ret;
}



/**************************************************************************
  Code to toggle workers before establishing a new trade route. This is
  not used in new warservers, because the maximum value should be reached
  in a such case.
**************************************************************************/
struct worked_tile {
  tile_t *ptile; /* A pointer to the tile. */
  int cx, cy;         /* Coordonates for the city map. */
  int trade_value;    /* The trade value of the tile. */
  bool used;          /* TRUE when the tile is used */
};

struct toggle_city {
  city_t *pcity;    /* The city */
  int importance;     /* Importance of switching tiles for it */
  struct worked_tile tiles[CITY_MAP_SIZE * CITY_MAP_SIZE];
  size_t tiles_num;
};

struct toggle_worker {
  city_t *pcity;    /* The city */
  int cx, cy;         /* Coordonates for the city map */
  bool was_used;      /* Initial configuration */
};

#define SPECLIST_TAG toggle_worker
#define SPECLIST_TYPE struct toggle_worker
#include "speclist.hh"

/* Recursive iteration. */
#define toggle_worker_list_iterate(list, ptw) \
  TYPED_LIST_ITERATE_REV(struct toggle_worker, list, ptw)
#define toggle_worker_list_iterate_end LIST_ITERATE_REV_END

/**************************************************************************
  Toggle a worker for the city. Ask the server to make the change.
**************************************************************************/
static struct toggle_worker *toggle_worker_new(city_t *pcity,
                                               int cx, int cy)
{
  struct toggle_worker *ptw = static_cast<struct toggle_worker *>(
      wc_malloc(sizeof(struct toggle_worker)));

  ptw->pcity = pcity;
  ptw->cx = cx;
  ptw->cy = cy;
  ptw->was_used = pcity->common.city_map[cx][cy] == C_TILE_WORKER;

  freelog(LOG_VERBOSE, "Apply: Toggling tile (%d, %d) to %s for %s",
          TILE_XY(city_map_to_map(pcity, cx, cy)),
          ptw->was_used ? "specialist" : "worker", pcity->common.name);

  if (ptw->was_used) {
    dsend_packet_city_make_specialist(&aconnection, pcity->common.id, cx, cy);
  } else {
    dsend_packet_city_make_worker(&aconnection, pcity->common.id, cx, cy);
  }

  return ptw;
}

/**************************************************************************
  Toggle a worker for the city. Ask the server to make the change.
**************************************************************************/
static void toggle_worker_free(struct toggle_worker *ptw)
{
  freelog(LOG_VERBOSE, "Release: Toggling tile (%d, %d) to %s for %s",
          TILE_XY(city_map_to_map(ptw->pcity, ptw->cx, ptw->cy)),
          ptw->was_used ? "worker" : "specialist", ptw->pcity->common.name);

  if (ptw->was_used) {
    dsend_packet_city_make_worker(&aconnection, ptw->pcity->common.id,
                                  ptw->cx, ptw->cy);
  } else {
    dsend_packet_city_make_specialist(&aconnection, ptw->pcity->common.id,
                                      ptw->cx, ptw->cy);
  }

  free(ptw);
}

/**************************************************************************
  ...
**************************************************************************/
static void sort_cities(struct toggle_city *cities, size_t size)
{
  int i, j;

  for (i = 0; i < size; i++) {
    for (j = 0; j < i; j++) {
      if (cities[i].importance > cities[j].importance) {
        struct toggle_city tc = cities[i];
        cities[i] = cities[j];
        cities[j] = tc;
      }
    }
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void drop_worked_tile(struct toggle_city *tcity, size_t tile)
{
  assert(tcity->tiles_num > 0);
  assert(tile < tcity->tiles_num);
  if (tile != tcity->tiles_num - 1) {
    memmove(&tcity->tiles[tile], &tcity->tiles[tile + 1],
            (tcity->tiles_num - tile - 1) * sizeof(struct worked_tile));
  }
  tcity->tiles_num--;
}

/**************************************************************************
  Returns TRUE if any city can still use this tile.
**************************************************************************/
static bool can_steal_tile_to_cities(struct toggle_city *cities, size_t size,
                                     tile_t *ptile)
{
  struct toggle_city *tcity;
  int i, j;

  for (i = 0; i < size; i++) {
    tcity = &cities[i];
    if (tcity->pcity->common.tile == ptile) {
      return FALSE;
    }
    for (j = 0; j < tcity->tiles_num && j < tcity->pcity->common.pop_size; j++) {
      if (tcity->tiles[j].ptile == ptile) {
        if (tcity->pcity->common.pop_size < tcity->tiles_num
            && tcity->tiles[tcity->pcity->common.pop_size - 1].trade_value
               == tcity->tiles[tcity->pcity->common.pop_size].trade_value) {
          /* We can use an other tile. */
          drop_worked_tile(tcity, j);
          return TRUE;
        } else {
          /* This tile is more important for this city. */
          return FALSE;
        }
      }
    }
  }
  /* No city requested it. */
  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static int best_trade(const void *a, const void *b)
{
  const struct worked_tile *pwta, *pwtb;

  pwta = (const struct worked_tile *)a;
  pwtb = (const struct worked_tile *)b;

  return pwta->trade_value < pwtb->trade_value
         || (pwta->trade_value == pwtb->trade_value && !pwta->used);

}

/**************************************************************************
  ...
**************************************************************************/
static void fill_worked_tiles(struct toggle_city *cities,
                              size_t size, size_t vec)
{
  struct toggle_city *tcity = &cities[vec];
  city_t *pcity = tcity->pcity;
  bool is_celebrating = base_city_celebrating(pcity);
  struct worked_tile *pwtile;
  int i;

  assert(vec < size);

  /* Get informations. */
  tcity->tiles_num = 0;
  city_map_checked_iterate(pcity->common.tile, cx, cy, ptile) {
    if (ptile == pcity->common.tile) {
      continue;
    }
    if (ptile->worked
        && ptile->worked->common.owner != pcity->common.owner) {
      continue;
    }
    if (!can_steal_tile_to_cities(cities, vec, ptile)) {
      /* Check if we can use this tile form the previous cities. */
      continue;
    }

    pwtile = &tcity->tiles[tcity->tiles_num++];
    pwtile->ptile = ptile;
    pwtile->cx = cx;
    pwtile->cy = cy;
    pwtile->trade_value =
        base_city_get_trade_tile(cx, cy, pcity, is_celebrating);
    pwtile->used = pcity->common.city_map[cx][cy] == C_TILE_WORKER;
  } city_map_checked_iterate_end;

  /* Sort them */
  qsort(tcity->tiles, tcity->tiles_num, sizeof(*tcity->tiles), best_trade);

  i = 0;
  while (i < tcity->tiles_num && i < pcity->common.pop_size) {
    if (!can_steal_tile_to_cities(cities, vec, tcity->tiles[i].ptile)) {
      drop_worked_tile(tcity, i);
    } else {
      i++;
    }
  }
}

/**************************************************************************
  ...
**************************************************************************/
static bool city_is_in_list(struct toggle_city *cities, size_t size,
                            city_t *pcity)
{
  int i;

  for (i = 0; i < size; i++) {
    if (cities[i].pcity == pcity) {
      return TRUE;
    }
  }
  return FALSE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool tile_is_in_list(struct worked_tile *tiles, size_t size,
                            int cx, int cy)
{
  int i;

  for (i = 0; i < size; i++) {
    if (tiles[i].cx == cx && tiles[i].cy == cy) {
      return TRUE;
    }
  }
  return FALSE;
}

/**************************************************************************
  Remove workers.
**************************************************************************/
static void city_set_specialists(struct toggle_city *cities, size_t size,
                                 size_t vec, struct toggle_worker_list *plist)
{
  struct toggle_city *tcity = &cities[vec];
  city_t *pcity = tcity->pcity;
  tile_t *ptile;
  int i, cx, cy;

  /* Check tiles used by others cities. */
  for (i = 0; i < tcity->tiles_num && i < tcity->pcity->common.pop_size; i++) {
    ptile = tcity->tiles[i].ptile;

    if (ptile->worked && ptile->worked != pcity) {
      if (!city_is_in_list(cities, size, ptile->worked)
          && map_to_city_map(&cx, &cy, ptile->worked, ptile)) {
        toggle_worker_list_append(plist,
                                  toggle_worker_new(ptile->worked, cx, cy));
      }
    }
  }

  /* Remove workers from tiles we won't use. */
  city_map_iterate(cx, cy) {
    if ((cx != CITY_MAP_RADIUS || cy != CITY_MAP_RADIUS)
        && pcity->common.city_map[cx][cy] == C_TILE_WORKER
        && !tile_is_in_list(tcity->tiles,
                            MIN(tcity->tiles_num, tcity->pcity->common.pop_size),
                            cx, cy))
    {
      toggle_worker_list_append(plist, toggle_worker_new(pcity, cx, cy));
    }
  } city_map_iterate_end;
}

/**************************************************************************
  Set new workers.
**************************************************************************/
static void city_set_workers(struct toggle_city *tcity,
                             struct toggle_worker_list *plist)
{
  city_t *pcity = tcity->pcity;
  struct worked_tile *pwtile;
  int i;

  for (i = 0; i < tcity->tiles_num && i < tcity->pcity->common.pop_size; i++) {
    pwtile = &tcity->tiles[i];
    if (pcity->common.city_map[pwtile->cx][pwtile->cy] != C_TILE_WORKER) {
      toggle_worker_list_append(plist, toggle_worker_new(pcity, pwtile->cx,
                                                         pwtile->cy));
    }
  }
}

/**************************************************************************
  Choose the best trade tiles for the cities.
**************************************************************************/
static struct toggle_worker_list *apply_trade_workers(city_t *pcity1,
                                                      city_t *pcity2)
{
  size_t size = 2 * (1 + game.traderoute_info.maxtraderoutes);
  struct toggle_city cities[size];
  struct toggle_worker_list *plist = toggle_worker_list_new();
  city_t *ocity;
  int i = 0, player_idx = get_player_idx();

  assert(server_has_extglobalinfo == FALSE);

  /* Initialize */
  if (pcity1->common.owner == player_idx) {
    cities[i].pcity = pcity1;
    cities[i].importance = 3;
    i++;
  }

  if (pcity2->common.owner == player_idx) {
    cities[i].pcity = pcity2;
    cities[i].importance = 3;
    i++;
  }

  established_trade_routes_iterate(pcity1, ptr) {
    ocity = OTHER_CITY(ptr, pcity1);
    if (ocity->common.owner != player_idx) {
      continue;
    }

    cities[i].pcity = ocity;
    cities[i].importance = have_cities_trade_route(pcity2, ocity) ? 2 : 1;
    i++;
  } established_trade_routes_iterate_end;

  established_trade_routes_iterate(pcity2, ptr) {
    ocity = OTHER_CITY(ptr, pcity1);
    if (have_cities_trade_route(pcity1, ocity)
        || ocity->common.owner != player_idx) {
      continue;
    }

    cities[i].pcity = ocity;
    cities[i].importance = 1;
    i++;
  } established_trade_routes_iterate_end;

  assert(i <= size);
  size = i;
  sort_cities(cities, size);

  /* Find best configuration */
  for (i = 0; i < size; i++) {
    fill_worked_tiles(cities, size, i);
  }

  /* Make the list */
  for (i = 0; i < size; i++) {
    city_set_specialists(cities, size, i, plist);
  }
  for (i = 0; i < size; i++) {
    city_set_workers(&cities[i], plist);
  }

  return plist;
}

/**************************************************************************
  Toggle back the toggled workers.
**************************************************************************/
static void release_trade_workers(struct toggle_worker_list *plist)
{
  assert(server_has_extglobalinfo == FALSE);

  if (!plist) {
    return;
  }

  toggle_worker_list_iterate(plist, ptw) {
    toggle_worker_free(ptw);
  } toggle_worker_list_iterate_end;
  toggle_worker_list_free(plist);
}



/**************************************************************************
  Add a trade route which will be added when all cities will be created.
**************************************************************************/
void delayed_trade_routes_add(int city, int trade[OLD_NUM_TRADEROUTES],
                              int trade_value[OLD_NUM_TRADEROUTES])
{
  struct delayed_trade_route *pdtr;
  int i;

  assert(server_has_extglobalinfo == FALSE);

  if (!delayed_trade_routes) {
    delayed_trade_routes = delayed_trade_route_list_new();
  }

  for (i = 0; i < OLD_NUM_TRADEROUTES; i++) {
    if (trade[i] != 0) {
      pdtr = static_cast<delayed_trade_route*>(
          wc_malloc(sizeof(struct delayed_trade_route)));

      pdtr->city1 = city;
      pdtr->city2 = trade[i];
      pdtr->value = trade_value[i];

      delayed_trade_route_list_append(delayed_trade_routes, pdtr);
    }
  }
}

/**************************************************************************
  Build all delayed trade routes we put on the stack.
**************************************************************************/
void delayed_trade_routes_build(void)
{
  if (!delayed_trade_routes) {
    return;
  }

  assert(server_has_extglobalinfo == FALSE);

  delayed_trade_routes_iterate(pdtr) {
    city_t *pcity1, *pcity2;
    struct trade_route *ptr;

    if (!(pcity1 = find_city_by_id(pdtr->city1))
        || !(pcity2 = find_city_by_id(pdtr->city2))) {
      freelog(LOG_ERROR, "Got a trade route with an unknown city.");
      continue;
    }

    ptr = game_trade_route_find(pcity1, pcity2);
    if (!ptr) {
      ptr = game_trade_route_add(pcity1, pcity2);
    }

    ptr->status = TR_ESTABLISHED;
    ptr->value = pdtr->value;

    free(pdtr);
  } delayed_trade_routes_iterate_end;

  delayed_trade_route_list_free(delayed_trade_routes);
  delayed_trade_routes = NULL;
}
