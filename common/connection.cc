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
#include "../config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#ifdef HAVE_SYS_SELECT_H
   /* For some platforms this must be below sys/types.h. */
#  include <sys/select.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifdef WIN32_NATIVE
#  include <winsock2.h>
#endif

#include "wc_intl.h"
#include "game.h"               /* game.all_connections */
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "packets.h"
#include "support.h"            /* mystr(n)casecmp */
#include "timing.h"
#include "wildcards.h"

#include "connection.h"

/* String used for connection.addr and related cases to indicate
 * blank/unknown/not-applicable address:
 */
const char blank_addr_str[] = "---.---.---.---";

/* NB Must match enum conn_pattern_type
   in common/connection.h */
char *conn_pattern_type_strs[NUM_CONN_PATTERN_TYPES] = {
  "address",
  "hostname",
  "username"
};

/**************************************************************************
  Command access levels for client-side use; at present, they are only
  used to control access to server commands typed at the client chatline.
**************************************************************************/
static const char *levelnames[] = {
  "none",
  "observer",
  "basic",
  "ctrl",
  "admin",
  "hack"
};

/**************************************************************************
  Get name associated with given level.  These names are used verbatim in
  commands, so should not be translated here.
**************************************************************************/
const char *cmdlevel_name(enum cmdlevel_id lvl)
{
  assert (lvl >= 0 && lvl < ALLOW_NUM);
  return levelnames[lvl];
}

/**************************************************************************
  Lookup level assocated with token, or ALLOW_UNRECOGNISED if no match.
  Only as many characters as in token need match, so token may be
  abbreviated -- current level names are unique at first character.
  Empty token will match first, i.e. level 'none'.
**************************************************************************/
enum cmdlevel_id cmdlevel_named(const char *token)
{
  enum cmdlevel_id i;
  size_t len = strlen(token);

  for (i = 0; i < ALLOW_NUM; i++) {
    if (strncmp(levelnames[i], token, len) == 0) {
      return i;
    }
  }

  return ALLOW_UNRECOGNIZED;
}


/**************************************************************************
  This callback is used when an error occurs trying to write to the
  connection.  The effect of the callback should be to close the
  connection.  This is here so that the server and client can take
  appropriate (different) actions: server lost a client, client lost
  connection to server.
**************************************************************************/
static CLOSE_FUN close_callback = NULL;

/**************************************************************************
  Register the close_callback:
**************************************************************************/
void close_socket_set_callback(CLOSE_FUN fun)
{
  close_callback = fun;
}

/**************************************************************************
  Call the callback to close the socket.
**************************************************************************/
void call_close_socket_callback(connection_t *pconn,
                                enum exit_state state)
{
  assert(pconn != NULL);
  assert(close_callback != NULL);

  pconn->exit_state = state;
  (*close_callback)(pconn);
}

/**************************************************************************
...
**************************************************************************/
static bool buffer_ensure_free_extra_space(struct socket_packet_buffer *buf,
                                           int extra_space)
{
  if (buf->nsize - buf->ndata >= extra_space) {
    /* There is enough free space already. */
    return TRUE;
  }

  /* Double the capacity each time we need more space to
   * avoid calling realloc too much. */
  while (buf->nsize < buf->ndata + extra_space) {
    buf->nsize *= 2;
  }

  /* Abort if we are taking up too much memory (probably due
   * to a bug somewhere). */
  if (buf->nsize > MAX_LEN_PACKET_BUFFER) {
    return FALSE;
  }

  buf->data = (unsigned char *) wc_realloc(buf->data, buf->nsize);
  return TRUE;
}

/**************************************************************************
  Read data from socket, and check if a packet is ready.
  Returns:
    -1  :  an error occurred
    -2  :  the connection was closed
    >0  :  number of bytes read
    =0  :  non-blocking sockets only; no data read, would block
**************************************************************************/
int read_socket_data(connection_t *pconn,
                     struct socket_packet_buffer *buffer)
{
  int nb;
  long err_no;
  int sock;

  if (pconn == NULL || buffer == NULL || !pconn->used
      || (is_server && pconn->u.server.is_closing) || pconn->sock < 0) {
    /* Hmm, not the best way to "ignore" a "bad" call
     * to this function. In any case, this situation
     * should not happen very often. */
    freelog(LOG_DEBUG, "Tried to read from an invalid connection: %s",
            conn_description(pconn));
    return -1;
  }

  sock = pconn->sock;

