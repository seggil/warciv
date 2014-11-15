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
#ifndef WC_SERVER_AIRGOTO_H
#define WC_SERVER_AIRGOTO_H

#include "shared.hh"             /* bool type */

#include "wc_types.hh"

struct refuel;

tile_t *get_refuel_tile(struct refuel *pRefuel);
unsigned int get_turns_to_refuel(struct refuel *pRefuel);

struct pqueue *refuel_iterate_init(player_t *pplayer, tile_t *ptile,
                                   tile_t *dst_tile,
                                   bool cities_only, int moves_left,
                                   int moves_per_turn, int max_moves);
struct refuel *refuel_iterate_next(struct pqueue *rp_list);
void refuel_iterate_end(struct pqueue *rp_list);
void refuel_iterate_process(struct pqueue *rp_list, struct refuel *pfrom);

bool find_air_first_destination(unit_t *punit, tile_t **dst_tile);

#endif  /* WC_SERVER_AIRGOTO_H */
