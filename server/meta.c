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
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "support.h"
#include "timing.h"

#include "capstr.h"
#include "connection.h"
#include "dataio.h"
#include "map.h"
#include "version.h"

#include "civserver.h"
#include "console.h"
#include "srv_main.h"
#include "stdinhand.h"

#include "meta.h"

static bool server_is_open = FALSE;
static int metaname_lookup_id;

static union my_sockaddr meta_addr;
static char  metaname[MAX_LEN_ADDR];
static int   metaport;
static char *metaserver_path;

static int metaserver_sock = -1;
static char *metaserver_buffer;
static int metaserver_buffer_length;
static int metaserver_buffer_offset;
static int metaserver_success_count;

#define META_USER_AGENT "Mozilla/5.001 (windows; U; NT4.0; en-us) Gecko/25250101"

static char meta_patches[256] = "";
static char meta_topic[256] = "NEW GAME";
static char meta_message[256] = "NEW GAME";

static time_t last_metaserver_fail = 0;

/*************************************************************************
 the default metaserver patches for this server
*************************************************************************/
const char *default_meta_patches_string(void)
{
  return warclient_name_version();
}

/*************************************************************************
 the default metaserver topic
*************************************************************************/
const char *default_meta_topic_string(void)
{
  return "NEW GAME";
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
  Contructs the metaserver POST message and returns it in a dynamically
  allocated buffer. The size of the buffer is returned in 'pbuflen'.
  NB: You must free() the return value when it is no longer needed.
*************************************************************************/
static char *generate_metaserver_post(enum meta_flag flag, int *pbuflen)
{
  struct astring headers, content;
  int available_players = 0, len;
  char buf[512], *ret, nation[128];
  const char *type, *state;

  assert(pbuflen != NULL);

  switch(server_state) {
  case PRE_GAME_STATE:
    state = "Pregame";
    break;
  case SELECT_RACES_STATE:
    state = "Nation Select";
    break;
  case RUN_GAME_STATE:
    state = "Running";
    break;
  case GAME_OVER_STATE:
    state = "Game Ended";
    break;
  default:
    state = "Unknown";
    break;
  }

  astr_init(&content);
  astr_minsize(&content, 1024);

  astr_append_printf(&content, "host=%s", my_url_encode(srvarg.metasendhost));
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

    if (get_num_human_and_ai_players() < 1
        && conn_list_size(game.est_connections) < 1) {
      astr_append(&content, "&dropplrs=1");
    } else {
      available_players = 0;

      players_iterate(plr) {
        struct connection *pconn;
        pconn = find_conn_by_user(plr->username);

        if (!plr->is_alive) {
          type = "Dead";
        } else if (is_barbarian(plr)) {
          type = "Barbarian";
        } else if (plr->ai.control) {
          type = "A.I.";
        } else {
          type = "Human";
        }

        astr_append_printf(&content, "&plu[]=%s",
                           my_url_encode(plr->username));
        astr_append_printf(&content, "&plt[]=%s", type);
        astr_append_printf(&content, "&pll[]=%s",
                           my_url_encode(plr->name));

        if (plr->nation != NO_NATION_SELECTED) {
          if (plr->team != TEAM_NONE) {
            my_snprintf(nation, sizeof(nation), "T%d %s", plr->team,
                        get_nation_name_plural(plr->nation));
          } else {
            sz_strlcpy(nation, get_nation_name_plural(plr->nation));
          }
        } else {
          sz_strlcpy(nation, "none");
        }
        astr_append_printf(&content, "&pln[]=%s", my_url_encode(nation));
        astr_append_printf(&content, "&plh[]=%s", pconn
                           ? my_url_encode(pconn->addr) : "");

        if (is_allowed_to_attach(plr, NULL, FALSE, NULL, 0)) {
          available_players++;
        }
      } players_iterate_end;

      conn_list_iterate(game.est_connections, pconn) {
        if (pconn->player && !pconn->observer) {
          continue;
        }

        /* NB: For this hack to work, player names (pll)
         * must be unique. */

        if (pconn->player && pconn->observer) {
          type = "Observer";
          my_snprintf(buf, sizeof(buf), "*(%s) %d",
                      pconn->player->name, pconn->id);
        } else if (!pconn->player && pconn->observer) {
          type = "Observer";
          my_snprintf(buf, sizeof(buf), "*global %d", pconn->id);
        } else {
          type = "Detached";
          my_snprintf(buf, sizeof(buf), "*detached %d", pconn->id);
        }
          
        astr_append_printf(&content, "&plu[]=%s",
                           my_url_encode(pconn->username));
        astr_append_printf(&content, "&plt[]=%s", type);
        astr_append_printf(&content, "&pll[]=%s", my_url_encode(buf));
        astr_append(&content, "&pln[]=none");
        astr_append_printf(&content, "&plh[]=%s",
                           my_url_encode(pconn->addr));
      } conn_list_iterate_end;

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
    astr_append_printf(&content, "&vn[]=generator&vv[]=%d",
                       map.server.generator);
    astr_append_printf(&content, "&vn[]=size&vv[]=%d",
                       map.server.size);

    astr_append_printf(&content, "&vn[]=ruleset&vv[]=%s",
                       my_url_encode(game.server.rulesetdir));
  }

  astr_init(&headers);
  astr_minsize(&headers, 512);
  astr_append_printf(&headers, "POST %s HTTP/1.1\r\n", metaserver_path);
  astr_append_printf(&headers, "User-Agent: %s\r\n", META_USER_AGENT);
  astr_append_printf(&headers, "Host: %s:%d\r\n", metaname, metaport);
  astr_append(&headers, "Content-Type: application/x-www-form-urlencoded;"
              " charset=\"utf-8\"\r\n");
  astr_append_printf(&headers, "Content-Length: %d\r\n",
                     (int) astr_size(&content));
  astr_append(&headers, "\r\n");

  /* sic: These two bytes are not included in Content-Length. */
  astr_append(&content, "\r\n");

  len = astr_size(&headers) + astr_size(&content);
  *pbuflen = len;
  ret = fc_malloc(len);
  memcpy(ret, astr_get_data(&headers), astr_size(&headers));
  memcpy(ret + astr_size(&headers), astr_get_data(&content),
         astr_size(&content));
  astr_free(&headers);
  astr_free(&content);

  return ret;
}

/*************************************************************************
  Create the a non-blocking socket and start the connection to the
  metaserver. Also generate the POST message and place it in the buffer.

  Returns FALSE on error.
*************************************************************************/
static bool send_to_metaserver(enum meta_flag flag)
{
  int sock, len;

  if (!server_is_open) {
    return FALSE;
  }

  if (metaserver_sock != -1) {
    my_closesocket(metaserver_sock);
    metaserver_sock = -1;
  }
  if (metaserver_buffer != NULL) {
    free(metaserver_buffer);
    metaserver_buffer = NULL;
    metaserver_buffer_offset = 0;
  }

  /* Don't bother sending BYE to a metaserver we
   * never successfully sent any data to anyway. */
  if (flag == META_GOODBYE && metaserver_success_count < 1) {
    return TRUE;
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    freelog(LOG_ERROR, "Metaserver: can't open stream socket: %s",
	    mystrsocketerror(mysocketerrno()));
    metaserver_failed();
    return FALSE;
  }

  /* Use non-blocking mode and the select loop in sniff_packets()
   * for all metaserver updates except for the BYE message. This
   * is because the BYE message is sent right before server exit,
   * and so we cannot wait for a call to select(). */
  if (flag != META_GOODBYE && my_set_nonblock(sock) == -1) {
    freelog(LOG_ERROR, "Metaserver: set non-blocking mode failed: %s",
            mystrsocketerror(mysocketerrno()));
    metaserver_failed();
    my_closesocket(sock);
    return FALSE;
  }

  if (connect(sock, (struct sockaddr *) &meta_addr, sizeof(meta_addr)) == -1) {
    long err_no = mysocketerrno();

    if (!my_socket_would_block(err_no)
        && !my_socket_operation_in_progess(err_no)) {
      freelog(LOG_ERROR, "Metaserver: connect failed: %s",
              mystrsocketerror(err_no));
      metaserver_failed();
      my_closesocket(sock);
      return FALSE;
    }
  }

  metaserver_buffer = generate_metaserver_post(flag, &len);
  metaserver_buffer_length = len;
  metaserver_buffer_offset = 0;
  metaserver_sock = sock;

  if (flag == META_GOODBYE) {
    /* Send BYE message directly, bypassing the select loop. */
    metaserver_handle_write_ready();
  }

  return TRUE;
}

/*************************************************************************
 
*************************************************************************/
void server_close_meta(void)
{
  if (metaname_lookup_id > 0) {
    cancel_net_lookup_service(metaname_lookup_id);
    metaname_lookup_id = 0;
  }
  server_is_open = FALSE;
}

/*************************************************************************
  Async callback for metaserver host name resolution.
*************************************************************************/
static bool metaname_lookup_callback(union my_sockaddr *addr, void *data)
{
  metaname_lookup_id = 0;
  if (addr == NULL) {
    freelog(LOG_ERROR, _("Metaserver: bad address: [%s:%d]."),
            metaname, metaport);
    metaserver_failed();
    return FALSE;
  }
  meta_addr.sockaddr_in = addr->sockaddr_in;
  server_is_open = TRUE;
  metaserver_success_count = 0;
  return TRUE;
}

/*************************************************************************
 lookup the correct address for the metaserver.
*************************************************************************/
void server_open_meta(void)
{
  int id;
  const char *path;

  if (metaname_lookup_id > 0) {
    cancel_net_lookup_service(metaname_lookup_id);
    metaname_lookup_id = 0;
  }
 
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

  id = net_lookup_service_async(metaname, metaport,
                                metaname_lookup_callback,
                                NULL, NULL);
  if (id < 0) {
    freelog(LOG_ERROR, _("Metaserver: internal error in "
                         "net_lookup_service_async() for "
                         "lookup of %s:%d."),
            metaname, metaport);
    metaserver_failed();
    return;
  }

  metaname_lookup_id = id;

  if (meta_patches[0] == '\0') {
    set_meta_patches_string(default_meta_patches_string());
  }
  if (meta_topic[0] == '\0') {
    set_meta_topic_string(default_meta_topic_string());
  }
  if (meta_message[0] == '\0') {
    set_meta_message_string(default_meta_message_string());
  }
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
  static bool want_update = FALSE;
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

  /* start a new timer if we haven't already */
  if (!last_send_timer) {
    last_send_timer = new_timer(TIMER_USER, TIMER_ACTIVE);
  }

  clear_timer_start(last_send_timer);
  want_update = FALSE;
  return send_to_metaserver(flag);
}

/**************************************************************************
  Returns the non-blocking socket connected to the metaserver, or -1.
**************************************************************************/
int metaserver_get_socket(void)
{
  return metaserver_sock;
}

/**************************************************************************
  Send any pending data in the metaserver buffer. If there is no more data
  or an error occurs, close the connection and free the buffer memory. If
  the write would block, just update the buffer offset according to how
  much data was sent.
**************************************************************************/
void metaserver_handle_write_ready(void)
{
  char *data;
  int len, nb, count;
  long err;

  if (metaserver_sock == -1) {
    return;
  }

  if (metaserver_buffer == NULL) {
    my_closesocket(metaserver_sock);
    metaserver_sock = -1;
    return;
  }

  data = metaserver_buffer + metaserver_buffer_offset;
  len = metaserver_buffer_length - metaserver_buffer_offset;
  count = 0;

  while (len > 0) {
    nb = my_writesocket(metaserver_sock, data, len);
    err = mysocketerrno();
    if (nb == -1) {
      if (my_socket_would_block(err)) {
        break;
      }
      freelog(LOG_ERROR, "Metaserver: socket write failed: %s",
              mystrsocketerror(err));
      goto DONE;
    }
    data += nb;
    len -= nb;
    count += nb;
  }

  if (len > 0) {
    metaserver_buffer_offset += count;
    return;
  }

  metaserver_success_count++;

DONE:
  my_closesocket(metaserver_sock);
  metaserver_sock = -1;
  free(metaserver_buffer);
  metaserver_buffer = NULL;
  metaserver_buffer_length = 0;
  metaserver_buffer_offset = 0;
}
