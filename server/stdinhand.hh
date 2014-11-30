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
#ifndef WC_SERVER_STDINHAND_H
#define WC_SERVER_STDINHAND_H

#include "wc_types.hh"
#include "connection.hh"         /* enum cmdlevel_id */

#define SERVER_COMMAND_PREFIX '/'
  /* the character to mark chatlines as server commands */
#define DEFAULT_ACTION_LIST_FILE "action.list"
#define DEFAULT_TEAM_NAMES_FILE "team_names.txt"

void stdinhand_init(void);
void stdinhand_turn(void);
void stdinhand_free(void);

bool conn_is_muted(const connection_t *pconn);
bool conn_is_kicked(const connection_t *pconn, int *time_remaining);

bool handle_stdin_input(connection_t *caller,
                        const char *str,
                        bool check);
void report_server_options(struct connection_list *dest, int which);
void report_settable_server_options(connection_t *dest, int which);
void set_ai_level_direct(player_t *pplayer, int level);
void set_ai_level_directer(player_t *pplayer, int level);
bool read_init_script(connection_t *caller, char *script_filename);
void show_players(connection_t *caller);
void clearallactions(void);

bool load_command(connection_t *caller, char *arg, bool check);
bool unloadmap_command(connection_t *caller, bool check);

bool require_command(connection_t *caller, char *arg, bool check);

void toggle_ai_player_direct(connection_t *caller,
                             player_t *pplayer);
bool is_allowed_to_attach(const player_t *pplayer,
                          const connection_t *caller, bool will_obs,
                          char *msgbuf, int msgbuf_len);

enum user_allow_behavior {
  USER_ALLOW_BEHAVIOR_OBSERVE = 0,
  USER_ALLOW_BEHAVIOR_GLOBAL_OBSERVE,
  USER_ALLOW_BEHAVIOR_PLAYER_OBSERVE,
  USER_ALLOW_BEHAVIOR_TAKE,
  USER_ALLOW_BEHAVIOR_OTHER_TAKE,
  USER_ALLOW_BEHAVIOR_AI_TAKE,
  USER_ALLOW_BEHAVIOR_DEAD_ATTACH,
  USER_ALLOW_BEHAVIOR_DISPLACE,
  USER_ALLOW_BEHAVIOR_SWITCH,
  USER_ALLOW_BEHAVIOR_PAUSE,

  NUM_USER_ALLOW_BEHAVIOR
};

bool is_allowed(enum user_allow_behavior uab);

struct section_file;
void save_user_allow_behavior_state(struct section_file *file);
void load_user_allow_behavior_state(struct section_file *file);

#ifdef HAVE_LIBREADLINE
#ifdef HAVE_NEWLIBREADLINE
char **warciv_completion(const char *text, int start, int end);
#else
char **warciv_completion(char *text, int start, int end);
#endif
#endif

#endif /* WC_SERVER_STDINHAND_H */
