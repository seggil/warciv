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
#ifndef WC_SERVER_GAMEHAND_H
#define WC_SERVER_GAMEHAND_H

#include "connection.h"

struct section_file;

void init_new_game(void);
void send_year_to_clients(int year);
void send_game_info(struct connection_list *dest);
void send_game_state(struct connection_list *dest, int state);
void send_start_turn_to_clients(void);

int update_timeout(void);
void increase_timeout_because_unit_moved(void);

const char *new_challenge_filename(connection_t *pconn);

struct packet_single_want_hack_req;

void handle_single_want_hack_req(connection_t *pconn,
                                 const struct packet_single_want_hack_req *packet);

bool game_is_paused(void);
time_t game_set_pause(bool paused);
void game_save_timeout(void);
int game_get_saved_timeout(void);
void game_restore_timeout(void);

#endif  /* WC_SERVER_GAMEHAND_H */
