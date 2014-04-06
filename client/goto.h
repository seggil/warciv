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
#ifndef WC_CLIENT_GOTO_H
#define WC_CLIENT_GOTO_H

#include "map.h"

#include "aicore/path_finding.h"

void init_client_goto(void);
void free_client_goto(void);
void enter_goto_state(unit_t *punit);
void exit_goto_state(void);
bool goto_is_active(void);
tile_t *get_line_dest(void);
int get_goto_turns(void);
void goto_add_waypoint(void);
bool goto_pop_waypoint(void);

void draw_line(tile_t *dest_tile);
bool is_drawn_line(tile_t *dest_tile, int dir);

bool is_endpoint(tile_t *ptile);

void request_orders_cleared(unit_t *punit);
void send_goto_path(unit_t *punit, struct pf_path *path,
                    enum unit_activity final_activity);
void send_patrol_route(unit_t *punit);
void send_goto_route(unit_t *punit);
void send_connect_route(unit_t *punit, enum unit_activity activity);
void unit_goto(unit_t *punit, tile_t *ptile);

struct pf_path *path_to_nearest_allied_city(unit_t *punit);

int calculate_move_cost(unit_t *punit, tile_t *dest_tile);
tile_t *find_nearest_city(unit_t *punit, bool allies);

#endif  /* WC_CLIENT_GOTO_H */
