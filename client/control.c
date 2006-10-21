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
#include "options.h"
#include "tilespec.h"
#include "unit.h"
#include "control.h"
#include "support.h"

struct rally_point {
  int check_bytes;
  int x, y;
  int refcount;
};
static int rally_point_ref (struct rally_point *rp);
static int rally_point_unref (struct rally_point *rp);
#define RALLY_POINT_CHECK_BYTES 0xf5e4d3b2

struct hash_table *rally_table = NULL;

struct delayed_goto_data {
	int unit_id;
	int type;
	struct tile target_tile;
};

#define SPECLIST_TAG delayed_goto_data
#define SPECLIST_TYPE struct delayed_goto_data
#include "speclist.h"
#define delayed_goto_data_list_iterate(alist, pitem)\
  TYPED_LIST_ITERATE(struct delayed_goto_data, alist, pitem)
#define delayed_goto_data_list_iterate_end  LIST_ITERATE_END

struct patrol_data {
  int unit_id;
  int x, y;
};

#define SPECLIST_TAG tile
#define SPECLIST_TYPE struct tile
#include "speclist.h"
#define tile_list_iterate(alist, pitem)\
  TYPED_LIST_ITERATE(struct tile, alist, pitem)
#define tile_list_iterate_end  LIST_ITERATE_END

/* Delayed goto queue */
static struct delayed_goto_data_list DGqueue;

/* Airlist source tile list */
static struct tile_list ALqueue;

/* Patrol "queue" */
static struct hash_table *patrol_table = NULL;

int autowakeup_state = 1;
int goto_mode = 0;
int lastactivatedunit = 0;
int airliftunittype = 0;
int delayed_para_or_nuke = 0;
int unit_limit = 0;
int ctrl_state = 0;
int default_caravan_action = 0;//0=popup, 1=establish trade route, 2=help building wonder, 3=keep going
int default_diplomat_action = 0;
unsigned int inclusive_dgoto_filter = FILTER_ALL;
unsigned int exclusive_dgoto_filter = FILTER_OFF;


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
static struct unit *find_best_focus_candidate (bool accept_current);
static void store_focus (void);
static struct unit *quickselect (struct tile *ptile,
                                 enum quickselect_type qtype);
static void do_mass_order (enum unit_activity activity);
/**************************************************************************
...
**************************************************************************/
void control_queues_free (void)
{
  if (patrol_table) {
    hash_iterate (patrol_table, void *, key, struct patrol_data *, pd) {
      free (pd);
    } hash_iterate_end;
    hash_delete_all_entries (patrol_table);
    hash_free (patrol_table);
    patrol_table = NULL;
  }

  delayed_goto_data_list_iterate (DGqueue, dgd) {
    free (dgd);
  } delayed_goto_data_list_iterate_end;
  delayed_goto_data_list_unlink_all (&DGqueue);

  tile_list_unlink_all (&ALqueue);

  if (rally_table) {
    hash_iterate (rally_table, void *, key, struct rally_point *, rp) {
      rally_point_unref (rp);
    } hash_iterate_end;
    hash_delete_all_entries (rally_table);
    hash_free (rally_table);
    rally_table = NULL;
  }
}
  
