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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_LIBREADLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WIN32_NATIVE
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "fciconv.h"

#include "capability.h"
#include "dataio.h"
#include "events.h"
#include "fcintl.h"
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "packets.h"
#include "shared.h"
#include "support.h"
#include "timing.h"

#include "connecthand.h"
#include "console.h"
#include "database.h"
#include "meta.h"
#include "plrhand.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "vote.h"

#include "sernet.h"

static struct connection connections[MAX_NUM_CONNECTIONS];

#ifdef GENERATING_MAC      /* mac network globals */
TEndpointInfo serv_info;
EndpointRef serv_ep;
#else
static int sock;
static int socklan;
#endif

#if defined(__VMS)
#  if defined(_VAX_)
#    define lib$stop LIB$STOP
#    define sys$qiow SYS$QIOW
#    define sys$assign SYS$ASSIGN
#  endif
#  include <descrip.h>
#  include <iodef.h>
#  include <stsdef.h>
#  include <starlet.h>
#  include <lib$routines.h>
#  include <efndef.h>
   static unsigned long int tt_chan;
   static char input_char = 0;
   static char got_input = 0;
   void user_interrupt_callback();
#endif

#define SPECLIST_TAG timer
#define SPECLIST_TYPE struct timer
#include "speclist.h"

#define PROCESSING_TIME_STATISTICS 0

static int server_accept_connection(int sockfd);
static void start_processing_request(struct connection *pconn,
				     int request_id);
static void finish_processing_request(struct connection *pconn);
static void ping_connection(struct connection *pconn);
static void send_ping_times_to_all(void);

static void get_lanserver_announcement(void);
static void send_lanserver_response(void);

static bool no_input = FALSE;

/* Avoid compiler warning about defined, but unused function
 * by defining it only when needed */
#if defined(HAVE_LIBREADLINE) || \
    (!defined(SOCKET_ZERO_ISNT_STDIN) && !defined(HAVE_LIBREADLINE))  
/*****************************************************************************
  This happens if you type an EOF character with nothing on the current line.
*****************************************************************************/
static void handle_stdin_close(void)
{
  /* Note this function may be called even if SOCKET_ZERO_ISNT_STDIN, so
   * the preprocessor check has to come inside the function body.  But
   * perhaps we want to do this even when SOCKET_ZERO_ISNT_STDIN? */
#ifndef SOCKET_ZERO_ISNT_STDIN
  freelog(LOG_NORMAL,
	  _("Server cannot read standard input. Ignoring input."));
  no_input = TRUE;
#endif /* SOCKET_ZERO_ISNT_STDIN */
}

#endif /* HAVE_LIBREADLINE || (!SOCKET_ZERO_ISNT_STDIN && !HAVE_LIBREADLINE) */

#ifdef HAVE_LIBREADLINE
/****************************************************************************/

#define HISTORY_FILENAME  ".civserver_history"
#define HISTORY_LENGTH    100

static char *history_file = NULL;

static bool readline_handled_input = FALSE;

static bool readline_initialized = FALSE;

/*****************************************************************************
...
*****************************************************************************/
static void handle_readline_input_callback(char *line)
{
  char *line_internal;

  if (no_input)
    return;

  if (!line) {
    handle_stdin_close();	/* maybe print an 'are you sure?' message? */
    return;
  }

  if (line[0] != '\0')
    add_history(line);

  con_prompt_enter();		/* just got an 'Enter' hit */
  line_internal = local_to_internal_string_malloc(line);
  (void) handle_stdin_input(NULL, line_internal, FALSE);
  free(line_internal);

  readline_handled_input = TRUE;
}

#endif /* HAVE_LIBREADLINE */
/*****************************************************************************
...
*****************************************************************************/
static void close_connection(struct connection *pconn)
{
  cancel_connection_votes(pconn);

  if (pconn->server.ping_timers != NULL) {
    while (timer_list_size(pconn->server.ping_timers) > 0) {
      struct timer *timer = timer_list_get(pconn->server.ping_timers, 0);
      
      timer_list_unlink(pconn->server.ping_timers, timer);
      free_timer(timer);
    }
    assert(timer_list_size(pconn->server.ping_timers) == 0);
    timer_list_free(pconn->server.ping_timers);
    pconn->server.ping_timers = NULL;
  }

  /* safe to do these even if not in lists: */
  conn_list_unlink(game.all_connections, pconn);
  conn_list_unlink(game.est_connections, pconn);
  conn_list_unlink(game.game_connections, pconn);

  pconn->player = NULL;
  pconn->access_level = ALLOW_NONE;
  connection_common_close(pconn);

  send_conn_info(pconn->self, game.est_connections);
}

/*****************************************************************************
...
*****************************************************************************/
void close_connections_and_socket(void)
{
  int i;
  lsend_packet_server_shutdown(game.all_connections);

  clear_all_on_connect_user_actions();

  for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
    if (connections[i].used) {
      close_connection(&connections[i]);
    }
    conn_list_free(connections[i].self);
  }

  /* Remove the game connection lists and make sure they are empty. */
  assert(conn_list_size(game.all_connections) == 0);
  conn_list_free(game.all_connections);
  assert(conn_list_size(game.est_connections) == 0);
  conn_list_free(game.est_connections);
  assert(conn_list_size(game.game_connections) == 0);
  conn_list_free(game.game_connections);

  my_closesocket(sock);
  my_closesocket(socklan);

