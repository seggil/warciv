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
#include <config.h>
#endif

#include "capability.h"
#include "city.h"
#include "civclient.h"
#include "connection.h"
#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "packhand_gen.h"
#include "player.h"
#include "support.h"
#include "traderoute.h"
#include "unit.h"

#include "chatline_common.h" /* append_output_window()      */
#include "citydlg_g.h"       /* refresh_city_dialog()       */
#include "clinet.h"          /* aconnection                 */
#include "control.h"         /* request_new_unit_activity() */
#include "dialogs_g.h"
#include "goto.h"            /* request_orders_cleared()    */
#include "mapview_common.h"  /* update_map_canvas_visible() */
#include "multiselect.h"     /* multi_select_iterate()      */
#include "menu_g.h"          /* update_auto_caravan_menu()  */
#include "options.h"

#include "trade.h"

struct delayed_trade_route {
  int city1;
  int city2;
  int value;
};

#define SPECLIST_TAG delayed_trade_route
#define SPECLIST_TYPE struct delayed_trade_route
#include "speclist.h"

#define delayed_trade_routes_iterate(pdtr) \
  TYPED_LIST_ITERATE(struct delayed_trade_route, delayed_trade_routes, pdtr)
#define delayed_trade_routes_iterate_end LIST_ITERATE_END

static struct city_list *trade_cities = NULL;
static struct trade_planning_calculation *trade_planning_calc = NULL;
static struct delayed_trade_route_list *delayed_trade_routes = NULL;

void request_unit_trade_route(struct unit *punit, struct city *pcity);

/**************************************************************************
  ...
**************************************************************************/
void trade_init(void)
{
  if (!trade_cities) {
    trade_cities = city_list_new();
  }
}

/**************************************************************************
  ...
**************************************************************************/
void trade_free(void)
{
  if (trade_cities) {
    city_list_free(trade_cities);
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
    refresh_city_dialog(ptr->pcity1);
    refresh_city_dialog(ptr->pcity2);
  }
  update_auto_caravan_menu();
}

/**************************************************************************
  Receive trade packet from the server
**************************************************************************/
void handle_trade_route_info(struct packet_trade_route_info *packet)
{
  struct city *pcity1, *pcity2;
  struct trade_route *ptr;
  struct unit *pold_unit;

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
          pcity1->name, pcity2->name, packet->status);

  pold_unit = ptr->punit;
  ptr->pcity1 = pcity1; /* pcity1 and pcity2 could have been swapped */
  ptr->pcity2 = pcity2; /* pcity1 and pcity2 could have been swapped */
  ptr->punit = find_unit_by_id(packet->unit_id); /* find_unit_by_id(0) = NULL */
  ptr->status = packet->status;

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
void handle_trade_route_remove(int city1, int city2)
{
  struct city *pcity1, *pcity2;
  struct trade_route *ptr;

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
          pcity1->name, pcity2->name);

  if (ptr->punit) {
    ptr->punit->ptr = NULL;
  }

  update_trade_route_line(ptr);
  game_trade_route_remove(ptr);

  refresh_city_dialog(pcity1);
  refresh_city_dialog(pcity2);
  update_auto_caravan_menu();
}

/**************************************************************************
  Add a city in the trade city list.
**************************************************************************/
void add_city_in_trade_planning(struct city *pcity, bool allow_remove)
{
  if (pcity->owner != get_player_idx()) {
    return;
  }

  char buf[256];

  if (city_list_search(trade_cities, pcity)) {
    if (allow_remove) {
      city_list_unlink(trade_cities, pcity);
      my_snprintf(buf, sizeof(buf),
		  _("Warclient: Removing %s to the trade planning."),
		  pcity->name);
      append_output_window(buf);
      update_auto_caravan_menu();
    }
  } else {
    city_list_append(trade_cities, pcity);
    my_snprintf(buf, sizeof(buf),
		_("Warclient: Adding %s to the trade planning."), pcity->name);
    append_output_window(buf);
    if (allow_remove) {
      update_auto_caravan_menu();
    }
  }
}

