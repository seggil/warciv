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
#ifndef WC_AI_AIDIPLOMAT_H
#define WC_AI_AIDIPLOMAT_H

#include "shared.h"             /* bool type */

#include "wc_types.h"
#include "city.h"               /* struct ai_choice */

void ai_manage_diplomat(player_t *pplayer, unit_t *punit);
void ai_choose_diplomat_defensive(player_t *pplayer,
                                  city_t *pcity,
                                  struct ai_choice *choice, int def);
void ai_choose_diplomat_offensive(player_t *pplayer,
                                  city_t *pcity,
                                  struct ai_choice *choice);

#endif  /* WC_AI_AIDIPLOMAT_H */
