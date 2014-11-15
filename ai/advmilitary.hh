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
#ifndef WC_AI_ADVMILITARY_H
#define WC_AI_ADVMILITARY_H

#include "wc_types.h"
#include "unittype.h"

struct ai_choice;

Unit_Type_id ai_choose_defender_versus(city_t *pcity, Unit_Type_id v);
void military_advisor_choose_tech(player_t *pplayer,
                                  struct ai_choice *choice);
void  military_advisor_choose_build(player_t *pplayer, city_t *pcity,
                                    struct ai_choice *choice);
void assess_danger_player(player_t *pplayer);
int assess_defense_quadratic(city_t *pcity);
int assess_defense_unit(city_t *pcity, unit_t *punit, bool igwall);
int assess_defense(city_t *pcity);
int ai_unit_defence_desirability(Unit_Type_id i);
int ai_unit_attack_desirability(Unit_Type_id i);

#endif  /* WC_AI_ADVMILITARY_H */
