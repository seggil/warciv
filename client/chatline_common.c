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
#  include "../config.h"
#endif

#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "astring.h"
#include "log.h"
#include "packets.h"
#include "support.h"

#include "city.h"
#include "map.h"
#include "unit.h"

#include "civclient.h"
#include "clinet.h"
#include "options.h"

#include "include/chatline_g.h"

#include "chatline_common.h"

/* Stored up buffer of lines for the chatline */
struct remaining {
  char *text;
  int conn_id;
};
#define SPECLIST_TAG remaining
#include "speclist.h"
#define remaining_list_iterate(rlist, pline) \
  TYPED_LIST_ITERATE(struct remaining, rlist, pline)
#define remaining_list_iterate_end LIST_ITERATE_END

static struct remaining_list *remains;

/**************************************************************************
  Initialize data structures.
**************************************************************************/
void chatline_common_init(void)
{
  remains = remaining_list_new();
}

/**************************************************************************
  Send the message as a chat to the server.
**************************************************************************/
void send_chat(const char *message)
{
  dsend_packet_chat_msg_req(&aconnection, message);
}

static int frozen_level = 0;

/**************************************************************************
  Turn on buffering, using a counter so that calls may be nested.
**************************************************************************/
void output_window_freeze()
{
  frozen_level++;

  if (frozen_level == 1) {
    assert(remaining_list_size(remains) == 0);
  }
}

/**************************************************************************
  Turn off buffering if internal counter of number of times buffering
  was turned on falls to zero, to handle nested freeze/thaw pairs.
  When counter is zero, append the picked up data.
**************************************************************************/
void output_window_thaw()
{
  frozen_level--;
  if (frozen_level < 0) {
    frozen_level = 0;
  }

  if (frozen_level == 0) {
    remaining_list_iterate(remains, pline) {
      append_output_window_full(pline->text, pline->conn_id);
      free(pline->text);
      free(pline);
    } remaining_list_iterate_end;
    remaining_list_unlink_all(remains);
  }
}

/**************************************************************************
  Turn off buffering and append the picked up data.
**************************************************************************/
void output_window_force_thaw()
{
  if (frozen_level > 0) {
    frozen_level = 1;
    output_window_thaw();
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void log_chat(const char *text)
{
  FILE *f;
  char filepath[MAX_LEN_PATH], datebuf[64], *p;
  const char *host, *name;
  int port;
  time_t now;
  struct tm *nowtm;

  if (!enable_chat_logging) {
    return;
  }

  if (chat_log_directory[0] == '\0') {
    return;
  }

  if (!make_dir(chat_log_directory)) {
    long err_no = myerrno();
    freelog(LOG_ERROR, "Failed to create directory \"%s\": %s",
            chat_log_directory, mystrerror(err_no));
    freelog(LOG_ERROR, "Disabling chat logging on error.");
    enable_chat_logging = FALSE;
    return;
  }

  if (server_host[0] == '\0') {
    host = "NO_SERVER";
  } else {
    host = server_host;
  }

  if (server_port > 0) {
    port = server_port;
  } else {
    port = 0;
  }

  if (default_user_name[0] == '\0') {
    name = "UNNAMED";
  } else {
    name = default_user_name;
  }

  now = time(NULL);
  nowtm = localtime(&now);
  strftime(datebuf, sizeof(datebuf), "%y%m%d", nowtm);

  interpret_tilde(filepath, sizeof(filepath), chat_log_directory);
  cat_snprintf(filepath, sizeof(filepath),
               "%cchatlog-%s-%s-%d-%s.txt",
               '/', datebuf, host, port, name);

  /* Replace annoying characters with underscores. */
  for (p = strrchr(filepath, '/') + 1; *p != '\0'; p++) {
    if (my_isspace(*p) || *p == '\'' || *p == '"' || *p == '('
        || *p == ')' || *p == '*' || *p == '&' || *p == '$'
        || *p == '!' || *p == '#' || *p == ';' || *p == '>'
        || *p == '<' || *p == '/' || *p == '\\' || *p == '{'
        || *p == '}' || *p == '[' || *p == ']' || *p == '`') {
      *p = '_';
    }
  }

  if (!(f = fopen(filepath, "a"))) {
    long err_no = myerrno();
    freelog(LOG_ERROR, "Failed to open \"%s\" for appending: %s",
            filepath, mystrerror(err_no));
    freelog(LOG_ERROR, "Disabling chat logging on error.");
    enable_chat_logging = FALSE;
    return;
  }

  strftime(datebuf, sizeof(datebuf), "[%H:%M:%S] ", nowtm);

  if (fputs(datebuf, f) == EOF
      || fputs(text, f) == EOF
      || fputs("\n", f) == EOF) {
    long err_no = myerrno();
    freelog(LOG_ERROR, "Failed to write to \"%s\": %s",
            filepath, mystrerror(err_no));
    freelog(LOG_ERROR, "Disabling chat logging on error.");
    enable_chat_logging = FALSE;
  }

  fclose(f);
}

/**************************************************************************
  Add a line of text to the output ("chatline") window.
**************************************************************************/
void append_output_window(const char *msg)
{
  append_output_window_full(msg, -1);
}

/**************************************************************************
  Same as above, but here we know the connection id of the sender of the
  text in question.
**************************************************************************/
void append_output_window_full(const char *astring, int conn_id)
{
  if (frozen_level == 0) {
    real_append_output_window(astring, conn_id);
  } else {
    struct remaining *premain = wc_malloc(sizeof(*premain));

    remaining_list_append(remains, premain);
    premain->text = mystrdup(astring);
    premain->conn_id = conn_id;
  }

  log_chat(astring);
}

/**************************************************************************
  ...
**************************************************************************/
static void link_escape_city_name(char *buf, size_t buflen, const char *name)
{
  const char *end = buf + buflen - 1;
  while (buf < end) {
    if (*name == '"' || *name == '\\') {
      *buf++ = '\\';
      if (buf == end) {
        break;
      }
      *buf++ = '"';
      name++;
    } else {
      *buf++ = *name++;
    }
  }
  *buf++ = 0;
}

/**************************************************************************
  NB If you change any of the chat link formats, be sure to change
  the detection code in server/handchat.c as well!
**************************************************************************/
int insert_city_link(char *buf, size_t buflen, city_t *pcity)
{
  char safename[256];

  assert(pcity != NULL);

  link_escape_city_name(safename, sizeof(safename), pcity->common.name);
  return my_snprintf(buf, buflen,
                     CITY_LINK_PREFIX "%d\"%s\"", pcity->common.id, safename);
}

/**************************************************************************
  ...
**************************************************************************/
int insert_tile_link(char *buf, size_t buflen, tile_t *ptile)
{
  assert(ptile != NULL);

  return my_snprintf(buf, buflen, TILE_LINK_PREFIX "%d,%d", TILE_XY(ptile));
}

/**************************************************************************
  ...
**************************************************************************/
int insert_unit_link(char *buf, size_t buflen, struct unit *punit)
{
  assert(punit != NULL);

  return my_snprintf(buf, buflen, UNIT_LINK_PREFIX "%d\"%s\"",
                     punit->id, unit_name_orig(punit->type));
}
