/**********************************************************************
 Freeciv - Copyright (C) 2002 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef WC_AI_AISETTLER_H
#define WC_AI_AISETTLER_H

struct citytile {
  int food, shield, trade, reserved;
};

struct cityresult {
  tile_t *tile;
  int total;              /* total value of position */
  int result;             /* amortized and adjusted total value */
  int corruption;
  int waste;
  bool overseas;          /* have to use boat to get there */
  bool virt_boat;         /* virtual boat was used in search,
                           * so need to build one */
  tile_t *other_tile;     /* coords to best other tile */
  int o_x;
  int o_y;                /* city-relative coords for other tile */
  int city_center;        /* value of city center */
  int best_other;         /* value of best other tile */
  int remaining;          /* value of all other tiles */
  struct citytile citymap[CITY_MAP_SIZE][CITY_MAP_SIZE];
};

void cityresult_fill(player_t *pplayer,
                     struct ai_data *ai,
                     struct cityresult *result);
void find_best_city_placement(unit_t *punit, struct cityresult *best,
                              bool look_for_boat, bool use_virt_boat);
void ai_settler_init(player_t *pplayer);
void print_cityresult(player_t *pplayer, struct cityresult *cr,
                      struct ai_data *ai);

#endif  /* WC_AI_AISETTLER_H */
