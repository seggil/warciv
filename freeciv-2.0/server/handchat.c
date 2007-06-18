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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "fcintl.h"
#include "game.h"
#include "packets.h"
#include "player.h"
#include "shared.h"
#include "support.h"

#include "stdinhand.h"

#include "handchat.h"

#define MAX_LEN_CHAT_NAME (2*MAX_LEN_NAME+10)   /* for form_chat_name() names */

/**************************************************************************
  Formulate a name for this connection, prefering the player name when
  available and unambiguous (since this is the "standard" case), else
  use the username.
**************************************************************************/
static void form_chat_name(struct connection *pconn, char *buffer, size_t len)
{
  struct player *pplayer = pconn->player;

  if (!pplayer
      || pconn->observer
      || strcmp(pplayer->name, ANON_PLAYER_NAME) == 0) {
    my_snprintf(buffer, len, "(%s)", pconn->username);
  } else {
    my_snprintf(buffer, len, "%s", pplayer->name);
  }
}
				
/**************************************************************************
  Complain to sender that name was ambiguous.
  'player_conn' is 0 for player names, 1 for connection names,
  2 for attempt to send to an anonymous player.
**************************************************************************/
static void complain_ambiguous(struct connection *pconn, const char *name,
			       int player_conn)
{
  char message[MAX_LEN_MSG];

  switch(player_conn) {
  case 0:
    my_snprintf(message, sizeof(message),
		_("Game: %s is an ambiguous player name-prefix."), name);
    break;
  case 1:
    my_snprintf(message, sizeof(message),
		_("Game: %s is an ambiguous connection name-prefix."), name);
    break;
  case 2:
    my_snprintf(message, sizeof(message),
                _("Game: %s is an anonymous name. Use connection name"), name);
    break;
  default:
    assert(0);
  }
  dsend_packet_chat_msg(pconn, message, -1, -1, E_NOEVENT, -1);
}

/**************************************************************************
  Send private message to single connection.
**************************************************************************/
static void chat_msg_to_conn(struct connection *sender,
			     struct connection *dest, char *msg)
{
  char sender_name[MAX_LEN_CHAT_NAME], dest_name[MAX_LEN_CHAT_NAME];
  char message[MAX_LEN_MSG];
  
  msg = skip_leading_spaces(msg);
  
  form_chat_name(sender, sender_name, sizeof(sender_name));
  form_chat_name(dest, dest_name, sizeof(dest_name));

  my_snprintf(message, sizeof(message), "->*%s* %s", dest_name, msg);
  dsend_packet_chat_msg(sender, message, -1, -1, E_NOEVENT, sender->id);

  if (sender != dest) {
    my_snprintf(message, sizeof(message), "*%s* %s", sender_name, msg);
    dsend_packet_chat_msg(dest, message, -1, -1, E_NOEVENT, sender->id);
  }
}

/**************************************************************************
  Send private message to multi-connected player.
**************************************************************************/
static void chat_msg_to_player_multi(struct connection *sender,
				     struct player *pdest, char *msg)
{
  char sender_name[MAX_LEN_CHAT_NAME], message[MAX_LEN_MSG];

  msg = skip_leading_spaces(msg);
  
  form_chat_name(sender, sender_name, sizeof(sender_name));

  my_snprintf(message, sizeof(message), "->[%s] %s", pdest->name, msg);
  dsend_packet_chat_msg(sender, message, -1, -1, E_NOEVENT, sender->id);

  my_snprintf(message, sizeof(message), "[%s] %s", sender_name, msg);
  conn_list_iterate(pdest->connections, dest_conn) {
    if (dest_conn != sender) {
      dsend_packet_chat_msg(dest_conn, message,
			    -1, -1, E_NOEVENT, sender->id);
    }
  } conn_list_iterate_end;
}

