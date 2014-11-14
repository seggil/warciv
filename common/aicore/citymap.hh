/**********************************************************************
 Freeciv - Copyright (C) 2003 - Per I. Mathisen
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef WC_COMMON_AICORE_CITYMAP_H
#define WC_COMMON_AICORE_CITYMAP_H

#include "wc_intl.h"

void citymap_turn_init(player_t *pplayer);
void citymap_reserve_city_spot(tile_t *ptile, int id);
void citymap_free_city_spot(tile_t *ptile, int id);
void citymap_reserve_tile(tile_t *ptile, int id);
int citymap_read(tile_t *ptile);
bool citymap_is_reserved(tile_t *ptile);

#endif