#ifdef HAVE_LIBREADLINE
  if (history_file) {
    write_history(history_file);
    history_truncate_file(history_file, HISTORY_LENGTH);
  }
#endif

  send_server_info_to_metaserver(META_GOODBYE);
  server_close_meta();

  my_shutdown_network();
}

/*****************************************************************************
  Used for errors and other strangeness.  As well as some direct uses, is
  passed to packet routines as callback for when packet sending has a write
  error.  Closes the connection cleanly, calling lost_connection_to_client()
  to clean up server variables, notify other clients, etc.

  NB!! You should almost always use server_break_connection instead of this
  function.
*****************************************************************************/
static void server_close_socket_callback(struct connection *pc)
{
  lost_connection_to_client(pc);
  close_connection(pc);
}

/*****************************************************************************
  Break a client connection. Send the packet which will allow the client
  to reconnect. You should almost always use this function instead of
  calling server_close_socket_callback directly.
*****************************************************************************/
void server_break_connection(struct connection *pconn, enum exit_state state)
{
  pconn->exit_state = state;

  if (pconn->server.currently_processed_request_id > 0) {
    finish_processing_request(pconn);
  }
  flush_connection_send_buffer_all(pconn);

  server_close_socket_callback(pconn);
}

/****************************************************************************
  Attempt to flush all information in the send buffers for upto 'netwait'
  seconds (default netwait is 4 seconds).
*****************************************************************************/
void force_flush_packets(void)
{
  int i;
  int max_desc;
  fd_set writefs, exceptfs;
  struct timeval tv;
  time_t start;
  struct connection *pconn;

  start = time(NULL);

  freelog(LOG_DEBUG, "flush_packets");

  for (;;) {
    tv.tv_sec = (game.server.netwait - (time(NULL) - start));
    tv.tv_usec = 0;

    if (tv.tv_sec < 0) {
      return;
    }

    freelog(LOG_DEBUG, "tv.tv_sec = %lu", tv.tv_sec);

    MY_FD_ZERO(&writefs);
    MY_FD_ZERO(&exceptfs);
    max_desc = -1;

    for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
      pconn = &connections[i];
      if (!pconn->used || pconn->delayed_disconnect
          || pconn->send_buffer == NULL
          || pconn->send_buffer->ndata <= 0) {
        continue;
      }
      freelog(LOG_DEBUG, "pconn %s added to write set", conn_description(pconn));
      FD_SET(pconn->sock, &writefs);
      FD_SET(pconn->sock, &exceptfs);
      max_desc = MAX(pconn->sock, max_desc);
    }

    if (max_desc == -1) {
      return;
    }

    if (select(max_desc + 1, NULL, &writefs, &exceptfs, &tv) <= 0) {
      freelog(LOG_DEBUG, "flush_packets select returned <= 0");
      return;
    }

    for (i = 0; i < MAX_NUM_CONNECTIONS; i++) { 
      pconn = &connections[i];
      if (!pconn->used || pconn->delayed_disconnect
          || pconn->send_buffer == NULL
          || pconn->send_buffer->ndata <= 0) {
        continue;
      }

      if (FD_ISSET(pconn->sock, &exceptfs)) {
        freelog(LOG_NORMAL, _("Cutting connection %s due to "
                              "network exception."),
                conn_description(pconn));
        server_break_connection(pconn, ES_NETWORK_EXCEPTION);
        continue;
      }

      if (FD_ISSET(pconn->sock, &writefs)) {
        flush_connection_send_buffer_all(pconn);
      }

      if (game.server.tcptimeout != 0 && pconn->last_write != 0
          && (time(NULL) > pconn->last_write + game.server.tcptimeout)) {
        freelog(LOG_NORMAL, _("Cutting connection %s due to write lag "
                              "(waited too long for write)."),
                conn_description(pconn));
        server_break_connection(pconn, ES_LAGGING_CONN);
      }
    }
  }
}

