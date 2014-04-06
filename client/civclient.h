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
#ifndef WC_CLIENT_CIVCLIENT_H
#define WC_CLIENT_CIVCLIENT_H

#include "game.h"               /* enum client_states */
#include "packets.h"            /* enum report_type */
#include "shared.h"             /* MAX_LEN_NAME */
#include "version.h"

/*
 * Every TIMER_INTERVAL milliseconds real_timer_callback is
 * called. TIMER_INTERVAL has to stay 500 because real_timer_callback
 * also updates the timeout info.
 */
#define TIMER_INTERVAL 500

void handle_packet_input(void *packet, int type);

void send_goto_unit(unit_t *punit, tile_t *dest_tile);
void send_report_request(enum report_type type);
void send_attribute_block_request(void);
void send_turn_done(void);

void user_ended_turn(void);

void set_client_state(enum client_states newstate);
enum client_states get_client_state(void);

void client_remove_cli_conn(struct connection *pconn);
void client_remove_all_cli_conn(void);

extern char server_host[512];
extern int server_port;
extern char *logfile;
extern char *scriptfile;
extern bool auto_connect;
extern bool waiting_for_end_turn;
extern bool turn_done_sent;
extern time_t end_of_turn;
extern bool do_not_request_hack;

void wait_till_request_got_processed(int request_id);
bool client_is_observer(void);
bool client_is_player(void);
bool client_is_global_observer(void);
void real_timer_callback(void);
bool can_client_issue_orders(void);
bool can_client_change_view(void);
bool can_meet_with_player(player_t *pplayer);
bool can_intel_with_player(player_t *pplayer);

void client_game_init(void);
void client_game_free(void);
void ui_exit(void);

player_t *get_player_ptr(void);
int get_player_idx(void);

#endif  /* WC_CLIENT_CIVCLIENT_H */