/**************************************************************************
...
**************************************************************************/
void control_queues_init (void)
{
  if (!patrol_table) {
    patrol_table = hash_new (hash_fval_int, hash_fcmp_int);
  }
  delayed_goto_data_list_init (&DGqueue);
  tile_list_init (&ALqueue);
  if (!rally_table) {
    rally_table = hash_new (hash_fval_int, hash_fcmp_int);
  }
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
  if (punit)
    hover_unit = punit->id;
  else
    hover_unit = 0;
  hover_state = state;
  connect_activity = activity;
  exit_goto_state();
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

  if (punit && punit->owner != game.player_idx) {
    freelog(LOG_ERROR, "Trying to focus on another player's unit!");
    return;
  }

  if (punit != punit_focus) {
    store_focus();
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

  if(punit) {
    auto_center_on_focus_unit();

    punit->focus_status=FOCUS_AVAIL;
    refresh_tile_mapcanvas(punit->tile, FALSE);

    if (unit_has_orders(punit)) {
      /* Clear the focus unit's orders. */
      request_orders_cleared(punit);
    }
    if (punit->activity != ACTIVITY_IDLE || punit->ai.control)  {
      punit->ai.control = FALSE;
      refresh_unit_city_dialogs(punit);
      request_new_unit_activity(punit, ACTIVITY_IDLE);
    }
  }
  
  /* avoid the old focus unit disappearing: */
  if (punit_old_focus
      && (!punit || !same_pos(punit_old_focus->tile, punit->tile))) {
    refresh_tile_mapcanvas(punit_old_focus->tile, FALSE);
  }

  update_unit_info_label(punit);
  update_menus();
}

/**************************************************************************
 The only difference is that here we draw the "cross".
**************************************************************************/
void set_unit_focus_and_select(struct unit *punit)
{
  set_unit_focus(punit);
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
      || punit_focus->ai.control) {
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
  struct unit *punit_old_focus = punit_focus;
  struct unit *candidate = find_best_focus_candidate(FALSE);

  set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST);
  if (!can_client_change_view()) {
    return;
  }

  if(!candidate) {
    /* First try for "waiting" units. */
    unit_list_iterate(game.player_ptr->units, punit) {
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
  if (auto_turn_done && punit_old_focus && !punit_focus && non_ai_unit_focus) {
    key_end_turn();
  }
}

/**************************************************************************
 Find the nearest available unit for focus, excluding any current unit
 in focus unless "accept_current" is TRUE.  If the current focus unit
 is the only possible unit, or if there is no possible unit, returns NULL.
**************************************************************************/
static struct unit *find_best_focus_candidate(bool accept_current)
{
  struct unit *best_candidate;
  int best_dist = 99999;
  struct tile *ptile;

  if (punit_focus)  {
    ptile = punit_focus->tile;
  } else {
    ptile = get_center_tile_mapcanvas();
  }

  best_candidate = NULL;
  unit_list_iterate(game.player_ptr->units, punit) {
    if ((punit != punit_focus || accept_current)
      && punit->focus_status == FOCUS_AVAIL
      && punit->activity == ACTIVITY_IDLE
	&& !unit_has_orders(punit)
      && punit->moves_left > 0
      && !punit->done_moving
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
  if (unit_list_size(&ptile->units)==0) {
    return NULL;
  }

  /* If a unit is attacking we should show that on top */
  if (punit_attacking && same_pos(punit_attacking->tile, ptile)) {
    unit_list_iterate(ptile->units, punit)
      if(punit == punit_attacking) return punit;
    unit_list_iterate_end;
  }

  /* If a unit is defending we should show that on top */
  if (punit_defending && same_pos(punit_defending->tile, ptile)) {
    unit_list_iterate(ptile->units, punit)
      if(punit == punit_defending) return punit;
    unit_list_iterate_end;
  }

  /* If the unit in focus is at this tile, show that on top */
  if (punit_focus && same_pos(punit_focus->tile, ptile)) {
    unit_list_iterate(ptile->units, punit)
      if(punit == punit_focus) return punit;
    unit_list_iterate_end;
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
  unit_list_iterate(ptile->units, punit)
    if (unit_owner(punit) == game.player_ptr) {
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
  unit_list_iterate_end;

  return (panyowned ? panyowned : (ptptother ? ptptother : panyother));
}

/**************************************************************************
...
**************************************************************************/
void blink_active_unit(void)
{
  static bool is_shown;
  static struct unit *pblinking_unit;
  struct unit *punit;

  if ((punit = punit_focus)) {
    if (punit != pblinking_unit) {
      /* When the focus unit changes, we reset the is_shown flag. */
      pblinking_unit = punit;
      is_shown = TRUE;
    } else {
      /* Reverse the shown status. */
      is_shown = !is_shown;
    }
    set_focus_unit_hidden_state(!is_shown);
    refresh_tile_mapcanvas(punit->tile, TRUE);
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
    }
    unit_list_iterate_end;
    
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
  static struct genlist arrival_queue;
  static bool is_init_arrival_queue = FALSE;
  int *p_id;

  /* arrival_queue is a list of individually malloc-ed ints with
     punit.id values, for units which have arrived. */

  if (!is_init_arrival_queue) {
    genlist_init(&arrival_queue);
    is_init_arrival_queue = TRUE;
  }

  if (punit) {
    p_id = fc_malloc(sizeof(int));
    *p_id = punit->id;
    genlist_insert(&arrival_queue, p_id, -1);
  }

  /* There can only be one dialog at a time: */
  if (!default_caravan_action && caravan_dialog_is_open()) {
    return;
  }
  
  while (genlist_size(&arrival_queue) > 0) {
    int id;
    
    p_id = genlist_get(&arrival_queue, 0);
    genlist_unlink(&arrival_queue, p_id);
    id = *p_id;
    free(p_id);
    p_id = NULL;
    punit = player_find_unit_by_id(game.player_ptr, id);

    if (punit && (unit_can_help_build_wonder_here(punit)
		  || unit_can_est_traderoute_here(punit))
	&& (!game.player_ptr->ai.control || ai_popup_windows)) {
      struct city *pcity_dest = map_get_city(punit->tile);
      struct city *pcity_homecity = find_city_by_id(punit->homecity);
      if (!default_caravan_action && pcity_dest && pcity_homecity) {//if 0 popup dialog
        	popup_caravan_dialog(punit, pcity_homecity, pcity_dest);
        	return;
      }else {
            switch(default_caravan_action) {
                case 1:
                    dsend_packet_unit_establish_trade(&aconnection, punit->id); 
                    break;
                case 2:
                    dsend_packet_unit_help_build_wonder(&aconnection, punit->id); 
                    break;
                default://if 3 then do nothing
            ;}
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
  static struct genlist arrival_queue;
  static bool is_init_arrival_queue = FALSE;
  int *p_ids;

  /* arrival_queue is a list of individually malloc-ed int[2]s with
     punit.id and pcity.id values, for units which have arrived. */

  if (!is_init_arrival_queue) {
    genlist_init(&arrival_queue);
    is_init_arrival_queue = TRUE;
  }

  if (pdiplomat && victim_id != 0) {
    p_ids = fc_malloc(2*sizeof(int));
    p_ids[0] = pdiplomat->id;
    p_ids[1] = victim_id;
    genlist_insert(&arrival_queue, p_ids, -1);
  }

  /* There can only be one dialog at a time: */
  if (!default_diplomat_action && diplomat_dialog_is_open()) {
    return;
  }

  while (genlist_size(&arrival_queue) > 0) {
    int diplomat_id, victim_id;
    struct city *pcity;
    struct unit *punit;

    p_ids = genlist_get(&arrival_queue, 0);
    genlist_unlink(&arrival_queue, p_ids);
    diplomat_id = p_ids[0];
    victim_id = p_ids[1];
    free(p_ids);
    p_ids = NULL;
    pdiplomat = player_find_unit_by_id(game.player_ptr, diplomat_id);
    pcity = find_city_by_id(victim_id);
    punit = find_unit_by_id(victim_id);

    if (!pdiplomat || !unit_flag(pdiplomat, F_DIPLOMAT))
      continue;

    if (punit
	&& is_diplomat_action_available(pdiplomat, DIPLOMAT_ANY_ACTION,
					punit->tile)
	&& diplomat_can_do_action(pdiplomat, DIPLOMAT_ANY_ACTION,
				  punit->tile)) {//target is a unit
        switch(default_diplomat_action) {
            case 0: 
                popup_diplomat_dialog(pdiplomat, punit->tile);
                break;
            case 1://bribe unit
                request_diplomat_action(DIPLOMAT_BRIBE, diplomat_id,victim_id, 0);  
                break;
            case 2://sabotage unit
                request_diplomat_action(SPY_SABOTAGE_UNIT, diplomat_id,victim_id, 0);  
                break;
            default:
        ;}
      return;
    } else if (pcity
	       && is_diplomat_action_available(pdiplomat, DIPLOMAT_ANY_ACTION,
					       pcity->tile)
	       && diplomat_can_do_action(pdiplomat, DIPLOMAT_ANY_ACTION,
					 pcity->tile)) {//target is a city
        switch(default_diplomat_action) {
            case 0: 
                popup_diplomat_dialog(pdiplomat, pcity->tile);
                break;
            case 3://embassy
                request_diplomat_action(DIPLOMAT_EMBASSY, diplomat_id,victim_id, 0); 
                break;
            case 4://investigate
                request_diplomat_action(DIPLOMAT_INVESTIGATE, diplomat_id,victim_id, 0); 
                break;
            case 5://sabotage city
                request_diplomat_action(DIPLOMAT_SABOTAGE, diplomat_id,victim_id, 0); 
                break;
            case 6://steal tech
                request_diplomat_action(DIPLOMAT_STEAL, diplomat_id,victim_id, 0); 
                break;
            case 7://incite revolt
                request_diplomat_action(DIPLOMAT_INCITE, diplomat_id,victim_id, 0); 
                break;
            case 8://poison city
                request_diplomat_action(SPY_POISON, diplomat_id,victim_id, 0); 
                break;
            default:
        ;}
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

  if (!punit)
    return;

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
 unit filter (originally checkboxes, now radioitems)
 first we check inclusive filter for this particular unit
 then we check exclusive ("except") filter
**************************************************************************/
bool unit_satisfies_filter(struct unit *punit)
{
    bool result = FALSE;
    if((inclusive_dgoto_filter & FILTER_ALL)||
       ((inclusive_dgoto_filter & FILTER_NEW) && punit->is_new)||
       ((inclusive_dgoto_filter & FILTER_FORTIFIED) && (punit->activity == ACTIVITY_FORTIFIED))||
       ((inclusive_dgoto_filter & FILTER_SENTRIED) && (punit->activity == ACTIVITY_SENTRY))||
       ((inclusive_dgoto_filter & FILTER_VETERAN) && (punit->veteran > 0))||
       ((inclusive_dgoto_filter & FILTER_IDLE) && (punit->activity == ACTIVITY_IDLE))||
       ((inclusive_dgoto_filter & FILTER_ABLE_TO_MOVE) && (punit->moves_left > 0))) {
        result = TRUE;//unit satisfies any from inclusive filters
    }
    if((exclusive_dgoto_filter & FILTER_ALL)||
       ((exclusive_dgoto_filter & FILTER_NEW) && punit->is_new)||
       ((exclusive_dgoto_filter & FILTER_FORTIFIED) && (punit->activity == ACTIVITY_FORTIFIED))||
       ((exclusive_dgoto_filter & FILTER_SENTRIED) && (punit->activity == ACTIVITY_SENTRY))||
       ((exclusive_dgoto_filter & FILTER_VETERAN) && (punit->veteran > 0))||
       ((exclusive_dgoto_filter & FILTER_IDLE) && (punit->activity == ACTIVITY_IDLE))||
       ((exclusive_dgoto_filter & FILTER_ABLE_TO_MOVE) && (punit->moves_left > 0))) {
        result = FALSE;//unit satisfies at least one exclusive filter
    }    
    return result;
}
/**************************************************************************
  ...
**************************************************************************/
void key_select_patrol_tile (void)
{
  if (!get_unit_in_focus())
    return;
  hover_state = HOVER_MYPATROL;
}
/**************************************************************************
  Set the patrol tile for the currently focused unit to be the given tile,
  or the the tile it is currently on (if target_tile is NULL).
**************************************************************************/
void key_set_patrol_position (struct tile *target_tile)
{
  struct unit *punit;
  struct tile *ptile;
  struct patrol_data *pd = NULL, *old;
  char txt[255];

  punit = get_unit_in_focus();

  if (!punit || !punit->tile)
    return;
  
  ptile = target_tile ? target_tile : punit->tile;
  
  my_snprintf (txt, sizeof (txt), _("Warclient: %s %d patrolling "
                                    "(%d, %d)."),
               get_unit_type (punit->type)->name, punit->id, ptile->x,
               ptile->y);
  pd = fc_malloc (sizeof (struct patrol_data));
  pd->unit_id = punit->id;
  pd->x = ptile->x;
  pd->y = ptile->y;

  old = hash_replace (patrol_table, INT_TO_PTR (punit->id), pd);
  if (old) {
    if (old->x == ptile->x && old->y == ptile->y) {
      my_snprintf (txt, sizeof (txt), _("Warclient: %s %d stopped "
                                        "patrolling (%d, %d)."),
                   get_unit_type (punit->type)->name, punit->id,
                   ptile->x, ptile->y);
      hash_delete_entry (patrol_table, INT_TO_PTR (punit->id));
      free (pd);
    } else {
      my_snprintf (txt, sizeof (txt), _("Warclient: %s %d patrolling "
                                        "(%d, %d) instead of (%d, %d)."),
                   get_unit_type (punit->type)->name, punit->id,
                   ptile->x, ptile->y, old->x, old->y);
    }
    free (old);
  }
  
  append_output_window (txt);
}
/**************************************************************************
player wants to clear the queue
so we pop all items from the queue, free memory and at the end free the whole queue
**************************************************************************/
void request_clear_patrol_queue (void)
{
  hash_iterate (patrol_table, void *, key, struct patrol_data *, pd) {
    free (pd);
  } hash_iterate_end;
  hash_delete_all_entries (patrol_table);
  
  append_output_window (_("Warclient: Patrol queue cleared."));
}
/**************************************************************************
  ...
**************************************************************************/
void request_execute_patrol(void)
{
  struct unit *punit = NULL;
  
  if (hash_num_entries (patrol_table) <= 0)
    return;
  
  append_output_window (_("Warclient: Executing patrol."));
  hash_iterate (patrol_table, void *, key, struct patrol_data *, pd) {
    punit = player_find_unit_by_id (game.player_ptr, pd->unit_id);

    if (punit) {
      struct tile *ptile = map_pos_to_tile (pd->x, pd->y);
      if (!ptile)
        continue;
      send_goto_unit (punit, ptile);
      punit->is_new = FALSE;
    }
  } hash_iterate_end;
}

/**************************************************************************
 delayed goto stuff
**************************************************************************/
void schedule_delayed_airlift(struct tile *ptile)
{
  char txt[255];
  struct delayed_goto_data *dgd = NULL;

  my_snprintf(txt,sizeof(txt), _("Game: Scheduling delayed airlift for : target.x = %i, target.y = %i"),ptile->x, ptile->y);
  append_output_window(txt);
  dgd = fc_malloc (sizeof (struct delayed_goto_data));
  dgd->type = 2;//1 is delayed nuke, 2 is delayed airlift
  memcpy(&dgd->target_tile, ptile, sizeof(struct tile));
  delayed_goto_data_list_append (&DGqueue, dgd);
}

/**************************************************************************
  ...
**************************************************************************/
void add_unit_to_delayed_goto (struct tile *ptile)
{
  struct unit *punit = punit_focus;
  struct unit_list *ulist;
  struct delayed_goto_data *dgd = NULL;
  int count = 0;

  if (punit == NULL || hover_state != HOVER_DELAYED_GOTO)
    return;

  if (goto_mode != 0) {
    /* goto for all units of the selected unit type on the same tile */
    
    if (goto_mode == 3) {
      ulist = &game.player_ptr->units;
    } else {
      ulist = &punit->tile->units;
    }
    
    unit_list_iterate (*ulist, tunit) {
      if (!((goto_mode == 2 || tunit->type == punit->type)
          && tunit->tile->continent == punit->tile->continent
          && unit_satisfies_filter (tunit)))
        continue;
        
      dgd = fc_malloc (sizeof (struct delayed_goto_data));
      dgd->unit_id = tunit->id;
      dgd->type = delayed_para_or_nuke;
      memcpy (&dgd->target_tile, ptile, sizeof (struct tile));
      delayed_goto_data_list_append (&DGqueue, dgd);
      count++;
    } unit_list_iterate_end;
    
  } else { 
    /* single unit goto */

    dgd = fc_malloc (sizeof (struct delayed_goto_data));
    dgd->unit_id = punit->id;
    dgd->type = delayed_para_or_nuke;
    memcpy (&dgd->target_tile, ptile, sizeof (struct tile));
    delayed_goto_data_list_append (&DGqueue, dgd);
    count++;
  }
  
  if (count) {
    char txt[255];
    my_snprintf (txt, sizeof (txt),
        _("Warclient: Adding %d unit goto (%d, %d) to queue."),
        count, ptile->x, ptile->y);
    append_output_window (txt);
  }
  
  hover_state = HOVER_NONE;
}
/**************************************************************************
... init delayed goto for given unit
**************************************************************************/
void request_unit_delayed_goto(void)
{
  struct unit *punit = punit_focus;

  if(!punit)return;
  hover_state = HOVER_DELAYED_GOTO;
}
/**************************************************************************
... execute delayed goto
**************************************************************************/
void request_unit_execute_delayed_goto(void)
{
  struct unit *punit = NULL;
  int counter = 0;

  append_output_window(_("Game: Executing delayed goto"));
  delayed_goto_data_list_iterate (DGqueue, dgd) {
      if(unit_limit && (++counter > unit_limit))break;//unit_limit = 0 means unlimited
      if(dgd->type==2){//for delayed airlift punit is empty
          do_airlift(&dgd->target_tile);
      } else {
    	  punit = player_find_unit_by_id(game.player_ptr, dgd->unit_id);
    	  if(punit)
          {
              if(dgd->type==1){
                 if(punit->type == find_unit_type_by_name_orig("Paratroopers")) {//then paradrop them
                    do_unit_paradrop_to(punit, &dgd->target_tile);
                 }else {
                    send_goto_unit(punit, &dgd->target_tile);//send unit or nuke to target tile
                    if(punit->type == find_unit_type_by_name_orig("Nuclear")) //then explode nuke manually in case it won't explode
                       do_unit_nuke(punit);//just in case there is invisible stealth unit
                 }
              } else if(dgd->type==0){
        		  send_goto_unit(punit, &dgd->target_tile);
              }
            punit->is_new = FALSE;
    	  }
      }
	  free(dgd);
  } delayed_goto_data_list_iterate_end;
  delayed_goto_data_list_unlink_all (&DGqueue);
}
/**************************************************************************
... clear delayed orders
**************************************************************************/
void request_unit_clear_delayed_orders(void)
{
  delayed_goto_data_list_iterate (DGqueue, dgd) {
    free (dgd);
  } delayed_goto_data_list_iterate_end;
  delayed_goto_data_list_unlink_all (&DGqueue);
  
  append_output_window(_("Game: Delayed orders queue cleared"));
}
/**************************************************************************
  ...
**************************************************************************/
static int rally_point_ref (struct rally_point *rp)
{
  if (rp->check_bytes != RALLY_POINT_CHECK_BYTES) {
    freelog (LOG_ERROR, _("Invalid rally point %p passed to "
                          "rally_point_ref! This is a programming error "
                          "and should be reported."), rp);
    return 0;
  }
  
  freelog (LOG_DEBUG, "incrementing refcount for rally_point %p"
           " refcount=%d", rp, rp->refcount);
  rp->refcount++;
  
  return rp->refcount;
}
/**************************************************************************
  ...
**************************************************************************/
static int rally_point_unref (struct rally_point *rp)
{
  if (rp->check_bytes != RALLY_POINT_CHECK_BYTES) {
    freelog (LOG_ERROR, _("Invalid rally point %p passed to "
                          "rally_point_unref! This is a programming error "
                          "and should be reported."), rp);
    return 0;
  }
  
  if (rp->refcount <= 0) {
    freelog (LOG_ERROR, _("Attempt to call rally_point_unref on a rally "
                          "point %p with refcount %d! This is a "
                          "programming error and should be reported."),
             rp, rp->refcount);
    return 0;
  }
  
  freelog (LOG_DEBUG, "decrementing refcount for rally_point %p"
           " refcount=%d", rp, rp->refcount);
  rp->refcount--;
  
  if (rp->refcount == 0) {
    freelog (LOG_DEBUG, "refcount for rally_point %p reached 0, freeing",
             rp);
    memset (&rp->check_bytes, 0, sizeof (rp->check_bytes));
    free (rp);
    return 0;
  }
  
  return rp->refcount;
}
/**************************************************************************
  Decrement the refcount for rallypoints whose cities no longer exist
  (or the player no longer controls).
**************************************************************************/
void check_dead_rally_sources (void)
{
  struct city *pcity;
  hash_iterate (rally_table, void *, key, struct rally_point *, rp) {
    int id = PTR_TO_INT (key);
    pcity = find_city_by_id (id);
    if (!pcity || pcity->owner != game.player_idx) {
      if (0 == rally_point_unref (rp))
        hash_delete_entry (rally_table, key);
    }
  } hash_iterate_end;
}
/**************************************************************************
  ... 
**************************************************************************/
void key_select_rally_point (void)
{
  int n = 0;
  if (!tiles_hilited_cities)
    return;
  
  city_list_iterate (game.player_ptr->cities, pcity) {
    if (is_city_hilited (pcity))
      n++;
  } city_list_iterate_end;
  
  if (n > 0) {
    char buf[128];
    my_snprintf (buf, sizeof (buf),
        _("Warclient: Select rally point for %d %s."),
        n, PL_("city", "cities", n));
    append_output_window (buf);
    
    hover_state = HOVER_RALLY_POINT;
  }
}
/**************************************************************************
... automatic airlifting destination selection
**************************************************************************/
void request_auto_airlift_destination_selection(void)
{
     hover_state = HOVER_AIRLIFT_DEST;
}
/**************************************************************************
... automatic airlifting source selection
**************************************************************************/

void request_auto_airlift_source_selection(void)
{
  if (tiles_hilited_cities) {
    city_list_iterate (game.player_ptr->cities, pcity) {
      if (!is_city_hilited (pcity))
        continue;
      add_city_to_auto_airlift_queue (pcity->tile);
    } city_list_iterate_end;
  } else {
    hover_state = HOVER_AIRLIFT_SOURCE;
  }
}

/**************************************************************************
... add city to queue
**************************************************************************/
void add_city_to_auto_airlift_queue(struct tile *ptile)
{
  char txt[255];
  
  if (!ptile)
    return;
  
  if (!ptile->city) {
    append_output_window (_("Game: You need to select a tile with city"));
    return;
  }

  tile_list_insert (&ALqueue, ptile);
  my_snprintf (txt, sizeof (txt), _("Game: Adding city %s to autolift queue"),
               ptile->city->name);
  append_output_window (txt);
}
/**************************************************************************
  ...
**************************************************************************/
void do_airlift (struct tile *ptile)
{
  if (!ptile)
    return;
  if (!ptile->city) {
    append_output_window (_("Game: You need to select a tile with a city"));
    return;                     
  }

  if (tile_list_size (&ALqueue) <= 0)
    return;
  
  append_output_window (_("Game: Airlifting units"));

  tile_list_iterate (ALqueue, srcptile) {
    if (!srcptile->city)
      continue;
    if (!srcptile->city->owner == game.player_idx)
      continue;
	  if (unit_list_size (&srcptile->units) <= 0)
      continue;
    unit_list_iterate (srcptile->units, punit) {
      if (punit->type == airliftunittype) {
        punit->is_new = FALSE;
        request_unit_airlift (punit, ptile->city);
        break;
      }
    } unit_list_iterate_end; 
  } tile_list_iterate_end;
}
/**************************************************************************
... clear autolift queue
**************************************************************************/
void request_clear_auto_airlift_queue(void)
{
  tile_list_unlink_all (&ALqueue);
  append_output_window (_("Game: Autolift queue cleared"));
}
/**************************************************************************
  show cities in autolift queue
**************************************************************************/
void show_cities_in_airlift_queue(void)
{
  if (tile_list_size (&ALqueue) <= 0)
    return;
  
  append_output_window (_("Game: Cities in airlift queue:"));
  tile_list_iterate (ALqueue, ttile) {
    if (!ttile->city)
      continue;
    append_output_window (ttile->city->name);
  } tile_list_iterate_end;
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
  struct unit *plast = NULL;

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

      if (pcargo->owner == game.player_idx) {
	plast = pcargo;
      }
    }
  } unit_list_iterate_end;

  if (plast) {
    /* If the above unloading failed this focus will still happen.  That's
     * probably a feature. */
    set_unit_focus(plast);
  }
}

/**************************************************************************
...
**************************************************************************/
void request_unit_airlift(struct unit *punit, struct city *pcity)
{
  punit->is_new = FALSE;
  dsend_packet_unit_airlift(&aconnection, punit->id,pcity->id);
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

    if (punit->hp + turns * get_player_bonus(game.player_ptr,
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
  punit->is_new = FALSE;
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
  dsend_packet_unit_diplomat_action(&aconnection, dipl_id,action,target_id,value);
}

void wakeup_sentried_units(struct tile *ptile)
{
	unit_list_iterate(ptile->units, punit) {
	if(punit->activity==ACTIVITY_SENTRY && game.player_idx==punit->owner) {
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
  lastactivatedunit = punit->id;
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
  struct city *pcity=map_get_city(punit->tile);
  
  if (pcity) {
    dsend_packet_unit_change_homecity(&aconnection, punit->id, pcity->id);
  }
}

/**************************************************************************
...
**************************************************************************/
void request_unit_upgrade(struct unit *punit)
{
  struct city *pcity=map_get_city(punit->tile);

  if (pcity) {
    dsend_packet_unit_upgrade(&aconnection, punit->id);
  }
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
/**********************************************************
                            Mass load
***********************************************************/
void request_unit_mass_load(struct unit *punit)
{
  struct unit *tunit;
  struct unit_list *ulist;

  tunit = NULL;
  if (punit == NULL)return;
  punit->is_new = FALSE;

  if (goto_mode && ctrl_state)
  {//goto for all units of the selected unit type on the same tile
    if(goto_mode==3)ulist = &game.player_ptr->units;
            else ulist = &punit_focus->tile->units;
    unit_list_iterate(*ulist, tunit)
    {
       if((goto_mode == 2 || tunit->type == punit_focus->type)&&(tunit->tile->continent == punit_focus->tile->continent))
       {         
          if(unit_satisfies_filter(tunit))request_unit_load(tunit,NULL);
       }
    }unit_list_iterate_end; 
  }else //goto_mode==0 - single unit
  {
       request_unit_load(punit,NULL);
  }
  ctrl_state = 0;    
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
  if(punit->moves_left == 0)
    do_unit_nuke(punit);
  else {
    set_hover_state(punit, HOVER_NUKE, ACTIVITY_LAST);
    update_unit_info_label(punit);
  }
}

/**************************************************************************
...
**************************************************************************/
void request_unit_paradrop(struct unit *punit)
{
  if(!unit_flag(punit, F_PARATROOPERS)) {
    append_output_window(_("Game: Only paratrooper units can do this."));
    return;
  }
  if(!can_unit_paradrop(punit))
    return;

  set_hover_state(punit, HOVER_PARADROP, ACTIVITY_LAST);
  update_unit_info_label(punit);
}

/**************************************************************************
...
***********
***************************************************************/
void request_unit_patrol(void)
{
  struct unit *punit = punit_focus;

  if (!punit)
    return;

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
  if(punit->activity!=ACTIVITY_SENTRY &&
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
  if(punit->activity!=ACTIVITY_FORTIFYING &&
     can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
    punit->is_new = FALSE;
    request_new_unit_activity(punit, ACTIVITY_FORTIFYING);
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

  if ((game.rgame.pillage_select) &&
      ((pspresent & (~(psworking | would))) != S_NO_SPECIAL)) {
    punit->is_new = FALSE;
    popup_pillage_dialog(punit, (pspresent & (~psworking)));
  } else {
    punit->is_new = FALSE;
    request_new_unit_activity_targeted(punit, ACTIVITY_PILLAGE, what);
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

  draw_map_grid^=1;
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
}
/**************************************************************************
 Toggle display of city traderoutes
**************************************************************************/
void request_toggle_city_traderoutes(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_city_traderoutes ^= 1;
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  update_map_canvas_visible();
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
  punit->focus_status=FOCUS_WAIT;
  if (punit == punit_focus) {
    advance_unit_focus();
    /* set_unit_focus(punit_focus); */  /* done in advance_unit_focus */
  }
}

/**************************************************************************
...
**************************************************************************/
void request_unit_move_done(void)
{
  if (punit_focus) {
    punit_focus->focus_status = FOCUS_DONE;
    advance_unit_focus();
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

  unit_list_unlink(&ptile->units, punit);

  if (game.player_idx == punit->owner
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
			width, height);
      overview_update_tile(ptile);
    } else {
      refresh_tile_mapcanvas(ptile, FALSE);
    }

    if (do_animation) {
      int dx, dy;

      /* For the duration of the animation the unit exists at neither
       * tile. */
      map_distance_vector(&dx, &dy, punit->tile,
			  target_unit->tile);
      move_unit_map_canvas(punit, ptile, dx, dy);
    }
  }
    
  punit->tile = target_unit->tile;

  unit_list_insert(&punit->tile->units, punit);

  if (punit_focus == punit) update_menus();
}
/**************************************************************************
  ...
**************************************************************************/
void key_clear_rally_point_for_selected_cities (void)
{
  struct rally_point *rp;
  char buf[1024];
  int count = 0;
  
  assert (rally_table != NULL);

  if (!tiles_hilited_cities)
    return;
  
  my_snprintf (buf, sizeof (buf), _("Warclient: Removed rally points: "));

  city_list_iterate (game.player_ptr->cities, pcity) {
    if (!is_city_hilited (pcity))
      continue;
    rp = hash_delete_entry (rally_table, INT_TO_PTR (pcity->id));
    if (!rp)
      continue;
    rally_point_unref (rp);
    count++;
    cat_snprintf (buf, sizeof (buf), "%s%s (%d, %d)",
                  count == 1 ? "" : ", ",
                  pcity->name, rp->x, rp->y);
  } city_list_iterate_end;

  if (count > 0) {
    cat_snprintf (buf, sizeof (buf), ".");
    append_output_window (buf);
  }
}
/**************************************************************************
  ...
**************************************************************************/
void check_rally_points (struct city *pcity, struct unit *punit)
{
  struct rally_point *rp;
  
  assert (rally_table != NULL);

  if (!pcity || !punit || !punit->is_new)
    return;
  
  rp = hash_lookup_data (rally_table, INT_TO_PTR (pcity->id));
  if (rp) {
    struct tile *ptile = map_pos_to_tile (rp->x, rp->y);
    if (ptile) {
      send_goto_unit (punit, ptile);
    }
  }
}
/**************************************************************************
  ...
**************************************************************************/
static void set_rally_point_for_selected_cities (struct tile *ptile)
{
  struct rally_point *rp, *oldrp;
  char buf[1024];
  
  assert (rally_table != NULL);

  if (!tiles_hilited_cities || !ptile)
    return;
    
  rp = fc_malloc (sizeof (struct rally_point));
  rp->check_bytes = RALLY_POINT_CHECK_BYTES;
  rp->x = ptile->x;
  rp->y = ptile->y;
  rp->refcount = 0;
  freelog (LOG_DEBUG, "new rally_point %p refcount=%d\n", rp, rp->refcount);
  rally_point_ref (rp);

  my_snprintf (buf, sizeof (buf), _("Warclient: Rallying to (%d, %d) "
                                    "from: "),
               rp->x, rp->y);
  
  city_list_iterate (game.player_ptr->cities, pcity) {
    if (!is_city_hilited (pcity))
      continue;
    oldrp = hash_replace (rally_table, INT_TO_PTR (pcity->id), rp);
    rally_point_ref (rp);
    if (oldrp) {
      rally_point_unref (oldrp);
    }
    cat_snprintf (buf, sizeof (buf), "%s%s",
                  rp->refcount == 2 ? "" : ", ",
                  pcity->name);
  } city_list_iterate_end;

  if (rally_point_unref (rp) > 0) {
    cat_snprintf (buf, sizeof (buf), ".");
    append_output_window (buf);
  }
}
/**************************************************************************
 Handles everything when the user clicked a tile
**************************************************************************/
void do_map_click(struct tile *ptile, enum quickselect_type qtype)
{
  struct city *pcity = map_get_city(ptile);
  struct unit *punit = player_find_unit_by_id(game.player_ptr, hover_unit);
  bool maybe_goto = FALSE;

  if(ptile && hover_state==HOVER_AIRLIFT_SOURCE) {
      add_city_to_auto_airlift_queue(ptile);       
      hover_state = HOVER_NONE;
      return;
  }else if(ptile && hover_state==HOVER_AIRLIFT_DEST) {
      do_airlift(ptile);
      hover_state = HOVER_NONE;
      return;
  }else if(ptile && hover_state==HOVER_MYPATROL) {
       key_set_patrol_position (ptile);
       hover_state = HOVER_NONE;
       return;
  }else if(ptile && hover_state==HOVER_DELAYED_GOTO) {
       add_unit_to_delayed_goto(ptile);
       hover_state = HOVER_NONE;
       return;
  } else if (ptile && hover_state == HOVER_RALLY_POINT) {
    set_rally_point_for_selected_cities (ptile);
    hover_state = HOVER_NONE;
    return;
  }else if(ptile && hover_state==HOVER_DELAYED_AIRLIFT) {
       schedule_delayed_airlift(ptile);
       hover_state = HOVER_NONE;
       return;
  }
  if (punit && hover_state != HOVER_NONE) {
    switch (hover_state) {
    case HOVER_NONE:
      die("well; shouldn't get here :)");
    case HOVER_GOTO:
      do_unit_goto(ptile);
      break;
    case HOVER_NUKE:
      if (SINGLE_MOVE * real_map_distance(punit->tile, ptile)
	  > punit->moves_left) {
        append_output_window(_("Game: Too far for this unit."));
      } else {
	do_unit_goto(ptile);
	/* note that this will be executed by the server after the goto */
	if (!pcity)
	  do_unit_nuke(punit);
      }
      break;
    case HOVER_PARADROP:
      do_unit_paradrop_to(punit, ptile);
      break;
    case HOVER_CONNECT:
      do_unit_connect(punit, ptile, connect_activity);
      break;
    case HOVER_PATROL:
      do_unit_patrol_to(punit, ptile);
      break;
    default: ;
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
  else if (pcity && can_player_see_city_internals(game.player_ptr, pcity)) {
    popup_city_dialog(pcity, FALSE);
  }
  else if (unit_list_size(&ptile->units) == 0 && !pcity
           && punit_focus) {
    maybe_goto = keyboardless_goto;
  }
  else if (unit_list_size(&ptile->units) == 1
      && !unit_list_get(&ptile->units, 0)->occupy) {
    struct unit *punit=unit_list_get(&ptile->units, 0);
    if(game.player_idx==punit->owner) {
      if(can_unit_do_activity(punit, ACTIVITY_IDLE)) {
        maybe_goto = keyboardless_goto;
	set_unit_focus_and_select(punit);
      }
    } else if (pcity) {
      /* Don't hide the unit in the city. */
      popup_unit_select_dialog(ptile);
    }
  }
  else if(unit_list_size(&ptile->units) > 0) {
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
  int listsize = unit_list_size(&ptile->units);
  struct unit *panytransporter = NULL,
              *panymovesea  = NULL, *panysea  = NULL,
              *panymoveland = NULL, *panyland = NULL,
              *panymoveunit = NULL, *panyunit = NULL;

  assert(qtype > SELECT_POPUP);

  if (listsize == 0) {
    return NULL;
  } else if (listsize == 1) {
    struct unit *punit = unit_list_get(&ptile->units, 0);
    return (game.player_idx == punit->owner) ? punit : NULL;
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
  if(game.player_idx != punit->owner || punit == punit_focus) {
    continue;
  }
  if (qtype == SELECT_SEA) {
    /* Transporter. */
    if (get_transporter_capacity(punit)) {
      if (punit->moves_left > 0) {
        return punit;
      } else if (!panytransporter) {
        panytransporter = punit;
      }
    }
    /* Any sea, pref. moves left. */
    else if (is_sailing_unit(punit)) {
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
 Finish the goto mode and let the unit which is stored in hover_unit move
 to a given location.
**************************************************************************/
void do_unit_goto(struct tile *ptile)
{
  struct unit *punit = player_find_unit_by_id(game.player_ptr, hover_unit);

  if (hover_unit == 0 || hover_state != HOVER_GOTO)
    return;

  if (punit) {
    punit->is_new = FALSE;
    if (!draw_goto_line) {
      send_goto_unit(punit, ptile);
    } else {
      struct tile *dest_tile;

      draw_line(ptile);
      dest_tile = get_line_dest();
      if (ptile == dest_tile) {
	send_goto_route(punit);
      } else {
	append_output_window(_("Game: Didn't find a route to the destination!"));
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

  ctrl_state = 0;
  cancel_tile_hiliting();

  if (hover_state == HOVER_GOTO || hover_state == HOVER_PATROL)
    if (draw_goto_line)
      popped = goto_pop_waypoint();

  if (hover_state != HOVER_NONE && !popped) {
    struct unit *punit = player_find_unit_by_id(game.player_ptr, hover_unit);

    set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST);
    update_unit_info_label(punit);

    keyboardless_goto_button_down = FALSE;
    keyboardless_goto_active = FALSE;
    keyboardless_goto_start_tile = NULL;
  }
  if (hover_state == HOVER_DELAYED_GOTO
      || hover_state == HOVER_AIRLIFT_SOURCE
      || hover_state == HOVER_AIRLIFT_DEST
      || hover_state == HOVER_MYPATROL
      || hover_state == HOVER_RALLY_POINT
      || hover_state == HOVER_DELAYED_AIRLIFT)
  {
      hover_state = HOVER_NONE;
  }
}

/**************************************************************************
  Center the mapview on the player's capital, or print a failure message.
**************************************************************************/
void key_center_capital(void)
{
  struct city *capital = find_palace(game.player_ptr);

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
  struct unit *punit = player_find_unit_by_id(game.player_ptr,
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
  if (punit_focus) {
    enum direction8 map_dir = gui_to_map_dir(gui_dir);
    punit_focus->is_new = FALSE;
    request_move_unit_direction(punit_focus, map_dir);
  }
}

/**************************************************************************
...
**************************************************************************/
void key_unit_build (void)
{
  if (get_unit_in_focus()) {
    struct unit *punit = get_unit_in_focus();
    /* Enable the button for adding to a city in all cases, so we
	  get an eventual error message from the server if we try. */
    if (can_unit_add_or_build_city (punit)) {
	    key_unit_build_city();
    } else {
	    key_unit_build_wonder();
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void key_unit_build_city(void)
{
  if (punit_focus) {
    request_unit_build_city(punit_focus);
  }
}

/**************************************************************************
...
**************************************************************************/
void key_unit_build_wonder(void)
{
  struct unit *tunit;
  struct unit_list *ulist;

  tunit = NULL;
  if (punit_focus == NULL)return;

  if (goto_mode && ctrl_state)//goto mode affects mass orders
  {//goto for all units of the selected unit type on the same tile
    if(goto_mode==3)ulist = &game.player_ptr->units;
            else ulist = &punit_focus->tile->units;
    unit_list_iterate(*ulist, tunit)
    {
       if((goto_mode == 2 || tunit->type == punit_focus->type)&&(tunit->tile->continent == punit_focus->tile->continent))
       {
         if(unit_flag(tunit, F_HELP_WONDER))
              request_unit_caravan_action(tunit, PACKET_UNIT_HELP_BUILD_WONDER);
       }
    }unit_list_iterate_end; 
  }else //goto_mode==0 - single unit
  {
    if(unit_flag(punit_focus, F_HELP_WONDER))
         request_unit_caravan_action(punit_focus, PACKET_UNIT_HELP_BUILD_WONDER);
  }
  ctrl_state = 0;
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
  if (punit_focus
     && is_diplomat_unit(punit_focus)
     && (pcity = map_get_city(punit_focus->tile))
     && !diplomat_dialog_is_open()    /* confusing otherwise? */
     && diplomat_can_do_action(punit_focus, DIPLOMAT_ANY_ACTION,
			       punit_focus->tile))
     process_diplomat_arrival(punit_focus, pcity->id);
}

/**************************************************************************
...
**************************************************************************/
void key_unit_done(void)
{
  if (punit_focus) {
    request_unit_move_done();
  }
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
  if (punit_focus && unit_flag(punit_focus, F_TRADE_ROUTE)) {
    request_unit_caravan_action(punit_focus, PACKET_UNIT_ESTABLISH_TRADE);
  }
}

/**************************************************************************
...
**************************************************************************/
void key_unit_unload_all(void)
{
  if (punit_focus) {
    request_unit_unload_all(punit_focus);
  }
}

/**************************************************************************
...
**************************************************************************/
void key_unit_wait(void)
{
  if (punit_focus) {
    request_unit_wait(punit_focus);
  }
}

/**************************************************************************
...
**************************************************************************/
void key_unit_delayed_goto(int flg)
{
    delayed_para_or_nuke = flg;
	request_unit_delayed_goto();
}

/**************************************************************************
...
**************************************************************************/

void key_unit_execute_delayed_goto(void)
{
	request_unit_execute_delayed_goto();
}
/**************************************************************************
...
***************************************************************************/

void key_unit_clear_delayed_orders(void)
{
	request_unit_clear_delayed_orders();
}

/**************************************************************************
...
***************************************************************************/
void key_unit_wakeup_others(void)
{
  if (punit_focus) {
    request_unit_wakeup(punit_focus);
  }
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

void enable_auto_mode(void)//mass auto version
{
  struct unit_list *ulist;

  if (punit_focus == NULL)return;

  if (goto_mode && ctrl_state)//ctrl_state is 1 when Q is pressed
  {//goto for all units of the selected unit type on the same tile
    if(goto_mode==3)ulist = &game.player_ptr->units;
            else ulist = &punit_focus->tile->units;
    unit_list_iterate(*ulist, tunit)
    {
       if((goto_mode == 2 || tunit->type == punit_focus->type)&&(tunit->tile->continent == punit_focus->tile->continent))
       {
         if(can_unit_do_auto(tunit))
             if(unit_satisfies_filter(tunit)) {
                  tunit->is_new = FALSE;
                  request_unit_auto(tunit);
             }
       }
    }unit_list_iterate_end; 
  }else //goto_mode==0 - single unit
  {
    if(can_unit_do_auto(punit_focus)) {
        punit_focus->is_new = FALSE;
        request_unit_auto(punit_focus);
    }
  }
  ctrl_state = 0; 
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
  if (punit_focus) {
    request_unit_disband(punit_focus);
  }
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
  struct unit *tunit;
  struct unit_list *ulist;

  tunit = NULL;
  if (punit_focus == NULL)return;

  if (goto_mode && ctrl_state)
  {//goto for all units of the selected unit type on the same tile
    if(goto_mode==3)ulist = &game.player_ptr->units;
            else ulist = &punit_focus->tile->units;
    unit_list_iterate(*ulist, tunit)
    {
       if((goto_mode == 2 || tunit->type == punit_focus->type)&&(tunit->tile->continent == punit_focus->tile->continent))
       {
         if(can_unit_do_activity(tunit, activity))
             if(unit_satisfies_filter(tunit)) {
                  tunit->is_new = FALSE;
                  request_new_unit_activity(tunit, activity);
             }
       }
    }unit_list_iterate_end; 
  }else //goto_mode==0 - single unit
  {
    if(can_unit_do_activity(punit_focus, activity)) {
        punit_focus->is_new = FALSE;
        request_new_unit_activity(punit_focus, activity);
    }
  }
  ctrl_state = 0;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_fortify(void)
{
   do_mass_order(ACTIVITY_FORTIFYING);
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
  if (punit_focus) {
    request_unit_change_homecity(punit_focus);
  }
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
void key_unit_mine(void)
{
     do_mass_order(ACTIVITY_MINE);
}

/**************************************************************************
...
**************************************************************************/
void key_unit_pillage(void)
{
     do_mass_order(ACTIVITY_PILLAGE);
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
void key_unit_road(void)
{
  if (punit_focus) {
    if(can_unit_do_activity(punit_focus, ACTIVITY_ROAD)) {
      punit_focus->is_new = FALSE;
      request_new_unit_activity(punit_focus, ACTIVITY_ROAD);
    } else if(can_unit_do_activity(punit_focus, ACTIVITY_RAILROAD)) {
      punit_focus->is_new = FALSE;
      request_new_unit_activity(punit_focus, ACTIVITY_RAILROAD);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void key_unit_sentry(void)
{
     do_mass_order(ACTIVITY_SENTRY);
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

void key_toggle_autowakeup(void)
{
	autowakeup_state=1 ^ autowakeup_state;

}
/**************************************************************************
...
**************************************************************************/

void key_set_goto_mode(int mode)
{
	goto_mode=mode;

}
void key_unit_delayed_airlift(void)
{
  hover_state = HOVER_DELAYED_AIRLIFT;
}
