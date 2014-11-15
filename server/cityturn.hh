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

#ifndef WC_SERVER_CITYTURN_H
#define WC_SERVER_CITYTURN_H

#include "shared.h"             /* bool type */

#include "wc_types.h"
#include "aicore/cm.h"

void city_refresh(city_t *pcity);          /* call if city has changed */
void global_city_refresh(player_t *pplayer); /* tax/govt changed */

void auto_arrange_workers(city_t *pcity); /* will arrange the workers */
void apply_cmresult_to_city(city_t *pcity, struct cm_result *cmr);

bool city_reduce_size(city_t *pcity, int pop_loss);
void send_global_city_turn_notifications(struct connection_list *dest);
void send_city_turn_notifications(struct connection_list *dest, city_t *pcity);
void update_city_activities(player_t *pplayer);
int city_incite_cost(player_t *pplayer, city_t *pcity);
void remove_obsolete_buildings_city(city_t *pcity, bool refresh);
void remove_obsolete_buildings(player_t *pplayer);

void advisor_choose_build(player_t *pplayer, city_t *pcity);

void nullify_prechange_production(city_t *pcity);
#endif  /* WC_SERVER_CITYTURN_H */