/*****************************************************************************
Get and handle:
- new connections,
- input from connections,
- input from server operator in stdin
Returns:
  0 if went past end-of-turn timeout
  2 if force_end_of_sniff found to be set
  1 otherwise (got and processed something?)
This function also handles prompt printing, via the con_prompt_*
functions.  That is, other functions should not need to do so.  --dwp
*****************************************************************************/
int sniff_packets(void)
{
  int i, nb;
  int max_desc;
  fd_set readfs, writefs, exceptfs;
  struct timeval tv;
  static int year;
#ifdef SOCKET_ZERO_ISNT_STDIN
  char *bufptr;
#endif
  struct connection *pconn = NULL;

  con_prompt_init();

#ifdef HAVE_LIBREADLINE
  if (!no_input && !readline_initialized) {
    char *home_dir = user_home_dir();
    if (home_dir) {
      history_file = fc_malloc(strlen(home_dir) + 1
                               + strlen(HISTORY_FILENAME) + 1);
      if (history_file) {
        strcpy(history_file, home_dir);
        strcat(history_file, "/");
        strcat(history_file, HISTORY_FILENAME);
        using_history();
        read_history(history_file);
      }
    }

    rl_initialize();
    rl_callback_handler_install("> ", handle_readline_input_callback);
    rl_attempted_completion_function = freeciv_completion;
    atexit(rl_callback_handler_remove);

    readline_initialized = TRUE;
  }
#endif /* HAVE_LIBREADLINE */


  if (year != game.info.year && server_state == RUN_GAME_STATE) {
    year = game.info.year;
  }

  if (game.info.timeout == 0) {
    /* Just in case someone sets timeout we keep game.turn_start updated */
    game.server.turn_start = time(NULL);
  }


  while (TRUE) {

    /* Show the console prompt. */
    con_prompt_on();

    if (force_end_of_sniff) {
      force_end_of_sniff = FALSE;
      con_prompt_off();
      return 2;
    }

    get_lanserver_announcement();

    /* End server if no players for 'srvarg.quitidle' seconds. */
    if (srvarg.quitidle != 0 && server_state != PRE_GAME_STATE) {
      static time_t last_noplayers = 0;

      if (game.est_connections != NULL
          && conn_list_size(game.est_connections) == 0) {
        char buf[256];

        if (last_noplayers != 0) {
          if (time(NULL) > last_noplayers + srvarg.quitidle) {
            if (srvarg.exit_on_end) {
              save_game_auto();
            }
            my_snprintf(buf, sizeof(buf),
                        _("Restarting for lack of players"));
            set_meta_message_string(buf);
            freelog(LOG_NORMAL, "%s", buf);
            send_server_info_to_metaserver(META_INFO);

            server_state = GAME_OVER_STATE;
            force_end_of_sniff = TRUE;

            if (srvarg.exit_on_end) {
              /* No need for anything more; just quit. */
              /* XXX What about FCDB update? */
              server_quit();
            }
          }
        } else {
          last_noplayers = time(NULL);

          my_snprintf(buf, sizeof(buf),
              _("Restarting in %d seconds for lack of players."),
              srvarg.quitidle);
          set_meta_message_string(buf);
          freelog(LOG_NORMAL, "%s", buf);

          send_server_info_to_metaserver(META_INFO);
        }
      } else {
        last_noplayers = 0;
      }
    } /* quitidle */


    /* Report ping times and ping all connections. */
    if (time(NULL) > (game.server.last_ping + game.server.pingtime)) {
      struct timer *ping_timer;

      /* Send data about the previous pings. */
      send_ping_times_to_all();

      for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
        pconn = &connections[i];
        if (!pconn->used || !pconn->established
            || !pconn->server.ping_timers) {
          continue;
        }

        if (timer_list_size(pconn->server.ping_timers) > 0) {
          ping_timer = timer_list_get(pconn->server.ping_timers, 0);
        } else {
          ping_timer = NULL;
        }

        if ((ping_timer != NULL
             && read_timer_seconds(ping_timer) > game.server.pingtimeout)
            || pconn->ping_time > game.server.pingtimeout) {

          /* Disconnect timed-out users, except for hack-level ones. */
          if (pconn->access_level == ALLOW_HACK) {
            freelog(LOG_NORMAL, _("Ignoring ping timeout for "
                                  "hack-level connection %s."),
                    conn_description(pconn));
          } else {
            freelog(LOG_NORMAL, "Disconnecting %s due to ping timeout.",
                    conn_description(pconn));

            server_break_connection(pconn, ES_PING_TIMEOUT);
          }
        } else {
          ping_connection(pconn);
        }
      }
      game.server.last_ping = time(NULL);
    }


    /* if we've waited long enough after a failure, respond to the client */
    if (srvarg.auth.enabled) {
      conn_list_iterate(game.all_connections, pconn) {
        if (pconn->server.status != AS_ESTABLISHED) {
          process_authentication_status(pconn);
        }
      } conn_list_iterate_end;
    }


    /* Don't wait if timeout == -1 (i.e. on auto games) */
    if (server_state != PRE_GAME_STATE && game.info.timeout == -1) {
      send_server_info_to_metaserver(META_REFRESH);
      return 0;
    }

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    MY_FD_ZERO(&readfs);
    MY_FD_ZERO(&writefs);
    MY_FD_ZERO(&exceptfs);

    if (!no_input) {
#ifdef SOCKET_ZERO_ISNT_STDIN
      my_init_console();
#else
#   if !defined(__VMS)
      FD_SET(0, &readfs);
#   endif
#endif
    }

    FD_SET(sock, &readfs);
    FD_SET(sock, &exceptfs);
    max_desc = sock;

    for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
      if (!connections[i].used) {
        continue;
      }
      FD_SET(connections[i].sock, &readfs);
      if (connections[i].send_buffer->ndata > 0) {
        FD_SET(connections[i].sock, &writefs);
      }
      FD_SET(connections[i].sock, &exceptfs);
      max_desc = MAX(connections[i].sock, max_desc);
    }

    if (adns_is_available()) {
      FD_SET(adns_get_socket_fd(), &readfs);
      max_desc = MAX(adns_get_socket_fd(), max_desc);
    }

    /* For any messages occuring after this, don't generate a new prompt. */
    con_prompt_off();

    if (select(max_desc + 1, &readfs, &writefs, &exceptfs, &tv) == 0) {
      /* The select statement timed-out. */

      send_server_info_to_metaserver(META_REFRESH);

      if (game.info.timeout != 0
	  && time(NULL) > game.server.turn_start + game.info.timeout
          && server_state == RUN_GAME_STATE) {
        con_prompt_off();
        return 0;
      }

      if (!no_input) {
#if defined(__VMS)
        struct {
          short numchars;
          char firstchar;
          char reserved;
          int reserved2;
        } ttchar;
        unsigned long status;
        status =
            sys$qiow(EFN$C_ENF, tt_chan, IO$_SENSEMODE | IO$M_TYPEAHDCNT,
                     0, 0, 0, &ttchar, sizeof(ttchar), 0, 0, 0, 0);
        if (!$VMS_STATUS_SUCCESS(status)) {
          lib$stop(status);
        }
        if (ttchar.numchars) {
          FD_SET(0, &readfs);
        } else {
          continue;
        }
#else /* !__VMS */
#ifndef SOCKET_ZERO_ISNT_STDIN
        continue;
#endif /* SOCKET_ZERO_ISNT_STDIN */
#endif /* !__VMS */
      }

    } /* select */

    if (game.info.timeout == 0) {
      /* Just in case someone sets timeout we keep game.turn_start updated. */
      game.server.turn_start = time(NULL);
    }

    if (FD_ISSET(sock, &exceptfs)) {
      /* handle Ctrl-Z suspend/resume */
      continue;
    }

    if (FD_ISSET(sock, &readfs)) {
      /* Server got a new connection. */

      freelog(LOG_VERBOSE, "got new connection");
      if (server_accept_connection(sock) == -1) {
        freelog(LOG_NORMAL, _("Failed accepting connection"));
      }
    }

    /* Check connections for network exceptions. */
    for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
      pconn = &connections[i];
      if (!pconn->used) {
        continue;
      }

      if (!FD_ISSET(pconn->sock, &exceptfs)) {
        continue;
      }
      freelog(LOG_ERROR, "Disconnecting %s due to network "
              "data exception.",
              conn_description(pconn));
      server_break_connection(pconn, ES_NETWORK_EXCEPTION);
    }

    /* Check for adns responses */
    if (adns_is_available()) {
      if (FD_ISSET(adns_get_socket_fd(), &readfs)) {
        adns_poll();
      }
      adns_check_expired();
    }

