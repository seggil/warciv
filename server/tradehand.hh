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

#ifndef WC_SERVER_TRADE_HAND_H
#define WC_SERVER_TRADE_HAND_H

#include "traderoute.hh"

bool connection_supports_server_trade(connection_t *pconn);

void send_trade_route_info(struct connection_list *dest, struct trade_route *ptr);
void send_trade_route_remove(struct connection_list *dest, struct trade_route *ptr);
void server_remove_trade_route(struct trade_route *ptr);

void server_establish_trade_route(city_t *pcity1, city_t *pcity2);
void unit_establish_trade_route(unit_t *punit, city_t *pcity1,
                                city_t *pcity2);
void trade_free_unit(unit_t *punit);

void reset_trade_route_planning(player_t *pplayer);

#endif /* WC_SERVER_TRADE_HAND_H */
