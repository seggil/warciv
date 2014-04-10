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
#ifndef WC_COMMON_CONNECTION_H
#define WC_COMMON_CONNECTION_H

#include <time.h>       /* time_t */

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#define USE_COMPRESSION

/**************************************************************************
  The connection struct and related stuff.
  Includes cmdlevel stuff, which is connection-based.
***************************************************************************/

#include "shared.h"             /* MAX_LEN_ADDR, bool type */

#include "wc_types.h"

struct hash_table;
struct timer_list;
struct timer;

#define MAX_LEN_PACKET   4096

#define MAX_LEN_PACKET_BUFFER   (MAX_LEN_PACKET * 128)
#define MAX_LEN_CAPSTR    512
#define MAX_LEN_PASSWORD  512 /* do not change this under any circumstances */

/**************************************************************************
  Command access levels for client-side use; at present, they are only
  used to control access to server commands typed at the client chatline.
***************************************************************************/
enum cmdlevel_id {    /* access levels for users to issue commands        */
  ALLOW_NONE = 0,     /* user may issue no commands at all                */
  ALLOW_OBSERVER,         /* user may issue observer commands            */
  ALLOW_BASIC,         /* user may issue basic commands            */
  ALLOW_CTRL,         /* user may issue commands that affect game & users */
  ALLOW_ADMIN,         /* admin user */
  ALLOW_HACK,         /* user may issue *all* commands - dangerous!       */

  ALLOW_NUM,          /* the number of levels                             */
  ALLOW_UNRECOGNIZED  /* used as a failure return code                    */
};
/*  the set command is a special case:                                    */
/*    - ALLOW_CTRL is required for SSET_TO_CLIENT options                 */
/*    - ALLOW_HACK is required for SSET_TO_SERVER options                 */

/***************************************************************************
  On the distinction between nations(formerly races), players, and users,
  see doc/HACKING
***************************************************************************/

/* where the connection is in the authentication process */
enum auth_status {
  AS_NOT_ESTABLISHED = 0,
  AS_FAILED,
  AS_REQUESTING_NEW_PASS,
  AS_REQUESTING_OLD_PASS,
  AS_ESTABLISHED
};

/* Describes how a connection was closed. */
/* NB: If you change these values, be sure to
 * update exit_state_name in conn_description. */
enum exit_state {
  EXIT_STATUS_NONE = 0,
  EXIT_STATUS_UNKNOWN,        /* Should not be used, unless unavoidable. */
  EXIT_STATUS_DECODING_ERROR, /* Failed decompressing or parsing packets. */
  EXIT_STATUS_PING_TIMEOUT,
  EXIT_STATUS_NETWORK_EXCEPTION,
  EXIT_STATUS_BUFFER_OVERFLOW,
  EXIT_STATUS_LAGGING_CONN,   /* Waited too long to write. */
  EXIT_STATUS_BANNED,
  EXIT_STATUS_AUTH_FAILED,
  EXIT_STATUS_CUT_COMMAND,
  EXIT_STATUS_WRITE_ERROR,
  EXIT_STATUS_READ_ERROR,
  EXIT_STATUS_REMOTE_CLOSE,
  EXIT_STATUS_REJECTED,       /* handle_packet_input returned FALSE. */
  EXIT_STATUS_KICKED,
  EXIT_STATUS_IDLECUT
};

/* get 'struct connection_list' and related functions: */
/* do this with forward definition of struct connection_s, so that
 * connection struct can contain a struct connection_list */
struct connection_s;
#define SPECLIST_TAG connection
#define SPECLIST_TYPE struct connection_s
#include "speclist.h"

#define connection_list_iterate(connlist, pconn) \
    TYPED_LIST_ITERATE(struct connection_s, connlist, pconn)
#define connection_list_iterate_end  LIST_ITERATE_END

