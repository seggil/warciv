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
#ifndef FC__MYAI_H
#define FC__MYAI_H

#include "city.h"
#include "unit.h"
#include "shared.h"

#include "multiselect.h"

/********************************************************************** 
  Main and common functions.
***********************************************************************/
int my_ai_count_activity(enum my_ai_activity activity);
const char *my_ai_unit_activity(struct unit *punit);
const char *my_ai_unit_orders(struct unit *punit);
const struct unit_list *my_ai_get_units(enum my_ai_activity activity);

void my_ai_caravan(struct unit *punit);

void my_ai_orders_free(struct unit *punit);
void my_ai_unit_execute(struct unit *punit);
void my_ai_execute(void);

void my_ai_city_free(struct city *pcity);

void my_ai_init(void);
void my_ai_free(void);

/********************************************************************** 
  Trade functions.
***********************************************************************/
#define MAX_ESTIMATED_TURNS 32

void my_ai_trade_route_alloc(struct trade_route *ptr);
void my_ai_trade_route_alloc_city(struct unit *punit, struct city *pcity);
void my_ai_trade_route_free(struct unit *punit);
void my_ai_trade_route_execute(struct unit *punit);
void my_ai_trade_route_execute_all(void);

struct trade_route *trade_route_new(struct unit *punit, struct city *pc1,
				    struct city *pc2, bool planned);
const struct trade_route_list *estimate_non_ai_trade_route(void);
int estimate_non_ai_trade_route_number(struct city *pcity);
void non_ai_trade_change(struct unit *punit,int action);
int count_trade_routes(struct city *pcity);
void set_trade_planning(const struct trade_route_list *ptrl);
const struct trade_route_list *my_ai_trade_plan_get(void);
const struct city_list *my_ai_get_trade_cities(void);

void my_ai_add_trade_city(struct city *pcity, bool multi);
void show_cities_in_trade_plan(void);
void recalculate_trade_plan(void);
void clear_my_ai_trade_cities(void);
void show_free_slots_in_trade_plan(void);
void recalculate_trade_plan(void);
void calculate_trade_estimation(void);

/********************************************************************** 
  Patrol functions.
***********************************************************************/
void my_ai_patrol_alloc(struct unit *punit, struct tile *ptile);
void my_ai_patrol_execute(struct unit *punit);
void my_ai_patrol_execute_all(void);
void my_ai_patrol_free(struct unit *punit);

/********************************************************************** 
  Spread units.
***********************************************************************/
void my_ai_spread_execute(void);

#endif /* FC__MYAI_H */
