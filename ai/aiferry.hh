/**********************************************************************
 Freeciv - Copyright (C) 2002 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef WC_AI_AIFERRY_H
#define WC_AI_AIFERRY_H

#include "shared.hh"             /* bool type */

#include "wc_types.hh"

struct pf_path;

/*
 * Initialize ferrybaot-related statistics in the ai data.
 */
void aiferry_init_stats(player_t *pplayer);

/*
 * Find the nearest boat.  Can be called from inside the continents too
 */
int aiferry_find_boat(unit_t *punit, int cap, struct pf_path **path);

/*
 * Release the boat reserved in punit's ai.ferryboat field.
 */
void aiferry_clear_boat(unit_t *punit);

/*
 * Go to the destination by hitching a ride on a boat.  Will try to find
 * a beachhead but it works better if (dest_x, dest_y) is on the coast.
 */
bool aiferry_gobyboat(player_t *pplayer, unit_t *punit,
                      tile_t *dst_tile);

/*
 * Main boat managing function.  Gets units on board to where they want to
 * go and then looks for new passengers or (if it fails) for a city which
 * will build a passenger soon.
 */
void ai_manage_ferryboat(player_t *pplayer, unit_t *punit);

#endif /* WC_AI_AIFERRY_H */