#ifdef SOCKET_ZERO_ISNT_STDIN
    if (!no_input && (bufptr = my_read_console())) {
      char *bufptr_internal = local_to_internal_string_malloc(bufptr);

      con_prompt_enter();       /* will need a new prompt, regardless */
      handle_stdin_input(NULL, bufptr_internal, FALSE);
      free(bufptr_internal);
    }

#else /* !SOCKET_ZERO_ISNT_STDIN */

    if (!no_input && FD_ISSET(0, &readfs)) {  /* input from server operator */

#ifdef HAVE_LIBREADLINE

      rl_callback_read_char();
      if (readline_handled_input) {
        readline_handled_input = FALSE;
        con_prompt_enter_clear();
      }
      continue;

#else /* !HAVE_LIBREADLINE */

      char buf[BUF_SIZE + 1];
      char *buf_internal;

      nb = read(0, buf, BUF_SIZE);
      if (nb <= 0) {
        handle_stdin_close();
        nb = 0; /* Avoid buffer underrun below. */
      }

      buf[nb] = '\0';
      con_prompt_enter();       /* will need a new prompt, regardless */
      buf_internal = local_to_internal_string_malloc(buf);
      handle_stdin_input(NULL, buf_internal, FALSE);
      free(buf_internal);

#endif /* !HAVE_LIBREADLINE */

    } else

