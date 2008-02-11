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
#include <string.h>
#include <time.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SELECT_H
/* For some platforms this must be below sys/types.h. */
#include <sys/select.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WIN32_NATIVE
#include <winsock2.h>
#endif

#include "fcintl.h"
#include "game.h"		/* game.all_connections */
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "packets.h"
#include "support.h"		/* mystr(n)casecmp */
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
void call_close_socket_callback(struct connection *pc,
                                enum exit_state state)
{
  assert(pc != NULL);
  assert(close_callback != NULL);

  pc->exit_state = state;
  (*close_callback)(pc);
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

  buf->data = (unsigned char *) fc_realloc(buf->data, buf->nsize);
  return TRUE;
}

/**************************************************************************
  Read data from socket, and check if a packet is ready.
  Returns:
    -1  :  an error occurred OR the connection was closed
    >0  :  number of bytes read
    =0  :  non-blocking sockets only; no data read, would block
**************************************************************************/
int read_socket_data(int sock, struct socket_packet_buffer *buffer)
{
  int nb;

  if (!buffer_ensure_free_extra_space(buffer, MAX_LEN_PACKET)) {
    freelog(LOG_ERROR, "Failed to increase size of packet read buffer.");
    return -1;
  }

  freelog(LOG_DEBUG, "read_socket_data: trying to read %d bytes",
          buffer->nsize - buffer->ndata);
  nb = my_readsocket(sock, (char *) (buffer->data + buffer->ndata),
                     buffer->nsize - buffer->ndata);
  freelog(LOG_DEBUG, "my_readsocket nb=%d: %s",
          nb, mystrsocketerror(mysocketerrno()));

  if (nb > 0) {
    buffer->ndata += nb;
    return nb;
  }

  if (nb == 0) {
    freelog(LOG_DEBUG, "read_socket_data: peer disconnected");
    return -1;
  }

#ifdef NONBLOCKING_SOCKETS
  if (my_socket_would_block()) {
    return 0;
  }
#endif

  return -1;
}

/**************************************************************************
  Returns -1 on error and >= 0 otherwise.
  NB: May call call_close_socket_callback on write error.
**************************************************************************/
static int write_socket_data(struct connection *pc,
                             struct socket_packet_buffer *buf)
{
  int count = 0, nput = 0, nblock = 0, ret = 0;

  if (pc == NULL || buf == NULL || buf->ndata <= 0
      || !pc->used || pc->delayed_disconnect) {
    return 0;
  }

