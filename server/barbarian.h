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

#ifndef WC_SERVER_BARBARIAN_H
#define WC_SERVER_BARBARIAN_H

#include "shared.h"             /* bool type */

#include "wc_types.h"

#define MIN_UNREST_DIST   5
#define MAX_UNREST_DIST   8

#define UPRISE_CIV_SIZE  10
#define UPRISE_CIV_MORE  30
#define UPRISE_CIV_MOST  50

#define MAP_FACTOR     2000  /* adjust this to get a good uprising frequency */

bool unleash_barbarians(tile_t *ptile);
void summon_barbarians(void);
bool is_land_barbarian(player_t *pplayer);

#endif  /* WC_SERVER_BARBARIAN_H */
