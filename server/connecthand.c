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
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "shared.h"
#include "support.h"
#include "wildcards.h"

#include "capstr.h"
#include "events.h"
#include "packets.h"
#include "player.h"
#include "version.h"

#include "citytools.h"
#include "database.h"
#include "diplhand.h"
#include "gamehand.h"
#include "gamelog.h"
#include "maphand.h"
#include "meta.h"
#include "plrhand.h"
#include "report.h"
#include "ruleset.h"
#include "sernet.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "stdinhand_info.h"
#include "tradehand.h"
#include "unittools.h"
#include "vote.h"

#include "connecthand.h"

struct user_action_list *on_connect_user_actions = NULL;

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

static void grant_access_level(struct connection *pconn);
static int generate_welcome_message(char *buf, int buf_len,
                                    char *welcome_msg);
static bool is_banned(struct connection *pconn, enum conn_pattern_type type);
static bool conn_can_be_established(struct connection *pconn);
static void check_connection(struct connection *pconn);

/**************************************************************************
  ...
**************************************************************************/
bool can_control_a_player(struct connection *pconn, bool message)
{
  char *cap[256];
  int ntokens = 0, i;
  bool ret = TRUE;

  if (!strlen(srvarg.required_cap)) {
    return TRUE;
  }
 
  ntokens = get_tokens(srvarg.required_cap, cap, 256, TOKEN_DELIMITERS);

  for (i = 0; i < ntokens; i++) {
    if (ret && !has_capability(cap[i], pconn->capability)) {
      if (message) {
        notify_conn(pconn->self,
                    _("Server: Sorry, you haven't got the '%s' capability, "
                      "which is required to play on this server."), cap[i]);
      }
      ret = FALSE;
    }
    free(cap[i]);
  }
  
  return ret;
}

