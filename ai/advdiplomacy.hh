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
#ifndef WC_AI_ADVDIPLOMACY_H
#define WC_AI_ADVDIPLOMACY_H

#include "wc_types.hh"

struct ai_choice;
struct Treaty;
struct Clause;
struct ai_data;

void ai_diplomacy_calculate(player_t *pplayer, struct ai_data *ai);
void ai_diplomacy_actions(player_t *pplayer);

void ai_treaty_evaluate(player_t *pplayer, player_t *aplayer,
                        struct Treaty *ptreaty);
void ai_treaty_accepted(player_t *pplayer, player_t *aplayer,
                        struct Treaty *ptreaty);

#endif
