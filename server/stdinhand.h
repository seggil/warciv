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
#ifndef FC__STDINHAND_H
#define FC__STDINHAND_H

#include "connection.h"		/* enum cmdlevel_id */
#include "fc_types.h"

#define SERVER_COMMAND_PREFIX '/'
  /* the character to mark chatlines as server commands */
#define DEFAULT_ACTION_LIST_FILE "action.list"

void stdinhand_init(void);
void stdinhand_turn(void);
void stdinhand_free(void);

bool conn_is_muted(struct connection *pconn);

bool handle_stdin_input(struct connection *caller,
                        const char *str,
                        bool check);
void report_server_options(struct conn_list *dest, int which);
void report_settable_server_options(struct connection *dest, int which);
void set_ai_level_direct(struct player *pplayer, int level);
void set_ai_level_directer(struct player *pplayer, int level);
bool read_init_script(struct connection *caller, char *script_filename);
void show_players(struct connection *caller);
void clearallactions(void);

bool load_command(struct connection *caller, char *arg, bool check);

bool require_command(struct connection *caller, char *arg, bool check);

void toggle_ai_player_direct(struct connection *caller,
			     struct player *pplayer);

#ifdef HAVE_LIBREADLINE
#ifdef HAVE_NEWLIBREADLINE
char **freeciv_completion(const char *text, int start, int end);
#else
char **freeciv_completion(char *text, int start, int end);
#endif
#endif

#endif /* FC__STDINHAND_H */
