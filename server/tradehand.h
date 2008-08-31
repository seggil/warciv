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

#ifndef FC__TRADE_HAND_H
#define FC__TRADE_HAND_H

#include "traderoute.h"

bool connection_supports_server_trade(struct connection *pconn);

void send_trade_route_info(struct conn_list *dest, struct trade_route *ptr);
void send_trade_route_remove(struct conn_list *dest, struct trade_route *ptr);
void server_remove_trade_route(struct trade_route *ptr);

void server_establish_trade_route(struct city *pcity1, struct city *pcity2);
void unit_establish_trade_route(struct unit *punit, struct city *pcity1,
                                struct city *pcity2);
void trade_free_unit(struct unit *punit);

#endif /* FC__TRADE_HAND_H */