/**************************************************************************
  ...
**************************************************************************/
void clear_all_on_connect_user_actions(void)
{
  user_action_list_iterate(on_connect_user_actions, pua) {
    user_action_free(pua);
  } user_action_list_iterate_end;
  user_action_list_unlink_all(on_connect_user_actions);
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
static enum action_type find_action_for_connection(struct connection *pconn)
{
  user_action_list_iterate(on_connect_user_actions, pua) {
#ifdef DEBUG
    char buf[128];
    user_action_as_str(pua, buf, sizeof(buf));
    freelog(LOG_DEBUG, "testing action %s against %s",
	    buf, conn_description(pconn));
#endif /* DEBUG */

    if (conn_pattern_match(pua->conpat, pconn)) {
      freelog(LOG_DEBUG, "  matched! returning action %d", pua->action);
      return pua->action;
    }
  } user_action_list_iterate_end;

  return NUM_ACTION_TYPES;	/* i.e. nothing applicable found */
}

/**************************************************************************
  ...
**************************************************************************/
static void grant_access_level(struct connection *pconn)
{
  switch (find_action_for_connection(pconn)) {
    case ACTION_BAN:
      /* This should never happen as bans should have already been
       * handled. */
      die("Tried to grant access level for a banned connection %d "
          "(%s from %s)!", pconn->id, pconn->username, pconn->addr);
      break;
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
      if (user_action_list_size(on_connect_user_actions) > 0) {
        freelog(LOG_NORMAL,
		_("Warning: There was no match in the "
                  "action list for connection %d (%s from %s). It will "
                  "have access level 'none'."),
                pconn->id, pconn->username, pconn->server.ipaddr);
      }
      pconn->granted_access_level = pconn->access_level = ALLOW_NONE;
      return;
  }

  if (pconn->access_level > ALLOW_OBSERVER 
      && !can_control_a_player(pconn, FALSE)) {
    pconn->granted_access_level = pconn->access_level = ALLOW_OBSERVER;
  }

  freelog(LOG_VERBOSE, "Giving access '%s' to connection %d (%s from %s).",
	  cmdlevel_name(pconn->granted_access_level),
	  pconn->id, pconn->username, pconn->addr);
}

/**************************************************************************
  ...
**************************************************************************/
static bool is_banned(struct connection *pconn, enum conn_pattern_type type)
{
  user_action_list_iterate(on_connect_user_actions, pua) {
    if (pua->action == ACTION_BAN && pua->conpat->type == type
        && conn_pattern_match(pua->conpat, pconn)) {
      return TRUE;
    }
  } user_action_list_iterate_end;

  return FALSE;
}

/**************************************************************************
  Returns FALSE if some clues are missing for the action list.
**************************************************************************/
static bool conn_can_be_established(struct connection *pconn)
{
  return pconn != NULL
	 && pconn->server.ipaddr != '\0'
	 && pconn->addr != '\0'
	 && pconn->server.adns_id == -1
	 && pconn->server.received_username;
}

/**************************************************************************
  ...
**************************************************************************/
static void check_connection(struct connection *pconn)
{
  if (user_action_list_size(on_connect_user_actions) > 0
      && conn_can_be_established(pconn)) {
    grant_access_level(pconn);
    if (pconn->server.delay_establish) {
      pconn->server.delay_establish = FALSE;
      establish_new_connection(pconn);
    }
  }
}

/**************************************************************************
  Returns FALSE if the connection must be/has been closed.
**************************************************************************/
bool receive_ip(struct connection *pconn, const char *ipaddr)
{
  assert(ipaddr != NULL);

  sz_strlcpy(pconn->server.ipaddr, ipaddr);
  sz_strlcpy(pconn->addr, ipaddr);

  if (is_banned(pconn, CPT_ADDRESS)) {
    freelog(LOG_NORMAL, _("The address %s is banned from this server"), ipaddr);
    server_break_connection(pconn, ES_BANNED);
    return FALSE;
  }

  check_connection(pconn);
  return TRUE;
}

/**************************************************************************
  Returns FALSE if the connection must be/has been closed.
**************************************************************************/
bool receive_hostname(struct connection *pconn, const char *addr)
{
  if (addr) {
    sz_strlcpy(pconn->addr, addr);
  }

  if (is_banned(pconn, CPT_HOSTNAME)) {
    freelog(LOG_NORMAL,
	    _("The hostname %s is banned from this server"), pconn->addr);
    server_break_connection(pconn, ES_BANNED);
    return FALSE;
  }

  check_connection(pconn);
  return TRUE;
}

/**************************************************************************
  Returns FALSE if the connection must be/has been closed.
**************************************************************************/
bool receive_username(struct connection *pconn, const char *username)
{
  assert(username != NULL);

  sz_strlcpy(pconn->username, username);

  if (is_banned(pconn, CPT_USERNAME)) {
    /* Do not close this connection here, it will be 
     * closed by sniff_packets (command_ok = FALSE). */
    return FALSE;
  }

  pconn->server.received_username = TRUE;
  check_connection(pconn);
  return TRUE;
}

/**************************************************************************
  This is used when a new player joins a server, before the game
  has started.  If pconn is NULL, is an AI, else a client.

  N.B. this only attachs a connection to a player if 
       pconn->name == player->username
**************************************************************************/
void establish_new_connection(struct connection *pconn)
{
  struct conn_list *dest = pconn ? pconn->self : NULL;
  struct player *pplayer;
  struct packet_server_join_reply packet;
  char hostname[512];

  if (user_action_list_size(on_connect_user_actions) > 0
      && !conn_can_be_established(pconn)) {
    freelog(LOG_VERBOSE, _("Cannot establish connection %d (%s) now, "
                           "delay it"), pconn->id, pconn->username);
    pconn->server.delay_establish = TRUE;
    return;
  }

  /* Give a warning if we give access NONE when the hack
   * challenge is disabled and there is no action list. */
  if (user_action_list_size(on_connect_user_actions) == 0
      && pconn->access_level == ALLOW_NONE
      && srvarg.hack_request_disabled) {
    freelog(LOG_NORMAL, _("Warning: Without an action list, connection %d "
                          "(%s) has been set to access level NONE."),
            pconn->id, pconn->username);
  }

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
      notify_conn(aconn->self, _("Server: %s has connected from %s."),
                  pconn->username, pconn->addr);
    }
  } conn_list_iterate_end;

  /* a player has already been created for this user, reconnect him */
  if ((pplayer = find_player_by_user(pconn->username))) {
    attach_connection_to_player(pconn, pplayer);

    if (server_state == SELECT_RACES_STATE) {
      send_packet_freeze_hint(pconn);
      send_rulesets(dest);		
      send_player_info(NULL, NULL);
      send_packet_thaw_hint(pconn);
      if (pplayer->nation == NO_NATION_SELECTED) {
        send_select_nation(pplayer);
      }
    } else if (server_state >= RUN_GAME_STATE) {
      /* Player and other info is only updated when the game is running.
       * See the comment in lost_connection_to_client(). */
      send_packet_freeze_hint(pconn);
      send_rulesets(dest);
      send_all_info(dest);
      send_game_state(dest, CLIENT_GAME_RUNNING_STATE);
      send_player_info(NULL, NULL);
      send_diplomatic_meetings(pconn);
      send_packet_thaw_hint(pconn);
      send_packet_start_turn(pconn);
      if (server_state == GAME_OVER_STATE) {
	report_final_scores(pconn->self);
	report_game_rankings(pconn->self);
      }
    }

    /* This must be done after the above info is sent, because it will
     * generate a player-packet which can't be sent until (at least)
     * rulesets are sent. */
    if (game.server.auto_ai_toggle && pplayer->ai.control) {
      toggle_ai_player_direct(NULL, pplayer);
    }

    gamelog(GAMELOG_PLAYER, pplayer);

  } else if (server_state == PRE_GAME_STATE && game.server.is_new_game) {
    if (!attach_connection_to_player(pconn, NULL)) {
      notify_conn(dest, _("Server: Couldn't attach your connection to new player."));
      freelog(LOG_VERBOSE, "%s is not attached to a player",
	      pconn->username);
      restore_access_level(pconn);
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

  /* if need be, tell who we're waiting on to end the game.info.turn */
  if (server_state == RUN_GAME_STATE && game.server.turnblock) {
    players_iterate(cplayer) {
      if (cplayer->is_alive && !cplayer->ai.control
	  && !cplayer->turn_done && cplayer != pconn->player) {
	/* Skip current player */
	notify_conn(dest, _("Game: Turn-blocking game play: "
                            "waiting on %s to finish turn..."),
                    cplayer->name);
      }
    } players_iterate_end;
  }

  /* if the game is running, players can just view the Players menu? --dwp */
  if (server_state < RUN_GAME_STATE || !pconn->player) {
    show_players(pconn);
  }

  if (server_state == RUN_GAME_STATE
      && game.server.spectatorchat
      && !connection_controls_player(pconn)) {
    notify_conn(dest, _("Server: Spectator chat is in effect. "
                        "Your public messages will only be seen "
                        "by users not in the game."));
  }

  send_conn_info(dest, game.est_connections);
  conn_list_append(game.est_connections, pconn);

  send_running_votes(pconn);
  send_updated_vote_totals(NULL);

  if (server_state == RUN_GAME_STATE) {
    send_diplomatic_meetings(pconn);
  }

  if (conn_list_size(game.est_connections) == 1) {
    /* First connection
     * Replace "restarting in x seconds" meta message */
     maybe_automatic_meta_message(default_meta_message_string());
     send_server_info_to_metaserver(META_INFO);
  }
  send_conn_info(game.est_connections, dest);
  send_server_info_to_metaserver(META_INFO);
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
  int kick_time_remaining;
  
  freelog(LOG_NORMAL, _("Connection request from %s from %s%s"),
	  req->username, pconn->addr, pconn->server.adns_id > 0 ?
	  _(" (hostname lookup in progress)") : "");

  remove_leading_trailing_spaces(req->username);
  if (!receive_username(pconn, req->username)) {
    reject_new_connection(_("You are banned from this server."), pconn);
    return FALSE;
  }

  if (conn_is_kicked(pconn, &kick_time_remaining)) {
    my_snprintf(msg, sizeof(msg), _("You have been kicked from this server "
                                    "and cannot reconnect for %d seconds."),
                kick_time_remaining);
    reject_new_connection(msg, pconn);
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
    if (aconn == pconn) {
      continue;
    }
    if (mystrcasecmp(req->username, aconn->username) == 0) { 
      my_snprintf(msg, sizeof(msg), _("'%s' already connected."), 
                  req->username);
      reject_new_connection(msg, pconn);
      freelog(LOG_NORMAL, _("%s was rejected: Duplicate login name [%s]."),
              req->username, pconn->addr);
      return FALSE;
    }
  } conn_list_iterate_end;

  if (srvarg.auth.enabled) {
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
  if (nation_used_count == game.ruleset_control.playable_nation_count) {   /* barb */
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

  if (!pconn->established) {
    return;
  }

  /* _Must_ avoid sending to pconn, in case pconn connection is
   * really lost (as opposed to server shutting it down) which would
   * trigger an error on send and recurse back to here.
   * Safe to unlink even if not in list: */
  conn_list_unlink(game.est_connections, pconn);
  pconn->is_closing = TRUE;
  notify_conn(game.est_connections, _("Game: Lost connection: %s."), desc);

  if (!pplayer) {
    return;
  }

  unattach_connection_from_player(pconn);

  send_conn_info_remove(pconn->self, game.est_connections);
  if (server_state == RUN_GAME_STATE) {
    /* Player info is only updated when the game is running; this must be
     * done consistently or the client will end up with inconsistent errors.
     * At other times, the conn info (send_conn_info) is used by the client
     * to display player information.  See establish_new_connection(). */
    send_player_info(pplayer, NULL);
  }

  if (game.server.is_new_game && !pplayer->is_connected	/* eg multiple controllers */
      && !pplayer->ai.control    /* eg created AI player */
      && (server_state == PRE_GAME_STATE 
	  || (server_state == SELECT_RACES_STATE
              && select_random_nation(NULL) == NO_NATION_SELECTED))) {
    server_remove_player(pplayer);
  } else {
    if (game.server.auto_ai_toggle
        && !pplayer->ai.control
        && !pplayer->is_connected /* eg multiple controllers */) {
      toggle_ai_player_direct(NULL, pplayer);
    }

    gamelog(GAMELOG_PLAYER, pplayer);

    check_for_full_turn_done();
  }
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
  on 'remove' arg.  Sends conn_info packets for 'src' to 'dest', setting
  FALSE 'packet.used' if 'remove' is specified.
**************************************************************************/
static void send_conn_info_arg(struct conn_list *src,
                               struct conn_list *dest, bool remove)
{
  struct packet_conn_info packet;

  if (!src || !dest) {
    return;
  }
  
  conn_list_iterate(src, psrc) {
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
    if (game.info.nplayers >= game.info.max_players 
 	|| game.info.nplayers >= MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS
 	|| !can_control_a_player(pconn, TRUE)) {
      return FALSE; 
    } else {
      pplayer = &game.players[game.info.nplayers];
      game.info.nplayers++;
    }
  }

  if (!pconn->observer) {
    sz_strlcpy(pplayer->username, pconn->username);
    pplayer->is_connected = TRUE;
    if (!has_capability("extglobalinfo", pconn->capability)) {
      /*
       * Reset all datas that an other user has set,
       * because the new client cannot access to them.
       */
      reset_trade_route_planning(pplayer);
      reset_air_patrol(pplayer);
      reset_rally_points(pplayer);
      reset_city_manager_params(pplayer);
    }
  }

  pconn->player = pplayer;
  conn_list_append(pplayer->connections, pconn);
  conn_list_append(game.game_connections, pconn);

  restore_access_level(pconn);

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

  conn_list_unlink(pconn->player->connections, pconn);
  conn_list_unlink(game.game_connections, pconn);

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

  restore_access_level(pconn);

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

/**************************************************************************
  Restore access level for the given connection (user). Used when taking
  a player, observing, or detaching.
**************************************************************************/
void restore_access_level(struct connection *pconn)
{
  /* Restore previous privileges. */
  pconn->access_level = pconn->granted_access_level;

  /* Detached connections must have at most the same privileges as
   * observers, unless the action list gave them something higher
   * than ALLOW_BASIC in the first place. */
  if ((pconn->observer || !pconn->player)
      && pconn->access_level == ALLOW_BASIC) {
    pconn->access_level = ALLOW_OBSERVER;
  }
}

/**************************************************************************
  Called when a connection does something to indicate that it is not
  idle.
**************************************************************************/
void conn_reset_idle_time(struct connection *pconn)
{
  if (!pconn || !pconn->used || pconn->is_closing) {
    return;
  }
  pconn->server.idle_time = time(NULL);
}

/**************************************************************************
  ...
**************************************************************************/
void check_idle_connections(void)
{
  time_t now;
  int num_connections;

  if (game.server.idlecut <= 0) {
    return;
  }

  now = time(NULL);
  num_connections = conn_list_size(game.est_connections);

  if (server_state == PRE_GAME_STATE
      && num_connections > 1) {
    return;
  }

  conn_list_iterate(game.all_connections, pconn) {
    if (!pconn->used || pconn->is_closing
        || pconn->server.idle_time <= 0
        || pconn->access_level >= ALLOW_ADMIN
        || (server_state == RUN_GAME_STATE
            && pconn->observer
            && num_connections > 1)) {
      continue;
    }
    if (now >= pconn->server.idle_time + game.server.idlecut) {
      server_break_connection(pconn, ES_IDLECUT);
    }
  } conn_list_iterate_end;
}
