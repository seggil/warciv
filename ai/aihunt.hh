/**********************************************************************
 Freeciv - Copyright (C) 2003 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef WC_AI_AIHUNT_H
#define WC_AI_AIHUNT_H

#include "shared.hh"          /* bool type */

#include "wc_types.hh"

struct ai_choice;

void ai_hunter_choice(player_t *pplayer, city_t *pcity,
                      struct ai_choice *choice);
bool ai_hunter_qualify(player_t *pplayer, unit_t *punit);
int ai_hunter_findjob(player_t *pplayer, unit_t *punit);
bool ai_hunter_manage(player_t *pplayer, unit_t *punit);

#endif