  while (buf->ndata - count > 0) {
    nblock = MIN(buf->ndata - count, MAX_LEN_PACKET);

    freelog(LOG_DEBUG, "trying to write %d bytes to %s",
            nblock, conn_description(pc));
    nput = my_writesocket(pc->sock, buf->data + count, nblock);
    freelog(LOG_DEBUG, "write returns nput=%d: %s",
            nput, mystrsocketerror(mysocketerrno()));


    if (nput == -1) {
#ifdef NONBLOCKING_SOCKETS
      if (my_socket_would_block()) {
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
    pc->last_write = time(NULL);
    if (ret != -1) {
      ret = count;
    }
    pc->statistics.bytes_send += count;
  }

  if (ret == -1) {
    /* I guess we have to close it here. */
    call_close_socket_callback(pc, ES_WRITE_ERROR);
  }

  return ret;
}


/**************************************************************************
  NB: May call call_close_socket_callback if write fails.
  Returns -1 on error, or the number of bytes flushed.
**************************************************************************/
int flush_connection_send_buffer_all(struct connection *pc)
{
  int ret = 0;

  if (pc == NULL || !pc->used || pc->send_buffer == NULL
      || pc->send_buffer->ndata <= 0) {
    return 0;
  }

  freelog(LOG_DEBUG, "flushing data for %s", conn_description(pc));

  ret = write_socket_data(pc, pc->send_buffer);

  if (pc->notify_of_writable_data) {
    pc->notify_of_writable_data(pc, pc->send_buffer->ndata > 0);
  }

  return ret;
}

/**************************************************************************
  NB: May call call_close_socket_callback if buffer space allocation
  fails. In that case FALSE will be returned.
**************************************************************************/
static bool add_connection_data(struct connection *pc,
				const unsigned char *data, int len)
{
  struct socket_packet_buffer *buf;

  if (pc == NULL || !pc->used || pc->send_buffer == NULL) {
    return TRUE;
  }

  buf = pc->send_buffer;

  if (!buffer_ensure_free_extra_space(buf, len)) {
    call_close_socket_callback(pc, ES_BUFFER_OVERFLOW);
    return FALSE;
  }

  memcpy(buf->data + buf->ndata, data, len);
  buf->ndata += len;

  freelog(LOG_DEBUG, "added %d bytes to buffer for %s",
          len, conn_description(pc));

  return TRUE;
}

/**************************************************************************
  NB: May call call_close_socket_callback if an error occurs.
**************************************************************************/
int send_connection_data(struct connection *pc, const unsigned char *data,
			  int len)
{
  if (pc == NULL || !pc->used || pc->delayed_disconnect
      || pc->send_buffer == NULL || data == NULL || len <= 0) {
    return 0;
  }

  if (!add_connection_data(pc, data, len)) {
    return -1;
  }

  if (pc->send_buffer->do_buffer_sends > 0) {
    /* Data will be sent on the last call to
     * connection_do_unbuffer. */
    freelog(LOG_DEBUG, "send_connection_data waiting for unbuffer");
    return 0;
  }

  return flush_connection_send_buffer_all(pc);
}

/**************************************************************************
  Turn on buffering, using a counter so that calls may be nested.
**************************************************************************/
void connection_do_buffer(struct connection *pc)
{
  if (pc == NULL || !pc->used || pc->send_buffer == NULL) {
    return;
  }

  pc->send_buffer->do_buffer_sends++;

  freelog(LOG_DEBUG, "connection_do_buffer @ %d",
          pc->send_buffer->do_buffer_sends);
}

/**************************************************************************
  Turn off buffering if internal counter of number of times buffering
  was turned on falls to zero, to handle nested buffer/unbuffer pairs.
  When counter is zero, flush any pending data.
**************************************************************************/
void connection_do_unbuffer(struct connection *pc)
{
  if (pc == NULL || !pc->used || pc->send_buffer == NULL) {
    return;
  }

  pc->send_buffer->do_buffer_sends--;

  freelog(LOG_DEBUG, "connection_do_unbuffer @ %d",
          pc->send_buffer->do_buffer_sends);

  if (pc->send_buffer->do_buffer_sends < 0) {
    freelog(LOG_ERROR, "connection_do_unbuffer called too many "
            "times on %s!", conn_description(pc));
    pc->send_buffer->do_buffer_sends = 0;
  }

  if (pc->send_buffer->do_buffer_sends == 0) {
    flush_connection_send_buffer_all(pc);
  }
}

/**************************************************************************
  Convenience functions to buffer/unbuffer a list of connections:
**************************************************************************/
void conn_list_do_buffer(struct conn_list *dest)
{
  if (dest == NULL) {
    return;
  }

  conn_list_iterate(dest, pconn) {
    connection_do_buffer(pconn);
  } conn_list_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void conn_list_do_unbuffer(struct conn_list *dest)
{
  if (dest == NULL) {
    return;
  }

  conn_list_iterate(dest, pconn) {
    connection_do_unbuffer(pconn);
  } conn_list_iterate_end;
}

/***************************************************************
  Find connection by exact user name, from game.all_connections,
  case-insensitve.  Returns NULL if not found.
***************************************************************/
struct connection *find_conn_by_user(const char *user_name)
{
  conn_list_iterate(game.all_connections, pconn) {
    if (mystrcasecmp(user_name, pconn->username)==0) {
      return pconn;
    }
  } conn_list_iterate_end;
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
  return conn_list_get(game.all_connections, i)->username;
}

struct connection *find_conn_by_user_prefix(const char *user_name,
                                            enum m_pre_result *result)
{
  int ind;

  *result = match_prefix(connection_accessor,
			 conn_list_size(game.all_connections),
			 MAX_LEN_NAME-1, mystrncasecmp, user_name, &ind);
  
  if (*result < M_PRE_AMBIGUOUS) {
    return conn_list_get(game.all_connections, ind);
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
struct connection *find_conn_by_id(int id)
{
  conn_list_iterate(game.all_connections, pconn) {
    if (pconn->id == id) {
      return pconn;
    }
  } conn_list_iterate_end;
  return NULL;
}

/**************************************************************************
  Return malloced struct, appropriately initialized.
**************************************************************************/
struct socket_packet_buffer *new_socket_packet_buffer(void)
{
  struct socket_packet_buffer *buf;

  buf = (struct socket_packet_buffer *)fc_malloc(sizeof(*buf));
  buf->ndata = 0;
  buf->do_buffer_sends = 0;
  buf->nsize = 10*MAX_LEN_PACKET;
  buf->data = (unsigned char *)fc_malloc(buf->nsize);
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
const char *conn_description(const struct connection *pconn)
{
  static char buffer[MAX_LEN_NAME * 2 + MAX_LEN_ADDR + 128];
  static const char *exit_state_name[] = {
    NULL,
    N_("unknown"),
    N_("deconding error"),
    N_("ping timeout"),
    N_("network exception"),
    N_("buffer overflow"),
    N_("lagging connection"),
    N_("banned"),
    N_("auth failed"),
    N_("cut connection"),
    N_("write error"),
    N_("peer disconnected"),
    N_("rejected")
  };

  if (pconn == NULL) {
    return "(NULL)";
  }

  buffer[0] = '\0';

  if (pconn->username[0] != '\0') {
    if (!strcmp(pconn->addr, pconn->server.ipaddr)) {
      my_snprintf(buffer, sizeof(buffer), _("%s from %s"),
                  pconn->username, pconn->addr); 
    } else {
      my_snprintf(buffer, sizeof(buffer), _("%s from %s, ip %s"),
                  pconn->username, pconn->addr, pconn->server.ipaddr);
    }
  } else {
    sz_strlcpy(buffer, "server");
  }
  if (exit_state_name[pconn->exit_state]) {
    cat_snprintf(buffer, sizeof(buffer),
		 " (%s)", _(exit_state_name[pconn->exit_state]));
  } else if (!pconn->established) {
    sz_strlcat(buffer, _(" (connection incomplete)"));
    return buffer;
  }
  if (pconn->player) {
    cat_snprintf(buffer, sizeof(buffer), _(" (player %s)"),
		 pconn->player->name);
  }
  if (pconn->observer) {
    sz_strlcat(buffer, _(" (observer)"));
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
void free_compression_queue(struct connection *pc)
{
#ifdef USE_COMPRESSION
  byte_vector_free(&pc->compression.queue);
#endif
}

/**************************************************************************
 ...
**************************************************************************/
static void init_packet_hashs(struct connection *pc)
{
  enum packet_type i;

  pc->phs.sent = fc_malloc(sizeof(*pc->phs.sent) * PACKET_LAST);
  pc->phs.received = fc_malloc(sizeof(*pc->phs.received) * PACKET_LAST);
  pc->phs.variant = fc_malloc(sizeof(*pc->phs.variant) * PACKET_LAST);

  for (i = 0; i < PACKET_LAST; i++) {
    pc->phs.sent[i] = NULL;
    pc->phs.received[i] = NULL;
    pc->phs.variant[i] = -1;
  }
}

/**************************************************************************
 ...
**************************************************************************/
static void free_packet_hashes(struct connection *pc)
{
  int i;

  conn_clear_packet_cache(pc);

  if (pc->phs.sent) {
    for (i = 0; i < PACKET_LAST; i++) {
      if (pc->phs.sent[i] != NULL) {
	hash_free(pc->phs.sent[i]);
	pc->phs.sent[i] = NULL;
      }
    }
    free(pc->phs.sent);
    pc->phs.sent = NULL;
  }

  if (pc->phs.received) {
    for (i = 0; i < PACKET_LAST; i++) {
      if (pc->phs.received[i] != NULL) {
	hash_free(pc->phs.received[i]);
	pc->phs.received[i] = NULL;
      }
    }
    free(pc->phs.received);
    pc->phs.received = NULL;
  }

  if (pc->phs.variant) {
    free(pc->phs.variant);
    pc->phs.variant = NULL;
  }
}

/**************************************************************************
 ...
**************************************************************************/
void connection_common_init(struct connection *pconn)
{
  pconn->established = FALSE;
  pconn->used = TRUE;
  pconn->last_write = 0;
  pconn->buffer = new_socket_packet_buffer();
  pconn->send_buffer = new_socket_packet_buffer();
  pconn->statistics.bytes_send = 0;
  pconn->access_level = pconn->granted_access_level = ALLOW_NONE;
  pconn->exit_state = ES_NONE;
  if (is_server) {
    pconn->server.ignore_list = ignore_list_new();
  } else {
    pconn->server.ignore_list = NULL;
  }

  init_packet_hashs(pconn);

#ifdef USE_COMPRESSION
  byte_vector_init(&pconn->compression.queue);
#endif
}

/**************************************************************************
 ...
**************************************************************************/
void connection_common_close(struct connection *pconn)
{
  if (!pconn->used) {
    freelog(LOG_ERROR, "WARNING: Trying to close already closed connection");
    return;
  }

  if (pconn->sock >= 0) {
    my_closesocket(pconn->sock);
    pconn->sock = -1;
  }
  if (is_server && pconn->server.adns_id > 0) {
    adns_cancel(pconn->server.adns_id);
    pconn->server.adns_id = -1;
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

  if (pconn->server.ignore_list) {
    ignore_list_iterate(pconn->server.ignore_list, cp) {
      conn_pattern_free(cp);
    } ignore_list_iterate_end;
    ignore_list_free(pconn->server.ignore_list);
    pconn->server.ignore_list = NULL;
  }
}

/**************************************************************************
 Remove all cached packets from the connection. This resets the
 delta-state.
**************************************************************************/
void conn_clear_packet_cache(struct connection *pc)
{
  int i;

  for (i = 0; i < PACKET_LAST; i++) {
    if (pc->phs.sent != NULL && pc->phs.sent[i] != NULL) {
      struct hash_table *hash = pc->phs.sent[i];
      while (hash_num_entries(hash) > 0) {
	const void *key = hash_key_by_number(hash, 0);
	hash_delete_entry(hash, key);
	free((void *) key);
      }
    }
    if (pc->phs.received != NULL && pc->phs.received[i] != NULL) {
      struct hash_table *hash = pc->phs.received[i];
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
bool parse_conn_pattern(const char *pattern, char *buf,
                        int buflen, int *ptype,
                        char *errbuf, int errbuflen)
{
  char tmp[1024], *p;
  int type, i;

  if (!pattern)
    return FALSE;

  sz_strlcpy(tmp, pattern);
  remove_leading_trailing_spaces(tmp);

  if (!tmp[0]) {
    my_snprintf(errbuf, errbuflen, _("The pattern must not be empty"));
    return FALSE;
  }

  type = CPT_HOSTNAME;
  if (ptype && *ptype != NUM_CONN_PATTERN_TYPES)
    type = *ptype;

  if ((p = strchr(tmp, '='))) {
    *p++ = 0;
    type = NUM_CONN_PATTERN_TYPES;
    for (i = 0; i < NUM_CONN_PATTERN_TYPES; i++) {
      if (!mystrcasecmp(tmp, conn_pattern_type_strs[i])) {
	type = i;
	break;
      }
    }
    if (type == NUM_CONN_PATTERN_TYPES) {
      my_snprintf(errbuf, errbuflen, _("Invalid pattern type \"%s\""), tmp);
      return FALSE;
    }
  } else {
    p = tmp;
  }
  if (ptype)
    *ptype = type;
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

  cp = fc_malloc(sizeof(struct conn_pattern));
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
bool conn_pattern_match(struct conn_pattern *cp, struct connection *pconn)
{
  switch (cp->type) {
    case CPT_ADDRESS:
      return wildcardfit(cp->pattern, pconn->server.ipaddr);
    case CPT_HOSTNAME:
      return wildcardfit(cp->pattern, pconn->addr);
    case CPT_USERNAME:
      return wildcardfit(cp->pattern, pconn->username);
    default:
      break;
  }

  return FALSE;
}