  if (!buffer_ensure_free_extra_space(buffer, MAX_LEN_PACKET)) {
    freelog(LOG_ERROR, "Failed to increase size of packet read buffer.");
    return -1;
  }

  freelog(LOG_DEBUG, "read_socket_data: trying to read %d bytes",
          buffer->nsize - buffer->ndata);
  nb = my_readsocket(sock, (char *) (buffer->data + buffer->ndata),
                     buffer->nsize - buffer->ndata);
  err_no = mysocketerrno();
  pconn->error_code = err_no;
  freelog(LOG_DEBUG, "my_readsocket nb=%d: %s",
          nb, mystrsocketerror(err_no));

  if (nb > 0) {
    buffer->ndata += nb;
    return nb;
  }

  if (nb == 0) {
    freelog(LOG_DEBUG, "read_socket_data: peer disconnected");
    return -2;
  }

#ifdef NONBLOCKING_SOCKETS
  if (my_socket_would_block(err_no)) {
    return 0;
  }
#endif

  return -1;
}

/**************************************************************************
  Returns -1 on error and >= 0 otherwise.
  NB: May call call_close_socket_callback on write error.
**************************************************************************/
static int write_socket_data(connection_t *pconn,
                             struct socket_packet_buffer *buf)
{
  int count = 0, nput = 0, nblock = 0, ret = 0;
  long err_no;

  if (pconn == NULL || buf == NULL || buf->ndata <= 0
      || !pconn->used || (is_server && pconn->u.server.is_closing)) {
    return 0;
  }

  while (buf->ndata - count > 0) {
    nblock = MIN(buf->ndata - count, MAX_LEN_PACKET);

    freelog(LOG_DEBUG, "trying to write %d bytes to %s",
            nblock, conn_description(pconn));
    nput = my_writesocket(pconn->sock, buf->data + count, nblock);
    err_no = mysocketerrno();
    pconn->error_code = err_no;
    freelog(LOG_DEBUG, "write returns nput=%d: %s",
            nput, mystrsocketerror(err_no));


    if (nput == -1) {
#ifdef NONBLOCKING_SOCKETS
      if (my_socket_would_block(err_no)) {
        freelog(LOG_DEBUG, "write would block");
        break;
      }
#endif
      ret = -1;
      break;
    }

    if (nput == 0) {
      freelog(LOG_DEBUG, "wrote nothing... trying later");
      /* Try again later. */
      break;
    }

    count += nput;
  }

  if (count > 0) {
    buf->ndata -= count;
    memmove(buf->data, buf->data + count, buf->ndata);

    pconn->write_wait_time = 0.0;
    pconn->statistics.bytes_send += count;

    if (ret != -1) {
      ret = count;
    }
  }

  if (ret == -1) {
    /* I guess we have to close it here. */
    call_close_socket_callback(pconn, EXIT_STATUS_WRITE_ERROR);
  }

  return ret;
}


/**************************************************************************
  NB: May call call_close_socket_callback if write fails.
  Returns -1 on error, or the number of bytes flushed.
**************************************************************************/
int flush_connection_send_buffer_all(connection_t *pconn)
{
  int ret = 0;

  if (pconn == NULL || !pconn->used
      || (is_server && pconn->u.server.is_closing)
      || pconn->send_buffer == NULL || pconn->send_buffer->ndata <= 0)
  {
    return 0;
  }

  freelog(LOG_DEBUG, "flushing data for %s", conn_description(pconn));

  ret = write_socket_data(pconn, pconn->send_buffer);

  if (!is_server
      && pconn->u.client.notify_of_writable_data
      && pconn->send_buffer)
  {
    pconn->u.client.notify_of_writable_data(pconn, pconn->send_buffer->ndata > 0);
  }

  return ret;
}

/**************************************************************************
  NB: May call call_close_socket_callback if buffer space allocation
  fails. In that case FALSE will be returned.
**************************************************************************/
static bool add_connection_data(connection_t *pconn,
                                const unsigned char *data, int len)
{
  struct socket_packet_buffer *buf;

  if (pconn == NULL || !pconn->used
      || (is_server && pconn->u.server.is_closing)
      || pconn->send_buffer == NULL || data == NULL || len <= 0) {
    return TRUE;
  }

  buf = pconn->send_buffer;

  if (!buffer_ensure_free_extra_space(buf, len)) {
    call_close_socket_callback(pconn, EXIT_STATUS_BUFFER_OVERFLOW);
    return FALSE;
  }

  memcpy(buf->data + buf->ndata, data, len);
  buf->ndata += len;

  freelog(LOG_DEBUG, "added %d bytes to buffer for %s",
          len, conn_description(pconn));

  return TRUE;
}

