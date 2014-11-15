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
#ifndef WC_SERVER_GOTOHAND_H
#define WC_SERVER_GOTOHAND_H

#include "map.h"                /* MAP_MAX_ */

/*
 * The below GOTO result values are ordered by priority, e.g. if unit
 * fought and run out of movepoints, GR_OUT_OF_MOVEPOINTS should be
 * returned.  Not that it is of any importance...
 */
enum goto_result {
  GR_DIED,               /* pretty obvious that */
  GR_ARRIVED,            /* arrived to the destination */
  GR_OUT_OF_MOVEPOINTS,  /* no moves left */
  GR_WAITING,            /* waiting due to danger, has moves */
  GR_FOUGHT,             /* was stopped due to fighting, has moves */
  GR_FAILED              /* failed for some other reason, has moves */
};

bool is_dist_finite(int dist);
enum goto_result do_unit_goto(unit_t *punit,
                              enum goto_move_restriction restriction,
                              bool trigger_special_ability);
void generate_warmap(city_t *pcity, unit_t *punit);
void really_generate_warmap(city_t *pcity, unit_t *punit,
                            enum unit_move_type move_type);
int calculate_move_cost(unit_t *punit, tile_t *dst_tile);
int air_can_move_between(int moves, tile_t *src_tile,
                         tile_t *dst_tile, player_t *pplayer);

/* all other functions are internal */

#define THRESHOLD 12


bool goto_is_sane(unit_t *punit, tile_t *ptile, bool omni);

struct move_cost_map {
  unsigned char *cost;
  unsigned char *seacost;
  unsigned char *vector;
  int size;

  city_t *warcity; /* so we know what we're dealing with here */
  unit_t *warunit; /* so we know what we're dealing with here */
  tile_t *orig_tile;
};

extern struct move_cost_map warmap;

#define WARMAP_COST(ptile) (warmap.cost[(ptile)->index])
#define WARMAP_SEACOST(ptile) (warmap.seacost[(ptile)->index])
#define WARMAP_VECTOR(ptile) (warmap.vector[(ptile)->index])

#endif  /* WC_SERVER_GOTOHAND_H */
