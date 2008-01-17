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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef WIN32_NATIVE
#include <winsock2.h>
#endif

#include "capstr.h"
#include "connection.h"
#include "dataio.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "support.h"
#include "timing.h"
#include "version.h"

#include "civserver.h"
#include "console.h"
#include "srv_main.h"

#include "meta.h"

static bool server_is_open = FALSE;

static union my_sockaddr meta_addr;
static char  metaname[MAX_LEN_ADDR];
static int   metaport;
static char *metaserver_path;

static char meta_patches[256] = PEPSERVER_VERSION;
static char meta_topic[256] = "NEW GAME";
static char meta_message[256] = "NEW GAME";

static time_t last_metaserver_fail = 0;

/*************************************************************************
 the default metaserver patches for this server
*************************************************************************/
const char *default_meta_patches_string(void)
{
  return meta_patches;
}

/*************************************************************************
 the default metaserver topic
*************************************************************************/
const char *default_meta_topic_string(void)
{
  return meta_topic;
}

/*************************************************************************
  Return static string with default info line to send to metaserver.
*************************************************************************/
const char *default_meta_message_string(void)
{
#if IS_BETA_VERSION
  return "unstable pre-" NEXT_STABLE_VERSION ": beware";
#else  /* IS_BETA_VERSION */
#if IS_DEVEL_VERSION
  return "development version: beware";
#else  /* IS_DEVEL_VERSION */
  return "-";
#endif /* IS_DEVEL_VERSION */
#endif /* IS_BETA_VERSION */
}

/*************************************************************************
 the metaserver patches
*************************************************************************/
const char *get_meta_patches_string(void)
{
  return meta_patches;
}

/*************************************************************************
 the metaserver topic
*************************************************************************/
const char *get_meta_topic_string(void)
{
  return meta_topic;
}

/*************************************************************************
 the metaserver message
*************************************************************************/
const char *get_meta_message_string(void)
{
  return meta_message;
}

/*************************************************************************
 The metaserver message set by user
*************************************************************************/
const char *get_user_meta_message_string(void)
{
  if (game.server.meta_info.user_message_set) {
    return game.server.meta_info.user_message;
  }

  return NULL;
}

/*************************************************************************
  Update meta message. Set it to user meta message, if it is available.
  Otherwise use provided message.
  It is ok to call this with NULL message. Then it only replaces current
  meta message with user meta message if available.
*************************************************************************/
void maybe_automatic_meta_message(const char *automatic)
{
  const char *user_message;

  user_message = get_user_meta_message_string();

  if (user_message == NULL) {
    /* No user message */
    if (automatic != NULL) {
      set_meta_message_string(automatic);
    }
    return;
  }

  set_meta_message_string(user_message);
}

/*************************************************************************
 set the metaserver patches string
*************************************************************************/
void set_meta_patches_string(const char *string)
{
  sz_strlcpy(meta_patches, string);
}

/*************************************************************************
 set the metaserver topic string
*************************************************************************/
void set_meta_topic_string(const char *string)
{
  sz_strlcpy(meta_topic, string);
}

/*************************************************************************
 set the metaserver message string
*************************************************************************/
void set_meta_message_string(const char *string)
{
  sz_strlcpy(meta_message, string);
}

/*************************************************************************
 set user defined metaserver message string
*************************************************************************/
void set_user_meta_message_string(const char *string)
{
  if (string != NULL && string[0] != '\0') {
    sz_strlcpy(game.server.meta_info.user_message, string);
    game.server.meta_info.user_message_set = TRUE;
    set_meta_message_string(string);
  } else {
    /* Remove user meta message. We will use automatic messages instead */
    game.server.meta_info.user_message[0] = '\0';
    game.server.meta_info.user_message_set = FALSE;
    set_meta_message_string(default_meta_message_string());    
  }
}

/*************************************************************************
...
*************************************************************************/
char *meta_addr_port(void)
{
  return srvarg.metaserver_addr;
}

/*************************************************************************
 we couldn't find or connect to the metaserver.
*************************************************************************/
static void metaserver_failed(void)
{
  if (srvarg.metaserver_fail_wait_time <= 0) {
    freelog (LOG_VERBOSE, _("Not reporting to the metaserver in this "
                            "game."));
  } else {
    freelog (LOG_VERBOSE,
        _("Waiting for at least %d seconds before attempting to "
          "communicate with the metaserver again."),
        srvarg.metaserver_fail_wait_time);
    last_metaserver_fail = time(NULL);
  }
  con_flush();

  server_close_meta();
}

