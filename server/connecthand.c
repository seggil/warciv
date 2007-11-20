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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <time.h>

#include "capability.h"
#include "capstr.h"
#include "events.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "packets.h"
#include "player.h"
#include "rand.h"
#include "shared.h"
#include "support.h"
#include "version.h"
#include "wildcards.h"

#include "auth.h"
#include "diplhand.h"
#include "gamehand.h"
#include "gamelog.h"
#include "maphand.h"
#include "meta.h"
#include "plrhand.h"
#include "ruleset.h"
#include "sernet.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "stdinhand_info.h"

#include "connecthand.h"

struct user_action_list on_connect_user_actions;

/* NB Must match enum action_type in connecthand.h */
char *user_action_type_strs[NUM_ACTION_TYPES] = {
  "ban",
  "none",
  "observer",
  "basic",
  "ctrl",
  "admin",
  "hack"
};

static int generate_welcome_message(char *buf, int buf_len,
				    char *welcome_msg);


bool can_control_a_player(struct connection *pconn, bool message)
{
  char *cap[256];
  int ntokens = 0, i;

  if(!strlen(srvarg.required_cap))
    return TRUE;
 
  ntokens = get_tokens(srvarg.required_cap, cap, 256, TOKEN_DELIMITERS);

  for(i = 0; i < ntokens; i++) {
    if(!has_capability(cap[i], pconn->capability)) {
      if(message)
        notify_conn(&pconn->self, _("Server: Sorry, you haven't got the '%s' capability, "
        	"which is required to play on this server."), cap[i]);
      return FALSE;
    }
  }

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
void clear_all_on_connect_user_actions(void)
{
  user_action_list_iterate(on_connect_user_actions, pua) {
    user_action_free(pua);
  } user_action_list_iterate_end;
  user_action_list_unlink_all(&on_connect_user_actions);
}

/**************************************************************************
  ...
**************************************************************************/
int user_action_as_str(struct user_action *pua, char *buf, int buflen)
{
  char buf2[128];
  conn_pattern_as_str(pua->conpat, buf2, sizeof(buf2));
  return my_snprintf(buf, buflen, "<%s %s>",
		     user_action_type_strs[pua->action], buf2);
}

/**************************************************************************
  ...
**************************************************************************/
static enum action_type
find_action_for_connection(char *username,
			   struct connection *pconn, int pref)
{
  bool match = FALSE;
  char buf[128];

  user_action_list_iterate(on_connect_user_actions, pua) {
    user_action_as_str(pua, buf, sizeof(buf));
    freelog(LOG_DEBUG, "testing action %s against %s "
	    " (username=\"%s\", pref=%d)", buf,
	    conn_description(pconn), username, pref);

    if (0 <= pref && pref < NUM_ACTION_TYPES && pua->action != pref)
      continue;

    match = conn_pattern_match(pua->conpat, pconn, username);

    if (match) {
      freelog(LOG_DEBUG, "  matched! returning action %d", pua->action);
      return pua->action;
    }
  } user_action_list_iterate_end;

  return NUM_ACTION_TYPES;	/* i.e. nothing applicable found */
}

/**************************************************************************
  ...
**************************************************************************/
bool is_banned(char *username, struct connection *pconn)
{
  int action;

  action = find_action_for_connection(username, pconn, ACTION_BAN);
  return action == ACTION_BAN;
}

/**************************************************************************
  ...
**************************************************************************/
void grant_access_level(struct connection *pconn)
{
  switch (find_action_for_connection(NULL, pconn, -1)) {
  case ACTION_BAN:
    notify_conn(&pconn->self, _("You are banned from this server."));
    server_break_connection(pconn);
    return;
  case ACTION_GIVE_NONE:
    pconn->granted_access_level = pconn->access_level = ALLOW_NONE;
    break;
  case ACTION_GIVE_OBSERVER:
    pconn->granted_access_level = pconn->access_level = ALLOW_OBSERVER;
    break;
  case ACTION_GIVE_BASIC:
    pconn->granted_access_level = pconn->access_level = ALLOW_BASIC;
    break;
  case ACTION_GIVE_CTRL:
    pconn->granted_access_level = pconn->access_level = ALLOW_CTRL;
    break;
  case ACTION_GIVE_ADMIN:
    pconn->granted_access_level = pconn->access_level = ALLOW_ADMIN;
    break;
  case ACTION_GIVE_HACK:
    pconn->granted_access_level = pconn->access_level = ALLOW_HACK;
    break;
  default:
    /* Use the default access level from sernet.c */
    pconn->granted_access_level = pconn->access_level;
    break;
  }

  /* Maybe this connection doesn't have the required capabilities */
  if(pconn->access_level > ALLOW_OBSERVER
     && !can_control_a_player(pconn, FALSE)) {
    pconn->granted_access_level = pconn->access_level = ALLOW_OBSERVER;
  }
}

/**************************************************************************
  This is used when a new player joins a server, before the game
  has started.  If pconn is NULL, is an AI, else a client.

  N.B. this only attachs a connection to a player if 
       pconn->name == player->username
**************************************************************************/
void establish_new_connection(struct connection *pconn)
{
  struct conn_list *dest = &pconn->self;
  struct player *pplayer;
  struct packet_server_join_reply packet;
  char hostname[512];

  /* zero out the password */
  memset(pconn->server.password, 0, sizeof(pconn->server.password));
  /* send off login_replay packet */
  packet.you_can_join = TRUE;
  sz_strlcpy(packet.capability, our_capability);
  my_snprintf(packet.message, sizeof(packet.message), _("%s Welcome"),
              pconn->username);
  sz_strlcpy(packet.challenge_file, new_challenge_filename(pconn));
  packet.conn_id = pconn->id;
  send_packet_server_join_reply(pconn, &packet);

  /* "establish" the connection */
  pconn->established = TRUE;
  pconn->server.status = AS_ESTABLISHED;

  /* introduce the server to the connection */
  if (!welcome_message) {
    /* The default. */
  if (my_gethostname(hostname, sizeof(hostname)) == 0) {
    notify_conn(dest, _("Welcome to the %s Server running at %s port %d."),
                freeciv_name_version(), hostname, srvarg.port);
  } else {
    notify_conn(dest, _("Welcome to the %s Server at port %d."),
                freeciv_name_version(), srvarg.port);
  }
  } else {
    /* Though it is possible that maxlen will not be enough
       space for all the expansions, at worst it will result
       in a truncated message. */
    int maxlen = strlen(welcome_message)
	+ 64 * strchrcount(welcome_message, '%') + 1;
    char *buf = fc_malloc(maxlen), *line, *p;
    generate_welcome_message(buf, maxlen, welcome_message);

    /* Send the welcome message line by line. This way we don't
       run into the MAX_LEN_MSG limit notify_conn has (at least
       not easily). */
    for (line = buf; line;) {
      if ((p = strchr(line, '\n')))
	*p++ = '\0';
      notify_conn(dest, "%s", line);
      line = p;
    }
    free(buf);
  }

  /* FIXME: this (getting messages about others logging on) should be a 
   * message option for the client with event */

  /* notify the console and other established connections that you're here */
  freelog(LOG_NORMAL, _("%s has connected from %s."),
          pconn->username, pconn->addr);
  conn_list_iterate(game.est_connections, aconn) {
    if (aconn != pconn) {
      notify_conn(&aconn->self, _("Server: %s has connected from %s."),
                  pconn->username, pconn->addr);
    }
  } conn_list_iterate_end;

  /* a player has already been created for this user, reconnect him */
  if ((pplayer = find_player_by_user(pconn->username))) {
    attach_connection_to_player(pconn, pplayer);

    if(server_state == SELECT_RACES_STATE) {
      send_packet_freeze_hint(pconn);
      send_rulesets(dest);		
      send_player_info(NULL, NULL);
      send_packet_thaw_hint(pconn);
      if(pplayer->nation == NO_NATION_SELECTED) {
        send_select_nation(pplayer);
      }
    } else if (server_state == RUN_GAME_STATE) {
      /* Player and other info is only updated when the game is running.
       * See the comment in lost_connection_to_client(). */
      send_packet_freeze_hint(pconn);
      send_rulesets(dest);
      send_player_info(NULL, NULL);
      send_all_info(dest);
      send_game_state(dest, CLIENT_GAME_RUNNING_STATE);
      send_diplomatic_meetings(pconn);
      send_packet_thaw_hint(pconn);
      send_packet_start_turn(pconn);
    }

    /* This must be done after the above info is sent, because it will
     * generate a player-packet which can't be sent until (at least)
     * rulesets are sent. */
    if (game.auto_ai_toggle && pplayer->ai.control) {
      toggle_ai_player_direct(NULL, pplayer);
    }

    gamelog(GAMELOG_PLAYER, pplayer);

  } else if (server_state == PRE_GAME_STATE && game.is_new_game) {
    if (!attach_connection_to_player(pconn, NULL)) {
      notify_conn(dest, _("Server: Couldn't attach your connection to new player."));
      freelog(LOG_VERBOSE, "%s is not attached to a player",
	      pconn->username);
    } else {
      sz_strlcpy(pconn->player->name, pconn->username);
    }
  }

  /* remind the connection who he is */
  if (!pconn->player) {
    notify_conn(dest, _("Server: You are logged in as '%s' connected to no player."),
                pconn->username);
  } else if (strcmp(pconn->player->name, ANON_PLAYER_NAME) == 0) {
    notify_conn(dest, _("Server: You are logged in as '%s' connected to an "
			"anonymous player."), pconn->username);
  } else {
    notify_conn(dest, _("Server: You are logged in as '%s' connected to %s."),
                pconn->username, pconn->player->name);
  }

  /* if need be, tell who we're waiting on to end the game turn */
  if (server_state == RUN_GAME_STATE && game.turnblock) {
    players_iterate(cplayer) {
      if (cplayer->is_alive && !cplayer->ai.control && !cplayer->turn_done && cplayer != pconn->player) {	/* skip current player */
	notify_conn(dest, _("Game: Turn-blocking game play: "
                            "waiting on %s to finish turn..."),
                    cplayer->name);
      }
    } players_iterate_end;
  }

  send_conn_info(dest, &game.est_connections);
  conn_list_append(&game.est_connections, pconn);

  if (server_state != RUN_GAME_STATE) {
    show_connections(pconn);
  } else {
    show_players(pconn);
  }

  if (conn_list_size(&game.est_connections) == 1) {
    /* First connection
     * Replace "restarting in x seconds" meta message */
     maybe_automatic_meta_message(default_meta_message_string());
     (void) send_server_info_to_metaserver(META_INFO);
  }
  send_conn_info(&game.est_connections, dest);
  (void) send_server_info_to_metaserver(META_INFO);
}

/**************************************************************************
  send the rejection packet to the client.
**************************************************************************/
void reject_new_connection(const char *msg, struct connection *pconn)
{
  struct packet_server_join_reply packet;

  /* zero out the password */
  memset(pconn->server.password, 0, sizeof(pconn->server.password));

  packet.you_can_join = FALSE;
  sz_strlcpy(packet.capability, our_capability);
  sz_strlcpy(packet.message, msg);
  packet.challenge_file[0] = '\0';
  packet.conn_id = -1;
  send_packet_server_join_reply(pconn, &packet);
  freelog(LOG_NORMAL, _("Client %s rejected: %s"),
	  conn_description(pconn), msg);
  flush_connection_send_buffer_all(pconn);
}

/**************************************************************************
 Returns FALSE if the clients gets rejected and the connection should be
 closed. Returns TRUE if the client get accepted.
**************************************************************************/
bool handle_login_request(struct connection *pconn, 
                          struct packet_server_join_req *req)
{
  char msg[MAX_LEN_MSG];
  
  freelog(LOG_NORMAL, _("Connection request from %s from %s%s"),
	  req->username, pconn->addr, pconn->adns_id > 0 ?
	  _(" (hostname lookup in progress)") : "");

  if ((game.maxconnections != 0)
      && (conn_list_size(&game.all_connections) > game.maxconnections)) {
    reject_new_connection(_
			  ("Maximum number of connections for this server exceeded."),
			  pconn);
    return FALSE;
  }

  if (is_banned(req->username, pconn)) {
    reject_new_connection(_("You are banned from this server."), pconn);
    return FALSE;
  }
  
  /* print server and client capabilities to console */
  freelog(LOG_NORMAL, _("%s has client version %d.%d.%d%s"),
          pconn->username, req->major_version, req->minor_version,
          req->patch_version, req->version_label);
  freelog(LOG_VERBOSE, "Client caps: %s", req->capability);
  freelog(LOG_VERBOSE, "Server caps: %s", our_capability);
  sz_strlcpy(pconn->capability, req->capability);
  
  /* Make sure the server has every capability the client needs */
  if (!has_capabilities(our_capability, req->capability)) {
    my_snprintf(msg, sizeof(msg),
		_
		("The client is missing a capability that this server needs.\n"
                   "Server version: %d.%d.%d%s Client version: %d.%d.%d%s."
		 "  Upgrading may help!"), MAJOR_VERSION, MINOR_VERSION,
		PATCH_VERSION, VERSION_LABEL, req->major_version,
		req->minor_version, req->patch_version, req->version_label);
    reject_new_connection(msg, pconn);
    freelog(LOG_NORMAL, _("%s was rejected: Mismatched capabilities."),
            req->username);
    return FALSE;
  }

  /* Make sure the client has every capability the server needs */
  if (!has_capabilities(req->capability, our_capability)) {
    my_snprintf(msg, sizeof(msg),
		_
		("The server is missing a capability that the client needs.\n"
                   "Server version: %d.%d.%d%s Client version: %d.%d.%d%s."
		 "  Upgrading may help!"), MAJOR_VERSION, MINOR_VERSION,
		PATCH_VERSION, VERSION_LABEL, req->major_version,
		req->minor_version, req->patch_version, req->version_label);
    reject_new_connection(msg, pconn);
    freelog(LOG_NORMAL, _("%s was rejected: Mismatched capabilities."),
            req->username);
    return FALSE;
  }

  remove_leading_trailing_spaces(req->username);

  /* Name-sanity check: could add more checks? */
  if (!is_valid_username(req->username)) {
    my_snprintf(msg, sizeof(msg), _("Invalid username '%s'"), req->username);
    reject_new_connection(msg, pconn);
    freelog(LOG_NORMAL, _("%s was rejected: Invalid name [%s]."),
            req->username, pconn->addr);
    return FALSE;
  } 

  /* don't allow duplicate logins */
  conn_list_iterate(game.all_connections, aconn) {
    if (mystrcasecmp(req->username, aconn->username) == 0) { 
      my_snprintf(msg, sizeof(msg), _("'%s' already connected."), 
                  req->username);
      reject_new_connection(msg, pconn);
      freelog(LOG_NORMAL, _("%s was rejected: Duplicate login name [%s]."),
              req->username, pconn->addr);
      return FALSE;
    }
  } conn_list_iterate_end;

  if (srvarg.auth_enabled) {
    return authenticate_user(pconn, req->username);
  } else {
    sz_strlcpy(pconn->username, req->username);
    establish_new_connection(pconn);
    return TRUE;
  }
}

/**************************************************************************
  ...
**************************************************************************/
bool server_assign_random_nation(struct player *pplayer)
{
  Nation_Type_id nation_no = select_random_nation(NULL);
  if (nation_no == NO_NATION_SELECTED) {
    freelog(LOG_NORMAL,
            _("Ran out of nations for random assignment for %s!"),
            pplayer->username);
    return FALSE;
  }

  server_assign_nation(pplayer, nation_no, pplayer->username,
                       myrand(2) == 1, get_nation_city_style(nation_no));

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
void server_assign_nation(struct player *pplayer,                          
                          Nation_Type_id nation_no,
                          const char *name,
                          bool is_male,
                          int city_style)
{
  int nation_used_count;

  pplayer->nation = nation_no;
  sz_strlcpy(pplayer->name, name);
  pplayer->name[0] = my_toupper(pplayer->name[0]);
  pplayer->is_male = is_male;
  pplayer->city_style = city_style;

  mark_nation_as_used(nation_no);

  /* tell the other players, that the nation is now unavailable */
  nation_used_count = 0;

  players_iterate(other_player) {
    if (other_player->nation == NO_NATION_SELECTED) {
      send_select_nation(other_player);
    } else {
      nation_used_count++;	/* count used nations */
    }
  } players_iterate_end;

  /* if there's no nation left, reject remaining players, sorry */
  if (nation_used_count == game.playable_nation_count) {   /* barb */
    players_iterate(other_player) {
      if (other_player->nation == NO_NATION_SELECTED) {
	freelog(LOG_NORMAL, _("No nations left: Removing player %s."),
		other_player->name);
	notify_player(other_player,
		      _("Game: Sorry, there are no nations left."));
	server_remove_player(other_player);
      }
    } players_iterate_end;
  }
}

/**************************************************************************
  High-level server stuff when connection to client is closed or lost.
  Reports loss to log, and to other players if the connection was a
  player.  Also removes player if in pregame, applies auto_toggle, and
  does check for turn done (since can depend on connection/ai status).
  Note caller should also call close_connection() after this, to do
  lower-level close stuff.
**************************************************************************/
void lost_connection_to_client(struct connection *pconn)
{
  struct player *pplayer = pconn->player;
  const char *desc = conn_description(pconn);

  freelog(LOG_NORMAL, _("Lost connection: %s."), desc);
  
  /* _Must_ avoid sending to pconn, in case pconn connection is
   * really lost (as opposed to server shutting it down) which would
   * trigger an error on send and recurse back to here.
   * Safe to unlink even if not in list: */
  conn_list_unlink(&game.est_connections, pconn);
  delayed_disconnect++;
  pconn->delayed_disconnect = TRUE;
  notify_conn(&game.est_connections, _("Game: Lost connection: %s."), desc);

  if (!pplayer) {
    delayed_disconnect--;
    return;
  }

  unattach_connection_from_player(pconn);

  send_conn_info_remove(&pconn->self, &game.est_connections);
  if (server_state == RUN_GAME_STATE) {
    /* Player info is only updated when the game is running; this must be
     * done consistently or the client will end up with inconsistent errors.
     * At other times, the conn info (send_conn_info) is used by the client
     * to display player information.  See establish_new_connection(). */
    send_player_info(pplayer, NULL);
  }

  /* Cancel diplomacy meetings */
  if (!pplayer->is_connected) { /* may be still true if multiple connections */
    players_iterate(other_player) {
      if (find_treaty(pplayer, other_player)) {
	handle_diplomacy_cancel_meeting_req(pplayer,
					    other_player->player_no);
      }
    } players_iterate_end;
  }

  if (game.is_new_game && !pplayer->is_connected	/* eg multiple controllers */
      && !pplayer->ai.control    /* eg created AI player */
      && (server_state == PRE_GAME_STATE 
	  || (server_state == SELECT_RACES_STATE
              && select_random_nation(NULL) == NO_NATION_SELECTED))) {
    server_remove_player(pplayer);
  } else {
    if (game.auto_ai_toggle
        && !pplayer->ai.control
        && !pplayer->is_connected /* eg multiple controllers */) {
      toggle_ai_player_direct(NULL, pplayer);
    }

    gamelog(GAMELOG_PLAYER, pplayer);

    check_for_full_turn_done();
  }

  delayed_disconnect--;
}

/**************************************************************************
  Fill in packet_conn_info from full connection struct.
**************************************************************************/
static void package_conn_info(struct connection *pconn,
                              struct packet_conn_info *packet)
{
  packet->id           = pconn->id;
  packet->used         = pconn->used;
  packet->established  = pconn->established;
  packet->player_num   = pconn->player ? pconn->player->player_no : -1;
  packet->observer     = pconn->observer;
  packet->access_level = pconn->access_level;

  sz_strlcpy(packet->username, pconn->username);
  sz_strlcpy(packet->addr, pconn->addr);
  sz_strlcpy(packet->capability, pconn->capability);
}

/**************************************************************************
  Handle both send_conn_info() and send_conn_info_removed(), depending
  on 'remove' arg.  Sends conn_info packets for 'src' to 'dest', turning
  off 'used' if 'remove' is specified.
**************************************************************************/
static void send_conn_info_arg(struct conn_list *src,
                               struct conn_list *dest, bool remove)
{
  struct packet_conn_info packet;
  
  conn_list_iterate(*src, psrc) {
    package_conn_info(psrc, &packet);
    if (remove) {
      packet.used = FALSE;
    }
    lsend_packet_conn_info(dest, &packet);
  } conn_list_iterate_end;
}

/**************************************************************************
  Send conn_info packets to tell 'dest' connections all about
  'src' connections.
**************************************************************************/
void send_conn_info(struct conn_list *src, struct conn_list *dest)
{
  send_conn_info_arg(src, dest, FALSE);
}

/**************************************************************************
  Like send_conn_info(), but turn off the 'used' bits to tell clients
  to remove info about these connections instead of adding it.
**************************************************************************/
void send_conn_info_remove(struct conn_list *src, struct conn_list *dest)
{
  send_conn_info_arg(src, dest, TRUE);
}

/**************************************************************************
  Setup pconn as a client connected to pplayer:
  Updates pconn->player, pplayer->connections, pplayer->is_connected.

  If pplayer is NULL, take the next available player that is not already 
  associated.
  Note "observer" connections do not count for is_connected. You must set
       pconn->obserber to TRUE before attaching!
**************************************************************************/
bool attach_connection_to_player(struct connection *pconn,
                                 struct player *pplayer)
{
  /* if pplayer is NULL, attach to first non-connected player slot */
  if (!pplayer) {
    if (game.nplayers >= game.max_players 
 	|| game.nplayers >= MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS
 	|| !can_control_a_player(pconn, TRUE) ) {
      return FALSE; 
    } else {
      pplayer = &game.players[game.nplayers];
      game.nplayers++;
    }
  }

  if (!pconn->observer) {
    sz_strlcpy(pplayer->username, pconn->username);
    pplayer->is_connected = TRUE;
  }

  pconn->player = pplayer;
  conn_list_append(&pplayer->connections, pconn);
  conn_list_append(&game.game_connections, pconn);

  return TRUE;
}
  
/**************************************************************************
  Remove pconn as a client connected to pplayer:
  Update pplayer->connections, pplayer->is_connected.

  pconn remains a member of game.est_connections.
**************************************************************************/
bool unattach_connection_from_player(struct connection *pconn)
{
  if (!pconn->player) {
    return FALSE; /* no player is attached to this conn */
  }

  conn_list_unlink(&pconn->player->connections, pconn);
  conn_list_unlink(&game.game_connections, pconn);

  pconn->player->is_connected = FALSE;
  pconn->observer = FALSE;

  /* If any other (non-observing) conn is attached to 
   * this player, the player is still connected. */
  conn_list_iterate(pconn->player->connections, aconn) {
    if (!aconn->observer) {
      pconn->player->is_connected = TRUE;
      break;
    }
  } conn_list_iterate_end;
  /*if is_connected is FALSE, nobody is controlling the player so we have to reset
     the player data structure during pre-game */

  pconn->player = NULL;

  return TRUE;
}

/**************************************************************************
  Fills in the escape sequences in the given welcome message with the
  appropriate values. Returns the number of characters written.
**************************************************************************/
static int generate_welcome_message(char *buf, int buf_len,
				    char *welcome_msg)
{
  char *in = welcome_msg, *out = buf;
  int rem = buf_len, len;
  char hostname[512];
  time_t now;
  struct tm *nowtm;

  now = time(NULL);
  nowtm = localtime(&now);

  while (rem > 0 && *in != '\0') {
    if (*in == '%') {
      in++;
      switch (*in) {
      case 'h':
	if (my_gethostname(hostname, sizeof(hostname)) == 0) {
	  mystrlcpy(out, hostname, rem);
	  len = strlen(hostname);
	} else {
	  len = 0;
	}
	break;
      case 'p':
	len = my_snprintf(out, rem, "%d", srvarg.port);
	break;
      case 'v':
	len = my_snprintf(out, rem, "%s", freeciv_name_version());
	break;
      case 'd':
	len = strftime(out, rem, "%x", nowtm);
	break;
      case 't':
	len = strftime(out, rem, "%X", nowtm);
	break;
      case '%':
	*out = '%';
	len = 1;
	break;
      default:
	len = 0;
	break;
      }
      out += len;
      rem -= len;
      if (*in == '\0')
	break;
      in++;
    } else {
      *out++ = *in++;
      rem--;
    }
  }
  *out++ = '\0';
  rem--;

  return out - buf;
}
/**************************************************************************
  ...
**************************************************************************/
struct user_action *user_action_new(const char *pattern, int type,
                                    int action)
{
  struct user_action *pua;

  assert(pattern != NULL);
  assert(0 <= type && type < NUM_CONN_PATTERN_TYPES);
  assert(0 <= action && action < NUM_ACTION_TYPES);

  pua = fc_malloc(sizeof(struct user_action));
  pua->conpat = conn_pattern_new(pattern, type);
  pua->action = action;
  
  return pua;
}
