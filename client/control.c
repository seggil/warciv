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

#include <assert.h>
#include <stdio.h>
#include <memory.h>

#include "fcintl.h"
#include "hash.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "support.h"
#include "unit.h"

#include "audio.h"
#include "chatline_g.h"
#include "citydlg_g.h"
#include "civclient.h"
#include "climap.h"
#include "climisc.h"
#include "clinet.h"
#include "dialogs_g.h"
#include "goto.h"
#include "gui_main_g.h"
#include "mapctrl_g.h"
#include "mapview_g.h"
#include "menu_g.h"
#include "multiselect.h"
#include "options.h"
#include "tilespec.h"
#include "trade.h"

#include "control.h"

bool moveandattack_state;
bool autowakeup_state;
enum default_caravan_unit_actions default_caravan_action;
enum default_diplomat_unit_actions default_diplomat_unit_action;
enum default_diplomat_city_actions default_diplomat_city_action;
bool default_diplomat_ignore_allies;

enum new_unit_action default_action_type;
bool default_action_locked;
bool default_action_military_only;

bool focus_turn = TRUE;
static struct unit *plast = NULL;

/* gui-dep code may adjust depending on tile size etc: */
int num_units_below = MAX_NUM_UNITS_BELOW;

/* unit_focus points to the current unit in focus */
static struct unit *punit_focus = NULL;

/* The previously focused unit.  Focus can generally be recalled on this
 * unit with keypad 5.  FIXME: this is not reset when the client
 * disconnects. */
static int previous_focus_id = -1;

/* These should be set via set_hover_state() */
int hover_unit = 0; /* id of unit hover_state applies to */
enum cursor_hover_state hover_state = HOVER_NONE;
enum unit_activity connect_activity;
/* This may only be here until client goto is fully implemented.
   It is reset each time the hower_state is reset. */
bool draw_goto_line = TRUE;

/* units involved in current combat */
static struct unit *punit_attacking = NULL;
static struct unit *punit_defending = NULL;

/*
 * This variable is TRUE iff a NON-AI controlled unit was focused this
 * turn.
 */
bool non_ai_unit_focus;

/*************************************************************************/

static struct unit *find_best_focus_candidate(bool accept_current);
static void store_focus(void);
static struct unit *quickselect(struct tile *ptile,
                        enum quickselect_type qtype);
static void do_mass_order(enum unit_activity activity);
/**************************************************************************
...
**************************************************************************/
void control_queues_free(void)
{
  exit_goto_state();
  hover_state = HOVER_NONE;
  hover_unit = 0;
  multi_select_clear_all();
  delayed_goto_clear_all();
  airlift_queue_clear_all();
}

/**************************************************************************
...
**************************************************************************/
void control_queues_init(void)
{
  multi_select_init_all();
  delayed_goto_init_all();
  airlift_queue_init_all();
}

/**************************************************************************
...
**************************************************************************/
void set_hover_state(struct unit *punit, enum cursor_hover_state state,
		     enum unit_activity activity)
{
  assert(punit != NULL || state == HOVER_NONE);
  assert(state == HOVER_CONNECT || activity == ACTIVITY_LAST);
  draw_goto_line = TRUE;
  hover_unit = punit ? punit->id : 0;
  hover_state = state;
  connect_activity = activity;
  exit_goto_state();
  delayed_goto_need_tile_for = -1;
  airlift_queue_need_city_for = -1;
}

/**************************************************************************
Center on the focus unit, if off-screen and auto_center_on_unit is true.
**************************************************************************/
void auto_center_on_focus_unit(void)
{
  if (punit_focus && auto_center_on_unit &&
      !tile_visible_and_not_on_border_mapcanvas(punit_focus->tile)) {
    center_tile_mapcanvas(punit_focus->tile);
  }
}

/**************************************************************************
note: punit can be NULL
We make sure that the previous focus unit is refreshed, if necessary,
_after_ setting the new focus unit (otherwise if the previous unit is
in a city, the refresh code draws the previous unit instead of the city).

 This function can be called directly from packhand.c as a result of
 Wakeup Focus (when a unit goes from Sentried to Idle in the server).
**************************************************************************/
void set_unit_focus(struct unit *punit)
{
  struct unit *punit_old_focus = punit_focus;
  bool ms_clear = FALSE;

  if (punit && punit->owner != get_player_idx()) {
    freelog(LOG_ERROR, "Trying to focus on another player's unit!");
    return;
  }

  if (punit != punit_focus) {
    if (!is_unit_in_multi_select(0, punit)) {
      ms_clear = TRUE;
    }
    store_focus();
  } else {
    ms_clear = TRUE;
  }

  /*
   *  This should be the ONLY place we _modify_ punit_focus.
   */
  punit_focus = punit;

  if (!can_client_change_view()) {
    /* This function can be called to set the focus to NULL when
     * disconnecting.  In this case we don't want any other actions! */
    assert(punit == NULL);
    return;
  }

  if (ms_clear) {
    multi_select_set_unit(0, punit);
  } else {
    multi_select_set_unit_focus(0, punit);
  }

  if (punit) {
    auto_center_on_focus_unit();

    punit->focus_status = FOCUS_AVAIL;
    refresh_tile_mapcanvas(punit->tile, MUT_NORMAL);
  }
  
  /* avoid the old focus unit disappearing: */
  if (punit_old_focus
      && (!punit || !same_pos(punit_old_focus->tile, punit->tile))) {
    refresh_tile_mapcanvas(punit_old_focus->tile, MUT_NORMAL);
  }

  update_unit_info_label(punit);
  update_menus();
}

/**************************************************************************
  with activation
**************************************************************************/
void set_unit_focus_and_active(struct unit *punit)
{
  request_active_unit(punit);
  set_unit_focus(punit);
}

/**************************************************************************
 The only difference is that here we draw the "cross".
**************************************************************************/
void set_unit_focus_and_select(struct unit *punit)
{
  set_unit_focus_and_active(punit);
  if (punit) {
    put_cross_overlay_tile(punit->tile);
  }
}

/**************************************************************************
  Store the focus unit.  This is used so that we can return to the
  previously focused unit with an appropriate keypress.
**************************************************************************/
static void store_focus(void)
{
  if (punit_focus) {
    previous_focus_id = punit_focus->id;
  }
}

/**************************************************************************
If there is no unit currently in focus, or if the current unit in
focus should not be in focus, then get a new focus unit.
We let GOTO-ing units stay in focus, so that if they have moves left
at the end of the goto, then they are still in focus.
**************************************************************************/
void update_unit_focus(void)
{
  if (!punit_focus
      || (punit_focus->activity != ACTIVITY_IDLE
	  && !unit_has_orders(punit_focus)
	  && punit_focus->activity != ACTIVITY_GOTO)
      || punit_focus->done_moving
      || punit_focus->moves_left == 0
      || punit_focus->air_patrol_tile
      || punit_focus->ptr
      || punit_focus->ai.control) {
    if (punit_focus && moveandattack_state == 1) {
      /* Ignore focus change for move and attack mode */
      if (punit_focus->ai.control) {
	return;
      }
    }
    advance_unit_focus();
  }
}

/**************************************************************************
...
**************************************************************************/
struct unit *get_unit_in_focus(void)
{
  return punit_focus;
}

/**************************************************************************
 This function may be called from packhand.c, via update_unit_focus(),
 as a result of packets indicating change in activity for a unit. Also
 called when user press the "Wait" command.
 
 FIXME: Add feature to focus only units of a certain category.
**************************************************************************/
void advance_unit_focus(void)
{
  if (client_is_global_observer() || map_is_empty()) {
    return;
  }

  struct unit *punit_old_focus = punit_focus;
  struct unit *candidate = find_best_focus_candidate(FALSE);

  if (punit_focus && punit_focus->id == hover_unit) {
    set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST);
  }
  if (!can_client_change_view()) {
    return;
  }

  if (!candidate) {
    /* First try for "waiting" units. */
    unit_list_iterate(get_player_ptr()->units, punit) {
      if(punit->focus_status == FOCUS_WAIT) {
        punit->focus_status = FOCUS_AVAIL;
      }
    } unit_list_iterate_end;
    candidate = find_best_focus_candidate(FALSE);
  }

  /* Accept current focus unit as last resort. */
  if (!candidate) {
    candidate = find_best_focus_candidate(TRUE);
  }

  set_unit_focus(candidate);

  /* 
   * Is the unit which just lost focus a non-AI unit? If yes this
   * enables the auto end turn. 
   */
  if (punit_old_focus && !punit_old_focus->ai.control) {
    non_ai_unit_focus = TRUE;
  }

  /* 
   * Handle auto-turn-done mode: If a unit was in focus (did move),
   * but now none are (no more to move) and there was at least one
   * non-AI unit this turn which was focused, then fake a Turn Done
   * keypress.
   */
  if (punit_old_focus && !punit_focus && non_ai_unit_focus) {
    if (focus_turn) {
      delayed_goto_event(AUTO_NO_UNIT_SELECTED, NULL);
      focus_turn = FALSE;
    }
    if (auto_turn_done) {
      key_end_turn();
    }
  }
}

/**************************************************************************
 Find the nearest available unit for focus, excluding any current unit
 in focus unless "accept_current" is TRUE.  If the current focus unit
 is the only possible unit, or if there is no possible unit, returns NULL.
**************************************************************************/
static struct unit *find_best_focus_candidate(bool accept_current)
{
  if (client_is_global_observer()) {
    return NULL;
  }

  struct unit *best_candidate;
  int best_dist = 99999;
  struct tile *ptile;

  if (punit_focus)  {
    ptile = punit_focus->tile;
  } else {
    ptile = get_center_tile_mapcanvas();
  }