#endif /* !SOCKET_ZERO_ISNT_STDIN */

    { 
      /* Check input from a user connections. */
      for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
        void *packet;
        enum packet_type type;
        bool result, accepted;

#if PROCESSING_TIME_STATISTICS
        int err;
        struct timeval start, end;
        struct timezone tz;
        long us;
#endif

  	pconn = &connections[i];

	if (!pconn->used || !FD_ISSET(pconn->sock, &readfs)) {
          continue;
        }

        nb = read_socket_data(pconn->sock, pconn->buffer);
	if (nb < 0) {
          /* Read error or connection closed. */
          /* FIXME It should distinguish between read errors
           * and simple peer disconnects. */
          server_break_connection(pconn, ES_REMOTE_CLOSE);
	}

        while (TRUE) {
          /* decode "freeciv packets" from real packet */
          packet = get_packet_from_connection(pconn, &type, &result);

          if (!result) {
            break;
          }

          pconn->server.packets_received++;

          pconn->server.last_request_id_seen =
              get_next_request_id(pconn->server.last_request_id_seen);

#if PROCESSING_TIME_STATISTICS
          err = gettimeofday(&start, &tz);
          assert(!err);
#endif
          connection_do_buffer(pconn);
          start_processing_request(pconn,
                                   pconn->server.
                                   last_request_id_seen);
          accepted = handle_packet_input(pconn, packet, type);
          if (packet) {
            free(packet);
            packet = NULL;
          }

          /* Maybe changed */
          if (!pconn->used) {
            break;
          }

          finish_processing_request(pconn);
          connection_do_unbuffer(pconn);
          if (!accepted) {
            server_break_connection(pconn, ES_REJECTED);
          }

#if PROCESSING_TIME_STATISTICS
          err = gettimeofday(&end, &tz);
          assert(!err);

          us = (end.tv_sec - start.tv_sec) * 1000000 +
              end.tv_usec - start.tv_usec;

          freelog(LOG_NORMAL,
                  "processed request %d in %ld.%03ldms",
                  pconn->server.last_request_id_seen, us / 1000,
                  us % 1000);
#endif /* PROCESSING_TIME_STATISTICS */

	} /* decode packet loop */
      } /* connection iterate */

      /* Handle user connection write readiness. */
      for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
        pconn = &connections[i];

        if (!pconn->used || !pconn->send_buffer
            || pconn->send_buffer->ndata <= 0) {
          continue;
        }

        if (FD_ISSET(pconn->sock, &writefs)) {
          flush_connection_send_buffer_all(pconn);
          continue;
        }

        if (game.server.tcptimeout != 0 && pconn->last_write != 0
            && (time(NULL) > pconn->last_write + game.server.tcptimeout)) {
          freelog(LOG_ERROR, "Disconnecting %s due to timeout waiting "
                  "for write readiness.",
                  conn_description(pconn));
          server_break_connection(pconn, ES_LAGGING_CONN);
        }
      }
    }

    break;

  } /* while (TRUE) */

  con_prompt_off();

  if (game.info.timeout != 0
      && (time(NULL) > game.server.turn_start + game.info.timeout)) {
    /* Went past end of turn timeout. */
    return 0;
  }

  return 1;
}

/********************************************************************
  Make up a name for the connection, before we get any data from
  it to use as a sensible name.  Name will be 'c' + integer,
  guaranteed not to be the same as any other connection name,
  nor player name nor user name, nor connection id (avoid possible
  confusions).   Returns pointer to static buffer, and fills in
  (*id) with chosen value.
********************************************************************/
static const char *makeup_connection_name(int *id)
{
  static unsigned short i = 0;
  static char name[MAX_LEN_NAME];

  for(;;) {
    if (i == (unsigned short) -1)
      i++;			/* don't use 0 */
    my_snprintf(name, sizeof(name), "c%u", (unsigned int)++i);
    if (!find_player_by_name(name)
	&& !find_player_by_user(name)
	&& !find_conn_by_id(i)
	&& !find_conn_by_user(name)) {
      *id = i;
      return name;
    }
  }
}
  
/********************************************************************
  ...
********************************************************************/
static void reverse_lookup_cb(const unsigned char *addr,
                              int addrlen, void *data)
{
  struct connection *pconn = data;
  const char *hostname = (const char *) addr;

  assert(pconn != NULL);

  freelog(LOG_DEBUG, "rlc reverse_lookup_cb addr=%p addrlen=%d data=%p",
          addr, addrlen, data);

  if (!pconn->used) {
    freelog(LOG_DEBUG, "rlc callback for non-existant pconn!");
    return;
  }

  pconn->server.adns_id = -1;

  if (addrlen > 0) {
    freelog(LOG_VERBOSE, "ADNS found hostname \"%s\" for "
            "connection (%s) from %s", hostname, pconn->username,
            pconn->server.ipaddr);
    receive_hostname(pconn, hostname);
  } else {
    freelog(LOG_VERBOSE, "ADNS could not find hostname for "
            "connection %s (%s)", pconn->server.ipaddr,
            pconn->username);
    receive_hostname(pconn, NULL);
  }
}

