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
#ifndef FC__MULTISELECT_H
#define FC__MULTISELECT_H

#include "shared.h"
#include "unit.h"
#include "unittype.h"

/********************************************************************** 
  This file contains:
  - filter and selection modes functions.
  - automatic processus (common interface to get events from the game).
  - unit multi-selection functions.
  - delayed goto functions.
  - airlift functions.
***********************************************************************/

/* Selected units iterator macro. */
#define multi_select_iterate(clear_selection, punit) {                     \
  bool _clear_selection = clear_selection;                                 \
  struct unit *_punit, *_punit_next_focus = NULL;                          \
  bool _cond = (multi_select_size(0) > 1);                                 \
  connection_do_buffer(&aconnection);                                      \
  unit_list_iterate(multi_select_get_units_focus(), punit) {               \
    _punit = punit;                                                        \
    if ((_cond                                                             \
	 && !unit_satisfies_filter(punit, multi_select_inclusive_filter,   \
				   multi_select_exclusive_filter))         \
	|| punit->focus_status == FOCUS_DONE) {                            \
      continue;                                                            \
    }
#define multi_select_iterate_end                                           \
    if (unit_satisfies_filter(_punit, multi_select_inclusive_filter,       \
			      multi_select_exclusive_filter)               \
	&&_punit->focus_status == FOCUS_AVAIL) {                           \
      _punit_next_focus = _punit;                                          \
    }                                                                      \
  } unit_list_iterate_end;                                                 \
    connection_do_unbuffer(&aconnection);                                  \
    if (_clear_selection || (_cond && !_punit_next_focus)) {               \
      multi_select_clear(0);                                               \
    } else if (_cond                                                       \
               && !unit_satisfies_filter(get_unit_in_focus(),              \
                                         multi_select_inclusive_filter,    \
                                         multi_select_exclusive_filter)) { \
      if (_punit_next_focus) {                                             \
        set_unit_focus(_punit_next_focus);                                 \
      } else {                                                             \
        advance_unit_focus();                                              \
      }                                                                    \
    }                                                                      \
  }

/********************************************************************** 
  Filters and selection modes. This is a common code for multi-selection
  and delayed goto code. This allow the user to select units as safe as
  possible with simple options.

  The values FILTER_ALL and FILTER_OFF or the filters cannot be combined
  with any other. If you want to change a filter value, use
  filter_change().

  The selection mode is splited in 2 categories: place and unit type.
***********************************************************************/

/* filters RadioItems (All & Off)/CheckItems (all others) */
typedef unsigned int filter;

/*
 * Hard-coded dependences in:
 * - multiselect.c
 * - gui-gtk-2.0/menu.c
 * - options.c: get_utype_mode_name()
 */
enum filter_value {
  FILTER_ALL          = 1 << 0, /* Always statisfed. != all others conditions */
  FILTER_NEW          = 1 << 1,
  FILTER_FORTIFIED    = 1 << 2,
  FILTER_SENTRIED     = 1 << 3,
  FILTER_VETERAN      = 1 << 4,
  FILTER_AUTO         = 1 << 5,
  FILTER_IDLE         = 1 << 6,
  FILTER_ABLE_TO_MOVE = 1 << 7,
  FILTER_FULL_MOVES   = 1 << 8,
  FILTER_FULL_HP      = 1 << 9,
  FILTER_MILITARY     = 1 << 10,
  FILTER_OFF	      = 1 << 11
};

/* Selections mode */

/*
 * Hard-coded dependences in:
 * - control.c
 * - gui-gtk-2.0/menu.c
 * - options.c: get_place_mode_name()
 */
enum place_value {
  PLACE_SINGLE_UNIT,
  PLACE_IN_TRANSPORTER,
  PLACE_ON_TILE,
  PLACE_ON_CONTINENT,
  PLACE_EVERY_WHERE
};

/*
 * Hard-coded dependences in:
 * - control.c
 * - gui-gtk-2.0/menu.c
 * - options.c: get_utype_mode_name()
 */
enum utype_value {
  UTYPE_SAME_TYPE,
  UTYPE_SAME_MOVE_TYPE,
  UTYPE_ALL
};

bool filter_change(filter *pfilter, enum filter_value value);
bool unit_satisfies_filter(struct unit *punit,
                           filter inclusive_filter,
                           filter exclusive_filter);

/**************************************************************************
  Unit multi selection tools.
**************************************************************************/
#define MULTI_SELECT_NUM 10

struct multi_select {
  struct unit_list *ulist;	/* The list of units */
  struct unit *punit_focus;	/* The units which is/was in focus */
};

extern filter multi_select_inclusive_filter;
extern filter multi_select_exclusive_filter;
extern enum place_value multi_select_place;
extern enum utype_value multi_select_utype;

extern bool multi_select_count_all;
extern bool multi_select_blink_all;
extern bool multi_select_blink;
extern bool multi_select_map_selection;
extern bool multi_select_spread_airport_cities;
extern bool multi_select_spread_allied_cities;

bool is_unit_in_multi_select(int multi, struct unit *punit);
void multi_select_active_all(int multi);
void multi_select_add_unit(struct unit *punit);
void multi_select_add_or_remove_unit(struct unit *punit);
void multi_select_add_units(struct unit_list *ulist);
void multi_select_blink_update(void);
void multi_select_cat(int dest, int src);
void multi_select_clear(int multi);
void multi_select_clear_all(void);
void multi_select_copy(int dest, int src);
const struct unit_list *multi_select_get_units_focus(void);
void multi_select_init_all(void);
void multi_select_remove_unit(struct unit *punit);
int multi_select_satisfies_filter(int multi);
void multi_select_set_unit(int multi, struct unit *punit);
void multi_select_set_unit_focus(int multi, struct unit *punit);
int multi_select_size(int multi);
void multi_select_spread(void);
void multi_select_wipe_up_unit(struct unit *punit);
Unit_Type_id multi_select_unit_type(int multi);

