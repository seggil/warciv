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
#ifndef WC_SERVER_DIPLHAND_H
#define WC_SERVER_DIPLHAND_H

#define REPUTATION_LOSS_NUKE (GAME_MAX_REPUTATION * 0.03)

#include "wc_types.h"

#include "hand_gen.h"
#include "connection.h"

struct Treaty;
struct packet_diplomacy_info;

void establish_embassy(player_t *pplayer, player_t *aplayer);

void diplhand_init(void);
void diplhand_free(void);
void free_treaties(void);

struct Treaty *find_treaty(player_t *plr0, player_t *plr1);

void send_diplomatic_meetings(connection_t *dest);
void cancel_all_meetings(player_t *pplayer);
void cancel_diplomacy(player_t *pplayer);

#endif  /* WC_SERVER_DIPLHAND_H */
