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
#ifndef WC_SERVER_SPACERACE_H
#define WC_SERVER_SPACERACE_H

#include "wc_types.h"
#include "packets.h"

struct player_spaceship;

void spaceship_calc_derived(struct player_spaceship *ship);
void send_spaceship_info(player_t *src, struct connection_list *dest);
void spaceship_lost(player_t *pplayer);
void check_spaceship_arrivals(void);

void handle_spaceship_launch(player_t *pplayer);
void handle_spaceship_place(player_t *pplayer,
                            enum spaceship_place_type type, int num);

#endif /* WC_SERVER_SPACERACE_H */
