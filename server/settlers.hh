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
#ifndef WC_SERVER_SETTLERS_H
#define WC_SERVER_SETTLERS_H

#include "wc_types.hh"
#include "map.hh"

void auto_settlers_init(void);
void auto_settlers_player(player_t *pplayer);
int find_boat(player_t *pplayer, tile_t **boat_tile, int cap);

#define MORT 24

int amortize(int benefit, int delay);
void ai_manage_settler(player_t *pplayer, unit_t *punit);

void init_settlers(void);

int city_tile_value(city_t *pcity, int x, int y,
                    int foodneed, int prodneed);
void initialize_infrastructure_cache(player_t *pplayer);

void contemplate_terrain_improvements(city_t *pcity);
void contemplate_new_city(city_t *pcity);

unit_t *other_passengers(unit_t *punit);

extern signed int *minimap;
#define MINIMAP(ptile) minimap[(ptile)->index]

#endif   /* WC_SERVER_SETTLERS_H */