void multi_select_select(void);

/**************************************************************************
  Delayed goto...
**************************************************************************/
#define DELAYED_GOTO_NUM 4

enum delayed_goto_type {
  DGT_NORMAL = 0,
  DGT_NUKE,
  DGT_PARADROP,
  DGT_AIRLIFT,
  DGT_BREAK
};

struct delayed_goto_data {
  int id;
  enum delayed_goto_type type;
  struct tile *ptile;
};

#define SPECLIST_TAG delayed_goto_data
#define SPECLIST_TYPE struct delayed_goto_data
#include "speclist.h"
#define delayed_goto_data_list_iterate(alist, pitem)            \
  TYPED_LIST_ITERATE(struct delayed_goto_data, alist, pitem)
#define delayed_goto_data_list_iterate_end LIST_ITERATE_END

struct delayed_goto {
  struct delayed_goto_data_list *dglist; /* The datas of the delayed orders */
  filter automatic_execution;            /* When it will be executed */
  struct player *pplayer;                /* The supposed target player */
};

/*
 * Hard-coded dependences in:
 * - multiselect.c: delayed_goto_get_auto_name()
 */
enum automatic_execution {
  AUTO_NEW_YEAR		= 1 << 0, /* in handle_new_year() (packhand.c) */
  AUTO_PRESS_TURN_DONE	= 1 << 1, /* in send_turn_done() (civclient.c) */
  AUTO_NO_UNIT_SELECTED	= 1 << 2, /* in advance_unit_focus() (control.c) */
  AUTO_50_TIMEOUT	= 1 << 3, /* in auto_timers_update() (multiselect.c) */
  AUTO_80_TIMEOUT	= 1 << 4, /* in auto_timers_update() (multiselect.c) */
  AUTO_90_TIMEOUT	= 1 << 5, /* in auto_timers_update() (multiselect.c) */
  AUTO_95_TIMEOUT	= 1 << 6, /* in auto_timers_update() (multiselect.c) */
  AUTO_5_SECONDS	= 1 << 7, /* in auto_timers_update() (multiselect.c) */
  AUTO_WAR_DIPLSTATE	= 1 << 8, /* in handle_player_info() (packhand.c) */
  AUTO_OFF		= 1 << 9, /* no signal */
};

/* For timeout events */
struct auto_timer {
  long seconds;
  bool npassed;
};

extern filter delayed_goto_inclusive_filter;
extern filter delayed_goto_exclusive_filter;
extern enum place_value delayed_goto_place;
extern enum utype_value delayed_goto_utype;

extern enum delayed_goto_type delayed_goto_state;
extern int delayed_goto_unit_limit;
extern int delayed_goto_need_tile_for;

extern struct delayed_goto delayed_goto_list[DELAYED_GOTO_NUM];

void delayed_goto_add_unit(int dg, int id, int type, struct tile *ptile);
bool delayed_goto_auto_filter_change(filter *pfilter,
				     enum automatic_execution value);
void delayed_goto_auto_timers_init(void);
void delayed_goto_auto_timers_update(void);
void delayed_goto_cat(int dest, int src);
void delayed_goto_clear(int dg);
void delayed_goto_clear_all(void);
void delayed_goto_copy(int dest, int src);
void delayed_goto_event(enum automatic_execution event, struct player *pplayer);
const char *delayed_goto_get_auto_name(enum automatic_execution value);
void delayed_goto_init_all(void);
void delayed_goto_move(int dest, int src);
int delayed_goto_size(int dg);
struct player *get_tile_player(struct tile *ptile);

void add_unit_to_delayed_goto(struct tile *ptile);
void request_player_execute_delayed_goto(struct player *pplayer, int dg);
void request_unit_execute_delayed_goto(int dg);
void request_execute_delayed_goto(struct tile *ptile, int dg);
void schedule_delayed_airlift(struct tile *ptile);
void add_pause_delayed_goto(void);

/**************************************************************************
  Airlift queues...
**************************************************************************/
#define AIRLIFT_QUEUE_NUM 7

struct airlift_queue {
  struct tile_list *tlist;		/* The airlift sources */
  char namemenu[U_LAST +1 ][256];	/* acces name for unit type menus */
  Unit_Type_id utype;			/* The selected unit type */
};

extern int airlift_queue_need_city_for;

void airlift_queue_cat(int dest,int src);
void airlift_queue_clear(int aq);
void airlift_queue_clear_all(void);
void airlift_queue_copy(int dest, int src);
const struct airlift_queue *airlift_queue_get(int aq);
const char *airlift_queue_get_menu_name(int aq, Unit_Type_id utype);
Unit_Type_id airlift_queue_get_unit_type(int aq);
void airlift_queue_init_all(void);
void airlift_queue_move(int dest,int src);
void airlift_queue_set_menu_name(int aq, Unit_Type_id utype, 
                                 const char *namemenu);
void airlift_queue_set_unit_type(int aq, Unit_Type_id utype);
void airlift_queue_show(int aq);
int airlift_queue_size(int aq);

void add_city_to_auto_airlift_queue(struct tile *ptile, bool multi);
void request_auto_airlift_source_selection_with_airport(void);
void do_airlift_for(int aq, struct city *pcity);
void do_airlift(struct tile *ptile);

#endif /* FC__MULTISELECT_H */
