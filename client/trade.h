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

#ifndef FC__TRADE_H
#define FC__TRADE_H

#include "city.h"
#include "unit.h"

void trade_init(void);
void trade_free(void);

void update_trade_route_infos(struct trade_route *ptr);

void add_city_in_trade_planning(struct city *pcity, bool allow_remove);
void trade_remove_city(struct city *pcity);
void clear_trade_city_list(void);
void clear_trade_planning(void);
void do_trade_planning_calculation(void);
struct trade_planning_calculation *get_trade_planning_calculation(void);

void show_trade_estimation(void);
void show_cities_in_trade_planning(void);
void show_free_slots_in_trade_planning(struct trade_route_list *ptrlist);

bool is_trade_city_list(void);
bool is_trade_planning(void);
bool is_trade_route_in_route(void);
int in_route_trade_route_number(struct city *pcity);

void request_trade_route(struct city *pcity);
void request_cancel_trade_route(struct city *pcity1, struct city *pcity2);
void execute_trade_orders(struct unit *punit);
bool trade_free_unit(struct unit *punit);

void delayed_trade_routes_add(int city, int trade[OLD_NUM_TRADEROUTES],
			      int trade_value[OLD_NUM_TRADEROUTES]);
void delayed_trade_routes_build(void);

#endif /* FC__TRADE_H */
