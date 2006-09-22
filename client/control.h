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
#ifndef FC__CONTROL_H
#define FC__CONTROL_H

#include "packets.h"

enum cursor_hover_state {
  HOVER_NONE = 0,
  HOVER_GOTO,
  HOVER_DELAYED_GOTO,
  HOVER_DELAYED_AIRLIFT,
  HOVER_NUKE,
  HOVER_PARADROP,
  HOVER_CONNECT,
  HOVER_PATROL,
  HOVER_MYPATROL,
  HOVER_AIRLIFT_SOURCE,
  HOVER_AIRLIFT_DEST,
};

#define  FILTER_ALL 1
#define  FILTER_NEW 2
#define  FILTER_FORTIFIED 4
#define  FILTER_SENTRIED 8
#define  FILTER_VETERAN 16
#define  FILTER_IDLE 32
#define  FILTER_ABLE_TO_MOVE 64
#define  FILTER_OFF 128

/* Selecting unit from a stack without popup. */
enum quickselect_type {
  SELECT_POPUP = 0, SELECT_SEA, SELECT_LAND
};

extern int hover_unit; /* unit hover_state applies to */
extern enum cursor_hover_state hover_state;
extern enum unit_activity connect_activity;
extern bool draw_goto_line;
extern bool non_ai_unit_focus;
extern int autowakeup_state;
extern int goto_mode;
extern int lastactivatedunit;
extern int airliftunittype;
extern int unit_limit;
extern int look_into_allied_city;
extern int ctrl_state;
extern int default_caravan_action;
extern int default_diplomat_action;
extern unsigned int inclusive_dgoto_filter;
extern unsigned int exclusive_dgoto_filter;

bool unit_satisfies_filter(struct unit *punit);
bool can_unit_do_connect(struct unit *punit, enum unit_activity activity);
void add_unit_to_delayed_goto(struct tile *ptile);
void add_city_to_auto_airlift_queue(struct tile *ptile);
void request_clear_auto_airlift_queue(void);
void request_auto_airlift_source_selection(void);
void request_auto_airlift_destination_selection(void);
void do_airlift(struct tile *ptile);
void show_cities_in_airlift_queue(void);
void key_set_patrol_position(struct tile *ptile);
void key_select_patrol_tile(void);
void request_clear_patrol_queue(void);
void request_execute_patrol(void);
void key_unit_delayed_airlift(void);
void schedule_delayed_airlift(struct tile *ptile);
void enable_auto_mode(void);

void do_move_unit(struct unit *punit, struct unit *target_unit);
void do_unit_goto(struct tile *ptile);
void do_unit_nuke(struct unit *punit);
void do_unit_paradrop_to(struct unit *punit, struct tile *ptile);
void do_unit_patrol_to(struct unit *punit, struct tile *ptile);
void do_unit_connect(struct unit *punit, struct tile *ptile,
		     enum unit_activity activity);
void do_map_click(struct tile *ptile, enum quickselect_type qtype);

void set_hover_state(struct unit *punit, enum cursor_hover_state state,
		     enum unit_activity activity);
void request_center_focus_unit(void);
void request_move_unit_direction(struct unit *punit, int dir);
void request_new_unit_activity(struct unit *punit, enum unit_activity act);
void request_new_unit_activity_targeted(struct unit *punit,
					enum unit_activity act,
					enum tile_special_type tgt);
void request_unit_load(struct unit *pcargo, struct unit *ptransporter);
void request_unit_unload(struct unit *pcargo);
void request_unit_auto(struct unit *punit);
void request_unit_build_city(struct unit *punit);
void request_unit_caravan_action(struct unit *punit, enum packet_type action);
void request_unit_change_homecity(struct unit *punit);
void request_unit_connect(enum unit_activity activity);
void request_unit_disband(struct unit *punit);
void request_unit_fortify(struct unit *punit);
void request_unit_goto(void);
void request_unit_move_done(void);
void request_unit_nuke(struct unit *punit);
void request_unit_paradrop(struct unit *punit);
void request_unit_patrol(void);
void request_unit_pillage(struct unit *punit);
void request_unit_sentry(struct unit *punit);
void request_unit_unload_all(struct unit *punit);
void request_unit_airlift(struct unit *punit, struct city *pcity);
void request_unit_return(struct unit *punit);
void request_unit_upgrade(struct unit *punit);
void request_unit_wait(struct unit *punit);
void request_unit_wakeup(struct unit *punit);
void request_unit_delayed_goto(void);
void request_unit_execute_delayed_goto(void);
void request_unit_clear_delayed_orders(void);
void request_diplomat_action(enum diplomat_actions action, int dipl_id,
			     int target_id, int value);