/**************************************************************************
  Handle a chat message packet from client:
  1. Work out whether it is a server command and if so run it;
  2. Otherwise work out whether it is directed to a single player, or
     to a single connection, and send there.  (For a player, send to
     all clients connected as that player, in multi-connect case);
  3. Or it may be intended for all allied players.
  4. Else send to all connections (game.game_connections).

  In case 2, there can sometimes be ambiguity between player and
  connection names.  By default this tries to match player name first,
  and only if that fails tries to match connection name.  User can
  override this and specify connection only by using two colons ("::")
  after the destination name/prefix, instead of one.

  The message sent will name the sender, and via format differences
  also indicates whether the recipient is either all connections, a
  single connection, or multiple connections to a single player.

  Message is also echoed back to sender (with different format),
  avoiding sending both original and echo if sender is in destination
  set.
**************************************************************************/
void handle_chat_msg_req(struct connection *pconn, char *message)
{
  char sender_name[MAX_LEN_CHAT_NAME], chat[MAX_LEN_MSG];
  char *cp;
  bool double_colon;

  /* this loop to prevent players from sending multiple lines
   * which can be abused */
  for (cp = message; *cp != '\0'; cp++) {
    if (*cp == '\n' || *cp == '\r') {
      *cp='\0';
      break;
    }
  }

  /* Server commands are prefixed with '/', which is an obvious
     but confusing choice: even before this feature existed,
     novice players were trying /who, /nick etc.
     So consider this an incentive for IRC support,
     or change it in stdinhand.h - rp
  */
  if (message[0] == SERVER_COMMAND_PREFIX) {
    /* pass it to the command parser, which will chop the prefix off */
    (void) handle_stdin_input(pconn, message, FALSE);
    return;
  }

  /* Send to allies command */
  if (message[0] == ALLIESCHAT_COMMAND_PREFIX) {
    char sender_name[MAX_LEN_CHAT_NAME];

    /* this won't work if we aren't attached to a player */
    if (!pconn->player) {
      my_snprintf(chat, sizeof(chat),
                  _("Game: You are not attached to a player."));
      dsend_packet_chat_msg(pconn, chat, -1, -1, E_NOEVENT, -1);
      return;
    }

    message[0] = ' '; /* replace command prefix */
    form_chat_name(pconn, sender_name, sizeof(sender_name));
    my_snprintf(chat, sizeof(chat),
                _("%s to allies: %s"), sender_name,
                skip_leading_spaces(message));
    /* FIXME: there should be a special case for the sender, like in
     * chat_msg_to_player_multi(). */
    players_iterate(aplayer) {
      if (!pplayers_allied(pconn->player, aplayer)) {
        continue;
      }
      dlsend_packet_chat_msg(&aplayer->connections, chat, -1, -1,
			     E_NOEVENT, pconn->id);
    } players_iterate_end;
    return;
  }

  /* Want to allow private messages with "player_name: message",
     (or "connection_name: message"), including unambiguously
     abbreviated player/connection name, but also want to allow
     sensible use of ':' within messages, and _also_ want to
     notice intended private messages with (eg) mis-spelt name.

     Approach:
     
     If there is no ':', or ':' is first on line,
          message is global (send to all players)
     else if the ':' is double, try matching part before "::" against
          connection names: for single match send to that connection,
	  for multiple matches complain, else goto heuristics below.
     else try matching part before (single) ':' against player names:
          for single match send to that player, for multiple matches
	  complain
     else try matching against connection names: for single match send
          to that connection, for multiple matches complain
     else if some heuristics apply (a space anywhere before first ':')
          then treat as global message,
     else complain (might be a typo-ed intended private message)
  */
  
  cp=strchr(message, ':');

  if (cp && (cp != &message[0])) {
    enum m_pre_result match_result_player, match_result_conn;
    struct player *pdest = NULL;
    struct connection *conn_dest = NULL;
    char name[MAX_LEN_NAME];
    char *cpblank;

    (void) mystrlcpy(name, message,
		     MIN(sizeof(name), cp - message + 1));

    double_colon = (*(cp+1) == ':');
    if (double_colon) {
      conn_dest = find_conn_by_user_prefix(name, &match_result_conn);
      if (match_result_conn == M_PRE_AMBIGUOUS) {
	complain_ambiguous(pconn, name, 1);
	return;
      }
      if (conn_dest && match_result_conn < M_PRE_AMBIGUOUS) {
	chat_msg_to_conn(pconn, conn_dest, cp+2);
	return;
      }
    } else {
      /* single colon */
      pdest = find_player_by_name_prefix(name, &match_result_player);
      if (match_result_player == M_PRE_AMBIGUOUS) {
	complain_ambiguous(pconn, name, 0);
	return;
      }
      if (pdest && strcmp(pdest->name, ANON_PLAYER_NAME) == 0) {
        complain_ambiguous(pconn, name, 2);
        return;
      }
      if (pdest && match_result_player < M_PRE_AMBIGUOUS) {
	int nconn = conn_list_size(&pdest->connections);
	if (nconn==1) {
	  chat_msg_to_conn(pconn, conn_list_get(&pdest->connections, 0), cp+1);
	  return;
	} else if (nconn>1) {
	  chat_msg_to_player_multi(pconn, pdest, cp+1);
	  return;
	}
	/* else try for connection name match before complaining */
      }
      conn_dest = find_conn_by_user_prefix(name, &match_result_conn);
      if (match_result_conn == M_PRE_AMBIGUOUS) {
	complain_ambiguous(pconn, name, 1);
	return;
      }
      if (conn_dest && match_result_conn < M_PRE_AMBIGUOUS) {
	chat_msg_to_conn(pconn, conn_dest, cp+1);
	return;
      }
      if (pdest && match_result_player < M_PRE_AMBIGUOUS) {
	/* Would have done something above if connected */
	my_snprintf(chat, sizeof(chat),
		    _("Game: %s is not connected."), pdest->name);
	dsend_packet_chat_msg(pconn, chat, -1, -1, E_NOEVENT, -1);
	return;
      }
    }
    /* Didn't match; check heuristics to see if this is likely
     * to be a global message
     */
    cpblank=strchr(message, ' ');
    if (!cpblank || (cp < cpblank)) {
      if (double_colon) {
	my_snprintf(chat, sizeof(chat),
		    _("Game: There is no connection by the name %s."), name);
      } else {
	my_snprintf(chat, sizeof(chat),
		    _("Game: There is no player nor connection by the name %s."),
		    name);
      }
      dsend_packet_chat_msg(pconn, chat, -1, -1, E_NOEVENT, -1);
      return;
    }
  }
  /* global message: */
  form_chat_name(pconn, sender_name, sizeof(sender_name));
  my_snprintf(chat, sizeof(chat),
	      "<%s> %s", sender_name, message);
  dlsend_packet_chat_msg(&game.est_connections, chat,
			 -1, -1, E_NOEVENT, pconn->id);
}
