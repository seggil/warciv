/**********************************************************************
 Freeciv - Copyright (C) 2003 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef WC_COMMON_AICORE_AISUPPORT_H
#define WC_COMMON_AICORE_AISUPPORT_H

#include "../wc_types.hh"

player_t *player_leading_spacerace(void);
int player_distance_to_player(player_t *pplayer, player_t *target);
int city_gold_worth(city_t *pcity);

#endif