void request_unit_mass_load(struct unit *punit);

void request_toggle_map_grid(void);
void request_toggle_map_borders(void);
void request_toggle_city_names(void);
void request_toggle_city_growth(void);
void request_toggle_city_productions(void);
void request_toggle_city_production_buy_cost(void);
void request_toggle_city_traderoutes(void);
void request_toggle_terrain(void);
void request_toggle_coastline(void);
void request_toggle_roads_rails(void);
void request_toggle_irrigation(void);
void request_toggle_mines(void);
void request_toggle_fortress_airbase(void);
void request_toggle_specials(void);
void request_toggle_pollution(void);
void request_toggle_cities(void);
void request_toggle_units(void);
void request_toggle_focus_unit(void);
void request_toggle_fog_of_war(void);

void wakeup_sentried_units(struct tile *ptile);

void auto_center_on_focus_unit(void);
void advance_unit_focus(void);
struct unit *get_unit_in_focus(void);
void set_unit_focus(struct unit *punit);
void set_unit_focus_and_select(struct unit *punit);
void update_unit_focus(void);
struct unit *find_visible_unit(struct tile *ptile);
void set_units_in_combat(struct unit *pattacker, struct unit *pdefender);
void blink_active_unit(void);
void update_unit_pix_label(struct unit *punit);

void process_caravan_arrival(struct unit *punit);
void process_diplomat_arrival(struct unit *pdiplomat, int victim_id);

void key_cancel_action(void);
void key_center_capital(void);
void key_city_names_toggle(void);
void key_city_growth_toggle(void);
void key_city_productions_toggle(void);
void key_city_production_buy_cost_toggle(void);
void key_city_traderoutes_toggle(void);
void key_terrain_toggle(void);
void key_coastline_toggle(void);
void key_roads_rails_toggle(void);
void key_irrigation_toggle(void);
void key_mines_toggle(void);
void key_fortress_airbase_toggle(void);
void key_specials_toggle(void);
void key_pollution_toggle(void);
void key_cities_toggle(void);
void key_units_toggle(void);
void key_focus_unit_toggle(void);
void key_fog_of_war_toggle(void);
void key_end_turn(void);
void key_map_grid_toggle(void);
void key_map_borders_toggle(void);
void key_quickselect(enum quickselect_type qtype);
void key_recall_previous_focus_unit(void);
void key_unit_move(enum direction8 gui_dir);
void key_unit_build(void);
void key_unit_airbase(void);
void key_unit_auto_attack(void);
void key_unit_auto_explore(void);
void key_unit_auto_settle(void);
void key_unit_build_city(void);
void key_unit_build_wonder(void);
void key_unit_connect(enum unit_activity activity);
void key_unit_diplomat_actions(void);
void key_unit_disband(void);
void key_unit_done(void);
void key_unit_fallout(void);
void key_unit_fortify(void);
void key_unit_fortress(void);
void key_unit_goto(void);
void key_unit_homecity(void);
void key_unit_irrigate(void);
void key_unit_mine(void);
void key_unit_nuke(void);
void key_unit_patrol(void);
void key_unit_paradrop(void);
void key_unit_pillage(void);
void key_unit_pollution(void);
void key_unit_road(void);
void key_unit_sentry(void);
void key_unit_traderoute(void);
void key_unit_transform(void);
void key_unit_unload_all(void);
void key_unit_wait(void);
void key_unit_wakeup_others(void);
void key_unit_delayed_goto(int flg);
void key_unit_execute_delayed_goto(void);
void key_unit_clear_delayed_orders(void);
void key_toggle_autowakeup(void);
void key_set_goto_mode(int mode);

/* don't change this unless you also put more entries in data/Freeciv */
#define MAX_NUM_UNITS_BELOW 4

extern int num_units_below;

void control_queues_init (void);
void control_queues_free (void);

#endif  /* FC__CONTROL_H */