/**************************************************************************
  NB: May call call_close_socket_callback if an error occurs.
**************************************************************************/
int send_connection_data(connection_t *pconn,
                         const unsigned char *data,
                         int len)
{
  if (pconn == NULL || !pconn->used
      || (is_server && pconn->u.server.is_closing)
      || pconn->send_buffer == NULL || data == NULL || len <= 0) {
    return 0;
  }

  if (!add_connection_data(pconn, data, len)) {
    return -1;
  }

  if (pconn->send_buffer->do_buffer_sends > 0) {
    /* Data will be sent on the last call to
     * connection_do_unbuffer. */
    freelog(LOG_DEBUG, "send_connection_data waiting for unbuffer");
    return 0;
  }

  return flush_connection_send_buffer_all(pconn);
}

/**************************************************************************
  Turn on buffering, using a counter so that calls may be nested.
**************************************************************************/
void connection_do_buffer(connection_t *pconn)
{
  if (pconn == NULL || !pconn->used
      || (is_server && pconn->u.server.is_closing)
      || pconn->send_buffer == NULL)
  {
    return;
  }

  pconn->send_buffer->do_buffer_sends++;

  freelog(LOG_DEBUG, "connection_do_buffer @ %d",
          pconn->send_buffer->do_buffer_sends);
}

/**************************************************************************
  Turn off buffering if internal counter of number of times buffering
  was turned on falls to zero, to handle nested buffer/unbuffer pairs.
  When counter is zero, flush any pending data.
**************************************************************************/
void connection_do_unbuffer(connection_t *pconn)
{
  if (pconn == NULL || !pconn->used
      || (is_server && pconn->u.server.is_closing)
      || pconn->send_buffer == NULL)
  {
    return;
  }

  pconn->send_buffer->do_buffer_sends--;

  freelog(LOG_DEBUG, "connection_do_unbuffer @ %d",
          pconn->send_buffer->do_buffer_sends);

  if (pconn->send_buffer->do_buffer_sends < 0) {
    freelog(LOG_ERROR, "connection_do_unbuffer called too many "
            "times on %s!", conn_description(pconn));
    pconn->send_buffer->do_buffer_sends = 0;
  }

  if (pconn->send_buffer->do_buffer_sends == 0) {
    flush_connection_send_buffer_all(pconn);
  }
}