  best_candidate = NULL;
  unit_list_iterate(get_player_ptr()->units, punit) {
    if ((punit != punit_focus || accept_current)
      && punit->focus_status == FOCUS_AVAIL
      && punit->activity == ACTIVITY_IDLE
      && !unit_has_orders(punit)
      && punit->moves_left > 0
      && !punit->done_moving
      && !punit->air_patrol_tile
      && !punit->ptr
      && !punit->ai.control) {
        int d = sq_map_distance(punit->tile, ptile);
        if (d < best_dist) {
          best_candidate = punit;
          best_dist = d;
        }
    }
  } unit_list_iterate_end;
  return best_candidate;
}

/**************************************************************************
Return a pointer to a visible unit, if there is one.
**************************************************************************/
struct unit *find_visible_unit(struct tile *ptile)
{
  struct unit *panyowned = NULL, *panyother = NULL, *ptptother = NULL;

  /* If no units here, return nothing. */
  if (unit_list_size(ptile->units) == 0) {
    return NULL;
  }

  /* If a unit is attacking we should show that on top */
  if (punit_attacking && same_pos(punit_attacking->tile, ptile)) {
    unit_list_iterate(ptile->units, punit) {
      if (punit == punit_attacking) {
	return punit;
      }
    } unit_list_iterate_end;
  }

  /* If a unit is defending we should show that on top */
  if (punit_defending && same_pos(punit_defending->tile, ptile)) {
    unit_list_iterate(ptile->units, punit) {
      if (punit == punit_defending) {
	return punit;
      }
    } unit_list_iterate_end;
  }

  /* If the unit in focus is at this tile, show that on top */
  if (punit_focus && same_pos(punit_focus->tile, ptile)) {
    unit_list_iterate(ptile->units, punit) {
      if (punit == punit_focus) {
        return punit;
      }
    } unit_list_iterate_end;
  }

  /* If a unit is on the current multi-selection */
  if (punit_focus) {
    unit_list_iterate(ptile->units, punit) {
      if (is_unit_in_multi_select(0, punit)) {
        if (punit->focus_status != FOCUS_DONE
	    && unit_satisfies_filter(punit, multi_select_inclusive_filter,
				            multi_select_exclusive_filter)) {
	  return punit;
	} else if (multi_select_blink_all) {
	  return panyowned = punit;
	}
      }
    } unit_list_iterate_end;
    if (panyowned) {
      return panyowned;
    }
  }

  /* If a city is here, return nothing (unit hidden by city). */
  if (ptile->city) {
    return NULL;
  }

  /* Iterate through the units to find the best one we prioritize this way:
       1: owned transporter.
       2: any owned unit
       3: any transporter
       4: any unit
     (always return first in stack). */
  unit_list_iterate(ptile->units, punit) {
    if (unit_owner(punit) == get_player_ptr()) {
      if (punit->transported_by == -1) {
        if (get_transporter_capacity(punit) > 0) {
	  return punit;
        } else if (!panyowned) {
	  panyowned = punit;
        }
      }
    } else if (!ptptother && punit->transported_by == -1) {
      if (get_transporter_capacity(punit) > 0) {
	ptptother = punit;
      } else if (!panyother) {
	panyother = punit;
      }
    }
  } unit_list_iterate_end;

  return (panyowned ? panyowned : (ptptother ? ptptother : panyother));
}

/**************************************************************************
  ...
**************************************************************************/
void blink_active_unit(void)
{
  static bool is_shown;

  if (punit_focus) {
    is_shown = !is_shown;
    set_focus_unit_hidden_state(!is_shown);
    if (!multi_select_blink) {
      refresh_tile_mapcanvas(punit_focus->tile, MUT_NORMAL);
    } else {
      unit_list_iterate(multi_select_get_units_focus(), punit) {
	if (multi_select_blink_all
	    || punit == punit_focus
	    || unit_satisfies_filter(punit, multi_select_inclusive_filter,
					    multi_select_exclusive_filter)) {
	  refresh_tile_mapcanvas(punit->tile, MUT_NORMAL);
	}
      } unit_list_iterate_end;
    }
  }
}

/**************************************************************************
  Update unit icons (and arrow) in the information display, for specified
  punit as the active unit and other units on the same square.  In practice
  punit is almost always (or maybe strictly always?) the focus unit.
  
  Static vars store some info on current (ie previous) state, to avoid
  unnecessary redraws; initialise to "flag" values to always redraw first
  time.  In principle we _might_ need more info (eg ai.control, connecting),
  but in practice this is enough?
  
  Used to store unit_ids for below units, to use for callbacks (now done
  inside gui-dep set_unit_icon()), but even with ids here they would not
  be enough information to know whether to redraw -- instead redraw every
  time.  (Could store enough info to know, but is it worth it?)
**************************************************************************/
void update_unit_pix_label(struct unit *punit)
{
  int i;

  /* Check for any change in the unit's state.  This assumes that a unit's
   * orders cannot be changed directly but must be removed and then reset. */
  if (punit && get_client_state() != CLIENT_GAME_OVER_STATE) {
    /* There used to be a complicated and bug-prone check here to see if
     * the unit had actually changed.  This was misguided since the stacked
     * units (below) are redrawn in any case.  Unless we write a general
     * system for unit updates here we might as well just redraw it every
     * time. */
    set_unit_icon(-1, punit);

    i = 0;			/* index into unit_below_canvas */
    unit_list_iterate(punit->tile->units, aunit) {
      if (aunit != punit) {
	if (i < num_units_below) {
	  set_unit_icon(i, aunit);
	}
	i++;
      }
    } unit_list_iterate_end;
    
    if (i > num_units_below) {
      set_unit_icons_more_arrow(TRUE);
    } else {
      set_unit_icons_more_arrow(FALSE);
      for(; i < num_units_below; i++) {
	set_unit_icon(i, NULL);
      }
    }
  } else {
    for(i=-1; i<num_units_below; i++) {
      set_unit_icon(i, NULL);
    }
    set_unit_icons_more_arrow(FALSE);
  }
}

/**************************************************************************
...
**************************************************************************/
void set_units_in_combat(struct unit *pattacker, struct unit *pdefender)
{
  punit_attacking = pattacker;
  punit_defending = pdefender;

  if (punit_attacking == punit_focus || punit_defending == punit_focus) {
    /* If one of the units is the focus unit, make sure hidden-focus is
     * disabled.  We don't just do this as a check later because then
     * with a blinking unit it would just disappear again right after the
     * battle. */
    set_focus_unit_hidden_state(FALSE);
  }
}

/**************************************************************************
  Add punit to queue of caravan arrivals, and popup a window for the
  next arrival in the queue, if there is not already a popup, and
  re-checking that a popup is appropriate.
  If punit is NULL, just do for the next arrival in the queue.
**************************************************************************/
void process_caravan_arrival(struct unit *punit)
{
  static genlist *arrival_queue = NULL;
  int *p_id;

  /* arrival_queue is a list of individually malloc-ed ints with
     punit.id values, for units which have arrived. */

  if (!arrival_queue) {
    arrival_queue = genlist_new();
  }

  if (punit) {
    p_id = fc_malloc(sizeof(int));
    *p_id = punit->id;
    genlist_insert(arrival_queue, p_id, -1);
  }

  /* There can only be one dialog at a time: */
  if (default_caravan_action == DCA_POPUP_DIALOG
      && caravan_dialog_is_open()) {
    return;
  }
  
  while (genlist_size(arrival_queue) > 0) {
    int id;
    
    p_id = genlist_get(arrival_queue, 0);
    genlist_unlink(arrival_queue, p_id);
    id = *p_id;
    free(p_id);
    p_id = NULL;
    punit = player_find_unit_by_id(get_player_ptr(), id);

    if (punit && (unit_can_help_build_wonder_here(punit)
		  || unit_can_est_traderoute_here(punit))
	&& (!get_player_ptr()->ai.control || ai_popup_windows)) {
      struct city *pcity_dest = map_get_city(punit->tile);
      struct city *pcity_homecity = find_city_by_id(punit->homecity);

      if (default_caravan_action == DCA_POPUP_DIALOG
	  && pcity_dest && pcity_homecity) {
	popup_caravan_dialog(punit, pcity_homecity, pcity_dest);
	return;
      } else {
        switch (default_caravan_action) {
	  case DCA_POPUP_DIALOG:
	  case DCA_KEEP_MOVING:
	    break;
	  case DCA_ESTABLISH_TRADEROUTE:
	    dsend_packet_unit_establish_trade(&aconnection, punit->id); 
	    break;
	  case DCA_HELP_BUILD_WONDER:
	    dsend_packet_unit_help_build_wonder(&aconnection, punit->id); 
	    break;
        }
      }
    }
  }
}

