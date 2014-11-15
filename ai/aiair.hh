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
#ifndef WC_AI_AIAIR_H
#define WC_AI_AIAIR_H

#include "shared.hh"           /* bool type */

#include "wc_types.hh"

struct ai_choice;

void ai_manage_airunit(player_t *pplayer, unit_t *punit);
bool ai_choose_attacker_air(player_t *pplayer, city_t *pcity,
                            struct ai_choice *choice);

int ai_evaluate_tile_for_attack(unit_t *punit, tile_t *dst_tile);

#endif /* WC_AI_AIAIR_H */
