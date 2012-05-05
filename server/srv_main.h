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
#ifndef WC_SERVER_SRV_MAIN_H
#define WC_SERVER_SRV_MAIN_H

#include "wc_types.h"
#include "game.h"
#include "packets.h"

struct connection;

BV_DEFINE(bv_draw, MAX_NUM_PLAYERS);

struct server_arguments {
  /* metaserver information */
  bool metaserver_no_send;
  char metaserver_addr[256];
  char metasendhost[256];
        /* the name which should be send to metaserver as host=<> parameter*/
  unsigned short int metaserver_port;
  /* how long to wait before retrying to connect to the
     metaserver, if a connection attempt fails */
  int metaserver_fail_wait_time;
  /* address this server is to listen on (NULL => INADDR_ANY) */
  char *bind_addr;
  /* this server's listen port */
  int port;
  /* the log level */
  int loglevel;
  /* filenames */
  char *log_filename;
  char *gamelog_filename;
  char load_filename[512]; /* FIXME: may not be long enough? use MAX_PATH? */
  char *script_filename;
  char *saves_pathname;
  char serverid[256];
  /* save a ppm of the map? */
  bool save_ppm;
  /* quit if there no players after a given time interval */
  int quitidle;
  /* exit the server on game ending */
  bool exit_on_end;
  /* what kind of end game we should use */
  bv_draw draw;

  /* authentication options */
  struct server_arguments_auth {
    bool enabled;            /* defaults to FALSE */
    bool allow_guests;       /* defaults to TRUE */
    bool allow_newusers;     /* defaults to TRUE */
    bool salted;             /* defaults to FALSE */
  } auth;

  bool no_dns_lookup; /* defaults to FALSE */

  char required_cap[MAX_LEN_CAPSTR];
  bool allow_multi_line_chat;

  bool hack_request_disabled;

  struct server_arguments_wcdb {
    bool enabled;           /* Defaults to FALSE. */
    int min_rated_turns;    /* Defaults to 30. */
    bool save_maps;         /* Defaults to FALSE. */

    /* The current wcdb database format makes the
     * full /examine command query too slow (several
     * seconds) for databases with more than ~3000
     * games. This is a temporary work-around until
     * a better database format is implemented. */
    bool more_game_info;    /* Defaults to FALSE. */
  } wcdb;
};

void init_game_seed(void);
void srv_init(void);
void srv_main(void);
void server_quit(void);

void save_game_auto(void);

bool handle_packet_input(struct connection *pconn, void *packet, int type);
void start_game(void);
void save_game(char *orig_filename);
void pick_ai_player_name(Nation_Type_id nation, char *newname);
void send_all_info(struct conn_list *dest);
void check_for_full_turn_done(void);

void dealloc_id(int id);
void alloc_id(int id);
int get_next_id_number(void);
void server_game_init(void);
void server_game_free(void);
void server_init_player_maps(void);
void server_free_player_maps(void);

void check_for_full_turn_done(void);

extern struct server_arguments srvarg;

extern enum server_states server_state;
extern bool nocity_send;

extern bool force_end_of_sniff;

/* May be NULL, in which case the default welcome message is used. */
extern char *welcome_message;

Nation_Type_id select_random_nation(const char* class);
void send_select_nation(struct player *pplayer);
void mark_nation_as_used(Nation_Type_id nation);

typedef bool (*background_func)(void *context);
typedef void (*context_free_func)(void *context);
int register_background_function(background_func bf,
                                 void *context,
                                 context_free_func cff);
void unregister_background_function(int id);
void execute_background_functions(void);

#endif /* WC_SERVER_SRV_MAIN_H */