/********************************************************************
  Server accepts connection from client:
  Low level socket stuff, and basic-initialize the connection struct.
  Returns 0 on success, -1 on failure (bad accept(), or too many
  connections).
********************************************************************/
static int server_accept_connection(int sockfd)
{
  /* This used to have size_t for some platforms.  If this is necessary
   * it should be done with a configure check not a platform check. */
#ifdef HAVE_SOCKLEN_T
  socklen_t fromlen;
#else
  int fromlen;
#endif

  int new_sock;
  union my_sockaddr fromend;
  struct hostent *from = NULL;
  int i;
  struct connection *pconn = NULL;
  char ipaddr[MAX_LEN_ADDR];

  freelog(LOG_DEBUG, "sac server_accept_connection sockfd=%d", sockfd);
  fromlen = sizeof(fromend);

  if ((game.server.maxconnections > 0)
      && (conn_list_size(game.all_connections) > game.server.maxconnections)) {
    freelog(LOG_NORMAL, _("Maximum number of connections "
			  "for this server exceeded."));
    return -1;
  }

  if ((new_sock = accept(sockfd, &fromend.sockaddr, &fromlen)) == -1) {
    freelog(LOG_ERROR, "accept failed: %s", mystrsocketerror());
    return -1;
  }

  sz_strlcpy(ipaddr, inet_ntoa(fromend.sockaddr_in.sin_addr));
  if (game.server.maxconnectionsperhost > 0) {
    int count = 0;

    conn_list_iterate(game.all_connections, pconn) {
      if (strcmp(ipaddr, pconn->server.ipaddr) == 0) {
	if (++count >= game.server.maxconnectionsperhost) {
	  freelog(LOG_NORMAL, _("Maximum number of connections "
			        "for this host exceeded."));
	  my_closesocket(new_sock);
	  return -1;
	}
      }
    } conn_list_iterate_end;
  }

  if (-1 == my_nonblock(new_sock)) {
    my_closesocket(new_sock);
    return -1;
  }

  for(i=0; i<MAX_NUM_CONNECTIONS; i++) {
    if (!connections[i].used) {
      pconn = &connections[i];
      break;
    }
  }

  if (!pconn) {
    freelog(LOG_ERROR, "maximum number of connections reached");
    my_closesocket(new_sock);
    return -1;
  }

  connection_common_init(pconn);
  pconn->sock = new_sock;
  pconn->observer = FALSE;
  pconn->player = NULL;
  pconn->capability[0] = '\0';
  pconn->delayed_disconnect = FALSE;
  pconn->notify_of_writable_data = NULL;
  pconn->server.currently_processed_request_id = 0;
  pconn->server.last_request_id_seen = 0;
  pconn->server.auth_tries = 0;
  pconn->server.auth_settime = 0;
  pconn->server.delay_counter = 0;
  pconn->server.packets_received = 0;
  pconn->server.status = AS_NOT_ESTABLISHED;
  pconn->server.ping_timers = timer_list_new();
  pconn->ping_time = -1.0;
  pconn->incoming_packet_notify = NULL;
  pconn->outgoing_packet_notify = NULL;
  sz_strlcpy(pconn->username, makeup_connection_name(&pconn->id));
  pconn->server.ipaddr[0] = '\0';
  pconn->addr[0] = '\0';
  pconn->server.adns_id = -1;
  pconn->server.received_username = FALSE;
  pconn->granted_access_level = pconn->access_level = ALLOW_NONE;
  pconn->server.delay_establish = FALSE;

  if (!receive_ip(pconn, ipaddr)) {
    /* Banned, but not an error. */
    return 0;
  }

  if (!srvarg.no_dns_lookup) {
    if (adns_is_available()) {
      freelog(LOG_DEBUG, "sac making adns request");
      pconn->server.adns_id = adns_reverse_lookup(&fromend,
          reverse_lookup_cb, pconn, NULL);
      freelog(LOG_DEBUG, "sac got adns_id=%d", pconn->server.adns_id);
      if (pconn->server.adns_id == 0) {
        /* reverse_lookup_cb called already */
        pconn->server.adns_id = -1;
      }
      if (!pconn->used) {
	/* Have been banned (not an error) */
	return 0;
      }
    } else {
      from = gethostbyaddr((char *) &fromend.sockaddr_in.sin_addr,
                           sizeof(fromend.sockaddr_in.sin_addr), AF_INET);
      if (!receive_hostname(pconn, from ? from->h_name : NULL)) {
        /* Banned, but not an error. */
        return 0;
      }
    }
  }
  
  freelog(LOG_VERBOSE, "connection (%s) from %s (%s)", 
          pconn->username, pconn->addr, !from && pconn->server.adns_id > 0
          ? "hostname lookup in progress" : pconn->server.ipaddr);

  conn_list_append(game.all_connections, pconn);
  ping_connection(pconn);

  return 0;
}