#define global_observers_iterate(pconn)            \
    connection_list_iterate(game.est_connections, pconn) \
      if (conn_is_global_observer(pconn)) {
#define global_observers_iterate_end } connection_list_iterate_end

/***********************************************************
  This is a buffer where the data is first collected,
  whenever it arrives to the client/server.
***********************************************************/
struct socket_packet_buffer {
  int ndata;
  int do_buffer_sends;
  int nsize;
  unsigned char *data;
};

#define SPECVEC_TAG byte
#define SPECVEC_TYPE unsigned char
#include "specvec.h"

enum conn_pattern_type {
  CPT_ADDRESS = 0,
  CPT_HOSTNAME,
  CPT_USERNAME,

  NUM_CONN_PATTERN_TYPES
};

struct conn_pattern {
  char *pattern;
  int type;
};

extern char *conn_pattern_type_strs[NUM_CONN_PATTERN_TYPES];

/***********************************************************
  The connection struct represents a single client or server
  at the other end of a network connection.
***********************************************************/
struct connection_s {
  int id;                       /* used for server/client communication */
  int sock;
  bool used;
  bool established;             /* have negotiated initial packets */
  player_t *player;             /* NULL for connections not yet associated
                                   with a specific player */
  /*
   * connection is "observer", not controller; may be observing
   * specific player, or all (implementation incomplete).
   */
  bool observer;
  struct socket_packet_buffer *buffer;
  struct socket_packet_buffer *send_buffer;

  /* Cumulative fractional seconds that we have unsuccesfully
   * waited for this connection to become write ready. Reset
   * to zero whenever bytes are successfully written (and of
   * course on initialization). If this ever exceeds the value
   * of game.server.tcptimeout, then the connection is closed
   * (this is relevant only for the server). */
  double write_wait_time;

  double ping_time;

  struct connection_list *self;     /* list with this connection as single element */
  char username[MAX_LEN_NAME];
  char addr[MAX_LEN_ADDR];

  /*
   * "capability" gives the capability string of the executable (be it
   * a client or server) at the other end of the connection.
   */
  char capability[MAX_LEN_CAPSTR];

  /* Used to determine how the connection is broken */
  enum exit_state exit_state;

  /* When a read or write socket operation fails,
   * the error code (i.e. the return value of
   * mysocketerrno()) is saved to this field. */
  long error_code;

  /*
   * Called before an incoming packet is processed. The packet_type
   * argument should really be a "enum packet_type". However due
   * circular dependency this is impossible.
   */
  void (*incoming_packet_notify) (struct connection_s *pcon,
                                  int packet_type, int size);

  /*
   * Called before a packet is sent. The packet_type argument should
   * really be a "enum packet_type". However due circular dependency
   * this is impossible.
   */
  void (*outgoing_packet_notify) (struct connection_s *pcon,
                                  int packet_type, int size,
                                  int request_id);
  struct connection_phs {
    struct hash_table **sent;
    struct hash_table **received;
    int *variant;
  } phs;

#ifdef USE_COMPRESSION
  struct connection_compression {
    int frozen_level;

    struct byte_vector queue;
  } compression;
#endif
  struct connection_statistics {
    int bytes_send;
  } statistics;

  union connection_u {
    /* Specific client datas. */
    struct connection_client {
      void (*notify_of_writable_data) (struct connection_s *pcon,
                                       bool data_available_and_socket_full);

      /*
       * Increases for every packet send to the server.
       */
      int last_request_id_used;

      /*
       * Increases for every received PACKET_PROCESSING_FINISHED packet.
       */
      int last_processed_request_id_seen;

      /*
       * Holds the id of the request which caused this packet. Can be
       * zero.
       */
      int request_id_of_currently_handled_packet;
    } client;

    /* Specific server datas. */
    struct connection_server {
      /*
       * "access_level" stores the access granted to the client
       * corresponding to this connection.
       */
      enum cmdlevel_id access_level;
      enum cmdlevel_id granted_access_level; /* access level granted by the action list */

      /* Something has occurred that should have closed the
       * connection, but the closing has been postponed. If
       * this field is TRUE, under no circumstances should
       * writing be done to the connection. */
      bool is_closing;

      /*
       * Holds the id of the request which is processed now. Can be
       * zero.
       */
      int currently_processed_request_id;

      /*
       * Will increase for every received packet.
       */
      int last_request_id_seen;

      /*
       * The start times of the PACKET_CONN_PING which have been sent
       * but weren't PACKET_CONN_PONGed yet?
       */
      struct timer_list *ping_timers;

      /* Holds number of tries for authentication from client. */
      int auth_tries;

      /* the time that the server will respond after receiving an auth reply.
       * this is used to throttle the connection. Also used to reject a
       * connection if we've waited too long for a password. */
      time_t auth_settime;

      /* used to follow where the connection is in the authentication process */
      enum auth_status status;
      char password[MAX_LEN_PASSWORD];
      int salt; /* At least 32-bits. */

      /* for reverse lookup and blacklisting in db */
      char ipaddr[MAX_LEN_ADDR];

      unsigned int packets_received;
      unsigned int delay_counter;

      struct ignore_list *ignore_list;

      bool received_username;

      /* Request id of asynchronous dns query, if applicable. */
      int adns_id;

      /* Is TRUE when we need are waiting for the hostname before
       * complishing authentification and action list look up. */
      bool delay_establish;

      struct timer *flood_timer;
      double flood_counter;
      int flood_warning_level;

      /* Used for the idlecut setting. It holds the timestamp of
       * the last "non-idle" action by the connection. */
      time_t idle_time;

      /* Used for postponing observer switch until turn change. */
      bool observe_requested;
      player_t *observe_target;
    } server;
  } u;
};

typedef struct connection_s connection_t;

bool parse_conn_pattern(const char *str, char *patbuf,
                        int patbuflen, int *type,
                        char *errbuf, int errbuflen);
struct conn_pattern *conn_pattern_new(const char *pattern,
                                      int type);
void conn_pattern_free(struct conn_pattern *cp);
int conn_pattern_as_str(struct conn_pattern *cp, char *buf, int buflen);
bool conn_pattern_match(struct conn_pattern *cp, struct connection_s *pconn);

#define SPECLIST_TAG ignore
#define SPECLIST_TYPE struct conn_pattern
#include "speclist.h"
#define ignore_list_iterate(alist, ap) \
    TYPED_LIST_ITERATE(struct conn_pattern, alist, ap)
#define ignore_list_iterate_end  LIST_ITERATE_END

const char *cmdlevel_name(enum cmdlevel_id lvl);
enum cmdlevel_id cmdlevel_named(const char *token);


typedef void (*CLOSE_FUN) (connection_t *pcon);
void close_socket_set_callback(CLOSE_FUN fun);
void call_close_socket_callback(connection_t *pcon,
                                enum exit_state state);

int read_socket_data(connection_t *pcon,
                     struct socket_packet_buffer *buffer);
int flush_connection_send_buffer_all(connection_t *pcon);
int send_connection_data(connection_t *pcon, const unsigned char *data,
                         int len);

void connection_do_buffer(connection_t *pcon);
void connection_do_unbuffer(connection_t *pcon);

void connection_list_do_buffer(struct connection_list *dest);
void connection_list_do_unbuffer(struct connection_list *dest);

connection_t *find_conn_by_user(const char *user_name);
connection_t *find_conn_by_user_prefix(const char *user_name,
                                       enum m_pre_result *result);
connection_t *find_conn_by_id(int id);

struct socket_packet_buffer *new_socket_packet_buffer(void);
void connection_common_init(connection_t *pconn);
void connection_common_close(connection_t *pconn);
void free_compression_queue(connection_t *pconn);
void conn_clear_packet_cache(connection_t *pconn);

const char *conn_description(const connection_t *pconn);

int get_next_request_id(int old_request_id);

extern const char blank_addr_str[];

bool conn_controls_player(const connection_t *pconn);
bool conn_is_global_observer(const connection_t *pconn);
player_t *conn_get_player(const connection_t *pconn);
enum cmdlevel_id conn_get_access(const connection_t *pconn);
bool conn_is_valid(const connection_t *pconn);


#endif  /* WC_COMMON_CONNECTION_H */
