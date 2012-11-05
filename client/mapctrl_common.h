/**********************************************************************
 Freeciv - Copyright (C) 2002 - The Freeciv Poject
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef WC_CLIENT_MAPCTRL_COMMON_H
#define WC_CLIENT_MAPCTRL_COMMON_H

#include "map.h"                /* enum direction8 */
#include "shared.h"             /* bool type */

#include "control.h"            /* quickselect_type */

extern bool rbutton_down;
extern bool rectangle_active;
extern bool tiles_hilited_cities;

extern bool keyboardless_goto_button_down;
extern bool keyboardless_goto_active;
extern tile_t *keyboardless_goto_start_tile;

extern tile_t *dist_first_tile, *dist_last_tile;

void anchor_selection_rectangle(int canvas_x, int canvas_y);
void update_selection_rectangle(int canvas_x, int canvas_y);
void redraw_selection_rectangle(void);

void update_distance_tool(int canvas_x, int canvas_y);
void cancel_distance_tool(void);

bool is_city_hilited(city_t *pcity);

void cancel_tile_hiliting(void);
void toggle_tile_hilite(tile_t *ptile);

void key_city_overlay(int canvas_x, int canvas_y);
void key_cities_overlay(int canvas_x, int canvas_y);

void clipboard_copy_production(tile_t *ptile);
void clipboard_paste_production(city_t *pcity);
void upgrade_canvas_clipboard(void);

void release_right_button(int canvas_x, int canvas_y);

void release_goto_button(int canvas_x, int canvas_y);
void maybe_activate_keyboardless_goto(int canvas_x, int canvas_y);

bool get_turn_done_button_state(void);
void scroll_mapview(enum direction8 gui_dir);
void action_button_pressed(int canvas_x, int canvas_y,
                enum quickselect_type qtype);
void wakeup_button_pressed(int canvas_x, int canvas_y);
void adjust_workers_button_pressed(int canvas_x, int canvas_y);
void recenter_button_pressed(int canvas_x, int canvas_y);
void update_turn_done_button_state(void);
void update_line(int canvas_x, int canvas_y);
void overview_update_line(int overview_x, int overview_y);

bool get_chance_to_win(int *att_chance, int *def_chance,
                       tile_t *ptile);

void fill_tile_unit_list(tile_t *ptile, struct unit **unit_list);

extern city_t *city_workers_display;

#endif /* WC_CLIENT_MAPCTRL_COMMON_H */