/**************************************************************************
  Add punit/pcity to queue of diplomat arrivals, and popup a window for
  the next arrival in the queue, if there is not already a popup, and
  re-checking that a popup is appropriate.
  If punit is NULL, just do for the next arrival in the queue.
**************************************************************************/
void process_diplomat_arrival(struct unit *pdiplomat, int victim_id)
{
  static genlist *arrival_queue = NULL;
  int *p_ids;

  freelog(LOG_DEBUG, "process_diplomat_arrival pdiplomat=%p victim_id=%d",
          pdiplomat, victim_id);

  /* arrival_queue is a list of individually malloc-ed int[2]s with
     punit.id and pcity.id values, for units which have arrived. */

  if (!arrival_queue) {
    arrival_queue = genlist_new();
  }

  if (pdiplomat && victim_id != 0) {
    p_ids = fc_malloc(2 * sizeof(int));
    p_ids[0] = pdiplomat->id;
    p_ids[1] = victim_id;
    genlist_insert(arrival_queue, p_ids, -1);
  }

  /* There can only be one dialog at a time: */
  if (diplomat_dialog_is_open()
      && default_diplomat_unit_action == DDUA_POPUP_DIALOG
      && default_diplomat_city_action == DDCA_POPUP_DIALOG) {
    return;
  }

  while (genlist_size(arrival_queue) > 0) {
    int diplomat_id, victim_id;
    struct city *pcity;
    struct unit *punit;
    bool dipl_unit_ok, dipl_city_ok;

    p_ids = genlist_get(arrival_queue, 0);
    genlist_unlink(arrival_queue, p_ids);
    diplomat_id = p_ids[0];
    victim_id = p_ids[1];
    free(p_ids);
    p_ids = NULL;
    pdiplomat = player_find_unit_by_id(get_player_ptr(), diplomat_id);
    pcity = find_city_by_id(victim_id);
    punit = find_unit_by_id(victim_id);

    if (!pdiplomat || !unit_flag(pdiplomat, F_DIPLOMAT))
      continue;

    dipl_unit_ok = punit != NULL
	&& is_diplomat_action_available(pdiplomat, DIPLOMAT_ANY_ACTION,
					punit->tile)
	&& diplomat_can_do_action(pdiplomat, DIPLOMAT_ANY_ACTION,
                                  punit->tile);
    dipl_city_ok = pcity != NULL
	       && is_diplomat_action_available(pdiplomat, DIPLOMAT_ANY_ACTION,
					       pcity->tile)
	       && diplomat_can_do_action(pdiplomat, DIPLOMAT_ANY_ACTION,
                                  pcity->tile);

    if (dipl_unit_ok) {
      /* Target is a unit */

      freelog(LOG_DEBUG, "dipl_unit_ok");

      if (default_diplomat_ignore_allies
          && pplayers_allied(unit_owner(punit), unit_owner(pdiplomat))) {
        return;
      }

      switch (default_diplomat_unit_action) {
	case DDUA_POPUP_DIALOG:
	  popup_diplomat_dialog(pdiplomat, punit->tile);
	  break;
	case DDUA_BRIBE:
	  request_diplomat_action(DIPLOMAT_BRIBE, diplomat_id, victim_id, 0);
	  break;
	case DDUA_SABOTAGE:
	  request_diplomat_action(SPY_SABOTAGE_UNIT, diplomat_id, victim_id, 0);
	  break;
	case DDUA_KEEP_MOVING:
	  break;
      }
      return;
    }
    
    if (dipl_city_ok) {
      /* Target is a city */

      freelog(LOG_DEBUG, "dipl_city_ok");

      if (default_diplomat_ignore_allies
          && pplayers_allied(city_owner(pcity), unit_owner(pdiplomat))) {
        request_diplomat_action(DIPLOMAT_MOVE, diplomat_id, victim_id, 0);
        return;
      }

      switch (default_diplomat_city_action) {
	case DDCA_POPUP_DIALOG:
	  popup_diplomat_dialog(pdiplomat, pcity->tile);
	  break;
	case DDCA_EMBASSY:
	  request_diplomat_action(DIPLOMAT_EMBASSY, diplomat_id, victim_id, 0);
	  break;
	case DDCA_INVESTIGATE:
	  request_diplomat_action(DIPLOMAT_INVESTIGATE, diplomat_id,
				  victim_id, 0);
	  break;
	case DDCA_SABOTAGE:
	  request_diplomat_action(DIPLOMAT_SABOTAGE, diplomat_id,
				  victim_id, B_LAST + 1);
	  break;
	case DDCA_STEAL_TECH:
	  request_diplomat_action(DIPLOMAT_STEAL, diplomat_id,
				  victim_id, game.ruleset_control.num_tech_types);
	  break;
	case DDCA_INCITE_REVOLT:
	  request_diplomat_action(DIPLOMAT_INCITE, diplomat_id, victim_id, 0);
	  break;
	case DDCA_POISON:
	  request_diplomat_action(SPY_POISON, diplomat_id, victim_id, 0);
	  break;
	default:
          /* For anything else, assume we want to
           * just move into the city. */
          request_diplomat_action(DIPLOMAT_MOVE, diplomat_id, victim_id, 0);
	  break;
      }
      return;
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void request_unit_goto(void)
{
  struct unit *punit = punit_focus;

  if (!punit) {
    return;
  }

  if (hover_state != HOVER_GOTO) {
    set_hover_state(punit, HOVER_GOTO, ACTIVITY_LAST);
    update_unit_info_label(punit);
    /* Not yet implemented for air units, including helicopters. */
    if (is_air_unit(punit) || is_heli_unit(punit)) {
      draw_goto_line = FALSE;
    } else {
      enter_goto_state(punit);
      create_line_at_mouse_pos();
    }
  } else if (!is_air_unit(punit) && !is_heli_unit(punit)) {
    assert(goto_is_active());
    goto_add_waypoint();
  }
}

/**************************************************************************
... init delayed goto for given unit
**************************************************************************/
void request_unit_delayed_goto(void)
{
  if (!punit_focus) {
    return;
  }
  set_hover_state(punit_focus, HOVER_DELAYED_GOTO, ACTIVITY_LAST);
  update_unit_info_label(punit_focus);
}

/**************************************************************************
  ... 
**************************************************************************/
void key_select_rally_point(void)
{
  int n = 0;

  if (!tiles_hilited_cities) {
    return;
  }
  
  city_list_iterate(get_player_ptr()->cities, pcity) {
    if (is_city_hilited(pcity)) {
      n++;
    }
  } city_list_iterate_end;

  if (n > 0) {
    char buf[128];
    my_snprintf(buf, sizeof (buf),
                _("Warclient: Select rally point for %d %s."),
                n, PL_("city", "cities", n));
    append_output_window(buf);

    hover_state = HOVER_RALLY_POINT;
    hover_unit = 0;
    update_hover_cursor();
  }
}

/**************************************************************************
... automatic airlifting destination selection
**************************************************************************/
void request_auto_airlift_destination_selection(void)
{
  hover_state = HOVER_AIRLIFT_DEST;
  hover_unit = 0;
  update_hover_cursor();
}

/**************************************************************************
... automatic airlifting source selection
**************************************************************************/
void request_auto_airlift_source_selection(void)
{
  if (tiles_hilited_cities) {
    city_list_iterate (get_player_ptr()->cities, pcity) {
      if (!is_city_hilited (pcity)) {
        continue;
      }
      add_city_to_auto_airlift_queue(pcity->tile, TRUE);
    } city_list_iterate_end;
    update_airlift_menu(0);
  } else {
    hover_state = HOVER_AIRLIFT_SOURCE;
    hover_unit = 0;
    update_hover_cursor();
  }
}

/**************************************************************************
  Return TRUE if there are any units doing the activity on the tile.
**************************************************************************/
static bool is_activity_on_tile(struct tile *ptile,
				enum unit_activity activity)
{
  unit_list_iterate(ptile->units, punit) {
    if (punit->activity == ACTIVITY_MINE) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/**************************************************************************
  Return whether the unit can connect with given activity (or with
  any activity if activity arg is set to ACTIVITY_IDLE)

  This function is client-specific.
**************************************************************************/
bool can_unit_do_connect(struct unit *punit, enum unit_activity activity) 
{
  struct player *pplayer = unit_owner(punit);
  Terrain_type_id terrain = map_get_terrain(punit->tile);
  struct tile_type *ttype = get_tile_type(terrain);

  /* HACK: This code duplicates that in
   * can_unit_do_activity_targeted_at(). The general logic here is that
   * the connect is allowed if both:
   * (1) the unit can do that activity type, in general
   * (2) either
   *     (a) the activity has already been completed at this tile
   *     (b) it can be done by the unit at this tile. */
  switch (activity) {
    case ACTIVITY_ROAD:
      return terrain_control.may_road
	&& unit_flag(punit, F_SETTLERS)
	&& (tile_has_special(punit->tile, S_ROAD)
	    || (ttype->road_time != 0
		&& (!tile_has_special(punit->tile, S_RIVER)
		    || player_knows_techs_with_flag(pplayer, TF_BRIDGE))));
    case ACTIVITY_RAILROAD:
      /* There is no check for existing road/rail; the connect is allowed
       * regardless. It is assumed that if you know the TF_RAILROAD flag
       * you must also know the TF_BRIDGE flag. */
      return (terrain_control.may_road
	      && unit_flag(punit, F_SETTLERS)
	      && player_knows_techs_with_flag(pplayer, TF_RAILROAD));
    case ACTIVITY_IRRIGATE:
      /* Special case for irrigation: only irrigate to make S_IRRIGATION,
       * never to transform tiles. */
      return (terrain_control.may_irrigate
	      && unit_flag(punit, F_SETTLERS)
	      && (tile_has_special(punit->tile, S_IRRIGATION)
		  || (terrain == ttype->irrigation_result
		      && is_water_adjacent_to_tile(punit->tile)
		      && !is_activity_on_tile(punit->tile, ACTIVITY_MINE))));
    default:
      break;
  }

  return FALSE;
}

/**************************************************************************
  Active an unit
**************************************************************************/
void request_active_unit(struct unit *punit)
{
  if (!punit) {
    return;
  }
  if (!server_has_extglobalinfo && punit->ptr) {
    trade_free_unit(punit);
  } else {
    if (unit_has_orders(punit)) {
      request_orders_cleared(punit);
    }
    request_new_unit_activity(punit, ACTIVITY_IDLE);
  }
  if (punit->ai.control) {
    punit->ai.control = FALSE;
  }
  punit->focus_status = FOCUS_AVAIL;
}

/**************************************************************************
prompt player for entering destination point for unit connect
(e.g. connecting with roads)
**************************************************************************/
void request_unit_connect(enum unit_activity activity)
{
  if (!punit_focus || !can_unit_do_connect(punit_focus, activity)) {
    return;
  }

  if (hover_state != HOVER_CONNECT || connect_activity != activity) {
    /* Enter or change the hover connect state. */
    set_hover_state(punit_focus, HOVER_CONNECT, activity);
    update_unit_info_label(punit_focus);

    enter_goto_state(punit_focus);
    create_line_at_mouse_pos();
  } else {
    assert(goto_is_active());
    goto_add_waypoint();
  }
}

/**************************************************************************
  ...
**************************************************************************/
void request_unit_unload_all(struct unit *punit)
{
  struct tile *ptile = punit->tile;

  if(get_transporter_capacity(punit) == 0) {
    append_output_window(_("Game: Only transporter units can be unloaded."));
    return;
  }

  request_unit_wait(punit);    /* RP: unfocus the ship */
  punit->is_new = FALSE;

  unit_list_iterate(ptile->units, pcargo) {
    if (pcargo->transported_by == punit->id) {
      request_unit_unload(pcargo);

      if (pcargo->activity == ACTIVITY_SENTRY) {
	request_new_unit_activity(pcargo, ACTIVITY_IDLE);
      }

      if (pcargo->owner == get_player_idx()
	  && (!plast || !unit_flag(plast,F_MARINES))) {
	plast = pcargo;
      }
    }
  } unit_list_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void request_unit_airlift(struct unit *punit, struct city *pcity)
{
  punit->is_new = FALSE;
  dsend_packet_unit_airlift(&aconnection, punit->id, pcity->id);
}

/**************************************************************************
  Return-and-recover for a particular unit.  This sets the unit to GOTO
  the nearest city.
**************************************************************************/
void request_unit_return(struct unit *punit)
{
  struct pf_path *path;

  if (is_air_unit(punit) || is_heli_unit(punit)) {
    /* PF doesn't support air or helicopter units yet. */
    return;
  }
  punit->is_new = FALSE;

  if ((path = path_to_nearest_allied_city(punit))) {
    enum unit_activity activity = ACTIVITY_LAST;
    int turns = pf_last_position(path)->turn;

    if (punit->hp + turns * get_player_bonus(get_player_ptr(),
					     EFT_UNIT_RECOVER)
	< unit_type(punit)->hp) {
      activity = ACTIVITY_SENTRY;
    }
    send_goto_path(punit, path, activity);
    pf_destroy_path(path);
  }
}

/**************************************************************************
(RP:) un-sentry all my own sentried units on punit's tile
**************************************************************************/
void request_unit_wakeup(struct unit *punit)
{
  wakeup_sentried_units(punit->tile);
}

/**************************************************************************
  Request a diplomat to do a specific action.
  - action : The action to be requested.
  - dipl_id : The unit ID of the diplomatic unit.
  - target_id : The ID of the target unit or city.
  - value : For DIPLOMAT_STEAL or DIPLOMAT_SABOTAGE, the technology
            or building to aim for (spies only).
**************************************************************************/
void request_diplomat_action(enum diplomat_actions action, int dipl_id,
			     int target_id, int value)
{
  dsend_packet_unit_diplomat_action(&aconnection, dipl_id,
				    action, target_id, value);
}

/**************************************************************************
  ...
**************************************************************************/
void wakeup_sentried_units(struct tile *ptile)
{
  unit_list_iterate(ptile->units, punit) {
    if(punit->activity == ACTIVITY_SENTRY && get_player_idx() == punit->owner) {
      request_new_unit_activity(punit, ACTIVITY_IDLE);
    }
  }
  unit_list_iterate_end;
}

/**************************************************************************
Player pressed 'b' or otherwise instructed unit to build or add to city.
If the unit can build a city, we popup the appropriate dialog.
Otherwise, we just send a packet to the server.
If this action is not appropriate, the server will respond
with an appropriate message.  (This is to avoid duplicating
all the server checks and messages here.)
**************************************************************************/
void request_unit_build_city(struct unit *punit)
{
  if (can_unit_build_city(punit)) {
    dsend_packet_city_name_suggestion_req(&aconnection, punit->id);
    /* the reply will trigger a dialog to name the new city */
  } else {
    char name[] = "";
    dsend_packet_unit_build_city(&aconnection, punit->id, name);
  }
}

/**************************************************************************
  This function is called whenever the player pressed an arrow key.

  We do NOT take into account that punit might be a caravan or a diplomat
  trying to move into a city, or a diplomat going into a tile with a unit;
  the server will catch those cases and send the client a package to pop up
  a dialog. (the server code has to be there anyway as goto's are entirely
  in the server)
**************************************************************************/
void request_move_unit_direction(struct unit *punit, int dir)
{
  struct tile *dest_tile;

  /* Catches attempts to move off map */
  dest_tile = mapstep(punit->tile, dir);
  if (!dest_tile) {
    return;
  }
  punit->is_new = FALSE;

  if (punit->moves_left > 0) {
    dsend_packet_unit_move(&aconnection, punit->id,
			   dest_tile->x, dest_tile->y);
    attack_after_move(punit);
  } else {
    /* Initiate a "goto" with direction keys for exhausted units. */
    send_goto_unit(punit, dest_tile);
  }
}

/**************************************************************************
...
**************************************************************************/
void request_new_unit_activity(struct unit *punit, enum unit_activity act)
{
  if (!can_client_issue_orders()) {
    return;
  }

  if (act == ACTIVITY_IDLE) {
    punit->is_sleeping = FALSE;
  }

  dsend_packet_unit_change_activity(&aconnection, punit->id, act,
				    S_NO_SPECIAL);
}

/**************************************************************************
...
**************************************************************************/
void request_new_unit_activity_targeted(struct unit *punit,
					enum unit_activity act,
					enum tile_special_type tgt)
{
  dsend_packet_unit_change_activity(&aconnection, punit->id, act, tgt);
}

/**************************************************************************
...
**************************************************************************/
void request_unit_disband(struct unit *punit)
{
  dsend_packet_unit_disband(&aconnection, punit->id);
}

/**************************************************************************
...
**************************************************************************/
void request_unit_change_homecity(struct unit *punit)
{
  struct city *pcity = map_get_city(punit->tile);
  
  if (pcity) {
    dsend_packet_unit_change_homecity(&aconnection, punit->id, pcity->id);
  }
}

/**************************************************************************
...
**************************************************************************/
void request_unit_upgrade(struct unit *punit)
{
  struct city *pcity;
  
  if (punit == NULL || punit->tile == NULL) {
    return;
  }

  pcity = map_get_city(punit->tile);

  if (pcity == NULL) {
    return;
  }

  dsend_packet_unit_upgrade(&aconnection, punit->id);
}

/**************************************************************************
...
**************************************************************************/
void request_unit_auto(struct unit *punit)
{
  if (can_unit_do_auto(punit)) {
    punit->is_new = FALSE;
    dsend_packet_unit_auto(&aconnection, punit->id);
  } else {
    append_output_window(_("Game: Only settler units and military units"
			   " in cities can be put in auto-mode."));
  }
}

/****************************************************************************
  Send a request to the server that the cargo be loaded into the transporter.

  If ptransporter is NULL a transporter will be picked at random.
****************************************************************************/
void request_unit_load(struct unit *pcargo, struct unit *ptrans)
{
  if (!ptrans) {
    ptrans = find_transporter_for_unit(pcargo, pcargo->tile);
  }
   pcargo->is_new = FALSE;

  if (can_client_issue_orders()
      && can_unit_load(pcargo, ptrans)) {
    dsend_packet_unit_load(&aconnection, pcargo->id, ptrans->id);

    /* Sentry the unit.  Don't request_unit_sentry since this can give a
     * recursive loop. */
    dsend_packet_unit_change_activity(&aconnection, pcargo->id,
				      ACTIVITY_SENTRY, S_NO_SPECIAL);
  }
}

/****************************************************************************
  Send a request to the server that the cargo be unloaded from its current
  transporter.
****************************************************************************/
void request_unit_unload(struct unit *pcargo)
{
  struct unit *ptrans = find_unit_by_id(pcargo->transported_by);

  if (can_client_issue_orders()
      && ptrans
      && can_unit_unload(pcargo, ptrans)
      && can_unit_survive_at_tile(pcargo, pcargo->tile)) {
    pcargo->is_new = FALSE;
    dsend_packet_unit_unload(&aconnection, pcargo->id, ptrans->id);

    /* Activate the unit. */
    dsend_packet_unit_change_activity(&aconnection, pcargo->id,
				      ACTIVITY_IDLE, S_NO_SPECIAL);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void request_unit_caravan_action(struct unit *punit, enum packet_type action)
{
  if (!map_get_city(punit->tile)) {
    return;
  }

  if (action == PACKET_UNIT_ESTABLISH_TRADE) {
    dsend_packet_unit_establish_trade(&aconnection, punit->id);
  } else if (action == PACKET_UNIT_HELP_BUILD_WONDER) {
    dsend_packet_unit_help_build_wonder(&aconnection, punit->id);
  } else {
    freelog(LOG_ERROR, "Bad action (%d) in request_unit_caravan_action",
	    action);
  }
}

/**************************************************************************
  Explode nuclear at a tile without enemy units
**************************************************************************/
void request_unit_nuke(struct unit *punit)
{
  if(!unit_flag(punit, F_NUCLEAR)) {
    append_output_window(_("Game: Only nuclear units can do this."));
    return;
  }
  if (punit->moves_left == 0) {
    do_unit_nuke(punit);
  } else {
    set_hover_state(punit, HOVER_NUKE, ACTIVITY_LAST);
    update_unit_info_label(punit);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void request_unit_paradrop(struct unit *punit)
{
  if (!unit_flag(punit, F_PARATROOPERS)) {
    append_output_window(_("Game: Only paratrooper units can do this."));
    return;
  }
  if (!can_unit_paradrop(punit)) {
    return;
  }

  set_hover_state(punit, HOVER_PARADROP, ACTIVITY_LAST);
  update_unit_info_label(punit);
}

/**************************************************************************
  ...
**************************************************************************/
void request_unit_patrol(void)
{
  struct unit *punit = punit_focus;

  if (!punit) {
    return;
  }

  if (is_air_unit(punit) || is_heli_unit(punit)) {
    /* Same string as in do_unit_patrol_to. */
    append_output_window(_("Game: Sorry, airunit patrol "
			   "not yet implemented."));
    return;
  }

  if (hover_state != HOVER_PATROL) {
    set_hover_state(punit, HOVER_PATROL, ACTIVITY_LAST);
    update_unit_info_label(punit);
    /* Not yet implemented for air units, including helicopters. */
    if (is_air_unit(punit) || is_heli_unit(punit)) {
      draw_goto_line = FALSE;
    } else {
      enter_goto_state(punit);
      create_line_at_mouse_pos();
    }
  } else {
    assert(goto_is_active());
    goto_add_waypoint();
  }
}

/****************************************************************
  ...
*****************************************************************/
void request_unit_sentry(struct unit *punit)
{
  if (punit->is_sleeping) {
    punit->is_sleeping = FALSE;
    refresh_tile_mapcanvas(punit->tile, MUT_NORMAL);
    refresh_unit_city_dialogs(punit);
    update_unit_focus();
  } else if (punit->activity!=ACTIVITY_SENTRY &&
             can_unit_do_activity(punit, ACTIVITY_SENTRY)) {
    punit->is_new = FALSE;
    request_new_unit_activity(punit, ACTIVITY_SENTRY);
  }
}

/****************************************************************
  ...
*****************************************************************/
void request_unit_fortify(struct unit *punit)
{
  if (punit->activity == ACTIVITY_FORTIFYING) {
    return;
  }
  if(can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
    punit->is_new = FALSE;
    request_new_unit_activity(punit, ACTIVITY_FORTIFYING);
  }
}

/****************************************************************
  ...
*****************************************************************/
void request_unit_sleep(struct unit *punit)
{
  if (!can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
    punit->is_sleeping = TRUE;

    if (punit->activity == ACTIVITY_SENTRY) {

      refresh_tile_mapcanvas(punit->tile, MUT_NORMAL);
      refresh_unit_city_dialogs(punit);
      update_unit_focus();
    } else {
      punit->is_new = FALSE;
      request_new_unit_activity(punit, ACTIVITY_SENTRY);
    }
  }
}

/**************************************************************************
  ...
**************************************************************************/
void request_unit_pillage(struct unit *punit)
{
  struct tile *ptile = punit->tile;
  enum tile_special_type pspresent = get_tile_infrastructure_set(ptile);
  enum tile_special_type psworking =
         get_unit_tile_pillage_set(punit->tile);
  enum tile_special_type what =
         get_preferred_pillage(pspresent & (~psworking));
  enum tile_special_type would =
         what | map_get_infrastructure_prerequisite(what);

  if (punit->activity != ACTIVITY_PILLAGE &&
      can_unit_do_activity(punit, ACTIVITY_PILLAGE)) {
    if ((game.ruleset_game.pillage_select)
	&& ((pspresent & (~(psworking | would))) != S_NO_SPECIAL)) {
      punit->is_new = FALSE;
      popup_pillage_dialog(punit, (pspresent & (~psworking)));
    } else {
      punit->is_new = FALSE;
      request_new_unit_activity_targeted(punit, ACTIVITY_PILLAGE, what);
    }
  }
}

/**************************************************************************
  Toggle display of grid lines on the map
**************************************************************************/
void request_toggle_map_grid(void) 
{
  if (!can_client_change_view()) {
    return;
  }

  draw_map_grid ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
  Toggle display of national borders on the map
**************************************************************************/
void request_toggle_map_borders(void) 
{
  if (!can_client_change_view()) {
    return;
  }

  draw_borders ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
  Toggle display of city names
**************************************************************************/
void request_toggle_city_names(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_city_names ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}
 
/**************************************************************************
  Toggle display of city growth (turns-to-grow)
**************************************************************************/
void request_toggle_city_growth(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_city_growth ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of city productions
**************************************************************************/
void request_toggle_city_productions(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_city_productions ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of city production buy cost
**************************************************************************/
void request_toggle_city_production_buy_cost(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_city_production_buy_cost ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of city traderoutes
**************************************************************************/
void request_toggle_city_traderoutes(void)
{
  struct player *me = get_player_ptr();

  if (!can_client_change_view()) {
    return;
  }

  draw_city_traderoutes ^= 1;

  if (me != NULL) {
    city_list_iterate(me->cities, pcity) {
      pcity->client.traderoute_drawing_disabled = !draw_city_traderoutes;
    } city_list_iterate_end;
  }

  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of terrain
**************************************************************************/
void request_toggle_terrain(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_terrain ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of coastline
**************************************************************************/
void request_toggle_coastline(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_coastline ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of roads and rails
**************************************************************************/
void request_toggle_roads_rails(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_roads_rails ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of irrigation
**************************************************************************/
void request_toggle_irrigation(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_irrigation ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of mines
**************************************************************************/
void request_toggle_mines(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_mines ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of fortress and airbase
**************************************************************************/
void request_toggle_fortress_airbase(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_fortress_airbase ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of specials
**************************************************************************/
void request_toggle_specials(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_specials ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of pollution
**************************************************************************/
void request_toggle_pollution(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_pollution ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of cities
**************************************************************************/
void request_toggle_cities(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_cities ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of units
**************************************************************************/
void request_toggle_units(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_units ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of focus unit
**************************************************************************/
void request_toggle_focus_unit(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_focus_unit ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
}

/**************************************************************************
 Toggle display of fog of war
**************************************************************************/
void request_toggle_fog_of_war(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_fog_of_war ^= 1;
  update_map_canvas_visible(MUT_NORMAL);
  refresh_overview_canvas();
}

/**************************************************************************
...
**************************************************************************/
void request_center_focus_unit(void)
{
  if (punit_focus) {
    center_tile_mapcanvas(punit_focus->tile);
  }
}

/**************************************************************************
...
**************************************************************************/
void request_unit_wait(struct unit *punit)
{
  punit->focus_status = FOCUS_WAIT;
}

/**************************************************************************
...
**************************************************************************/
void request_unit_move_done(struct unit *punit)
{
  if (punit) {
    punit->focus_status = FOCUS_DONE;
    /* set_unit_focus(punit_focus); */  /* done in advance_unit_focus */
  }
}

/**************************************************************************
  Called to have the client move a unit from one location to another,
  updating the graphics if necessary.  The caller must redraw the target
  location after the move.
**************************************************************************/
void do_move_unit(struct unit *punit, struct unit *target_unit)
{
  struct tile *ptile;
  bool was_teleported, do_animation;

  was_teleported = !is_tiles_adjacent(punit->tile, target_unit->tile);
  do_animation = (!was_teleported && smooth_move_unit_msec > 0);

  ptile = punit->tile;
  punit->is_new = FALSE;

  if (!was_teleported
      && punit->activity != ACTIVITY_SENTRY
      && punit->transported_by == -1) {
    audio_play_sound(unit_type(punit)->sound_move,
		     unit_type(punit)->sound_move_alt);
  }

  unit_list_unlink(ptile->units, punit);

  if (get_player_idx() == punit->owner
      && auto_center_on_unit
      && !unit_has_orders(punit)
      && punit->activity != ACTIVITY_GOTO
      && punit->activity != ACTIVITY_SENTRY
      && !tile_visible_and_not_on_border_mapcanvas(target_unit->tile)) {
    center_tile_mapcanvas(target_unit->tile);
  }

  if (punit->transported_by == -1) {
    /* We have to refresh the tile before moving.  This will draw
     * the tile without the unit (because it was unlinked above). */
    if (unit_type_flag(punit->type, F_CITIES)
	&& punit->client.colored) {
      /* For settlers with an overlay, redraw the entire area of the
       * overlay. */
      int width = get_citydlg_canvas_width();
      int height = get_citydlg_canvas_height();
      int canvas_x, canvas_y;

      tile_to_canvas_pos(&canvas_x, &canvas_y, ptile);
      update_map_canvas(canvas_x - (width - NORMAL_TILE_WIDTH) / 2,
			canvas_y - (height - NORMAL_TILE_HEIGHT) / 2,
			width, height, MUT_NORMAL);
      overview_update_tile(ptile);
    }

    if (do_animation) {
      int dx, dy;
      freelog(LOG_DEBUG,"animating unit movement");
      /* For the duration of the animation the unit exists at neither
       * tile. */
      map_distance_vector(&dx, &dy, ptile, target_unit->tile);
      move_unit_map_canvas(punit, ptile, dx, dy);
    }
  }

  refresh_tile_mapcanvas(ptile, MUT_NORMAL);
  punit->tile = target_unit->tile;

  unit_list_prepend(punit->tile->units, punit);

  if (punit_focus == punit) {
    update_menus();
  }
}

/**************************************************************************
  ...
**************************************************************************/
void check_new_unit_action(struct unit *punit)
{
  if (!punit || (default_action_military_only && !is_military_unit(punit))) {
    return;
  }

  switch (default_action_type) {
  case ACTION_IDLE:
    break;
  case ACTION_SENTRY:
    request_unit_sentry(punit);
    break;
  case ACTION_FORTIFY:
    request_unit_fortify(punit);
    break;
  case ACTION_SLEEP:
    request_unit_sleep(punit);
    break;
  case ACTION_FORTIFY_OR_SLEEP:
    if (can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
      request_unit_fortify(punit);
    } else {
      request_unit_sentry(punit);
    }
    break;
  }
}

/**************************************************************************
  Handles everything when the user clicked a tile
**************************************************************************/
void do_map_click(struct tile *ptile, enum quickselect_type qtype)
{
  enum unit_activity ca=connect_activity;
  struct city *pcity = map_get_city(ptile);
  struct unit *punit = player_find_unit_by_id(get_player_ptr(), hover_unit);
  bool maybe_goto = FALSE;

  if (ptile && hover_state == HOVER_TRADE_CITY) {
    add_tile_in_trade_planning(ptile, TRUE);
    hover_state = HOVER_NONE;
    hover_unit = 0;
    update_hover_cursor();
  } else if (punit && pcity && hover_state == HOVER_TRADE_DEST) {
    request_trade_route(pcity);
    hover_state = HOVER_NONE;
    hover_unit = 0;
    update_hover_cursor();
  } else if (ptile && hover_state == HOVER_AIRLIFT_SOURCE) {
    add_city_to_auto_airlift_queue(ptile, FALSE);       
    hover_state = HOVER_NONE;
    hover_unit = 0;
    update_hover_cursor();
    return;
  } else if(ptile && hover_state == HOVER_AIRLIFT_DEST) {
    if (pcity) {
      do_airlift(ptile);
    } else {
      append_output_window(
	  _("Warclient: You need to select a tile with a city"));
    }
    airlift_queue_need_city_for = -1;
    hover_state = HOVER_NONE;
    hover_unit = 0;
    update_hover_cursor();
    return;
  } else if (ptile && hover_state == HOVER_DELAYED_GOTO) {
    if (delayed_goto_need_tile_for >= 0
	&& delayed_goto_need_tile_for < DELAYED_GOTO_NUM) {
      request_execute_delayed_goto(ptile, delayed_goto_need_tile_for);
      delayed_goto_need_tile_for = -1;
    } else {
      add_unit_to_delayed_goto(ptile);
    }
    hover_state = HOVER_NONE;
    hover_unit = 0;
    update_hover_cursor();
    return;
  } else if (ptile && hover_state == HOVER_RALLY_POINT) {
    set_rally_point_for_selected_cities(ptile);
    hover_state = HOVER_NONE;
    hover_unit = 0;
    update_hover_cursor();
    return;
  } else if (ptile && hover_state == HOVER_DELAYED_AIRLIFT) {
    schedule_delayed_airlift(ptile);
    hover_state = HOVER_NONE;
    hover_unit = 0;
    update_hover_cursor();
    return;
  } else if (punit && hover_state != HOVER_NONE) {
    switch (hover_state) {
      case HOVER_GOTO:
	if (multi_select_size(0) > 1) {
	  multi_select_iterate(FALSE, punit) {
	    send_goto_unit(punit, ptile);
	    attack_after_move(punit);
	  } multi_select_iterate_end;
	} else {
	  do_unit_goto(ptile);
	}
	break;
      case HOVER_NUKE:
	if (SINGLE_MOVE * real_map_distance(punit->tile, ptile)
	    > punit->moves_left) {
	  append_output_window(_("Game: Too far for this unit."));
	} else {
	  send_goto_unit(punit, ptile);
	  /* note that this will be executed by the server after the goto */
	  if (!pcity) {
	    do_unit_nuke(punit);
	  }
	}
	break;
      case HOVER_PARADROP:
	multi_select_iterate(FALSE, punit) {
	  do_unit_paradrop_to(punit, ptile);
	} multi_select_iterate_end;
	break;
      case HOVER_CONNECT: /* lie to don't crash !!! */
	exit_goto_state();
	lie_unit_focus_init();
	multi_select_iterate(TRUE, punit) {
	  hover_state = HOVER_CONNECT;
	  hover_unit = punit->id;
	  connect_activity = ca;
	  put_unit_focus(punit);
	  enter_goto_state(punit);
	  do_unit_connect(punit, ptile, connect_activity);
	} multi_select_iterate_end;
	put_last_unit_focus();
	break;
      case HOVER_PATROL: /* lie to don't crash !!! */
	exit_goto_state();
	lie_unit_focus_init();
	multi_select_iterate(TRUE, punit) {
	  hover_state = HOVER_PATROL;
	  hover_unit = punit->id;
	  put_unit_focus(punit);
	  enter_goto_state(punit);
	  do_unit_patrol_to(punit, ptile);
	} multi_select_iterate_end;
	put_last_unit_focus();
	break;
      case HOVER_AIR_PATROL:
	multi_select_iterate(FALSE, punit) {
	  do_unit_air_patrol(punit, ptile);
	} multi_select_iterate_end;
        break;
      default:
	break;
    }
    set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST);
    update_unit_info_label(punit);
  }

  /* Bypass stack or city popup if quickselect is specified. */
  else if (qtype) {
    struct unit *qunit = quickselect(ptile, qtype);
    if (qunit) {
      set_unit_focus_and_select(qunit);
      maybe_goto = keyboardless_goto;
    }
  }
  /* Otherwise use popups. */
  else if (pcity
	   && (client_is_global_observer()
	       || can_player_see_city_internals(get_player_ptr(), pcity))) {
    popup_city_dialog(pcity, FALSE);
  } else if (unit_list_size(ptile->units) == 0 && !pcity
           && punit_focus) {
    maybe_goto = keyboardless_goto;
  } else if (unit_list_size(ptile->units) == 1
	     && !unit_list_get(ptile->units, 0)->occupy) {
    struct unit *punit = unit_list_get(ptile->units, 0);
    if (get_player_idx() == punit->owner) {
      if (can_unit_do_activity(punit, ACTIVITY_IDLE)) {
        maybe_goto = keyboardless_goto;
	set_unit_focus_and_select(punit);
      }
    } else if (pcity) {
      /* Don't hide the unit in the city. */
      popup_unit_select_dialog(ptile);
    }
  } else if (unit_list_size(ptile->units) > 0) {
    /* The stack list is always popped up, even if it includes enemy units.
     * If the server doesn't want the player to know about them it shouldn't
     * tell him!  The previous behavior would only pop up the stack if you
     * owned a unit on the tile.  This gave cheating clients an advantage,
     * and also showed you allied units if (and only if) you had a unit on
     * the tile (inconsistent). */
    popup_unit_select_dialog(ptile);
  }

  /* See mapctrl_common.c */
  keyboardless_goto_start_tile = maybe_goto ? ptile : NULL;
  keyboardless_goto_button_down = maybe_goto;
  keyboardless_goto_active = FALSE;
}

/**************************************************************************
 Quickselecting a unit is normally done with <control> left, right click,
 or keypad / * for the current tile. Bypassing the stack popup is quite
 convenient, and can be tactically important in furious multiplayer games.
**************************************************************************/
static struct unit *quickselect(struct tile *ptile,
                          enum quickselect_type qtype)
{
  int listsize = unit_list_size(ptile->units);
  struct unit *panytransporter = NULL,
              *panymovesea  = NULL, *panysea  = NULL,
              *panymoveland = NULL, *panyland = NULL,
              *panymoveunit = NULL, *panyunit = NULL;

  assert(qtype > SELECT_POPUP);

  if (listsize == 0) {
    return NULL;
  } else if (listsize == 1) {
    struct unit *punit = unit_list_get(ptile->units, 0);
    return (get_player_idx() == punit->owner) ? punit : NULL;
  }

  /*  Quickselect priorities. Units with moves left
   *  before exhausted. Focus unit is excluded.
   *
   *    SEA:  Transporter
   *          Sea unit
   *          Any unit
   *
   *    LAND: Military land unit
   *          Non-combatant
   *          Sea unit
   *          Any unit
   */

  unit_list_iterate(ptile->units, punit)  {
    if(get_player_idx() != punit->owner || punit == punit_focus) {
      continue;
    }
    if (qtype == SELECT_SEA) {
      if (get_transporter_capacity(punit)) {
        /* Transporter. */
	if (punit->moves_left > 0) {
	  return punit;
	} else if (!panytransporter) {
	  panytransporter = punit;
	}
      } else if (is_sailing_unit(punit)) {
        /* Any sea, pref. moves left. */
	if (punit->moves_left > 0) {
	  if (!panymovesea) {
	    panymovesea = punit;
	  }
	} else if (!panysea) {
	    panysea = punit;
	}
      }
    } else if (qtype == SELECT_LAND) {
      if (is_ground_unit(punit))  {
	if (punit->moves_left > 0) {
	  if (is_military_unit(punit)) {
	    return punit;
	  } else if (!panymoveland) {
	      panymoveland = punit;
	  }
	} else if (!panyland) {
	  panyland = punit;
	}
      }
      else if (is_sailing_unit(punit)) {
	if (punit->moves_left > 0) {
	  panymovesea = punit;
	} else {
	  panysea = punit;
	}
      }
    }
    if (punit->moves_left > 0 && !panymoveunit) {
      panymoveunit = punit;
    }
    if (!panyunit) {
      panyunit = punit;
    }
  } unit_list_iterate_end;

  if (qtype == SELECT_SEA) {
    if (panytransporter) {
      return panytransporter;
    } else if (panymovesea) {
      return panymovesea;
    } else if (panysea) {
      return panysea;
    } else if (panymoveunit) {
      return panymoveunit;
    } else if (panyunit) {
      return panyunit;
    }
  }
  else if (qtype == SELECT_LAND) {
    if (panymoveland) {
      return panymoveland;
    } else if (panyland) {
      return panyland;
    } else if (panymovesea) {
      return panymovesea;
    } else if (panysea) {
      return panysea;
    } else if (panymoveunit) {
      return panymoveunit;
    } else if (panyunit) {
      return panyunit;
    }
  }
  return NULL;
}

/**************************************************************************

**************************************************************************/
void attack_after_move(struct unit *punit)
{
  if (moveandattack_state
      && can_unit_do_auto(punit)
      && is_military_unit(punit)) {
    request_unit_auto(punit);
    request_new_unit_activity(punit, ACTIVITY_IDLE);
  }
}

/**************************************************************************
 Finish the goto mode and let the unit which is stored in hover_unit move
 to a given location.
**************************************************************************/
void do_unit_goto(struct tile *ptile)
{
  struct unit *punit = player_find_unit_by_id(get_player_ptr(), hover_unit);

  if (hover_unit == 0 || (hover_state != HOVER_GOTO)) {
    return;
  }

  if (punit) {
    punit->is_new = FALSE;
    if (!draw_goto_line) {
      send_goto_unit(punit, ptile);
      attack_after_move(punit);
    } else {
      struct tile *dest_tile;

      draw_line(ptile);
      dest_tile = get_line_dest();
      if (ptile == dest_tile) {
	send_goto_route(punit);
	attack_after_move(punit);
      } else {
	append_output_window(_("Game: Didn't find a route to the "
                               "destination!"));
      }
    }
  }

  set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST);
}

/**************************************************************************
Explode nuclear at a tile without enemy units
**************************************************************************/
void do_unit_nuke(struct unit *punit)
{
  dsend_packet_unit_nuke(&aconnection, punit->id);
}

/**************************************************************************
  Paradrop to a location.
**************************************************************************/
void do_unit_paradrop_to(struct unit *punit, struct tile *ptile)
{
  dsend_packet_unit_paradrop_to(&aconnection, punit->id, ptile->x, ptile->y);
}
 
/**************************************************************************
  Patrol to a location.
**************************************************************************/
void do_unit_patrol_to(struct unit *punit, struct tile *ptile)
{
  if (is_air_unit(punit) || is_heli_unit(punit)) {
    append_output_window(_("Game: Sorry, airunit patrol not yet implemented."));
    return;
  } else {
    struct tile *dest_tile;

    punit->is_new = FALSE;
    draw_line(ptile);
    dest_tile = get_line_dest();
    if (ptile == dest_tile
        && !is_non_allied_unit_tile(ptile, unit_owner(punit))) {
      send_patrol_route(punit);
    } else {
      append_output_window(_("Game: Didn't find a route to the destination!"));
    }
  }

  set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST);
}
 
/**************************************************************************
  "Connect" to the given location.
**************************************************************************/
void do_unit_connect(struct unit *punit, struct tile *ptile,
		     enum unit_activity activity)
{
  if (is_air_unit(punit) || is_heli_unit(punit)) {
    append_output_window(_("Game: Sorry, airunit connect "
			   "not yet implemented."));
  } else {
    struct tile *dest_tile;
    punit->is_new = FALSE;
    draw_line(ptile);
    dest_tile = get_line_dest();
    if (same_pos(dest_tile, ptile)) {
      send_connect_route(punit, activity);
    } else {
      append_output_window(_("Game: Didn't find a route to "
			     "the destination!"));
    }
  }

  set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST);
}
 
/**************************************************************************
 The 'Escape' key.
**************************************************************************/
void key_cancel_action(void)
{
  bool popped = FALSE;

  cancel_tile_hiliting();

  if (hover_state == HOVER_GOTO || hover_state == HOVER_PATROL) {
    if (draw_goto_line) {
      popped = goto_pop_waypoint();
    }
  }

  if (hover_state != HOVER_NONE && !popped) {
    struct unit *punit = player_find_unit_by_id(get_player_ptr(), hover_unit);

    set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST);
    update_unit_info_label(punit);

    keyboardless_goto_button_down = FALSE;
    keyboardless_goto_active = FALSE;
    keyboardless_goto_start_tile = NULL;
  }
  if (hover_state == HOVER_DELAYED_GOTO
      || hover_state == HOVER_AIRLIFT_SOURCE
      || hover_state == HOVER_AIRLIFT_DEST
      || hover_state == HOVER_RALLY_POINT
      || hover_state == HOVER_DELAYED_AIRLIFT
      || hover_state == HOVER_TRADE_CITY
      || hover_state == HOVER_TRADE_DEST) {
    hover_state = HOVER_NONE;
    hover_unit = 0;
    update_hover_cursor();
  }
  delayed_goto_need_tile_for = -1;
  airlift_queue_need_city_for = -1;
}

/**************************************************************************
  Center the mapview on the player's capital, or print a failure message.
**************************************************************************/
void key_center_capital(void)
{
  struct city *capital = find_palace(get_player_ptr());

  if (capital)  {
    /* Center on the tile, and pop up the crosshair overlay. */
    center_tile_mapcanvas(capital->tile);
    put_cross_overlay_tile(capital->tile);
  } else {
    append_output_window(_("Game: Oh my! You seem to have no capital!"));
  }
}

/**************************************************************************
...
**************************************************************************/
void key_end_turn(void)
{
  send_turn_done();
}

/**************************************************************************
  Recall the previous focus unit and focus on it.  See store_focus().
**************************************************************************/
void key_recall_previous_focus_unit(void)
{
  struct unit *punit = player_find_unit_by_id(get_player_ptr(),
                                              previous_focus_id);
  if (punit) {
    set_unit_focus_and_select(punit);
  }
}

/**************************************************************************
  Move the focus unit in the given direction.  Here directions are
  defined according to the GUI, so that north is "up" in the interface.
**************************************************************************/
void key_unit_move(enum direction8 gui_dir)
{
  enum direction8 map_dir = gui_to_map_dir(gui_dir);

  multi_select_iterate(FALSE, punit) {
    punit->is_new = FALSE;
    request_move_unit_direction(punit,map_dir);
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_build(void)
{
  if (!punit_focus) {
    return;
  }
  /* Enable the button for adding to a city in all cases, so we
   * get an eventual error message from the server if we try. */
  if (can_unit_add_or_build_city(punit_focus)) {
    key_unit_build_city();
  } else {
    key_unit_build_wonder();
  }
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_build_city(void)
{
  multi_select_iterate(TRUE, punit) {
    request_unit_build_city(punit);
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_build_wonder(void)
{
  multi_select_iterate(TRUE, punit) {
    if (unit_flag(punit, F_HELP_WONDER)) {
      request_unit_caravan_action(punit, PACKET_UNIT_HELP_BUILD_WONDER);
    }
  } multi_select_iterate_end;
}

/**************************************************************************
  handle user pressing key for 'Connect' command
**************************************************************************/
void key_unit_connect(enum unit_activity activity)
{
  if (punit_focus) {
    request_unit_connect(activity);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_diplomat_actions(void)
{
  struct city *pcity;		/* need pcity->id */

  multi_select_iterate(TRUE, punit) {
    if (punit
	&& is_diplomat_unit(punit)
	&& (pcity = map_get_city(punit->tile))
	&& !diplomat_dialog_is_open()    /* confusing otherwise? */
	&& diplomat_can_do_action(punit, DIPLOMAT_ANY_ACTION, punit->tile)) {
      process_diplomat_arrival(punit, pcity->id);
    }
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_done(void)
{
  multi_select_iterate(TRUE, punit) {
    request_unit_move_done(punit);
  } multi_select_iterate_end;
  advance_unit_focus();
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_goto(void)
{
  if (punit_focus) {
    request_unit_goto();
  }
}

/**************************************************************************
  Explode nuclear at a tile without enemy units
**************************************************************************/
void key_unit_nuke(void)
{
  if (punit_focus) {
    request_unit_nuke(punit_focus);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_paradrop(void)
{
  if (punit_focus && can_unit_paradrop(punit_focus)) {
    request_unit_paradrop(punit_focus);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_patrol(void)
{
  if (punit_focus) {
    request_unit_patrol();
  }
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_traderoute(void)
{
  multi_select_iterate(TRUE, punit) {
    if (unit_flag(punit, F_TRADE_ROUTE)) {
      request_unit_caravan_action(punit, PACKET_UNIT_ESTABLISH_TRADE);
    }
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_unload(void)
{
  multi_select_iterate(FALSE, punit) {
    request_unit_unload(punit);
  } multi_select_iterate_end;
}

/**************************************************************************
  ... multi_select_iterate() deprecied here. Using a complete code...
**************************************************************************/
void key_unit_unload_all(void)
{
  bool cond = multi_select_size(0) > 1;

  plast = NULL;
  unit_list_iterate(multi_select_get_units_focus(), punit) {
    if ((cond && !unit_satisfies_filter(punit,
					multi_select_inclusive_filter,
					multi_select_exclusive_filter))
	|| punit->focus_status == FOCUS_DONE) {
      continue;
    }

    request_unit_unload_all(punit);
  } unit_list_iterate_end;
  if (plast) {
    set_unit_focus(plast);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_wait(void)
{
  multi_select_iterate(TRUE, punit) {
    request_unit_wait(punit);
  } multi_select_iterate_end;

  if (punit_focus && punit_focus->focus_status == FOCUS_WAIT) {
    advance_unit_focus();
  }
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_delayed_goto(enum delayed_goto_type dgtype)
{
  if (hover_state == HOVER_DELAYED_GOTO && delayed_goto_state == dgtype) {
    /* The user pressed the short cut twice. This means that he is requesting
     * a delayed goto to non-defined tile). */
    add_unit_to_delayed_goto(NULL);
    hover_state = HOVER_NONE;
    hover_unit = 0;
    update_hover_cursor();
  } else {
    delayed_goto_state = dgtype;
    request_unit_delayed_goto();
  }
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_execute_delayed_goto(void)
{
  request_unit_execute_delayed_goto(0);
}

/**************************************************************************
  ...
***************************************************************************/
void key_unit_clear_delayed_orders(void)
{
  delayed_goto_clear(0);
}

/**************************************************************************
  ...
***************************************************************************/
void key_unit_wakeup_others(void)
{
  multi_select_iterate(FALSE, punit) {
    request_unit_wakeup(punit);
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_airbase(void)
{
  do_mass_order(ACTIVITY_AIRBASE);
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_auto_attack(void)
{
  enable_auto_mode();
}

/**************************************************************************
  ...
**************************************************************************/
void enable_auto_mode(void)
{
  multi_select_iterate(TRUE, punit) {
    if(can_unit_do_auto(punit)) {
      punit->is_new = FALSE;
      request_unit_auto(punit);
    }
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_auto_explore(void)
{
  do_mass_order(ACTIVITY_EXPLORE);
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_auto_settle(void)
{
  enable_auto_mode();
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_disband(void)
{
  multi_select_iterate(TRUE, punit) {
    request_unit_disband(punit);
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_fallout(void)
{
  do_mass_order(ACTIVITY_FALLOUT);
}

/**************************************************************************
  ...
**************************************************************************/
static void do_mass_order(enum unit_activity activity)
{
  if (punit_focus == NULL) {
    return;
  }

  multi_select_iterate(TRUE, punit) {
    if (can_unit_do_activity(punit, activity)) {
      punit->is_new = FALSE;
      request_new_unit_activity(punit, activity);
    }
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_fortify(void)
{
  if (punit_focus == NULL) {
    return;
  }

  multi_select_iterate(TRUE, punit) {
    if (can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
      request_unit_fortify(punit);
    }
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_sleep(void)
{
  multi_select_iterate(TRUE, punit) {
    request_unit_sleep(punit);
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_fortress(void)
{
  do_mass_order(ACTIVITY_FORTRESS);
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_homecity(void)
{
  multi_select_iterate(FALSE, punit) {
    request_unit_change_homecity(punit);
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_irrigate(void)
{
  do_mass_order(ACTIVITY_IRRIGATE);
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_load(void)
{
  multi_select_iterate(TRUE, punit) {
    request_unit_load(punit, NULL);
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_mine(void)
{
  do_mass_order(ACTIVITY_MINE);
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_pillage(void)
{
  if (multi_select_satisfies_filter(0) == 1) {
    request_unit_pillage(punit_focus);
  } else {
    multi_select_iterate(TRUE, punit) {
      request_new_unit_activity(punit, ACTIVITY_PILLAGE);
    } multi_select_iterate_end;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_pollution(void)
{
  do_mass_order(ACTIVITY_POLLUTION);
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_return(void)
{
  multi_select_iterate(FALSE, punit) {
    request_unit_return(punit);
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_road(void)
{
  multi_select_iterate(TRUE, punit) {
    if (punit) {
      if(can_unit_do_activity(punit, ACTIVITY_ROAD)) {
	punit->is_new = FALSE;
	request_new_unit_activity(punit, ACTIVITY_ROAD);
      } else if(can_unit_do_activity(punit, ACTIVITY_RAILROAD)) {
	punit->is_new = FALSE;
	request_new_unit_activity(punit, ACTIVITY_RAILROAD);
      }
    }
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_sentry(void)
{
  multi_select_iterate(TRUE, punit) {
    request_unit_sentry(punit);
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_transform(void)
{
  do_mass_order(ACTIVITY_TRANSFORM);
}

/**************************************************************************
  ...
**************************************************************************/
void key_map_grid_toggle(void)
{
  request_toggle_map_grid();
}

/**************************************************************************
  Toggle map borders on the mapview on/off based on a keypress.
**************************************************************************/
void key_map_borders_toggle(void)
{
  request_toggle_map_borders();
}

/**************************************************************************
  ...
**************************************************************************/
void key_city_names_toggle(void)
{
  request_toggle_city_names();
}

/**************************************************************************
  Toggles the "show city growth turns" option by passing off the
  request to another function...
**************************************************************************/
void key_city_growth_toggle(void)
{
  request_toggle_city_growth();
}

/**************************************************************************
  ...
**************************************************************************/
void key_city_productions_toggle(void)
{
  request_toggle_city_productions();
}

/**************************************************************************
  ...
**************************************************************************/
void key_city_production_buy_cost_toggle(void)
{
  request_toggle_city_production_buy_cost();
}

/**************************************************************************
  ...
**************************************************************************/
void key_city_traderoutes_toggle(void)
{
  request_toggle_city_traderoutes();
}

/**************************************************************************
  ...
**************************************************************************/
void key_terrain_toggle(void)
{
  request_toggle_terrain();
}

/**************************************************************************
  ...
**************************************************************************/
void key_coastline_toggle(void)
{
  request_toggle_coastline();
}

/**************************************************************************
  ...
**************************************************************************/
void key_roads_rails_toggle(void)
{
  request_toggle_roads_rails();
}

/**************************************************************************
  ...
**************************************************************************/
void key_irrigation_toggle(void)
{
  request_toggle_irrigation();
}

/**************************************************************************
  ...
**************************************************************************/
void key_mines_toggle(void)
{
  request_toggle_mines();
}

/**************************************************************************
  ...
**************************************************************************/
void key_fortress_airbase_toggle(void)
{
  request_toggle_fortress_airbase();
}

/**************************************************************************
  ...
**************************************************************************/
void key_specials_toggle(void)
{
  request_toggle_specials();
}

/**************************************************************************
  ...
**************************************************************************/
void key_pollution_toggle(void)
{
  request_toggle_pollution();
}

/**************************************************************************
  ...
**************************************************************************/
void key_cities_toggle(void)
{
  request_toggle_cities();
}

/**************************************************************************
  ...
**************************************************************************/
void key_units_toggle(void)
{
  request_toggle_units();
}

/**************************************************************************
  ...
**************************************************************************/
void key_focus_unit_toggle(void)
{
  request_toggle_focus_unit();
}

/**************************************************************************
  ...
**************************************************************************/
void key_fog_of_war_toggle(void)
{
  request_toggle_fog_of_war();
}

/**************************************************************************
  ...
**************************************************************************/
void key_quickselect(enum quickselect_type qtype)
{
  struct unit *punit;

  if(punit_focus) {
    punit = quickselect(punit_focus->tile, qtype);
    set_unit_focus_and_select(punit);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void key_toggle_moveandattack(void)
{
  static char txt[255];

  moveandattack_state ^= 1;
  my_snprintf(txt, sizeof(txt), moveandattack_state
              ? _("Warclient: Move and attack mode is on.")
              : _("Warclient: Move and attack mode is off."));
  append_output_window(txt);
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_delayed_airlift(void)
{
  if (hover_state == HOVER_DELAYED_AIRLIFT) {
    schedule_delayed_airlift(NULL);
    hover_state = HOVER_NONE;
    hover_unit = 0;
  } else {
    hover_state = HOVER_DELAYED_AIRLIFT;
    hover_unit = 0;
  }
  update_hover_cursor();
}

/**************************************************************************
  ...
**************************************************************************/
void key_add_pause_delayed_goto(void)
{
  add_pause_delayed_goto();
}

/**************************************************************************
  ...
**************************************************************************/
void key_add_trade_city(void)
{
  if (!can_client_issue_orders()) {
    return;
  }

  if (tiles_hilited_cities) {
    city_list_iterate(get_player_ptr()->cities, pcity) {
      if (is_city_hilited(pcity)) {
	add_tile_in_trade_planning(pcity->tile, FALSE);
      }
    } city_list_iterate_end;
    update_auto_caravan_menu();
  } else {
    hover_state = HOVER_TRADE_CITY;
    hover_unit = 0;
    update_hover_cursor();
  }
}

/**************************************************************************
  ...
**************************************************************************/
void key_auto_caravan_goto(void)
{
  if (!punit_focus) {
    return;
  }
  set_hover_state(punit_focus, HOVER_TRADE_DEST, ACTIVITY_LAST);
  update_unit_info_label(punit_focus);
}

/**************************************************************************
  ...
**************************************************************************/
void key_auto_caravan(void)
{
  request_trade_route(NULL);
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_air_patrol(void)
{
  multi_select_iterate(FALSE, punit) {
    do_unit_air_patrol(punit, punit->tile);
  } multi_select_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void key_unit_air_patrol_dest(void)
{
  if (!punit_focus) {
    return;
  }
  set_hover_state(punit_focus, HOVER_AIR_PATROL, ACTIVITY_LAST);
  update_unit_info_label(punit_focus);
}

/**************************************************************************
  For connect and patrol
**************************************************************************/
void put_unit_focus(struct unit *punit)
{
  punit_focus = punit;
  update_unit_info_label(punit);
}

/**************************************************************************
  For connect and patrol
**************************************************************************/
void lie_unit_focus_init(void)
{
  plast = punit_focus;
}

/**************************************************************************
  For connect and patrol
**************************************************************************/
void put_last_unit_focus(void)
{
  if (!plast) {
    return;
  }
  punit_focus = plast;
  update_unit_info_label(punit_focus);
  plast = NULL;
}

/**************************************************************************
  Cycle through the available overview modes.
**************************************************************************/
void key_cycle_overview_modes(void)
{
  overview_mode = (overview_mode + 1) % NUM_OVERVIEW_MODES;
  update_overview_mode_label();
  refresh_overview_canvas();
}