/**************************************************************************
  Called when a city is removed.
**************************************************************************/
void trade_remove_city(struct city *pcity)
{
  if (aconnection.established && city_list_search(trade_cities, pcity)) {
    add_city_in_trade_planning(pcity, TRUE); /* = Remove */
  } else {
    /* Don't print any message, don't update menus */
    city_list_unlink(trade_cities, pcity);
  }
  /* Free the trade routes */
  trade_route_list_iterate(pcity->trade_routes, ptr) {
    if (server_has_extglobalinfo
	|| !ptr->punit || !trade_free_unit(ptr->punit)) {
      game_trade_route_remove(ptr);
    }
  } trade_route_list_iterate_end;
}

/**************************************************************************
  Clear the city list.
**************************************************************************/
void clear_trade_city_list(void)
{
  city_list_unlink_all(trade_cities);
  append_output_window(_("Warclient: Trade city list cleared."));
  update_auto_caravan_menu();
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

  struct city_list *plist = city_list_size(trade_cities) > 0
			    ? trade_cities : get_player_ptr()->cities;

  if (server_has_extglobalinfo) {
    connection_do_buffer(&aconnection);
    city_list_iterate(plist, pcity) {
      trade_route_list_iterate(pcity->trade_routes, ptr) {
        struct city *pother_city = OTHER_CITY(ptr, pcity);

        /* Try to don't send the packet twice */
        if (((include_in_route && ptr->status & TR_PLANNED)
	     || (!include_in_route && ptr->status == TR_PLANNED))
	    && (pcity->owner != pother_city->owner
		|| pcity->id < pother_city->id
		|| !city_list_search(plist, pother_city))) {
          dsend_packet_trade_route_remove(&aconnection, ptr->pcity1->id,
                                          ptr->pcity2->id);
        }
      } trade_route_list_iterate_end;
    } city_list_iterate_end;
    connection_do_unbuffer(&aconnection);
  } else {
    city_list_iterate(plist, pcity) {
      trade_route_list_iterate(pcity->trade_routes, ptr) {
        if ((include_in_route && ptr->status & TR_PLANNED)
	    || (!include_in_route && ptr->status == TR_PLANNED)) {
	  struct trade_route tr = *ptr;

	  ptr->status &= ~TR_PLANNED;
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
        dsend_packet_trade_route_plan(&aconnection, ptr->pcity1->id,
                                      ptr->pcity2->id);
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
  if (city_list_size(trade_cities) <= 0 || trade_planning_calc) {
    return;
  }

  trade_planning_calc =
      trade_planning_calculation_new(get_player_ptr(), trade_cities,
#ifndef ASYNC_TRADE_PLANNING
				     trade_time_limit,
#endif	/* ASYNC_TRADE_PLANNING */
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
}

/**************************************************************************
  Try to guess when the newt caravans will arrive.
**************************************************************************/
#define MAX_ESTIMATED_TURNS 30
void show_trade_estimation(void)
{
  if (!get_player_ptr()) {
    return;
  }

  char buf[MAX_ESTIMATED_TURNS][1024];
  int i, count[MAX_ESTIMATED_TURNS];
  bool at_least_one = FALSE;

  /* Initialize */
  for (i = 0; i < MAX_ESTIMATED_TURNS; i++) {
    buf[i][0] = '\0';
    count[i] = 0;
  }

  /* Get datas */
  city_list_iterate(get_player_ptr()->cities, pcity) {
    trade_route_list_iterate(pcity->trade_routes, ptr) {
      if (ptr->status & TR_IN_ROUTE
	  && ptr->pcity1->owner == get_player_idx()
          && ptr->pcity1 == pcity) {
        /* Don't estimate them twice */
        i = calculate_trade_move_turns(ptr);
        if (i >= MAX_ESTIMATED_TURNS) {
          continue;
        }
        cat_snprintf(buf[i], sizeof(buf[i]), "%s%s-%s",
                     count[i] > 0 ? ", " : "",
                     ptr->pcity1->name, ptr->pcity2->name);
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
  if(city_list_size(trade_cities) <= 0) {
    append_output_window(_("Warclient: No city in the trade city list."));
    return;
  }

  char buf[1024];
  bool first = TRUE;

  sz_strlcpy(buf, _("Warclient: Cities in trade planning: "));
  city_list_iterate(trade_cities, pcity) {
    cat_snprintf(buf, sizeof(buf), "%s%s", first ? "" : ", ", pcity->name);
    first = FALSE;
  } city_list_iterate_end;

  append_output_window(buf);
}

/**************************************************************************
  Return the futur number of trade routes.
**************************************************************************/
static int get_trade_route_num(struct city *pcity,
                               struct trade_route_list *ptrlist)
{
  int count = 0;

  /* Current trade routes */
  trade_route_list_iterate(pcity->trade_routes, ptr) {
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
  char buf[1024] = "\0";
  int num = 0, missing;

  city_list_iterate(trade_cities, pcity) {
    missing = game.traderoute_info.maxtraderoutes
              - (ptrlist ? get_trade_route_num(pcity, ptrlist)
                         : trade_route_list_size(pcity->trade_routes));
    if (missing > 0) {
      cat_snprintf(buf, sizeof(buf), "%s%s (%d)",
                   num > 0 ? ", " : "", pcity->name, missing);
      num += missing;
    }
  } city_list_iterate_end;

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
  return trade_cities ? city_list_size(trade_cities) > 0 : FALSE;
}

/**************************************************************************
  return TRUE if there is at least one trade route planned.
**************************************************************************/
bool is_trade_planning(void)
{
  if (!get_player_ptr()) {
    return FALSE;
  }

  city_list_iterate(get_player_ptr()->cities, pcity) {
    trade_route_list_iterate(pcity->trade_routes, ptr) {
      if (ptr->status == TR_PLANNED) {
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
  if (!get_player_ptr()) {
    return FALSE;
  }

  city_list_iterate(get_player_ptr()->cities, pcity) {
    trade_route_list_iterate(pcity->trade_routes, ptr) {
      if (ptr->status & TR_IN_ROUTE) {
        return TRUE;
      }
    } trade_route_list_iterate_end;
  } city_list_iterate_end;

  return FALSE;
}

/**************************************************************************
  Return the number of trade route in route of a city.
**************************************************************************/
int in_route_trade_route_number(struct city *pcity)
{
  int count = 0;

  trade_route_list_iterate(pcity->trade_routes, ptr) {
    if (ptr->status & TR_IN_ROUTE) {
      count++;
    }
  } trade_route_list_iterate_end;

  return count;
}

/**************************************************************************
  Send the unit to establish trade routes.
**************************************************************************/
void request_unit_trade_route(struct unit *punit, struct city *pcity)
{
  if (!can_client_issue_orders()
      || !punit || !unit_flag(punit, F_TRADE_ROUTE)) {
    return;
  }

  struct trade_route *ptr = NULL;
  struct city *phome_city;
  struct unit *ounit;
  int moves, turns;
  char buf[1024];

  if (pcity) {
    if (!(phome_city = player_find_city_by_id(get_player_ptr(),
                                              punit->homecity))) {
      my_snprintf(buf, sizeof(buf), _("Warclient: A caravan must have a "
				      "homecity to establish a trade route."));
      append_output_window(buf);
      return;
    }

    if (!can_cities_trade(phome_city, pcity)
        || !can_establish_trade_route(phome_city, pcity)) {
      my_snprintf(buf, sizeof(buf),
		  _("Warclient: You cannot create a trade "
		    "route between %s and %s."),
		  phome_city->name, pcity->name);
      append_output_window(buf);
      return;
    }

    if ((ptr = game_trade_route_find(phome_city, pcity))) {
      if (ptr->status & TR_IN_ROUTE) {
	my_snprintf(buf, sizeof(buf),
		    _("Warclient: The trade route between %s and %s "
		      "is already going to be established."),
		    phome_city->name, pcity->name);
	append_output_window(buf);
        return;
      }
    } else {
      if (trade_route_list_size(phome_city->trade_routes)
            >= game.traderoute_info.maxtraderoutes
          || trade_route_list_size(pcity->trade_routes)
               >= game.traderoute_info.maxtraderoutes) {
	my_snprintf(buf, sizeof(buf),
		    _("Warclient: Warning: the trade route "
		      "between %s and %s was not planned."),
		    phome_city->name, pcity->name);
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

  if ((ounit = ptr->punit)) {
    /* Free it */
    ounit->ptr = NULL;
    request_orders_cleared(ounit);
    request_new_unit_activity(ounit, ACTIVITY_IDLE);
    my_snprintf(buf, sizeof(buf),
		_("Warclient: The %s id %d which was going to establish a "
		  "trade route between %s and %s, has been stopped."),
		unit_name(ounit->type), ounit->id,
		ptr->pcity1->name, ptr->pcity2->name);
    append_output_window(buf);
  }

  /* Do it, waiting all units have been allocated */
  ptr->punit = punit;
  if (ptr->status & TR_IN_ROUTE) {
    ptr->status ^= TR_IN_ROUTE;
  }

  turns = calculate_trade_move_turns(ptr);
  moves = ptr->move_cost; /* Should have been updated */
  my_snprintf(buf, sizeof(buf),
	      _("Warclient: Sending the %s id %d to establish a "
		"trade route between %s and %s (%d %s, %d %s)."),
	      unit_name(punit->type), punit->id,
	      ptr->pcity1->name, ptr->pcity2->name,
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
void request_trade_route(struct city *pcity)
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
      trade_route_list_iterate(pcity->trade_routes, ptr) {
        if (ptr->status <= TR_PLANNED && ptr->punit) {
          dsend_packet_unit_trade_route(&aconnection, ptr->punit->id,
                                        ptr->pcity1->id, ptr->pcity2->id);
          ptr->punit = NULL;
        }
      } trade_route_list_iterate_end;
    } city_list_iterate_end;
  } else {
    city_list_iterate(get_player_ptr()->cities, pcity) {
      trade_route_list_iterate(pcity->trade_routes, ptr) {
        if (ptr->status <= TR_PLANNED && ptr->punit) {
          ptr->status |= TR_IN_ROUTE;
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
void request_cancel_trade_route(struct city *pcity1, struct city *pcity2)
{
  if (!can_client_issue_orders()) {
    return;
  }

  struct trade_route tr, *ptr = game_trade_route_find(pcity1, pcity2);

  if (!ptr || ptr->pcity1->owner != get_player_idx()) {
    return;
  }

  if (server_has_extglobalinfo) {
    dsend_packet_trade_route_remove(&aconnection,
                                    ptr->pcity1->id, ptr->pcity2->id);
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
void execute_trade_orders(struct unit *punit)
{
  if (!punit || !punit->ptr || punit->ptr->punit != punit) {
    return;
  }

  if (punit->homecity != punit->ptr->pcity1->id) {
    if (punit->tile != punit->ptr->pcity1->tile) {
      unit_goto(punit, punit->ptr->pcity1->tile);
      return;
    } else {
      dsend_packet_unit_change_homecity(&aconnection, punit->id,
					punit->ptr->pcity1->id);
    }
  }
  if (punit->tile != punit->ptr->pcity2->tile) {
    unit_goto(punit, punit->ptr->pcity2->tile);
  } else {
    dsend_packet_unit_establish_trade(&aconnection, punit->id);
  }
}

/**************************************************************************
  Free the trade orders of this unit.
  Retruns TRUE if the trade route was removed, else, returns FALSE.
**************************************************************************/
bool trade_free_unit(struct unit *punit)
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
      pdtr = fc_malloc(sizeof(struct delayed_trade_route));

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
    struct city *pcity1, *pcity2;
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
