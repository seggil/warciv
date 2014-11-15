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
#ifndef WC_AI_ADVDOMESTIC_H
#define WC_AI_ADVDOMESTIC_H

#include "wc_types.h"

struct ai_choice;

void ai_eval_threat_init(player_t *pplayer);
void ai_eval_threat_done(player_t *pplayer);
void domestic_advisor_choose_build(player_t *pplayer, city_t *pcity,
                                   struct ai_choice *choice);

#endif  /* WC_AI_ADVDOMESTIC_H */
