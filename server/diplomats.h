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
#ifndef WC_SERVER_DIPLOMATS_H
#define WC_SERVER_DIPLOMATS_H

#include "wc_types.h"
#include "unit.h"               /* Unit_Type_id, enum unit_activity */

#include "gotohand.h"           /* enum goto_move_restriction */

void diplomat_embassy(struct player *pplayer, struct unit *pdiplomat,
                          city_t *pcity);
void diplomat_investigate(struct player *pplayer, struct unit *pdiplomat,
                          city_t *pcity);
void spy_get_sabotage_list(struct player *pplayer, struct unit *pdiplomat,
                           city_t *pcity);
void spy_poison(struct player *pplayer, struct unit *pdiplomat,
                city_t *pcity);
void spy_sabotage_unit(struct player *pplayer, struct unit *pdiplomat,
                       struct unit *pvictim);
void diplomat_bribe(struct player *pplayer, struct unit *pdiplomat,
                    struct tile *ptile);
void diplomat_get_tech(struct player *pplayer, struct unit *pdiplomat,
                       city_t  *pcity, int technology);
void diplomat_incite(struct player *pplayer, struct unit *pdiplomat,
                     city_t *pcity);
void diplomat_sabotage(struct player *pplayer, struct unit *pdiplomat,
                       city_t *pcity, Impr_Type_id improvement);

int count_diplomats_on_tile(struct tile *ptile);
int unit_bribe_cost(struct unit *punit);

#endif  /* WC_SERVER_DIPLOMATS_H */