/*************************************************************************
 construct the POST message and send info to metaserver.
*************************************************************************/
static bool send_to_metaserver(enum meta_flag flag)
{
  static char msg[8192];
  static char str[8192];
  int rest = sizeof(str);
  int n = 0;
  char *s = str;
  char host[512];
  char state[20];
  int sock;

  if (!server_is_open
      && !srvarg.metaserver_no_send
      && srvarg.metaserver_fail_wait_time > 0
      && last_metaserver_fail > 0)
  {
    time_t now = time(NULL);
    if (now - last_metaserver_fail > srvarg.metaserver_fail_wait_time) {
      freelog (LOG_VERBOSE,
         _("Reopening connection to metaserver after waiting for %d "
            "seconds since the last failure."),
          (int) (now - last_metaserver_fail));
      server_open_meta();
    }
  }

  if (!server_is_open) {
    return FALSE;
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    freelog(LOG_ERROR, "Metaserver: can't open stream socket: %s",
	    mystrsocketerror());
    metaserver_failed();
    return FALSE;
  }

  if (connect(sock, (struct sockaddr *) &meta_addr, sizeof(meta_addr)) == -1) {
    freelog(LOG_ERROR, "Metaserver: connect failed: %s", mystrsocketerror());
    metaserver_failed();
    my_closesocket(sock);
    return FALSE;
  }

  switch(server_state) {
  case PRE_GAME_STATE:
    sz_strlcpy(state, "Pregame");
    break;
  case SELECT_RACES_STATE:
    sz_strlcpy(state, "Nation Select");
    break;
  case RUN_GAME_STATE:
    sz_strlcpy(state, "Running");
    break;
  case GAME_OVER_STATE:
    sz_strlcpy(state, "Game Ended");
    break;
  default:
    sz_strlcpy(state, "Unknown");
    break;
  }

  /* get hostname */
  if (my_gethostname(host, sizeof(host)) != 0) {
    sz_strlcpy(host, "unknown");
  }

  my_snprintf(s, rest, "host=%s&port=%d&state=%s&", host, srvarg.port, state);
  s = end_of_strn(s, &rest);

  if (flag == META_GOODBYE) {
    mystrlcpy(s, "bye=1&", rest);
    s = end_of_strn(s, &rest);
  } else {
    my_snprintf(s, rest, "version=%s&", my_url_encode(VERSION_STRING));
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "patches=%s&", 
                my_url_encode(get_meta_patches_string()));
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "capability=%s&", my_url_encode(our_capability));
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "topic=%s&",
                my_url_encode(get_meta_topic_string()));
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "serverid=%s&",
                my_url_encode(srvarg.serverid));
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "message=%s&",
                my_url_encode(get_meta_message_string()));
    s = end_of_strn(s, &rest);

    /* NOTE: send info for ALL players or none at all. */
    if (get_num_human_and_ai_players() == 0) {
      mystrlcpy(s, "dropplrs=1&", rest);
      s = end_of_strn(s, &rest);
    } else {
      n = 0; /* a counter for players_available */

      players_iterate(plr) {
        bool is_player_available = TRUE;
        char type[15];
        struct connection *pconn = find_conn_by_user(plr->username);

        if (!plr->is_alive) {
          sz_strlcpy(type, "Dead");
        } else if (is_barbarian(plr)) {
          sz_strlcpy(type, "Barbarian");
        } else if (plr->ai.control) {
          sz_strlcpy(type, "A.I.");
        } else {
          sz_strlcpy(type, "Human");
        }

        my_snprintf(s, rest, "plu[]=%s&", my_url_encode(plr->username));
        s = end_of_strn(s, &rest);

        my_snprintf(s, rest, "plt[]=%s&", type);
        s = end_of_strn(s, &rest);

        my_snprintf(s, rest, "pll[]=%s&", my_url_encode(plr->name));
        s = end_of_strn(s, &rest);

        my_snprintf(s, rest, "pln[]=%s&",
                    my_url_encode(plr->nation != NO_NATION_SELECTED 
                                  ? get_nation_name_plural(plr->nation)
                                  : "none"));
        s = end_of_strn(s, &rest);

        my_snprintf(s, rest, "plh[]=%s&",
                    pconn ? my_url_encode(pconn->addr) : "");
        s = end_of_strn(s, &rest);

        /* is this player available to take?
         * TODO: there's some duplication here with 
         * stdinhand.c:is_allowed_to_take() */
        if (is_barbarian(plr) && !strchr(game.server.allow_take, 'b')) {
          is_player_available = FALSE;
        } else if (!plr->is_alive && !strchr(game.server.allow_take, 'd')) {
          is_player_available = FALSE;
        } else if (plr->ai.control
            && !strchr(game.server.allow_take, (game.server.is_new_game ? 'A' : 'a'))) {
          is_player_available = FALSE;
        } else if (!plr->ai.control
            && !strchr(game.server.allow_take, (game.server.is_new_game ? 'H' : 'h'))) {
          is_player_available = FALSE;
        }

        if (pconn) {
          is_player_available = FALSE;
        }

        if (is_player_available) {
          n++;
        }
      } players_iterate_end;

      /* send the number of available players. */
      my_snprintf(s, rest, "available=%d&", n);
      s = end_of_strn(s, &rest);
    }

    /* send some variables: should be listed in inverted order
     * FIXME: these should be input from the settings array */
    my_snprintf(s, rest, "vn[]=%s&vv[]=%d&",
                my_url_encode("timeout"), game.info.timeout);
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "vn[]=%s&vv[]=%d&",
                my_url_encode("year"), game.info.year);
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "vn[]=%s&vv[]=%d&",
                my_url_encode("turn"), game.info.turn);
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "vn[]=%s&vv[]=%d&",
                my_url_encode("endyear"), game.info.end_year);
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "vn[]=%s&vv[]=%d&",
                my_url_encode("minplayers"), game.info.min_players);
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "vn[]=%s&vv[]=%d&",
                my_url_encode("maxplayers"), game.info.max_players);
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "vn[]=%s&vv[]=%s&",
                my_url_encode("allowtake"), game.server.allow_take);
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "vn[]=%s&vv[]=%d&",
                my_url_encode("generator"), map.generator);
    s = end_of_strn(s, &rest);

    my_snprintf(s, rest, "vn[]=%s&vv[]=%d&",
                my_url_encode("size"), map.size);
    s = end_of_strn(s, &rest);
  }

  n = my_snprintf(msg, sizeof(msg),
    "POST %s HTTP/1.1\r\n"
    "Host: %s:%d\r\n"
    "Content-Type: application/x-www-form-urlencoded; charset=\"utf-8\"\r\n"
    "Content-Length: %lu\r\n"
    "\r\n"
    "%s\r\n",
    metaserver_path,
    metaname,
    metaport,
    (unsigned long) (sizeof(str) - rest + 1),
    str
  );

  my_writesocket(sock, msg, n);

  my_closesocket(sock);

  return TRUE;
}

