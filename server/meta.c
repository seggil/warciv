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

#include "astring.h"
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
#include "stdinhand.h"

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
  struct astring headers, content;
  int available_players = 0;
  char host[512], state[20], type[15];
  const char *nation;
  struct connection *pconn;
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
	    mystrsocketerror(mysocketerrno()));
    metaserver_failed();
    return FALSE;
  }

  if (connect(sock, (struct sockaddr *) &meta_addr, sizeof(meta_addr)) == -1) {
    freelog(LOG_ERROR, "Metaserver: connect failed: %s",
            mystrsocketerror(mysocketerrno()));
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

  astr_init(&content);
  astr_minsize(&content, 1024);

  astr_append_printf(&content, "host=%s", my_url_encode(host));
  astr_append_printf(&content, "&port=%d", srvarg.port);
  astr_append_printf(&content, "&state=%s", my_url_encode(state));

  if (flag == META_GOODBYE) {
    astr_append(&content, "&bye=1");
  } else {
    astr_append_printf(&content, "&version=%s",
                       my_url_encode(VERSION_STRING));
    astr_append_printf(&content, "&patches=%s", 
                       my_url_encode(get_meta_patches_string()));
    astr_append_printf(&content, "&capability=%s",
                       my_url_encode(our_capability));
    astr_append_printf(&content, "&topic=%s",
                       my_url_encode(get_meta_topic_string()));
    astr_append_printf(&content, "&serverid=%s",
                       my_url_encode(srvarg.serverid));
    astr_append_printf(&content, "&message=%s",
                       my_url_encode(get_meta_message_string()));

    /* NOTE: send info for ALL players or none at all. */
    if (get_num_human_and_ai_players() == 0) {
      astr_append(&content, "&dropplrs=1");
    } else {
      available_players = 0;

      players_iterate(plr) {
        pconn = find_conn_by_user(plr->username);

        if (!plr->is_alive) {
          sz_strlcpy(type, "Dead");
        } else if (is_barbarian(plr)) {
          sz_strlcpy(type, "Barbarian");
        } else if (plr->ai.control) {
          sz_strlcpy(type, "A.I.");
        } else {
          sz_strlcpy(type, "Human");
        }

        astr_append_printf(&content, "&plu[]=%s",
                           my_url_encode(plr->username));
        astr_append_printf(&content, "&plt[]=%s", type);
        astr_append_printf(&content, "&pll[]=%s",
                           my_url_encode(plr->name));

        if (plr->nation != NO_NATION_SELECTED) {
          nation = get_nation_name_plural(plr->nation);
        } else {
          nation = "none";
        }
        astr_append_printf(&content, "&pln[]=%s", nation);
        astr_append_printf(&content, "&plh[]=%s", pconn
                           ? my_url_encode(pconn->addr) : "");

        if (is_allowed_to_take(plr, FALSE, NULL, 0)) {
          available_players++;
        }
      } players_iterate_end;

      astr_append_printf(&content, "&available=%d", available_players);
    }

    /* send some variables: should be listed in inverted order
     * FIXME: these should be input from the settings array */
    astr_append_printf(&content, "&vn[]=timeout&vv[]=%d",
                       game.info.timeout);
    astr_append_printf(&content, "&vn[]=year&vv[]=%d",
                       game.info.year);
    astr_append_printf(&content, "&vn[]=turn&vv[]=%d",
                       game.info.turn);
    astr_append_printf(&content, "&vn[]=endyear&vv[]=%d",
                       game.info.end_year);
    astr_append_printf(&content, "&vn[]=minplayers&vv[]=%d",
                       game.info.min_players);
    astr_append_printf(&content, "&vn[]=maxplayers&vv[]=%d",
                       game.info.max_players);
    astr_append_printf(&content, "&vn[]=allowtake&vv[]=%s",
                       game.server.allow_take);
    astr_append_printf(&content, "&vn[]=generator&vv[]=%d",
                       map.generator);
    astr_append_printf(&content, "&vn[]=size&vv[]=%d",
                       map.size);
  }

  astr_init(&headers);
  astr_minsize(&headers, 512);
  astr_append_printf(&headers, "POST %s HTTP/1.1\r\n", metaserver_path);
  astr_append_printf(&headers, "Host: %s:%d\r\n", metaname, metaport);
  astr_append(&headers, "Content-Type: application/x-www-form-urlencoded;"
              " charset=\"utf-8\"\r\n");
  astr_append_printf(&headers, "Content-Length: %d\r\n",
                     astr_size(&content));
  astr_append(&headers, "\r\n");

  /* sic: These two bytes are not included in Content-Length. */
  astr_append(&content, "\r\n");

  my_writesocket(sock, astr_get_data(&headers), astr_size(&headers));
  my_writesocket(sock, astr_get_data(&content), astr_size(&content));

  my_closesocket(sock);

  astr_free(&headers);
  astr_free(&content);

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