/********************************************************************
 open server socket to be used to accept client connections
 and open a server socket for server LAN announcements.
********************************************************************/
int server_open_socket(void)
{
  /* setup socket address */
  union my_sockaddr src;
  union my_sockaddr addr;
  struct ip_mreq mreq;
  const char *group;
  int opt;

  /* Create socket for client connections. */
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    die("socket failed: %s", mystrsocketerror());
  }

  opt=1; 
  if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
		(char *)&opt, sizeof(opt)) == -1) {
    freelog(LOG_ERROR, "SO_REUSEADDR failed: %s", mystrsocketerror());
  }

  if (!net_lookup_service(srvarg.bind_addr, srvarg.port, &src)) {
    freelog(LOG_ERROR, _("Server: bad address: [%s:%d]."),
	    srvarg.bind_addr, srvarg.port);
    exit(EXIT_FAILURE);
  }

  if(bind(sock, &src.sockaddr, sizeof (src)) == -1) {
    freelog(LOG_FATAL, "bind failed: %s", mystrsocketerror());
    exit(EXIT_FAILURE);
  }

  if(listen(sock, MAX_NUM_CONNECTIONS) == -1) {
    freelog(LOG_FATAL, "listen failed: %s", mystrsocketerror());
    exit(EXIT_FAILURE);
  }

  /* Create socket for server LAN announcements */
  if ((socklan = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    freelog(LOG_ERROR, "socket failed: %s", mystrsocketerror());
  }

  if (setsockopt(socklan, SOL_SOCKET, SO_REUSEADDR,
                 (char *)&opt, sizeof(opt)) == -1) {
    freelog(LOG_ERROR, "SO_REUSEADDR failed: %s", mystrsocketerror());
  }

  my_nonblock(socklan);

  group = get_multicast_group();

  memset(&addr, 0, sizeof(addr));
  addr.sockaddr_in.sin_family = AF_INET;
  addr.sockaddr_in.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sockaddr_in.sin_port = htons(SERVER_LAN_PORT);

  if (bind(socklan, &addr.sockaddr, sizeof(addr)) < 0) {
    freelog(LOG_ERROR, "bind failed: %s", mystrsocketerror());
  }

  mreq.imr_multiaddr.s_addr = inet_addr(group);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);

  if (setsockopt(socklan, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                 (const char*)&mreq, sizeof(mreq)) < 0) {
    freelog(LOG_ERROR, "setsockopt failed: %s", mystrsocketerror());
  }

  return 0;
}


/********************************************************************
  Called once and only once when the server starts.
********************************************************************/
void init_connections(void)
{
  int i;
  struct connection *pconn;

  game.all_connections = conn_list_new();
  game.est_connections = conn_list_new();
  game.game_connections = conn_list_new();
  on_connect_user_actions = user_action_list_new();

  for (i = 0; i < MAX_NUM_CONNECTIONS; i++) { 
    pconn = &connections[i];
    pconn->used = FALSE;
    pconn->self = conn_list_new();
    conn_list_append(pconn->self, pconn);
  }
#if defined(__VMS)
  {
    unsigned long status;
    $DESCRIPTOR (tt_desc, "SYS$INPUT");
    status = sys$assign(&tt_desc,&tt_chan,0,0);
    if (!$VMS_STATUS_SUCCESS(status))
      lib$stop(status);
  }
#endif

  close_socket_set_callback(server_close_socket_callback);
}

/**************************************************************************
...
**************************************************************************/
static void start_processing_request(struct connection *pconn,
				     int request_id)
{
  assert(request_id);
  assert(pconn->server.currently_processed_request_id == 0);
  freelog(LOG_DEBUG, "start processing packet %d from connection %d",
	  request_id, pconn->id);
  send_packet_processing_started(pconn);
  pconn->server.currently_processed_request_id = request_id;
}

/**************************************************************************
...
**************************************************************************/
static void finish_processing_request(struct connection *pconn)
{
  assert(pconn->server.currently_processed_request_id);
  freelog(LOG_DEBUG, "finish processing packet %d from connection %d",
	  pconn->server.currently_processed_request_id, pconn->id);
  send_packet_processing_finished(pconn);
  pconn->server.currently_processed_request_id = 0;
}

/**************************************************************************
...
**************************************************************************/
static void ping_connection(struct connection *pconn)
{
  freelog(LOG_DEBUG, "sending ping to %s (open=%d)",
	  conn_description(pconn),
	  timer_list_size(pconn->server.ping_timers));
  timer_list_append(pconn->server.ping_timers,
			 new_timer_start(TIMER_USER, TIMER_ACTIVE));
  send_packet_conn_ping(pconn);
}

/**************************************************************************
...
**************************************************************************/
void handle_conn_pong(struct connection *pconn)
{
  struct timer *timer;

  if (timer_list_size(pconn->server.ping_timers) == 0) {
    freelog(LOG_NORMAL, "got unexpected pong from %s",
	    conn_description(pconn));
    return;
  }

  timer = timer_list_get(pconn->server.ping_timers, 0);
  timer_list_unlink(pconn->server.ping_timers, timer);
  pconn->ping_time = read_timer_seconds_free(timer);
  freelog(LOG_DEBUG, "got pong from %s (open=%d); ping time = %fs",
	  conn_description(pconn),
	  timer_list_size(pconn->server.ping_timers), pconn->ping_time);
}

/**************************************************************************
...
**************************************************************************/
static void send_ping_times_to_all(void)
{
  struct packet_conn_ping_info packet;
  int i;

  i = 0;
  conn_list_iterate(game.game_connections, pconn) {
    if (!pconn->used) {
      continue;
    }
    i++;
  } conn_list_iterate_end;

  packet.connections = i;
  packet.old_connections = MIN(i, MAX_NUM_PLAYERS);

  i = 0;
  conn_list_iterate(game.game_connections, pconn) {
    if (!pconn->used) {
      continue;
    }
    assert(i < ARRAY_SIZE(packet.conn_id));
    packet.conn_id[i] = pconn->id;
    packet.ping_time[i] = pconn->ping_time;
    if (i < packet.old_connections) {
      packet.old_conn_id[i] = pconn->id;
      packet.old_ping_time[i] = pconn->ping_time;
    }
    i++;
  } conn_list_iterate_end;
  lsend_packet_conn_ping_info(game.est_connections, &packet);
}