/*************************************************************************
 
*************************************************************************/
void server_close_meta(void)
{
  server_is_open = FALSE;
}

/*************************************************************************
 lookup the correct address for the metaserver.
*************************************************************************/
void server_open_meta(void)
{
  const char *path;
 
  if (metaserver_path) {
    free(metaserver_path);
    metaserver_path = NULL;
  }
  
  if (!(path = my_lookup_httpd(metaname, &metaport, srvarg.metaserver_addr))) {
    freelog(LOG_ERROR, _("Metaserver: bad http server url: %s."),
            srvarg.metaserver_addr);
    metaserver_failed();
    return;
  }
  
  metaserver_path = mystrdup(path);

  if (!net_lookup_service(metaname, metaport, &meta_addr)) {
    freelog(LOG_ERROR, _("Metaserver: bad address: [%s:%d]."),
            metaname, metaport);
    metaserver_failed();
    return;
  }

  if (meta_patches[0] == '\0') {
    set_meta_patches_string(default_meta_patches_string());
  }
  if (meta_topic[0] == '\0') {
    set_meta_topic_string(default_meta_topic_string());
  }
  if (meta_message[0] == '\0') {
    set_meta_message_string(default_meta_message_string());
  }

  server_is_open = TRUE;
}

/**************************************************************************
 are we sending info to the metaserver?
**************************************************************************/
bool is_metaserver_open(void)
{
  return server_is_open;
}

/**************************************************************************
 control when we send info to the metaserver.
**************************************************************************/
bool send_server_info_to_metaserver(enum meta_flag flag)
{
  static struct timer *last_send_timer = NULL;
  static bool want_update;
  double last_send_time = 0.0;

  /* if we're bidding farewell, ignore all timers */
  if (flag == META_GOODBYE) { 
    if (last_send_timer) {
      free_timer(last_send_timer);
      last_send_timer = NULL;
    }
    return send_to_metaserver(flag);
  }

  if (last_send_timer) {
    last_send_time = read_timer_seconds(last_send_timer);
  }

  /* don't allow the user to spam the metaserver with updates */
  if (last_send_time != 0.0
      && last_send_time < METASERVER_MIN_UPDATE_INTERVAL) {
    if (flag == META_INFO) {
      want_update = TRUE; /* we couldn't update now, but update a.s.a.p. */
    }
    return FALSE;
  }

  /* if we're asking for a refresh, only do so if 
   * we've exceeded the refresh interval */
  if (flag == META_REFRESH && !want_update && last_send_time != 0.0
      && last_send_time < METASERVER_REFRESH_INTERVAL) {
    return FALSE;
  }

  /* start a new timer if we haven't already */
  if (!last_send_timer) {
    last_send_timer = new_timer(TIMER_USER, TIMER_ACTIVE);
  }

  clear_timer_start(last_send_timer);
  want_update = FALSE;
  return send_to_metaserver(flag);
}
