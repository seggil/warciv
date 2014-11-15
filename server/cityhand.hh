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
#ifndef WC_AI_CITYHAND_H
#define WC_AI_CITYHAND_H

#include "wc_types.hh"

#include "hand_gen.hh"

void really_handle_city_sell(player_t *pplayer, city_t *pcity,
                             Impr_Type_id id);
void really_handle_city_buy(player_t *pplayer, city_t *pcity);

#endif  /* WC_AI_CITYHAND_H */