/**************************************************************************
  Convenience functions to buffer/unbuffer a list of connections:
**************************************************************************/
void connection_list_do_buffer(struct connection_list *dest)
{
  if (dest == NULL) {
    return;
  }

  connection_list_iterate(dest, pconn) {
    connection_do_buffer(pconn);
  } connection_list_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void connection_list_do_unbuffer(struct connection_list *dest)
{
  if (dest == NULL) {
    return;
  }

  connection_list_iterate(dest, pconn) {
    connection_do_unbuffer(pconn);
  } connection_list_iterate_end;
}

/***************************************************************
  Find connection by exact user name, from game.all_connections,
  case-insensitve.  Returns NULL if not found.
***************************************************************/
connection_t *find_conn_by_user(const char *user_name)
{
  connection_list_iterate(game.all_connections, pconn) {
    if (mystrcasecmp(user_name, pconn->username)==0) {
      return pconn;
    }
  } connection_list_iterate_end;
  return NULL;
}

/***************************************************************
  Like find_conn_by_username(), but allow unambigous prefix
  (ie abbreviation).
  Returns NULL if could not match, or if ambiguous or other
  problem, and fills *result with characterisation of
  match/non-match (see shared.[ch])
***************************************************************/
static const char *connection_accessor(int i) {
  return connection_list_get(game.all_connections, i)->username;
}

connection_t *find_conn_by_user_prefix(const char *user_name,
                                       enum m_pre_result *result)
{
  int ind;

  *result = match_prefix(connection_accessor,
                         connection_list_size(game.all_connections),
                         MAX_LEN_NAME-1, mystrncasecmp, user_name, &ind);

  if (*result < M_PRE_AMBIGUOUS) {
    return connection_list_get(game.all_connections, ind);
  } else {
    return NULL;
  }
}

/***************************************************************
  Find connection by id, from game.all_connections.
  Returns NULL if not found.
  Number of connections will always be relatively small given
  current implementation, so linear search should be fine.
***************************************************************/
connection_t *find_conn_by_id(int id)
{
  connection_list_iterate(game.all_connections, pconn) {
    if (pconn->id == id) {
      return pconn;
    }
  } connection_list_iterate_end;
  return NULL;
}

/**************************************************************************
  Return malloced struct, appropriately initialized.
**************************************************************************/
struct socket_packet_buffer *new_socket_packet_buffer(void)
{
  struct socket_packet_buffer *buf;

  buf = (struct socket_packet_buffer *)calloc(1, sizeof(struct socket_packet_buffer));
  buf->ndata = 0;
  buf->do_buffer_sends = 0;
  buf->nsize = 10*MAX_LEN_PACKET;
  buf->data = (unsigned char *)calloc(1, 10*MAX_LEN_PACKET);
  return buf;
}

/**************************************************************************
  Free malloced struct
**************************************************************************/
static void free_socket_packet_buffer(struct socket_packet_buffer *buf)
{
  if (buf) {
    if (buf->data) {
      free(buf->data);
    }
    free(buf);
  }
}

/**************************************************************************
  Return pointer to static string containing a description for this
  connection, based on pconn->name, pconn->addr, and (if applicable)
  pconn->player->name.  (Also pconn->established and pconn->observer.)

  Note that if pconn is client's aconnection (connection to server),
  pconn->name and pconn->addr contain empty string, and pconn->player
  is NULL: in this case return string "server".
**************************************************************************/
const char *conn_description(const connection_t *pconn)
{
  static char buffer[MAX_LEN_NAME * 2 + MAX_LEN_ADDR + 128];
  static const char *exit_state_name[] = {
    NULL,
    N_("unknown"),
    N_("decoding error"),
    N_("ping timeout"),
    N_("network exception"),
    N_("buffer overflow"),
    N_("lagging connection"),
    N_("banned"),
    N_("auth failed"),
    N_("connection cut"),
    N_("write error"),
    N_("read error"),
    N_("client disconnected"),
    N_("rejected"),
    N_("kicked"),
    N_("idle cut")
  };

  if (pconn == NULL) {
    return "(NULL)";
  }

  buffer[0] = '\0';

  if (pconn->username[0] != '\0') {
    if (!is_server || !strcmp(pconn->addr, pconn->u.server.ipaddr)) {
      my_snprintf(buffer, sizeof(buffer), _("%s from %s"),
                  pconn->username, pconn->addr);
    } else {
      my_snprintf(buffer, sizeof(buffer), _("%s from %s, ip %s"),
                  pconn->username, pconn->addr, pconn->u.server.ipaddr);
    }
  } else {
    sz_strlcpy(buffer, "server");
  }
  if (exit_state_name[pconn->exit_state] != NULL) {
    if (pconn->exit_state == EXIT_STATUS_READ_ERROR
        || pconn->exit_state == EXIT_STATUS_WRITE_ERROR) {
      cat_snprintf(buffer, sizeof(buffer), " (%s: %s)",
                   _(exit_state_name[pconn->exit_state]),
                   mystrsocketerror(pconn->error_code));
    } else {
      cat_snprintf(buffer, sizeof(buffer), " (%s)",
                   _(exit_state_name[pconn->exit_state]));
    }
  } else if (!pconn->established) {
    sz_strlcat(buffer, _(" (connection incomplete)"));
    return buffer;
  }
  if (pconn->player) {
    if (pconn->observer) {
      cat_snprintf(buffer, sizeof(buffer), _(" (player %s observer)"),
                   pconn->player->name);
    } else {
      cat_snprintf(buffer, sizeof(buffer), _(" (player %s)"),
                   pconn->player->name);
    }
  } else if (pconn->observer) {
    sz_strlcat(buffer, _(" (global observer)"));
  }
  return buffer;
}

/**************************************************************************
  Get next request id. Takes wrapping of the 16 bit wide unsigned int
  into account.
**************************************************************************/
int get_next_request_id(int old_request_id)
{
  int result = old_request_id + 1;

  if ((result & 0xffff) == 0) {
    freelog(LOG_NORMAL,
            "INFORMATION: request_id has wrapped around; "
            "setting from %d to 2", result);
    result = 2;
  }
  assert(result);
  return result;
}

/**************************************************************************
 ...
**************************************************************************/
void free_compression_queue(connection_t *pconn)
{
#ifdef USE_COMPRESSION
  byte_vector_free(&pconn->compression.queue);
#endif
}

/**************************************************************************
 ...
**************************************************************************/
static void init_packet_hashs(connection_t *pconn)
{
  enum packet_type i;

  pconn->phs.sent = wc_malloc(sizeof(*pconn->phs.sent) * PACKET_LAST);
  pconn->phs.received = wc_malloc(sizeof(*pconn->phs.received) * PACKET_LAST);
  pconn->phs.variant = wc_malloc(sizeof(*pconn->phs.variant) * PACKET_LAST);

  for (i = 0; i < PACKET_LAST; i++) {
    pconn->phs.sent[i] = NULL;
    pconn->phs.received[i] = NULL;
    pconn->phs.variant[i] = -1;
  }
}

/**************************************************************************
 ...
**************************************************************************/
static void free_packet_hashes(connection_t *pconn)
{
  int i;

  conn_clear_packet_cache(pconn);

  if (pconn->phs.sent) {
    for (i = 0; i < PACKET_LAST; i++) {
      if (pconn->phs.sent[i] != NULL) {
        hash_free(pconn->phs.sent[i]);
        pconn->phs.sent[i] = NULL;
      }
    }
    free(pconn->phs.sent);
    pconn->phs.sent = NULL;
  }

  if (pconn->phs.received) {
    for (i = 0; i < PACKET_LAST; i++) {
      if (pconn->phs.received[i] != NULL) {
        hash_free(pconn->phs.received[i]);
        pconn->phs.received[i] = NULL;
      }
    }
    free(pconn->phs.received);
    pconn->phs.received = NULL;
  }

  if (pconn->phs.variant) {
    free(pconn->phs.variant);
    pconn->phs.variant = NULL;
  }
}

/**************************************************************************
 ...
**************************************************************************/
void connection_common_init(connection_t *pconn)
{
  pconn->established = FALSE;
  pconn->used = TRUE;
  pconn->write_wait_time = 0.0;
  pconn->buffer = new_socket_packet_buffer();
  pconn->send_buffer = new_socket_packet_buffer();
  pconn->statistics.bytes_send = 0;
  pconn->exit_state = EXIT_STATUS_NONE;
  if (is_server) {
    pconn->u.server.access_level = ALLOW_NONE;
    pconn->u.server.granted_access_level = ALLOW_NONE;
    pconn->u.server.ignore_list = ignore_list_new();
    pconn->u.server.flood_timer = new_timer_start(TIMER_USER, TIMER_ACTIVE);
    pconn->u.server.flood_counter = 0.0;
    pconn->u.server.flood_warning_level = 0;
    pconn->u.server.observe_requested = FALSE;
    pconn->u.server.observe_target = NULL;
  }

  init_packet_hashs(pconn);

#ifdef USE_COMPRESSION
  byte_vector_init(&pconn->compression.queue);
  pconn->compression.frozen_level = 0;
#endif
}

/**************************************************************************
 ...
**************************************************************************/
void connection_common_close(connection_t *pconn)
{
  if (!pconn->used) {
    freelog(LOG_ERROR, "WARNING: Trying to close already closed connection");
    return;
  }

  if (pconn->sock >= 0) {
    my_closesocket(pconn->sock);
    pconn->sock = -1;
  }
  if (is_server && pconn->u.server.adns_id > 0) {
    adns_cancel(pconn->u.server.adns_id);
    pconn->u.server.adns_id = -1;
  }
  pconn->used = FALSE;
  pconn->established = FALSE;

  if (pconn->buffer) {
    free_socket_packet_buffer(pconn->buffer);
    pconn->buffer = NULL;
  }

  if (pconn->send_buffer) {
    free_socket_packet_buffer(pconn->send_buffer);
    pconn->send_buffer = NULL;
  }

  free_compression_queue(pconn);
  free_packet_hashes(pconn);

  if (is_server) {
    ignore_list_iterate(pconn->u.server.ignore_list, cp) {
      conn_pattern_free(cp);
    } ignore_list_iterate_end;
    ignore_list_free(pconn->u.server.ignore_list);
    pconn->u.server.ignore_list = NULL;
  }
}

/**************************************************************************
 Remove all cached packets from the connection. This resets the
 delta-state.
**************************************************************************/
void conn_clear_packet_cache(connection_t *pconn)
{
  int i;

  for (i = 0; i < PACKET_LAST; i++) {
    if (pconn->phs.sent != NULL && pconn->phs.sent[i] != NULL) {
      struct hash_table *hash = pconn->phs.sent[i];
      while (hash_num_entries(hash) > 0) {
        const void *key = hash_key_by_number(hash, 0);
        hash_delete_entry(hash, key);
        free((void *) key);
      }
    }
    if (pconn->phs.received != NULL && pconn->phs.received[i] != NULL) {
      struct hash_table *hash = pconn->phs.received[i];
      while (hash_num_entries(hash) > 0) {
        const void *key = hash_key_by_number(hash, 0);
        hash_delete_entry(hash, key);
        free((void *) key);
      }
    }
  }
}

/**************************************************************************
...
**************************************************************************/
static const char *connpat_accessor(int i)
{
  return conn_pattern_type_strs[i];
}

/**************************************************************************
...
**************************************************************************/
bool parse_conn_pattern(const char *pattern, char *buf,
                        int buflen, int *ptype,
                        char *errbuf, int errbuflen)
{
  char tmp[1024], *p;
  enum m_pre_result res;
  int type;

  if (!pattern) {
    return FALSE;
  }

  sz_strlcpy(tmp, pattern);
  remove_leading_trailing_spaces(tmp);

  if (!tmp[0]) {
    my_snprintf(errbuf, errbuflen, _("The pattern must not be empty"));
    return FALSE;
  }

  type = CPT_HOSTNAME;
  if (ptype && *ptype != NUM_CONN_PATTERN_TYPES) {
    type = *ptype;
  }

  if ((p = strchr(tmp, '='))) {
    *p++ = 0;
    type = NUM_CONN_PATTERN_TYPES;

    res = match_prefix(connpat_accessor, NUM_CONN_PATTERN_TYPES,
                       0, mystrncasecmp, tmp, &type);
    if (res >= M_PRE_AMBIGUOUS || type == NUM_CONN_PATTERN_TYPES) {
      my_snprintf(errbuf, errbuflen, _("Invalid pattern type \"%s\""), tmp);
      return FALSE;
    }
  } else {
    p = tmp;
  }
  if (ptype) {
    *ptype = type;
  }
  mystrlcpy(buf, p, buflen);

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
struct conn_pattern *conn_pattern_new(const char *pattern,
                                      int type)
{
  struct conn_pattern *cp;

  assert(pattern != NULL);

  cp = wc_malloc(sizeof(struct conn_pattern));
  cp->pattern = mystrdup(pattern);
  cp->type = type;

  return cp;
}

/**************************************************************************
  ...
**************************************************************************/
void conn_pattern_free(struct conn_pattern *cp)
{
  if (cp->pattern) {
    free(cp->pattern);
    cp->pattern = NULL;
  }
  free(cp);
}

/**************************************************************************
  ...
**************************************************************************/
int conn_pattern_as_str(struct conn_pattern *cp, char *buf, int buflen)
{
  return my_snprintf(buf, buflen, "%s=%s",
                     conn_pattern_type_strs[cp->type],
                     cp->pattern);
}

/**************************************************************************
  ...
**************************************************************************/
bool conn_pattern_match(struct conn_pattern *cp, connection_t *pconn)
{
  switch (cp->type) {
    case CPT_ADDRESS:
      if ( is_server )
        return wildcardfit(cp->pattern, pconn->u.server.ipaddr);
      else
        return FALSE;
    case CPT_HOSTNAME:
      return wildcardfit(cp->pattern, pconn->addr);
    case CPT_USERNAME:
      return wildcardfit(cp->pattern, pconn->username);
    default:
      break;
  }

  return FALSE;
}

/**************************************************************************
  ...
**************************************************************************/
bool conn_controls_player(const connection_t *pconn)
{
  return pconn && pconn->player && !pconn->observer;
}

/**************************************************************************
  ...
**************************************************************************/
bool conn_is_global_observer(const connection_t *pconn)
{
  return pconn && !pconn->player && pconn->observer;
}

/**************************************************************************
  ...
**************************************************************************/
player_t *conn_get_player(const connection_t *pconn)
{
  if (!pconn) {
    return NULL;
  }
  return pconn->player;
}

/**************************************************************************
  NB: If 'pconn' is NULL, this function will return ALLOW_NONE.
**************************************************************************/
enum cmdlevel_id conn_get_access(const connection_t *pconn)
{
  if (!pconn || !is_server) {
    return ALLOW_NONE; /* Would not want to give hack on error... */
  }
  return pconn->u.server.access_level;
}

/**************************************************************************
  Returns TRUE if the connection is valid, i.e. not NULL, not closed, not
  closing, etc.
**************************************************************************/
bool conn_is_valid(const connection_t *pconn)
{
  return pconn && pconn->used
         && !(is_server && pconn->u.server.is_closing);
}