/********************************************************************
  Listen for UDP packets multicasted from clients requesting
  announcement of servers on the LAN.
********************************************************************/
static void get_lanserver_announcement(void)
{
  char msgbuf[128];
  struct data_in din;
  int type;
  fd_set readfs, exceptfs;
  struct timeval tv;

  FD_ZERO(&readfs);
  FD_ZERO(&exceptfs);
  FD_SET(socklan, &exceptfs);
  FD_SET(socklan, &readfs);

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  while (select(socklan + 1, &readfs, NULL, &exceptfs, &tv) == -1) {
#ifdef WIN32_NATIVE
      if (my_errno() != WSAEINTR) {
#else
      if (my_errno() != EINTR) {
#endif
      freelog(LOG_ERROR, "select failed: %s", mystrsocketerror());
      return;
    }
    /* EINTR can happen sometimes, especially when compiling with -pg.
     * Generally we just want to run select again. */
  }

  if (FD_ISSET(socklan, &readfs)) {
    if (0 < recvfrom(socklan, msgbuf, sizeof(msgbuf), 0, NULL, NULL)) {
      dio_input_init(&din, msgbuf, 1);
      dio_get_uint8(&din, &type);
      if (type == SERVER_LAN_VERSION) {
        freelog(LOG_DEBUG, "Received request for server LAN announcement.");
        send_lanserver_response();
      } else {
        freelog(LOG_DEBUG,
                "Received invalid request for server LAN announcement.");
      }
    }
  }
}

/********************************************************************
  This function broadcasts an UDP packet to clients with
  that requests information about the server state.
********************************************************************/
static void send_lanserver_response(void)
{
  unsigned char buffer[MAX_LEN_PACKET];
  char hostname[512];
  char port[256];
  char version[256];
  char players[256];
  char status[256];
  struct data_out dout;
  union my_sockaddr addr;
  int socksend, setting = 1;
  const char *group;
  size_t size;
  unsigned char ttl;

  /* Create a socket to broadcast to client. */
  if ((socksend = socket(AF_INET,SOCK_DGRAM, 0)) < 0) {
    freelog(LOG_ERROR, "socket failed: %s", mystrsocketerror());
    return;
  }

  /* Set the UDP Multicast group IP address of the packet. */
  group = get_multicast_group();
  memset(&addr, 0, sizeof(addr));
  addr.sockaddr_in.sin_family = AF_INET;
  addr.sockaddr_in.sin_addr.s_addr = inet_addr(group);
  addr.sockaddr_in.sin_port = htons(SERVER_LAN_PORT + 1);

  /* Set the Time-to-Live field for the packet.  */
  ttl = SERVER_LAN_TTL;
  if (setsockopt(socksend, IPPROTO_IP, IP_MULTICAST_TTL, 
                 (const char*)&ttl, sizeof(ttl))) {
    freelog(LOG_ERROR, "setsockopt failed: %s", mystrsocketerror());
    return;
  }

  if (setsockopt(socksend, SOL_SOCKET, SO_BROADCAST, 
                 (const char*)&setting, sizeof(setting))) {
    freelog(LOG_ERROR, "setsockopt failed: %s", mystrsocketerror());
    return;
  }

  /* Create a description of server state to send to clients.  */
  if (my_gethostname(hostname, sizeof(hostname)) != 0) {
    sz_strlcpy(hostname, "none");
  }

  my_snprintf(version, sizeof(version), "%d.%d.%d%s",
              MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION, VERSION_LABEL);

  switch (server_state) {
  case PRE_GAME_STATE:
    my_snprintf(status, sizeof(status), "Pregame");
    break;
  case RUN_GAME_STATE:
    my_snprintf(status, sizeof(status), "Running");
    break;
  case GAME_OVER_STATE:
    my_snprintf(status, sizeof(status), "Game over");
    break;
  default:
    my_snprintf(status, sizeof(status), "Waiting");
  }

   my_snprintf(players, sizeof(players), "%d",
               get_num_human_and_ai_players());
  my_snprintf(port, sizeof(port), "%d", srvarg.port);

  dio_output_init(&dout, buffer, sizeof(buffer));
  dio_put_uint8(&dout, SERVER_LAN_VERSION);
  dio_put_string(&dout, hostname);
  dio_put_string(&dout, port);
  dio_put_string(&dout, version);
  dio_put_string(&dout, status);
  dio_put_string(&dout, players);
  dio_put_string(&dout, get_meta_message_string());
  size = dio_output_used(&dout);

  /* Sending packet to client with the information gathered above. */
  if (sendto(socksend, buffer, size, 0, &addr.sockaddr, sizeof(addr)) < 0) {
    freelog(LOG_ERROR, "sendto failed: %s", mystrsocketerror());
    return;
  }

  my_closesocket(socksend);
}
