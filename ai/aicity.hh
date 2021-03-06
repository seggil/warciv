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
#ifndef WC_AI_AICITY_H
#define WC_AI_AICITY_H

#include "wc_types.hh"
#include "unit.hh"            /* enum unit_move_type */
#include "city.hh"
#include "aidata.hh"

struct ai_choice;

void ai_manage_cities(player_t *pplayer);
void ai_manage_buildings(player_t *pplayer);

Unit_Type_id ai_choose_defender_versus(struct city_s *pcity, Unit_Type_id v);
int ai_eval_calc_city(struct city_s *pcity, struct ai_data *ai);

#endif  /* WC_AI_AICITY_H */
