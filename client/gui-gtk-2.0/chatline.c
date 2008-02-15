/********************************************************************** 
 Freeciv - Copyright(C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or(at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include <gdk/gdkkeysyms.h>

#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "packets.h"
#include "support.h"
#include "registry.h"

#include "civclient.h"
#include "climisc.h"
#include "clinet.h"
#include "control.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "options.h"

#include "chatline.h"
#include "colors.h"
#include "my_cell_renderer_color.h"
#include "pages.h"

genlist *history_list = NULL;
int history_pos;

static struct tag_pattern_list *tagpats = NULL;

struct match_result {
  int start, end;
};

#define COLORS_FORMAT_VERSION 3

#define SPECLIST_TAG match_result
#define SPECLIST_TYPE struct match_result
#include "speclist.h"
#define match_result_list_iterate(alist, pitem)\
  TYPED_LIST_ITERATE(struct match_result, alist, pitem)
#define match_result_list_iterate_end  LIST_ITERATE_END

enum {
  COLUMN_TAG_NAME,
  COLUMN_PATTERN,
  COLUMN_FOREGROUND,
  COLUMN_BACKGROUND,
  COLUMN_AT_START,
  COLUMN_NEGATE,
  COLUMN_PREVIOUS,
  COLUMN_APPLY,
  COLUMN_STOP,
  COLUMN_IGNORE,
  COLUMN_INACTIVE,
  COLUMN_SWITCH_NEXT,
  COLUMN_JUMP_TARGET,

  NUM_COLUMNS
};

static GtkWidget *chatline_config_shell = NULL;
static GtkWidget *help_dialog = NULL;

static void textbuf_insert_time(GtkTextBuffer *buf, GtkTextIter *iter);
static bool match_tag_pattern(struct tag_pattern *ptagpat,
                               const char *buf, 
                               struct match_result_list *matches);
static void match_result_list_free_data(struct match_result_list *matches);
static void create_default_message_buffer_tag_patterns(void);
static void create_default_tag_patterns(struct tag_pattern_list *tpl);
static void put_tag_pattern_into_store(GtkListStore *store,
                                        GtkTreeIter *iter,
                                        struct tag_pattern *ptagpat);

static gboolean hovering_over_link = FALSE;
static GdkCursor *hand_cursor = NULL;
static GdkCursor *regular_cursor = NULL;

/**************************************************************************
  ...
**************************************************************************/
static void link_escape_city_name(char *buf, int buflen, const char *name)
{
  const char *end = buf + buflen - 1;
  while (buf < end) {
    if (*name == '"' || *name == '\\') {
      *buf++ = '\\';
      if (buf == end)
        break;
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
static int insert_city_name_and_id_link(char *buf, int buflen,
                                        struct tile *ptile)
{
  char safename[256];
  int nb;
  
  assert(ptile != NULL);
  assert(ptile->city != NULL);

  link_escape_city_name(safename, sizeof(safename), ptile->city->name);
  nb = my_snprintf(buf, buflen, "@F%d\"%s\"", ptile->city->id, safename);
  return nb;
}

/**************************************************************************
  ...
**************************************************************************/
static int insert_location_link(char *buf, int buflen, struct tile *ptile)
{
  assert(ptile != NULL);

  return my_snprintf(buf, buflen, "@L%d,%d", ptile->x, ptile->y);
}

/**************************************************************************
  ...
**************************************************************************/
static int insert_unit_link(char *buf, int buflen, struct unit *punit)
{
  assert(punit != NULL);

  return my_snprintf(buf, buflen, "@U%d\"%s\"",
                     punit->id, unit_name_orig(punit->type));
}

/**************************************************************************
  ...
**************************************************************************/
void insert_chat_link(struct tile *ptile, bool unit)
{
  char buf[256];

  if (unit) {
    struct unit *punit = find_visible_unit(ptile);
    if (!punit) {
      append_output_window(_("Warclient: No visible unit on this tile."));
      return;
    }
    insert_unit_link(buf, sizeof(buf), punit);
  } else if (ptile->city) {
    insert_city_name_and_id_link(buf, sizeof(buf), ptile);
  } else {
    insert_location_link(buf, sizeof(buf), ptile);
  }
  chatline_entry_append_text(buf);
  gtk_widget_grab_focus(inputline);
}

/**************************************************************************
...
**************************************************************************/
static bool is_public_message(const char *s)
{
  const char *p;
  
  if (s[0] == '/' || s[0] == '.') {
    return FALSE;
  }
  for (p = s[0] == '\'' || s[0] == '"' ? strchr(s + 1, s[0]) : s;
       p && *p != '\0'; p++) {
    if (my_isspace(*p)) {
      return TRUE;
    } else if (*p == ':') {
      return FALSE;
    }
  }
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
void inputline_return(GtkEntry *w, gpointer data)
{
  const char *theinput;

  theinput = gtk_entry_get_text(w);

  if (*theinput) {
    if (allied_chat_only && is_public_message(theinput)) {
      char buf[1024];
      my_snprintf(buf, sizeof(buf), ". %s", theinput);
      send_chat(buf);
    } else {
      send_chat(theinput);
    }

    assert(history_list != NULL);

    if (genlist_size(history_list) >= MAX_CHATLINE_HISTORY) {
      void *data;

      data = genlist_get(history_list, -1);
      genlist_unlink(history_list, data);
      free(data);
    }

    genlist_insert(history_list, mystrdup(theinput), 0);
    history_pos = -1;
  }

  gtk_entry_set_text(w, "");
}

/**************************************************************************
  ...
**************************************************************************/
static bool tag_pattern_is_control_only(struct tag_pattern *ptagpat)
{
    return ptagpat->flags & TPF_IS_CONTROL_ONLY;
}
/**************************************************************************
  ...
**************************************************************************/
static void
follow_if_link(GtkWidget   *text_view, 
                GtkTextIter *iter)
{
  GSList *tags = NULL, *tagp = NULL;
  gpointer data;
  gint link_type;
  int x, y, id;
  char buf[128];
  struct city *pcity;
  struct tile *ptile;
  struct unit *punit;

  tags = gtk_text_iter_get_tags(iter);
  for (tagp = tags;  tagp != NULL;  tagp = tagp->next) {
    GtkTextTag *tag = tagp->data;
    data = g_object_get_data(G_OBJECT(tag), "link_type");
    if (!data)
      continue;

    link_type = GPOINTER_TO_INT(data);

    id = -1;
    ptile = NULL;

    switch (link_type) {
    case LINK_CITY:
    case LINK_CITY_ID:
    case LINK_CITY_ID_AND_NAME:
      data = g_object_get_data(G_OBJECT(tag), "city_id");
      id = GPOINTER_TO_INT(data);
      pcity = find_city_by_id(id);
      if (!pcity) {
        my_snprintf(buf, sizeof(buf), _("Warclient: %d is not the ID "
            "of any city I know about :("), id);
        append_output_window(buf);
        continue;
      }
      ptile = pcity->tile;
      break;

    case LINK_LOCATION: 
      x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tag), "x"));
      y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tag), "y"));
      ptile = map_pos_to_tile(x, y);
      if (!ptile) {
        my_snprintf(buf, sizeof(buf), _("Warclient:(%d, %d) is not a valid "
            "location on this map!"), x, y);
        append_output_window(buf);
        continue;
      }
      id = ptile->index;
      break;

    case LINK_UNIT:
      data = g_object_get_data(G_OBJECT(tag), "unit_id");
      id = GPOINTER_TO_INT(data);
      punit = find_unit_by_id(id);
      if (!punit) {
        my_snprintf(buf, sizeof(buf), _("Warclient: %d is not the ID "
            "of any unit I know about :("), id);
        append_output_window(buf);
        continue;
      }
      ptile = punit->tile;
      break;

    default:
      break;
    }

    assert(id >= 0 && ptile != NULL);

    restore_link_mark(link_type, id);
    center_tile_mapcanvas(ptile);
    gtk_widget_grab_focus(GTK_WIDGET(map_canvas));
  }

  if (tags) 
    g_slist_free(tags);
}
/**************************************************************************
  ...
**************************************************************************/
static gboolean
event_after(GtkWidget *text_view,
             GdkEvent  *ev)
{
  GtkTextIter start, end, iter;
  GtkTextBuffer *buffer;
  GdkEventButton *event;
  gint x, y;

  if (ev->type != GDK_BUTTON_RELEASE)
    return FALSE;

  event = (GdkEventButton *) ev;

  if (event->button != 1)
    return FALSE;

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

  /* we shouldn't follow a link if the user has selected something */
  gtk_text_buffer_get_selection_bounds(buffer, &start, &end);
  if (gtk_text_iter_get_offset(&start)
      != gtk_text_iter_get_offset(&end)) {
    return FALSE;
  }

  gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), 
                                         GTK_TEXT_WINDOW_WIDGET,
                                         event->x, event->y, &x, &y);

  gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(text_view),
                                      &iter, x, y);

  follow_if_link(text_view, &iter);

  return FALSE;
}

/**************************************************************************
  ...
**************************************************************************/
static void
set_cursor_if_appropriate(GtkTextView    *text_view,
                           gint            x,
                           gint            y)
{
  GSList *tags = NULL, *tagp = NULL;
  GtkTextBuffer *buffer;
  GtkTextIter iter;
  gboolean hovering = FALSE;
  gint link_type;
  gpointer *data;
  GdkDisplay *display;
  GdkScreen *screen;

  screen = gdk_screen_get_default();
  display = gdk_screen_get_display(screen);

  if (!hand_cursor) {
    hand_cursor = gdk_cursor_new_for_display(display, GDK_HAND2);
  }
  if (!regular_cursor) {
    regular_cursor = gdk_cursor_new_for_display(display, GDK_XTERM);
  }
  
  buffer = gtk_text_view_get_buffer(text_view);

  gtk_text_view_get_iter_at_location(text_view, &iter, x, y);
  
  tags = gtk_text_iter_get_tags(&iter);
  for (tagp = tags;  tagp != NULL;  tagp = tagp->next) {
    GtkTextTag *tag = tagp->data;
    data = g_object_get_data(G_OBJECT(tag), "link_type");
    link_type = GPOINTER_TO_INT(data);
    
    if (data != 0) {
      hovering = TRUE;
      break;
    }
  }

  if (hovering != hovering_over_link) {
    hovering_over_link = hovering;

    if (hovering_over_link) {
      gdk_window_set_cursor(gtk_text_view_get_window(text_view,
          GTK_TEXT_WINDOW_TEXT), hand_cursor);
    } else {
      gdk_window_set_cursor(gtk_text_view_get_window(text_view,
          GTK_TEXT_WINDOW_TEXT), regular_cursor);
    }
  }

  if (tags) 
    g_slist_free(tags);
}
/**************************************************************************
  ...
**************************************************************************/
static gboolean
motion_notify_event(GtkWidget      *text_view,
                     GdkEventMotion *event)
{
  gint x, y;

  gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), 
                                         GTK_TEXT_WINDOW_WIDGET,
                                         event->x, event->y, &x, &y);

  set_cursor_if_appropriate(GTK_TEXT_VIEW(text_view), x, y);

  gdk_window_get_pointer(text_view->window, NULL, NULL, NULL);
  return FALSE;
}
/**************************************************************************
  ...
**************************************************************************/
static gboolean
visibility_notify_event(GtkWidget          *text_view,
                         GdkEventVisibility *event)
{
  gint wx, wy, bx, by;
  
  gdk_window_get_pointer(text_view->window, &wx, &wy, NULL);
  
  gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), 
                                         GTK_TEXT_WINDOW_WIDGET,
                                         wx, wy, &bx, &by);

  set_cursor_if_appropriate(GTK_TEXT_VIEW(text_view), bx, by);

  return FALSE;
}
/**************************************************************************
  ...
**************************************************************************/
void set_message_buffer_view_link_handlers(GtkTextView *view)
{
  g_signal_connect(view, "event-after",
                    G_CALLBACK(event_after), NULL);
  g_signal_connect(view, "motion-notify-event",
                    G_CALLBACK(motion_notify_event), NULL);
  g_signal_connect(view, "visibility-notify-event",
                    G_CALLBACK(visibility_notify_event), NULL);

}

/**************************************************************************
  ...
**************************************************************************/
static int parse_city_id_and_name_link(const char *str, 
                                        GtkTextBuffer *buf,
                                        GtkTextTag **tag,
                                        char *newtext,
                                        int newtext_maxlen)
{
  const char *p = str;
  char *q, idbuf[32];
  int id;
  struct city *pcity;
  
  if (*p != '@' || p[1] != 'F')
    return 0;
  p += 2;
  q = idbuf;
  while (*p && my_isdigit(*p)) {
    if (q >= idbuf + sizeof(idbuf))
      return 0;
    *q++ = *p++;
  }
  *q = 0;

  if (*p != '"')
    return 0;
  while (*p) {
    if (*p == '\\') {
      p++;
      if (!*p)
        break;
    }
    p++;
    if (*p == '"')
      break;
  }

  if (*p++ != '"')
    return 0;

  id = atoi(idbuf);
  if (id < 0)
    return 0;

  pcity = find_city_by_id(id);
  if (!pcity)
    return 0;
  
  *tag = gtk_text_buffer_create_tag(buf, NULL,
                                     "foreground", "green",
                                     "underline", PANGO_UNDERLINE_SINGLE,
                                     NULL);
  g_object_set_data(G_OBJECT(*tag), "link_type",
                     GINT_TO_POINTER(LINK_CITY));
  g_object_set_data(G_OBJECT(*tag), "city_id",
                     GINT_TO_POINTER(id)); 

  add_link_mark(LINK_CITY, id);

  my_snprintf(newtext, newtext_maxlen, "%s", pcity->name);
  
  return(int)(p - str);
}
/**************************************************************************
  ...
**************************************************************************/
static int parse_city_id_link(const char *str, 
                               GtkTextBuffer *buf,
                               GtkTextTag **tag,
                               char *newtext,
                               int newtext_maxlen)
{
  const char *p = str;
  char *q, idbuf[32];
  int id;
  struct city *pcity;
  
  if (*p != '@' || p[1] != 'I')
    return 0;
  p += 2;
  q = idbuf;
  while (*p && my_isdigit(*p)) {
    if (q >= idbuf + sizeof(idbuf))
      return 0;
    *q++ = *p++;
  }
  *q = 0;

  id = atoi(idbuf);
  if (id < 0)
    return 0;

  pcity = find_city_by_id(id);
  if (!pcity)
    return 0;
  
  *tag = gtk_text_buffer_create_tag(buf, NULL,
                                     "foreground", "green",
                                     "underline", PANGO_UNDERLINE_SINGLE,
                                     NULL);
  g_object_set_data(G_OBJECT(*tag), "link_type",
                     GINT_TO_POINTER(LINK_CITY));
  g_object_set_data(G_OBJECT(*tag), "city_id",
                     GINT_TO_POINTER(id)); 

  add_link_mark(LINK_CITY, id);

  my_snprintf(newtext, newtext_maxlen, "%s", pcity->name);
  
  return(int)(p - str);
}
/**************************************************************************
  ...
**************************************************************************/
static int parse_city_link(const char *str, 
                            GtkTextBuffer *buf,
                            GtkTextTag **tag,
                            char *newtext,
                            int newtext_maxlen)
{
  const char *p = str;
  char city_name[MAX_LEN_NAME], *q;
  struct city *pcity;
  
  if (*p != '@' || p[1] != 'C' || p[2] != '"')
    return 0;
  p += 3;
  q = city_name;
  while (*p && *p != '"') {
    if (q >= city_name + sizeof(city_name))
      return 0;
    *q++ = *p++;
  }
  p++;
  *q = 0;

  if (!(pcity = find_city_by_name_fast(city_name)))
    return 0;
  
  *tag = gtk_text_buffer_create_tag(buf, NULL,
                                     "foreground", "green",
                                     "underline", PANGO_UNDERLINE_SINGLE,
                                     NULL);
  g_object_set_data(G_OBJECT(*tag), "link_type",
                     GINT_TO_POINTER(LINK_CITY));
  g_object_set_data(G_OBJECT(*tag), "city_id",
                     GINT_TO_POINTER(pcity->id)); 

  add_link_mark(LINK_CITY, pcity->id);

  my_snprintf(newtext, newtext_maxlen, "%s", city_name);
  
  return(int)(p - str);
}
/**************************************************************************
  ...
**************************************************************************/
static int parse_location_link(const char *str,
                                GtkTextBuffer *buf,
                                GtkTextTag **tag,
                                char *newtext,
                                int newtext_maxlen)
{
  const char *p;
  int x, y;
  
  if (2 != sscanf(str, "@L%d,%d", &x, &y))
    return 0;
    
  p = str + 2;
  while (my_isdigit(*p) || *p == ',')
    p++;
  
  *tag = gtk_text_buffer_create_tag(buf, NULL,
                                     "foreground", "red",
                                     "underline", PANGO_UNDERLINE_SINGLE,
                                     NULL);
  g_object_set_data(G_OBJECT(*tag), "link_type",
                     GINT_TO_POINTER(LINK_LOCATION));
  g_object_set_data(G_OBJECT(*tag), "x", GINT_TO_POINTER(x)); 
  g_object_set_data(G_OBJECT(*tag), "y", GINT_TO_POINTER(y));

  my_snprintf(newtext, newtext_maxlen, "(%d, %d)", x, y);

  add_link_mark(LINK_LOCATION, map_pos_to_index(x, y));

  return(int)(p - str);
}
/**************************************************************************
  ...
**************************************************************************/
static int parse_unit_link(const char *str, 
                            GtkTextBuffer *buf,
                            GtkTextTag **tag,
                            char *newtext,
                            int newtext_maxlen)
{
  const char *p = str;
  char *q, idbuf[32];
  int id;
  struct unit *punit;

  if (*p != '@' || p[1] != 'U')
    return 0;
  p += 2;
  q = idbuf;
  while (*p && my_isdigit(*p)) {
    if (q >= idbuf + sizeof(idbuf))
      return 0;
    *q++ = *p++;
  }
  *q = 0;

  if (*p != '"')
    return 0;
  while (*p) {
    if (*p == '\\') {
      p++;
      if (!*p)
        break;
    }
    p++;
    if (*p == '"')
      break;
  }

  if (*p++ != '"')
    return 0;

  id = atoi(idbuf);
  if (id < 0)
    return 0;

  punit = find_unit_by_id(id);
  if (!punit)
    return 0;

  *tag = gtk_text_buffer_create_tag(buf, NULL,
                                     "foreground", "blue",
                                     "underline", PANGO_UNDERLINE_SINGLE,
                                     NULL);
  g_object_set_data(G_OBJECT(*tag), "link_type",
                     GINT_TO_POINTER(LINK_UNIT));
  g_object_set_data(G_OBJECT(*tag), "unit_id",
                     GINT_TO_POINTER(id)); 

  add_link_mark(LINK_UNIT, id);

  my_snprintf(newtext, newtext_maxlen, "%s", unit_name(punit->type));
  
  return(int)(p - str);
}

/**************************************************************************
  NB If you change any of the chat link formats, be sure to change
  the detection code in server/handchat.c as well!
**************************************************************************/
static void append_text_with_links(GtkTextBuffer *buf,
                                    const char *astring)
{
  const char *p, *q, *s;
  char newtext[256];
  int n;
  GtkTextIter iter;
  GtkTextTag *tag = NULL;

  gtk_text_buffer_get_end_iter(buf, &iter);

  for (s = p = astring; *p &&(q = strchr(p, '@')); p = q) {
    switch (q[1]) {
    case 'L':
      n = parse_location_link(q, buf, &tag, newtext, sizeof(newtext));
      break;
    case 'C':
      n = parse_city_link(q, buf, &tag, newtext, sizeof(newtext));
      break;
    case 'I':
      n = parse_city_id_link(q, buf, &tag, newtext, sizeof(newtext));
      break;
    case 'F':
      n = parse_city_id_and_name_link(q, buf, &tag, newtext, sizeof(newtext));
      break;
    case 'U':
      n = parse_unit_link(q, buf, &tag, newtext, sizeof(newtext));
      break;
    default:
      n = 0;
      break;
    }

    if (n > 0) {
      if (s < q)
        gtk_text_buffer_insert(buf, &iter, s,(int)(q - s));
      gtk_text_buffer_insert_with_tags(buf, &iter, newtext, -1, tag, NULL);
      q += n;
      s = q;
    } else {
      q++;
    }
  } 
  
  if (*s)
    gtk_text_buffer_insert(buf, &iter, s, -1);
}

/**************************************************************************
  ...
**************************************************************************/
static void scroll_if_necessary(GtkTextView *w,
                                GtkTextMark *scroll_target)
{
  GtkWidget *sw;
  GtkAdjustment *vadj;
  gdouble val, max, upper, page_size;

  sw = gtk_widget_get_parent(GTK_WIDGET(w));
  vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(sw));
  val = gtk_adjustment_get_value(GTK_ADJUSTMENT(vadj));
  g_object_get(G_OBJECT(vadj), "upper", &upper,
               "page-size", &page_size, NULL);
  max = upper - page_size;
  if (max - val < 10.0) {
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(w), scroll_target,
                                 0.0, TRUE, 1.0, 0.0);
  }
}

/**************************************************************************
  Appends the string to the chat output window.  The string should be
  inserted on its own line, although it will have no newline.
**************************************************************************/
void real_append_output_window(const char *astring, int conn_id)
{
  bool previous_matched = FALSE, switch_next = FALSE, do_stop = FALSE;
  char *jump_target = "";
  struct match_result_list *matches;
  const char *text;
  int text_start_offset, offset;

  GtkTextBuffer *buf;
  GtkTextIter iter, start, end;
  GtkTextMark *insert_start, *text_start, *scroll_target;

  buf = message_buffer;
  gtk_text_buffer_get_end_iter(buf, &iter);
  insert_start = gtk_text_buffer_create_mark(buf, NULL, &iter, TRUE);
  gtk_text_buffer_insert(buf, &iter, "\n", -1);
  scroll_target = gtk_text_buffer_create_mark(buf, NULL, &iter, TRUE);
  textbuf_insert_time(buf, &iter);

  text_start = gtk_text_buffer_create_mark(buf, NULL, &iter, TRUE);
  if (get_client_state() == CLIENT_GAME_RUNNING_STATE
      || get_client_state() == CLIENT_GAME_OVER_STATE)
  {
    append_text_with_links(buf, astring);
  } else {
    gtk_text_buffer_insert(buf, &iter, astring, -1);
  }

  matches = match_result_list_new();

  gtk_text_buffer_get_iter_at_mark(buf, &start, text_start);
  gtk_text_buffer_get_end_iter(buf, &end);
  text_start_offset = gtk_text_iter_get_offset(&start);
  text = gtk_text_buffer_get_slice(buf, &start, &end, FALSE);

  tag_pattern_list_iterate(tagpats, ptagpat) {

    if (jump_target[0]) {
      if (!ptagpat->tag_name[0]
          || 0 != strcmp(ptagpat->tag_name, jump_target)) {
        continue;
      }
      jump_target = "";
    }

    if (switch_next) {
      if (ptagpat->flags & TPF_INACTIVE) {
        ptagpat->flags &= ~TPF_INACTIVE;
      } else {
        ptagpat->flags |= TPF_INACTIVE;
      }
      switch_next = FALSE;
    }
    if (do_stop) {
      break;
    }

    if (ptagpat->flags & TPF_INACTIVE) {
      continue;
    }

    if (ptagpat->flags & TPF_REQUIRE_PREVIOUS_MATCH && !previous_matched) {
      continue;
    }
    
    if (!match_tag_pattern(ptagpat, text, matches)) {
      previous_matched = FALSE;
      continue;
  }

    previous_matched = TRUE;

    if (ptagpat->flags & TPF_SWITCH_NEXT) {
      switch_next = TRUE;
    } else {
      switch_next = FALSE;
    }

    if (ptagpat->jump_target[0]) {
      jump_target = ptagpat->jump_target;
    }

    if (ptagpat->flags & TPF_STOP_IF_MATCHED) {
      do_stop = TRUE;
    }

    if (ptagpat->flags & TPF_IGNORE_IF_MATCHED) {
      gtk_text_buffer_get_iter_at_mark(buf, &start, insert_start);
      gtk_text_buffer_get_end_iter(buf, &end);
      gtk_text_buffer_delete(buf, &start, &end);
      do_stop = TRUE;
    } else if (!tag_pattern_is_control_only(ptagpat)) {
      match_result_list_iterate(matches, pmres) {
        offset = text_start_offset + pmres->start;
        gtk_text_buffer_get_iter_at_offset(buf, &start, offset);
        offset = text_start_offset + pmres->end;
        gtk_text_buffer_get_iter_at_offset(buf, &end, offset);
        gtk_text_buffer_apply_tag_by_name(buf, ptagpat->tag_name,
                                          &start, &end);
      } match_result_list_iterate_end;
    }
    match_result_list_free_data(matches);

  } tag_pattern_list_iterate_end;
  match_result_list_free(matches);

  if (main_message_area) {
    scroll_if_necessary(GTK_TEXT_VIEW(main_message_area), scroll_target);
  }
  if (start_message_area) {
    scroll_if_necessary(GTK_TEXT_VIEW(start_message_area), scroll_target);
  }

  gtk_text_buffer_delete_mark(buf, insert_start);
  gtk_text_buffer_delete_mark(buf, scroll_target);
  gtk_text_buffer_delete_mark(buf, text_start);
}

/**************************************************************************
 I have no idea what module this belongs in -- Syela
 I've decided to put output_window routines in chatline.c, because
 the are somewhat related and append_output_window is already here.  --dwp
**************************************************************************/
void log_output_window(void)
{
  GtkTextIter start, end;
  gchar *txt;

  gtk_text_buffer_get_bounds(message_buffer, &start, &end);
  txt = gtk_text_buffer_get_text(message_buffer, &start, &end, TRUE);

  write_chatline_content(txt);
  g_free(txt);
}

/**************************************************************************
...
**************************************************************************/
void clear_output_window(void)
{
  set_output_window_text(_("Cleared output window."));
}

/**************************************************************************
...
**************************************************************************/
void set_output_window_text(const char *text)
{
  gtk_text_buffer_set_text(message_buffer, text, -1);
}
/**************************************************************************
  ...
**************************************************************************/
struct tag_pattern *tag_pattern_new(const char *name, const char *pattern,
                                     int flags, const char *jump_target,
                                     const char *foreground_color,
                                     const char *background_color)
{
  struct tag_pattern *ptagpat;

  assert(name != NULL);
  assert(pattern != NULL);
  assert(jump_target != NULL);
  
  ptagpat = fc_malloc(sizeof(struct tag_pattern));

  ptagpat->tag_name = mystrdup(name);
  ptagpat->pattern = mystrdup(pattern);
  ptagpat->flags = flags;
  ptagpat->jump_target = mystrdup(jump_target);

  ptagpat->foreground_color = color_from_str(foreground_color);
  ptagpat->background_color = color_from_str(background_color);

  return ptagpat;
}
/**************************************************************************
...
**************************************************************************/
void tag_pattern_free(struct tag_pattern *ptagpat)
{
  free(ptagpat->tag_name);
  free(ptagpat->pattern);
  free(ptagpat->jump_target);
  if (ptagpat->foreground_color) {
    gdk_color_free(ptagpat->foreground_color);
  }
  if (ptagpat->background_color) {
    gdk_color_free(ptagpat->background_color);
  }
  free(ptagpat);
}

/**************************************************************************
...
**************************************************************************/
void init_message_buffer_tag_patterns(void)
{
  if (!tagpats) {
    tagpats = tag_pattern_list_new();
  }
}

/**************************************************************************
...
**************************************************************************/
static void create_default_message_buffer_tag_patterns(void)
{
  init_message_buffer_tag_patterns();
  free_message_buffer_tag_patterns();
  
  create_default_tag_patterns(tagpats);
}

/**************************************************************************
...
**************************************************************************/
static void create_default_tag_patterns(struct tag_pattern_list *tpl)
{
  struct tag_pattern *ptagpat;
  char buf[256];

#define MK_TAG_PATTERN(name, pat, flags, jump, fg, bg) do{\
    ptagpat = tag_pattern_new(name, pat, flags, jump, fg, bg);\
    tag_pattern_list_append(tpl, ptagpat);\
  } while (0)


  MK_TAG_PATTERN("game start", "All players are ready",
                  TPF_MATCH_AT_START | TPF_STOP_IF_MATCHED,
                  "", "#00FF00", "#115511");
  MK_TAG_PATTERN("", "Game: All players are ready",
                  TPF_MATCH_AT_START | TPF_STOP_IF_MATCHED,
                  "", "#00FF00", "#115511");

  MK_TAG_PATTERN("server prompt", "(server prompt):",
                  TPF_MATCH_AT_START | TPF_STOP_IF_MATCHED,
                  "", "#FF0000", "#BEBEBE");
  MK_TAG_PATTERN("game message", "Game:",
                  TPF_MATCH_AT_START | TPF_STOP_IF_MATCHED,
                  "", "#8B0000", "");
  MK_TAG_PATTERN("server message", "Server:",
                  TPF_MATCH_AT_START | TPF_STOP_IF_MATCHED,
                  "", "#8B0000", "");
  MK_TAG_PATTERN("option message", "Option:",
                  TPF_MATCH_AT_START | TPF_STOP_IF_MATCHED,
                  "", "#8B0000", "");
  MK_TAG_PATTERN("Warclient message", "Warclient:",
                 TPF_MATCH_AT_START | TPF_STOP_IF_MATCHED,
                 "", "#0000FF", "");
  MK_TAG_PATTERN("PepClient message", "PepClient:",
                 TPF_MATCH_AT_START | TPF_STOP_IF_MATCHED,
                 "", "#0000FF", "");

  MK_TAG_PATTERN("commands", "/", TPF_MATCH_AT_START, "", "#006400", "");
  

  MK_TAG_PATTERN("chat message", "<", TPF_MATCH_AT_START, "", "#00008B", "");
  MK_TAG_PATTERN("private message", "*", TPF_MATCH_AT_START,
                  "", "#A020F0", "");
  MK_TAG_PATTERN("private message 2", "[", TPF_MATCH_AT_START,
                  "", "#A020F0", "");
  MK_TAG_PATTERN("private message sent", "->*", TPF_MATCH_AT_START,
                  "", "#A020F0", "");
  MK_TAG_PATTERN("private message sent 2", "->[", TPF_MATCH_AT_START,
                 "", "#A020F0", "");

  MK_TAG_PATTERN("", "<", TPF_IS_CONTROL_ONLY | TPF_MATCH_AT_START
                  | TPF_NEGATE, "", "", "");
  MK_TAG_PATTERN("ally message", "to allies: ",
                  TPF_REQUIRE_PREVIOUS_MATCH,
                  "", "#551166", "");

  MK_TAG_PATTERN("player emote", "^ ", TPF_MATCH_AT_START, "", "#006D6E", "");
  MK_TAG_PATTERN("server emote", "+ ", TPF_MATCH_AT_START, "", "#8C0015", "");
  

  MK_TAG_PATTERN("", "/show: ", TPF_IS_CONTROL_ONLY
                  | TPF_MATCH_AT_START | TPF_NEGATE,
                  "end of show stuff", "", "");
  MK_TAG_PATTERN("", "/show: All",
                  TPF_IS_CONTROL_ONLY | TPF_MATCH_AT_START
                  | TPF_STOP_IF_MATCHED, "", "", "");
  MK_TAG_PATTERN("", "/show: Vital",
                  TPF_IS_CONTROL_ONLY | TPF_MATCH_AT_START
                  | TPF_STOP_IF_MATCHED, "", "", "");
  MK_TAG_PATTERN("", "/show: Rare",
                  TPF_IS_CONTROL_ONLY | TPF_MATCH_AT_START
                  | TPF_STOP_IF_MATCHED, "", "", "");
  MK_TAG_PATTERN("", "/show: +",
                  TPF_IS_CONTROL_ONLY | TPF_MATCH_AT_START
                  | TPF_STOP_IF_MATCHED, "", "", "");
  MK_TAG_PATTERN("", "/show: Try",
                  TPF_IS_CONTROL_ONLY | TPF_MATCH_AT_START
                  | TPF_STOP_IF_MATCHED, "", "", "");
  MK_TAG_PATTERN("", "/show: Option",
                  TPF_IS_CONTROL_ONLY | TPF_MATCH_AT_START
                  | TPF_STOP_IF_MATCHED, "", "", "");
  MK_TAG_PATTERN("", "/show: ------------",
                  TPF_IS_CONTROL_ONLY | TPF_MATCH_AT_START
                  | TPF_STOP_IF_MATCHED, "", "", "");
  
  MK_TAG_PATTERN("", "/show", TPF_IS_CONTROL_ONLY
                  | TPF_MATCH_AT_START, "", "", "");
  MK_TAG_PATTERN("changed options", "=",
                  TPF_NEGATE | TPF_REQUIRE_PREVIOUS_MATCH
                  | TPF_STOP_IF_MATCHED, "", "#FF0000", "");
  MK_TAG_PATTERN("end of show stuff", "", TPF_IS_CONTROL_ONLY,
                  "", "", "");


  MK_TAG_PATTERN("new vote", "New vote",
                  TPF_MATCH_AT_START | TPF_STOP_IF_MATCHED, 
                  "", "#FFFFFF", "#AA0000");

  MK_TAG_PATTERN("new poll", "New poll ",
                 TPF_MATCH_AT_START | TPF_STOP_IF_MATCHED,
                 "", "#000000", "#FFF34D");

  MK_TAG_PATTERN("", "Vote ", TPF_IS_CONTROL_ONLY | TPF_NEGATE
                  | TPF_MATCH_AT_START, "vote stuff end", "", "");
  MK_TAG_PATTERN("vote passed", " is passed ", TPF_STOP_IF_MATCHED,
                  "", "#006400", "#AAFFAA");
  MK_TAG_PATTERN("vote failed", " failed ", TPF_STOP_IF_MATCHED,
                  "", "#8B0000", "#FFAAAA");
  MK_TAG_PATTERN("voted yes", "voted yes", TPF_STOP_IF_MATCHED,
                  "", "#000000", "#C8FFD5");
  MK_TAG_PATTERN("voted no", "voted no", TPF_STOP_IF_MATCHED,
                  "", "#000000", "#FFD2D2");
  MK_TAG_PATTERN("abstained", "chose to abstain", TPF_STOP_IF_MATCHED,
                  "", "#000000", "#E8E8E8");
  MK_TAG_PATTERN("vote stuff end", "", TPF_IS_CONTROL_ONLY, "", "", "");


  MK_TAG_PATTERN("", "/list:", TPF_IS_CONTROL_ONLY 
                  | TPF_MATCH_AT_START, "", "", "");
  MK_TAG_PATTERN("not ready", "not ready",
                  TPF_REQUIRE_PREVIOUS_MATCH | TPF_APPLY_TO_MATCH,
                  "", "#FF0000", "");

  MK_TAG_PATTERN("", "/list:", TPF_IS_CONTROL_ONLY
                  | TPF_MATCH_AT_START, "", "", "");
  MK_TAG_PATTERN("no nation", ", Human)", TPF_REQUIRE_PREVIOUS_MATCH,
                  "", "#FF0000", "");
  MK_TAG_PATTERN("", "/list:", TPF_IS_CONTROL_ONLY
                  | TPF_MATCH_AT_START, "", "", "");
  MK_TAG_PATTERN("", ", Human, team ",
                  TPF_REQUIRE_PREVIOUS_MATCH | TPF_IS_CONTROL_ONLY,
                  "", "", "");
  MK_TAG_PATTERN("no nation", " ready)",
                  TPF_REQUIRE_PREVIOUS_MATCH | TPF_NEGATE,
                  "", "#FF0000", "");
  

  my_snprintf(buf, sizeof(buf), "<%s>", default_user_name);
  MK_TAG_PATTERN("", buf, TPF_IS_CONTROL_ONLY 
                  | TPF_STOP_IF_MATCHED | TPF_MATCH_AT_START, "", "", "");
  my_snprintf(buf, sizeof(buf), "<(%s)>", default_user_name);
  MK_TAG_PATTERN("", buf, TPF_IS_CONTROL_ONLY 
                  | TPF_STOP_IF_MATCHED | TPF_MATCH_AT_START, "", "", "");
  my_snprintf(buf, sizeof(buf), "<[%s]>", default_user_name);
  MK_TAG_PATTERN("", buf, TPF_IS_CONTROL_ONLY 
                  | TPF_STOP_IF_MATCHED | TPF_MATCH_AT_START, "", "", "");
  my_snprintf(buf, sizeof(buf), "%s to allies:", default_user_name);
  MK_TAG_PATTERN("", buf, TPF_IS_CONTROL_ONLY 
                  | TPF_STOP_IF_MATCHED | TPF_MATCH_AT_START, "", "", "");
  my_snprintf(buf, sizeof(buf), "(%s) to allies:", default_user_name);
  MK_TAG_PATTERN("", buf, TPF_IS_CONTROL_ONLY 
                  | TPF_STOP_IF_MATCHED | TPF_MATCH_AT_START, "", "", "");

  MK_TAG_PATTERN("", "<", TPF_IS_CONTROL_ONLY 
                  | TPF_MATCH_AT_START, "name hilight", "", "");
  MK_TAG_PATTERN("", "*", TPF_IS_CONTROL_ONLY 
                  | TPF_MATCH_AT_START, "name hilight", "", "");
  MK_TAG_PATTERN("", "[", TPF_IS_CONTROL_ONLY
                  | TPF_MATCH_AT_START, "name hilight", "", "");
  MK_TAG_PATTERN("", " to allies:", TPF_IS_CONTROL_ONLY,
                  "name hilight", "", "");
  MK_TAG_PATTERN("", "", TPF_IS_CONTROL_ONLY 
                  | TPF_STOP_IF_MATCHED, "", "", "");
  MK_TAG_PATTERN("name hilight", default_user_name,
                  TPF_APPLY_TO_MATCH | TPF_REQUIRE_PREVIOUS_MATCH,
                  "", "#000000", "#FFFF00");
}
/**************************************************************************
...
**************************************************************************/
static void textbuf_insert_time(GtkTextBuffer *buf, GtkTextIter *iter)
{
  if (chat_time_format[0] != '\0') {
    char timestr[128];
    time_t now;
    struct tm *nowtm;

    now = time(NULL);
    nowtm = localtime(&now);
    strftime(timestr, sizeof(timestr), chat_time_format, nowtm);

    gtk_text_buffer_insert(buf, iter, timestr, -1);
    gtk_text_buffer_insert(buf, iter, " ", -1);
  }
}
/**************************************************************************
...
**************************************************************************/
static bool match_tag_pattern(struct tag_pattern *ptagpat,
                               const char *text,
                               struct match_result_list *matches)
{
  bool res = FALSE;
  struct match_result *pmres = NULL;
  int textlen = strlen(text), patlen = strlen(ptagpat->pattern);
  const char *curpos, *p;

  if (!ptagpat->pattern[0]) {
    res = !(ptagpat->flags & TPF_NEGATE);
    if (res) {
      pmres = fc_malloc(sizeof(struct match_result));
      pmres->start = 0;
      pmres->end = textlen;
      match_result_list_append(matches, pmres);
    }
    return res;
  }
  
  for (curpos = text;; curpos = p + patlen) {
    if (!(p = strstr(curpos, ptagpat->pattern)))
      break;
    if ((ptagpat->flags & TPF_MATCH_AT_START) && p != text)
      break;
    res = TRUE;

    pmres = fc_malloc(sizeof(struct match_result));
    match_result_list_append(matches, pmres);
    if (ptagpat->flags & TPF_APPLY_TO_MATCH) {
      pmres->start = p - text;
      pmres->end = pmres->start + patlen;
    } else {
      pmres->start = 0;
      pmres->end = textlen;
      break;
    }

    if (ptagpat->flags & TPF_MATCH_AT_START)
      break;
  }

  res = (ptagpat->flags & TPF_NEGATE) ? !res : res;
  if (res && match_result_list_size(matches) == 0) {
    pmres = fc_malloc(sizeof(struct match_result));
    pmres->start = 0;
    pmres->end = textlen;
    match_result_list_append(matches, pmres);
  }
  return res;
}

/**************************************************************************
...
**************************************************************************/
static void remove_tag_from_tag_table_func(gpointer key, gpointer value,
                                            gpointer user_data)
{
  GtkTextTag *tag;
  char *name = (char *) key;
  GtkTextTagTable *table = (GtkTextTagTable *) user_data;

  if ((tag = gtk_text_tag_table_lookup(table, name))) {
    gtk_text_tag_table_remove(table, tag);
  }
}
/**************************************************************************
...
**************************************************************************/
void refresh_message_buffer_tag_patterns(GHashTable *tags_to_delete)
{
  GtkTextTagTable *table;
  GtkTextTag *tag;

  init_message_buffer_tag_patterns();

  if (!GTK_TEXT_BUFFER(message_buffer))
    return;
  
  table = gtk_text_buffer_get_tag_table(message_buffer);

  if (tags_to_delete) {
    g_hash_table_foreach(tags_to_delete,
                         remove_tag_from_tag_table_func,
                         table);
  }

  
  tag_pattern_list_iterate(tagpats, ptagpat) {
    if (tag_pattern_is_control_only(ptagpat)) {
      continue;
    }
    tag = gtk_text_tag_table_lookup(table, ptagpat->tag_name);
    if (!tag) {
      tag = gtk_text_tag_new(ptagpat->tag_name);
      gtk_text_tag_table_add(table, tag);
    }
    
    if (ptagpat->foreground_color) {
      g_object_set(G_OBJECT(tag),
                    "foreground-gdk", ptagpat->foreground_color,
                    "foreground-set", TRUE,
                    NULL);
    } else {
      g_object_set(G_OBJECT(tag), "foreground-set",
                    FALSE, NULL);
    }
    if (ptagpat->background_color) {
      g_object_set(G_OBJECT(tag),
                    "background-gdk", ptagpat->background_color,
                    "background-set", TRUE,
                    NULL);
    } else {
      g_object_set(G_OBJECT(tag), "background-set",
                    FALSE, NULL);
    }
    
  } tag_pattern_list_iterate_end;
}
/**************************************************************************
...
**************************************************************************/
void free_message_buffer_tag_patterns(void)
{
  tag_pattern_list_iterate(tagpats, ptagpat) {
    tag_pattern_free(ptagpat);
  } tag_pattern_list_iterate_end;

  tag_pattern_list_unlink_all(tagpats);  
}
/**************************************************************************
  Note: does not actually free the passed in list itself, just all its
  data.
**************************************************************************/
static void match_result_list_free_data(struct match_result_list *matches)
{
  if (!matches) {
    return;
  }

  match_result_list_iterate(matches, pmres) {
    free(pmres);
  } match_result_list_iterate_end;
  match_result_list_unlink_all(matches);
}
/**************************************************************************
  ...
**************************************************************************/
void secfile_save_chatline_colors(struct section_file *sf)
{
  int version = COLORS_FORMAT_VERSION;
  
  secfile_insert_int(sf, version, "chatline.colors_version");
  secfile_save_message_buffer_tag_patterns(sf);
}
/**************************************************************************
  ...
**************************************************************************/
void secfile_load_chatline_colors(struct section_file *sf)
{
  int version;

  version = secfile_lookup_int_default(sf, -1, "chatline.colors_version");
  if (version != COLORS_FORMAT_VERSION) {
    freelog(LOG_ERROR,
        _("Chatline tag pattern format has changed, settings in file "
          "\"%s\" will not be loaded(please save the new settings)."),
        sf->filename);
    create_default_message_buffer_tag_patterns();
  } else {
    secfile_load_message_buffer_tag_patterns(sf);
  }
}
/**************************************************************************
  Result(if not NULL) must be freed(with gdk_color_free)  when no longer
  needed.
**************************************************************************/
static GdkColor *secfile_lookup_color(struct section_file *file,
                                       const char *fmt, ...)
{
  va_list ap;
  char buf[1024];
  int red, green, blue;
  GdkColor color;

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  red = secfile_lookup_int(file, "%s_r", buf);
  green = secfile_lookup_int(file, "%s_g", buf);
  blue = secfile_lookup_int(file, "%s_b", buf);
  if (red == -1 || green == -1 || blue == -1)
    return NULL;
  
  color.red = red;
  color.green = green;
  color.blue = blue;
  return gdk_color_copy(&color);
}
/**************************************************************************
  ...
**************************************************************************/
static void secfile_insert_color(struct section_file *file, GdkColor *color,
                                  const char *fmt, ...)
{
  va_list ap;
  char buf[1024];

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  if (!color) {
    secfile_insert_int(file, -1, "%s_r", buf);
    secfile_insert_int(file, -1, "%s_g", buf);
    secfile_insert_int(file, -1, "%s_b", buf);
    return;
  }
  
  secfile_insert_int(file, color->red, "%s_r", buf);
  secfile_insert_int(file, color->green, "%s_g", buf);
  secfile_insert_int(file, color->blue, "%s_b", buf);
}
/**************************************************************************
  ...
**************************************************************************/
void secfile_save_message_buffer_tag_patterns(struct section_file *file)
{
  int i = 0;
  secfile_insert_int_comment(file, tag_pattern_list_size(tagpats),
                             _("Do not attempt to edit the tag patterns"
                               " here by hand, unless you really know"
                               " what you're doing"),
                             "chatline.num_tagpats");
  tag_pattern_list_iterate(tagpats, ptagpat) {
    secfile_insert_str(file, ptagpat->tag_name,
                       "chatline.tagpat%d.tag_name", i);
    secfile_insert_str(file, ptagpat->pattern,
                       "chatline.tagpat%d.pattern", i);
    secfile_insert_int(file, ptagpat->flags,
                       "chatline.tagpat%d.flags", i);
    secfile_insert_str(file, ptagpat->jump_target,
                       "chatline.tagpat%d.jump_target", i);
    
    secfile_insert_color(file, ptagpat->foreground_color,
                         "chatline.tagpat%d.foreground_color", i);
    secfile_insert_color(file, ptagpat->background_color,
                         "chatline.tagpat%d.background_color", i);
    i++;
  } tag_pattern_list_iterate_end;
}
/**************************************************************************
  ...
**************************************************************************/
void secfile_load_message_buffer_tag_patterns(struct section_file *file)
{
  int num, i;
  struct tag_pattern *ptagpat;

  init_message_buffer_tag_patterns();
  
  num = secfile_lookup_int_default(file, -1, "chatline.num_tagpats");
  if (num == -1) {
    create_default_message_buffer_tag_patterns();
    return;
  }
  free_message_buffer_tag_patterns();
  
  for (i = 0; i < num; i++) {
    ptagpat = fc_malloc(sizeof(struct tag_pattern));
    ptagpat->tag_name = mystrdup(
        secfile_lookup_str(file, "chatline.tagpat%d.tag_name", i));
    ptagpat->pattern = mystrdup(
        secfile_lookup_str(file, "chatline.tagpat%d.pattern", i));
    ptagpat->flags = 
        secfile_lookup_int(file, "chatline.tagpat%d.flags", i);
    ptagpat->jump_target = mystrdup(
        secfile_lookup_str(file, "chatline.tagpat%d.jump_target", i));

    ptagpat->foreground_color = secfile_lookup_color(file,
        "chatline.tagpat%d.foreground_color", i);
    ptagpat->background_color = secfile_lookup_color(file,
        "chatline.tagpat%d.background_color", i);

    tag_pattern_list_append(tagpats, ptagpat);
  }
}

/**************************************************************************
  Free everything but the list itself.
**************************************************************************/
void tag_pattern_list_free_data(struct tag_pattern_list *tpl)
{
  tag_pattern_list_iterate(tpl, ptagpat) {
    tag_pattern_free(ptagpat);
  } tag_pattern_list_iterate_end; 
  tag_pattern_list_unlink_all(tpl);
}

/**************************************************************************
  ...
**************************************************************************/
static void save_callback(GtkWidget *w,
                           gpointer user_data)
{
  save_options();
}
/**************************************************************************
  ...
**************************************************************************/
static GtkWidget *create_help_dialog(GtkWidget *parent)
{
  GtkWidget *dialog, *text, *sw, *vbox, *button, *hbox;
  GtkTextBuffer *buf;
  GtkTextIter iter;

  /* create the window */
  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(dialog),
                        _("Chatline Color Configuration Help"));
  gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 500);
  gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

  gtk_widget_set_name(dialog, "Freeciv");
  setup_dialog(dialog, toplevel);

  g_signal_connect_swapped(dialog, "destroy",
      G_CALLBACK(gtk_widget_hide), dialog);

  vbox = gtk_vbox_new(FALSE, 8);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(vbox),
                      sw, TRUE, TRUE, 0);

  buf = gtk_text_buffer_new(NULL);
  text = gtk_text_view_new_with_buffer(buf);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_container_add(GTK_CONTAINER(sw), text);

  gtk_text_buffer_get_end_iter(buf, &iter);
  gtk_text_buffer_insert(buf, &iter, _(
      "Overview\n"
      "\n"
      "   Every line of text sent to the chat window is matched against"
      " each pattern line, starting from the top and moving down. The"
      " name of the pattern is the first field and must be unique if"
      " that pattern has color fields(i.e. on a match it applies the"
      " its foreground and background color values to the text). Other-"
      " wise the name may be blank and that pattern may only be used for"
      " control flow(see FLAGS section below). The pattern field is the"
      " the text to match, no wildcards and case sensitive. If it is"
      " blank, it matches everything.\n"
      "   Editing the patterns will not change the chat window text, to"
      " activate the new patterns after you are done editing, press the"
      " Apply button.\n"
      "\n"
      "\n"
      "Controls\n"
      "\n"
      "   To add a new named pattern click the \"Add\" button with an appropriate"
      " name in the input box. The \"Add Control Pattern\" button is used to"
      " add a pattern that will only be ised for control flow(it may or may"
      " have a name, and it cannot have colors assigned to it)."
      " To remove a pattern select it(by clicking on"
      " it) and press the \"Delete\" button(multiple patterns may be deleted"
      " this way at once."
      " To move patterns around use the \"Move Up\" and \"Move Down\""
      " buttons. The \"Save\" button will save the current patterns in your"
      " .civclientrc file. \"Reset\" will restore the"
      " default patterns. Finally, the \"Clear\" button removes all patterns.\n"
      "\n"
      "\n"
      "Flags\n"
      "\n"
      "  Flags allow you to alter the behavior described in the overview to apply"
      "colors in a more controlled fasion.\n"
      "\n"
      "  BEGIN - The pattern only matches at the beginning of the line,"
      " i.e. just after the time string(if any).\n"
      "  NOT - Negates the result of the pattern, i.e. the pattern"
      " accepts any line that it does not match.\n"
      "  PREV - Only matches if the pattern just above it matched(or the pattern"
      " jumped from).\n"
      "  MULTI - Match as many times as possible."
      " Colors are only applied to the matched text,"
      " not the entire line.\n"
      "  STOP - If this pattern matches, do not continue with any patterns"
      " below this one.\n"
      "  IGNORE - If this pattern matches, ignore the line of text completely."
      " The line is not printed in the chat window at all.\n"
      "  OFF - Skip this pattern(used in conjunction with SWITCH flag).\n"
      "  SWITCH - If the pattern matches toggle the OFF flag for the next"
      " pattern.\n"
      "  JUMP - Not really a flag, but if it is non empty and the pattern"
      " matches, skips down in the list of patterns until it reaches the"
      " pattern whose name is equal to the given name.\n"
      ), -1);


  button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  g_signal_connect_swapped(button, "clicked",
      G_CALLBACK(gtk_widget_hide), dialog);
  gtk_widget_set_size_request(button, 100, 30);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  
  
  gtk_widget_show_all(dialog);
   
  return dialog;
}
/**************************************************************************
  ...
**************************************************************************/
static void help_callback(GtkWidget *w, gpointer user_data)
{
  GtkWidget *dialog;

  dialog = (GtkWidget *) user_data;
  if (help_dialog == NULL)
    help_dialog = create_help_dialog(dialog);

  gtk_window_present(GTK_WINDOW(help_dialog));
}
/**************************************************************************
  ...
**************************************************************************/
static void reset_callback(GtkWidget *w, gpointer user_data)
{
  GtkWidget *dialog = (GtkWidget *) user_data;
  struct tag_pattern_list *tmptagpats = NULL;
  GtkTreeModel *model;
  GtkTreeIter iter;
  
  tmptagpats = g_object_get_data(G_OBJECT(dialog), "tmptagpats");
  tag_pattern_list_free_data(tmptagpats);
  create_default_tag_patterns(tmptagpats);

  model = g_object_get_data(G_OBJECT(dialog), "model");
  gtk_list_store_clear(GTK_LIST_STORE(model));
  
  tag_pattern_list_iterate(tmptagpats, ptagpat) {
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    put_tag_pattern_into_store(GTK_LIST_STORE(model),
                                &iter, ptagpat);
  } tag_pattern_list_iterate_end;
}
/**************************************************************************
  ...
**************************************************************************/
static void apply_callback(GtkWidget *w,
                            gpointer user_data)
{
  GtkWidget *dialog = (GtkWidget *) user_data;
  struct tag_pattern_list *tmptagpats = NULL;
  GHashTable *tags_to_delete = NULL;
  gpointer key, value;

    
  tags_to_delete = g_object_get_data(G_OBJECT(dialog), "tags_to_delete");
  tmptagpats = g_object_get_data(G_OBJECT(dialog), "tmptagpats");
  
  tag_pattern_list_free_data(tagpats);
  tag_pattern_list_iterate(tmptagpats, ptagpat) {
    if (g_hash_table_lookup_extended(tags_to_delete, ptagpat->tag_name,
                                      &key, &value)) {
      /* key is freed by the hash table */
      g_hash_table_remove(tags_to_delete, key);
    }
    tag_pattern_list_append(tagpats, tag_pattern_clone(ptagpat));
  } tag_pattern_list_iterate_end; 

  refresh_message_buffer_tag_patterns(tags_to_delete);
    
  /* free memory used by hash table(also frees any leftover keys),
     i.e. we clear the hash table here */
  g_hash_table_destroy(tags_to_delete);
  tags_to_delete = g_hash_table_new_full(g_str_hash, g_str_equal,
                                         free, NULL);
  g_object_set_data(G_OBJECT(dialog), "tags_to_delete", tags_to_delete);
}

/**************************************************************************
...
**************************************************************************/
static void cancel_callback(GtkWidget *w,
                            gpointer user_data)
{
  GtkWidget *dialog = (GtkWidget *) user_data;
  gtk_widget_destroy(dialog);
}
/**************************************************************************
...
**************************************************************************/
static void ok_callback(GtkWidget *w,
                            gpointer user_data)
{
  GtkWidget *dialog = (GtkWidget *) user_data;

  apply_callback(NULL, dialog);
  gtk_widget_destroy(dialog);
}
/**************************************************************************
...
**************************************************************************/
static void destroy_callback(GtkWidget *w,
                              gpointer user_data)
{
  struct tag_pattern_list *tmptagpats;
  GHashTable *tags_to_delete;
  
  tmptagpats = g_object_get_data(G_OBJECT(w), "tmptagpats");
  if (tmptagpats) {
    tag_pattern_list_free_all(tmptagpats);
  }
  
  tags_to_delete = g_object_get_data(G_OBJECT(w), "tags_to_delete");
  if (tags_to_delete) {
    g_hash_table_destroy(tags_to_delete);
  }
  chatline_config_shell = NULL;  

  if (help_dialog != NULL) {
    gtk_widget_hide(help_dialog);
  }
}
/**************************************************************************
  ...
**************************************************************************/
static void toggle_pattern_flag(int flag, GtkTreeModel *model,
                                 gchar *path_str, int column)
{
  GtkTreeIter  iter;
  GtkTreePath *path;
  gboolean flagval;
  struct tag_pattern_list *tmptagpats;
  struct tag_pattern *ptagpat;
  int i;

  tmptagpats = g_object_get_data(G_OBJECT(chatline_config_shell),
                                  "tmptagpats");
  
  path = gtk_tree_path_new_from_string(path_str);
  gtk_tree_model_get_iter(model, &iter, path);
  gtk_tree_model_get(model, &iter, column, &flagval, -1);

  
  i = gtk_tree_path_get_indices(path)[0];
  ptagpat = tag_pattern_list_get(tmptagpats, i);

  flagval ^= 1;
  ptagpat->flags = flagval ? ptagpat->flags | flag
                           : ptagpat->flags &(~flag);

  gtk_tree_model_get_iter(model, &iter, path);
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, column, flagval, -1);
  gtk_tree_path_free(path);
}
/**************************************************************************
  ...
**************************************************************************/
static void flag_toggled(GtkCellRendererToggle *cell,
                          gchar                 *path_str,
                          gpointer               data)
{
  GtkTreeModel *model;
  int col_idx = GPOINTER_TO_INT(data);
  int flag = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "flag"));
  model = g_object_get_data(G_OBJECT(cell), "model");
  toggle_pattern_flag(flag, model,
                       path_str, col_idx);
}
/**************************************************************************
  ...
**************************************************************************/
static void notify_user_error(GtkWidget *dialog, char *msg)
{
  GtkWidget *msglabel;
  char buf[1024];

  msglabel = g_object_get_data(G_OBJECT(dialog), "msglabel");
  assert(msglabel != NULL);
  
  my_snprintf(buf, sizeof(buf), "<span foreground=\"red\">"
               "%s</span>", msg);
  gtk_label_set_markup(GTK_LABEL(msglabel), buf);
}
/**************************************************************************
  ...
**************************************************************************/
static void cell_edited(GtkCellRendererText *cell,
                         const gchar         *path_string,
                         const gchar         *new_text,
                         gpointer             data)
{
  GtkTreeModel *model = (GtkTreeModel *) data;
  GtkTreePath *path = gtk_tree_path_new_from_string(path_string);
  GtkTreeIter iter;
  int i;
  struct tag_pattern *ptagpat;
  gpointer ptr;
  struct tag_pattern_list *tmptagpats;

  gint column = GPOINTER_TO_INT(
      g_object_get_data(G_OBJECT(cell), "column"));
  gtk_tree_model_get_iter(model, &iter, path);
  
  i = gtk_tree_path_get_indices(path)[0];
  ptr = g_object_get_data(G_OBJECT(chatline_config_shell),
                           "tmptagpats");
  tmptagpats = (struct tag_pattern_list *) ptr;
  ptagpat = tag_pattern_list_get(tmptagpats, i);
  
  switch (column) {
  case COLUMN_PATTERN:
    free(ptagpat->pattern);
    ptagpat->pattern = mystrdup(new_text);
    break;
  case COLUMN_JUMP_TARGET:
    free(ptagpat->jump_target);
    ptagpat->jump_target = mystrdup(new_text);
    break;
  default:
    assert(0); /* must never happend */
  }
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, column,
                      new_text, -1);
  gtk_tree_path_free(path);
}
/**************************************************************************
  ...
**************************************************************************/
static GtkWidget *create_my_color_selection_dialog(const char *title)
{
  GtkWidget *dialog, *colorsel;
  
  dialog = gtk_dialog_new_with_buttons(title, NULL, 0,
                                        "_No Color", GTK_RESPONSE_REJECT,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        "_OK", GTK_RESPONSE_OK,
                                        NULL);
  colorsel = gtk_color_selection_new();
  gtk_color_selection_set_has_opacity_control(
      GTK_COLOR_SELECTION(colorsel), FALSE);
  g_object_set_data(G_OBJECT(dialog), "colorsel", colorsel);
  gtk_color_selection_set_has_palette(
      GTK_COLOR_SELECTION(colorsel), FALSE);
  
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),
                     colorsel);
  gtk_widget_show(colorsel);
  
  return dialog;
}
/**************************************************************************
  ...
**************************************************************************/
static void add_columns(GtkTreeView *treeview,
                         GtkTreeModel *model)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

#define MK_STR_COLUMN(col_idx, title, editable, resizable, minwidth) do {\
  renderer = gtk_cell_renderer_text_new();\
  g_object_set(renderer, "editable", editable, NULL);\
  if (editable)\
    g_signal_connect(renderer, "edited",\
                      G_CALLBACK(cell_edited), model);\
  g_object_set_data(G_OBJECT(renderer), "column",\
                     GINT_TO_POINTER(col_idx));\
  column = gtk_tree_view_column_new_with_attributes(title, renderer,\
                                                     "text", col_idx,\
                                                     NULL);\
  gtk_tree_view_column_set_resizable(column, resizable);\
  gtk_tree_view_column_set_min_width(column, minwidth);\
  gtk_tree_view_append_column(treeview, column);\
} while (0)

#define MK_FLAG_COLUMN(col_idx, title, flag) do {\
  renderer = gtk_cell_renderer_toggle_new();\
  g_object_set_data(G_OBJECT(renderer), "model", model);\
  g_signal_connect(renderer, "toggled",\
                    G_CALLBACK(flag_toggled),\
                    GINT_TO_POINTER(col_idx));\
  g_object_set_data(G_OBJECT(renderer), "flag", GINT_TO_POINTER(flag));\
  column = gtk_tree_view_column_new_with_attributes(title, renderer,\
                                                     "active", col_idx,\
                                                     NULL);\
  gtk_tree_view_append_column(treeview, column);\
} while (0)

  MK_STR_COLUMN(COLUMN_TAG_NAME, "Name", FALSE, TRUE, 70);
  MK_STR_COLUMN(COLUMN_PATTERN, "Pattern", TRUE, TRUE, 70);
  

  /* Use custom renderer for foreground color */
  renderer = my_cell_renderer_color_new();
  g_object_set_data(G_OBJECT(renderer), "column",
                     GINT_TO_POINTER(COLUMN_FOREGROUND));
  column = gtk_tree_view_column_new_with_attributes("Foreground",
                                                     renderer,
                                                     "color",
                                                     COLUMN_FOREGROUND,
                                                     NULL);
  g_object_set(G_OBJECT(renderer), "xsize", 24, "ysize", 16, NULL);
  gtk_tree_view_column_set_resizable(column, FALSE);
  gtk_tree_view_column_set_min_width(column, 40);
  gtk_tree_view_append_column(treeview, column);
  
  
  /* Use custom renderer for foreground color */
  renderer = my_cell_renderer_color_new();
  g_object_set_data(G_OBJECT(renderer), "column",
                     GINT_TO_POINTER(COLUMN_BACKGROUND));
  column = gtk_tree_view_column_new_with_attributes("Background",
                                                     renderer,
                                                     "color",
                                                     COLUMN_BACKGROUND,
                                                     NULL);
  g_object_set(G_OBJECT(renderer), "xsize", 24, "ysize", 16, NULL);
  gtk_tree_view_column_set_resizable(column, FALSE);
  gtk_tree_view_column_set_min_width(column, 40);
  gtk_tree_view_append_column(treeview, column);
  
  
  MK_FLAG_COLUMN(COLUMN_AT_START, "BEGIN", TPF_MATCH_AT_START);
  MK_FLAG_COLUMN(COLUMN_NEGATE, "NOT", TPF_NEGATE);
  MK_FLAG_COLUMN(COLUMN_PREVIOUS, "PREV", TPF_REQUIRE_PREVIOUS_MATCH);
  MK_FLAG_COLUMN(COLUMN_APPLY, "MULTI", TPF_APPLY_TO_MATCH);
  MK_FLAG_COLUMN(COLUMN_STOP, "STOP", TPF_STOP_IF_MATCHED);
  MK_FLAG_COLUMN(COLUMN_IGNORE, "IGNORE", TPF_IGNORE_IF_MATCHED);
  MK_FLAG_COLUMN(COLUMN_INACTIVE, "OFF", TPF_INACTIVE);
  MK_FLAG_COLUMN(COLUMN_SWITCH_NEXT, "SWITCH", TPF_SWITCH_NEXT);
  MK_STR_COLUMN(COLUMN_JUMP_TARGET, "JUMP", TRUE, TRUE, 70);
}
/**************************************************************************
  ...
**************************************************************************/
static void put_tag_pattern_into_store(GtkListStore *store,
                                        GtkTreeIter *iter,
                                        struct tag_pattern *ptagpat)
{
  gtk_list_store_set(store, iter,
                      COLUMN_TAG_NAME, ptagpat->tag_name,
                      COLUMN_PATTERN, ptagpat->pattern,
                      COLUMN_FOREGROUND, ptagpat->foreground_color,
                      COLUMN_BACKGROUND, ptagpat->background_color,
                      COLUMN_AT_START, ptagpat->flags & TPF_MATCH_AT_START,
                      COLUMN_NEGATE, ptagpat->flags & TPF_NEGATE,
                      COLUMN_PREVIOUS, ptagpat->flags & TPF_REQUIRE_PREVIOUS_MATCH,
                      COLUMN_APPLY, ptagpat->flags & TPF_APPLY_TO_MATCH,
                      COLUMN_STOP, ptagpat->flags & TPF_STOP_IF_MATCHED,
                      COLUMN_IGNORE, ptagpat->flags & TPF_IGNORE_IF_MATCHED,
                      COLUMN_INACTIVE, ptagpat->flags & TPF_INACTIVE,
                      COLUMN_SWITCH_NEXT, ptagpat->flags & TPF_SWITCH_NEXT,
                      COLUMN_JUMP_TARGET, ptagpat->jump_target,
                      -1);
}

/**************************************************************************
  ...
**************************************************************************/
static GtkTreeModel *create_model(struct tag_pattern_list *tmptagpats)
{
  GtkListStore *model;
  GtkTreeIter iter;

  model = gtk_list_store_new(NUM_COLUMNS,
                             G_TYPE_STRING, /* name */
                             G_TYPE_STRING, /* pattern */
                             G_TYPE_POINTER, /* foreground */
                             G_TYPE_POINTER, /* background */
                             G_TYPE_BOOLEAN, /* at start */
                             G_TYPE_BOOLEAN, /* negate */
                             G_TYPE_BOOLEAN, /* previous */
                             G_TYPE_BOOLEAN, /* apply to match */
                             G_TYPE_BOOLEAN, /* stop */
                             G_TYPE_BOOLEAN, /* ignore */
                             G_TYPE_BOOLEAN, /* active */
                             G_TYPE_BOOLEAN, /* switch next */
                             G_TYPE_STRING); /* jump target */
  tag_pattern_list_iterate(tmptagpats, ptagpat) {
    gtk_list_store_append(model, &iter);
    put_tag_pattern_into_store(model, &iter, ptagpat);
  } tag_pattern_list_iterate_end;

  return GTK_TREE_MODEL(model);
}

/**************************************************************************
  Creates a deep copy of given tag_pattern_list. Result should be destroyed
  with tag_pattern_list_free_all when no longer needed.
**************************************************************************/
struct tag_pattern_list *tag_pattern_list_clone(struct tag_pattern_list *pold)
{
  struct tag_pattern_list *pnew;

  pnew = tag_pattern_list_new();
  tag_pattern_list_iterate(pold, ptagpat) {
    tag_pattern_list_append(pnew, tag_pattern_clone(ptagpat));
  } tag_pattern_list_iterate_end;

  return pnew;
}

/**************************************************************************
  ...
**************************************************************************/
void tag_pattern_list_free_all(struct tag_pattern_list *tpl)
{
  tag_pattern_list_iterate(tpl, ptagpat) {
    tag_pattern_free(ptagpat);
  } tag_pattern_list_iterate_end;
  tag_pattern_list_free(tpl);
}

/**************************************************************************
  ...
**************************************************************************/
struct tag_pattern *tag_pattern_clone(struct tag_pattern *old)
{
  struct tag_pattern *new = fc_malloc(sizeof(struct tag_pattern));
  
  new->tag_name = mystrdup(old->tag_name);
  new->pattern = mystrdup(old->pattern);
  new->jump_target = mystrdup(old->jump_target);
  new->flags = old->flags;
  new->foreground_color = old->foreground_color != NULL
      ? gdk_color_copy(old->foreground_color) : NULL;
  new->background_color = old->background_color != NULL
      ? gdk_color_copy(old->background_color) : NULL;

  return new;
}
/**************************************************************************
  ...
**************************************************************************/
static void move_tag_patterns(GtkWidget *w,
                               gpointer user_data,
                               bool up)
{
  GtkWidget *dialog = (GtkWidget *) user_data;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  GtkTreeView *treeview;
  GtkTreePath *path;
  struct tag_pattern **tpv;
  struct tag_pattern_list *tmptagpats;
  GList *rows = NULL, *rowrefs = NULL, *p;
  GtkTreeRowReference *rowref;
  int *new_order, len, i, limit, tmp;

/*xxxMYDEBUG   printf("move offset=%d\n", offset);*/
  
  tmptagpats = g_object_get_data(G_OBJECT(dialog), "tmptagpats");
  model = g_object_get_data(G_OBJECT(dialog), "model");
  treeview = g_object_get_data(G_OBJECT(dialog), "treeview");
  selection = gtk_tree_view_get_selection(treeview);

  rows = gtk_tree_selection_get_selected_rows(selection, NULL);
  if (g_list_length(rows) <= 0)
    return;
  
  for (p = rows; p != NULL; p = g_list_next(p)) {
    rowref = gtk_tree_row_reference_new(model, p->data);
    rowrefs = g_list_append(rowrefs, rowref);
  }
  g_list_foreach(rows,(GFunc) gtk_tree_path_free, NULL);
  g_list_free(rows);

  len = tag_pattern_list_size(tmptagpats);
  new_order = fc_malloc(sizeof(int) * len);
  for (i=0; i<len; i++)
    new_order[i] = i;
  
  limit = up ? 0 : len - 1;
  p = up ? rowrefs : g_list_last(rowrefs);
  for (; p != NULL; p = up ? g_list_next(p) : g_list_previous(p)) {
    rowref = (GtkTreeRowReference *) p->data;
    path = gtk_tree_row_reference_get_path(rowref);
    if (!path)
      continue;
  
    i = gtk_tree_path_get_indices(path)[0];
    
    if (i == limit) {
      limit += up ? 1 : -1;
      continue;
    }
    tmp = new_order[i +(up ? -1 : 1)];
    new_order[i +(up ? -1 : 1)] = new_order[i];
    new_order[i] = tmp;
  }

  tpv = fc_malloc(sizeof(struct tag_pattern *) * len);
  i = 0;
  tag_pattern_list_iterate(tmptagpats, ptagpat) {
    tpv[new_order[i++]] = ptagpat;
  } tag_pattern_list_iterate_end;
  tag_pattern_list_unlink_all(tmptagpats);
  for (i = 0; i < len; i++) {
    tag_pattern_list_append(tmptagpats, tpv[i]);
  }
  free(tpv);
  
  gtk_list_store_reorder(GTK_LIST_STORE(model), new_order);
  free(new_order);

  for (p = rowrefs; p != NULL; p = g_list_next(p)) {
    rowref = (GtkTreeRowReference *) p->data;
    path = gtk_tree_row_reference_get_path(rowref);
    gtk_tree_selection_select_path(selection, path);
  }

  g_list_foreach(rowrefs,(GFunc) gtk_tree_row_reference_free, NULL);
  g_list_free(rowrefs);
}
/**************************************************************************
  ...
**************************************************************************/
static void down_callback(GtkWidget *w, gpointer user_data)
{
  move_tag_patterns(w, user_data, FALSE);
}
/**************************************************************************
  ...
**************************************************************************/
static void up_callback(GtkWidget *w, gpointer user_data)
{
  move_tag_patterns(w, user_data, TRUE);
}
/**************************************************************************
  ...
**************************************************************************/
static void add_tag_pattern(struct tag_pattern *ptagpat,
                             GtkWidget *dialog)
{
  GtkTreeView *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  struct tag_pattern_list *tmptagpats;
  GList *rows;
  GtkTreePath *path;
  int i;
  GtkTreeIter iter;
  
  treeview = g_object_get_data(G_OBJECT(dialog), "treeview");
  selection = gtk_tree_view_get_selection(treeview);
  tmptagpats = g_object_get_data(G_OBJECT(dialog), "tmptagpats");
  model = g_object_get_data(G_OBJECT(dialog), "model");
  
  rows = gtk_tree_selection_get_selected_rows(selection, NULL);
  if (g_list_length(rows) > 0) {
    path = (GtkTreePath *) g_list_nth_data(rows, 0);
    i = gtk_tree_path_get_indices(path)[0];
    gtk_list_store_insert(GTK_LIST_STORE(model), &iter, i + 1);
    tag_pattern_list_insert(tmptagpats, ptagpat, i + 1);
  } else {
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    tag_pattern_list_append(tmptagpats, ptagpat);
  }
  g_list_foreach(rows,(GFunc) gtk_tree_path_free, NULL);
  g_list_free(rows);

  put_tag_pattern_into_store(GTK_LIST_STORE(model), &iter, ptagpat);
  gtk_tree_selection_unselect_all(selection);
  gtk_tree_selection_select_iter(selection, &iter);
}
/**************************************************************************
  ...
**************************************************************************/
static void add_control_callback(GtkWidget *w, gpointer user_data)
{
  GtkWidget *dialog, *entry;
  struct tag_pattern_list *tmptagpats;
  struct tag_pattern *ptagpat;
  const char *newname = "";

  dialog = (GtkWidget *) user_data;
  
  tmptagpats = g_object_get_data(G_OBJECT(dialog), "tmptagpats");
  entry = g_object_get_data(G_OBJECT(dialog), "entry");

  newname = gtk_entry_get_text(GTK_ENTRY(entry));
  ptagpat = tag_pattern_new(newname, "pattern", 
                             TPF_IS_CONTROL_ONLY, "", NULL, NULL);
  gtk_entry_set_text(GTK_ENTRY(entry), "");
  
  add_tag_pattern(ptagpat, dialog);
}
/**************************************************************************
  ...
**************************************************************************/
static void add_callback(GtkWidget *w, gpointer user_data)
{
  GtkWidget *dialog, *entry;
  struct tag_pattern_list *tmptagpats;
  struct tag_pattern *ptagpat;
  const char *newname = "";

  dialog = (GtkWidget *) user_data;
  
  tmptagpats = g_object_get_data(G_OBJECT(dialog), "tmptagpats");
  entry = g_object_get_data(G_OBJECT(dialog), "entry");

  newname = gtk_entry_get_text(GTK_ENTRY(entry));
  if (newname[0] == '\0') {
    notify_user_error(dialog, _("Please type in a name into"
        " the input box."));
    return;
  }

  tag_pattern_list_iterate(tmptagpats, ptagpat) {
    if (tag_pattern_is_control_only(ptagpat)) {
      continue;
    }
      
    if (!strcmp(newname, ptagpat->tag_name)) {
      char buf[512];
      my_snprintf(buf, sizeof(buf),
          _("A pattern named \"%s\" already exists, please"
            " choose a unique name."), newname);
      notify_user_error(dialog, buf);
      return;
    }
  } tag_pattern_list_iterate_end;
  
  ptagpat = tag_pattern_new(newname, "pattern", 0, "", NULL, NULL);
  gtk_entry_set_text(GTK_ENTRY(entry), "");
  
  add_tag_pattern(ptagpat, dialog);
}
/**************************************************************************
  ...
**************************************************************************/
static void clear_callback(GtkWidget *w, gpointer user_data)
{
  GtkWidget *dialog = (GtkWidget *) user_data;
  struct tag_pattern_list *tmptagpats = NULL;
  GtkTreeModel *model;
  GHashTable *tags_to_delete = NULL;
  
  tmptagpats = g_object_get_data(G_OBJECT(dialog), "tmptagpats");
  tags_to_delete = g_object_get_data(G_OBJECT(dialog), "tags_to_delete");

  tag_pattern_list_iterate(tmptagpats, ptagpat) {
    g_hash_table_replace(tags_to_delete, mystrdup(ptagpat->tag_name), NULL);
  } tag_pattern_list_iterate_end;
  
  tmptagpats = g_object_get_data(G_OBJECT(dialog), "tmptagpats");
  tag_pattern_list_free_data(tmptagpats);

  model = g_object_get_data(G_OBJECT(dialog), "model");
  gtk_list_store_clear(GTK_LIST_STORE(model));
}
/**************************************************************************
  ...
**************************************************************************/
static void delete_callback(GtkWidget *w, gpointer user_data)
{
  GtkWidget *dialog = (GtkWidget *) user_data;
  struct tag_pattern_list *tmptagpats;
  GHashTable *tags_to_delete = NULL;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  GtkTreeView *treeview;
  GtkTreeIter iter;
  int i;
  GtkTreePath *path;
  struct tag_pattern *ptagpat;
  GList *rows = NULL, *rowrefs = NULL, *p;
  GtkTreeRowReference *rowref;
  
/*xxxMYDEBUG   printf("delete_callback\n");*/
  
  tmptagpats = g_object_get_data(G_OBJECT(dialog), "tmptagpats");
  tags_to_delete = g_object_get_data(G_OBJECT(dialog), "tags_to_delete");
  model = g_object_get_data(G_OBJECT(dialog), "model");
  treeview = g_object_get_data(G_OBJECT(dialog), "treeview");

  selection = gtk_tree_view_get_selection(treeview);
  
  rows = gtk_tree_selection_get_selected_rows(selection, NULL);
  if (g_list_length(rows) <= 0)
    return;
                                               
  for (p = rows; p != NULL; p = g_list_next(p)) {
    rowref = gtk_tree_row_reference_new(model, p->data);
    rowrefs = g_list_append(rowrefs, rowref);
  }
  g_list_foreach(rows,(GFunc) gtk_tree_path_free, NULL);
  g_list_free(rows);

  for (p = rowrefs; p != NULL; p = g_list_next(p)) {
    rowref = (GtkTreeRowReference *) p->data;
    path = gtk_tree_row_reference_get_path(rowref);
    if (!path)
      continue;
  
    i = gtk_tree_path_get_indices(path)[0];
    if (!gtk_tree_model_get_iter(model, &iter, path))
      continue;
    
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
    
    ptagpat = tag_pattern_list_get(tmptagpats, i);
    tag_pattern_list_unlink(tmptagpats, ptagpat);
    g_hash_table_replace(tags_to_delete, mystrdup(ptagpat->tag_name), NULL);
    tag_pattern_free(ptagpat);
  }
  g_list_foreach(rowrefs,(GFunc) gtk_tree_row_reference_free, NULL);
  g_list_free(rowrefs);
}
/**************************************************************************
  ...
**************************************************************************/
static gboolean treeview_button_press_callback(GtkWidget *treeview,
                                                GdkEventButton *event,
                                                gpointer userdata)
{
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  GtkTreePath *path;
  GtkTreeViewColumn *col;
  GtkTreeIter iter;
  int i, column;
  struct tag_pattern_list *tmptagpats;
  struct tag_pattern *ptagpat;
  GList *rendlist;
  GtkCellRenderer *renderer;
  GtkWidget *dialog;
  GtkColorSelection *colorsel;
  gint response;
  GdkColor color;

  if (!(event->type == GDK_2BUTTON_PRESS && event->button == 1))
    return FALSE;
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  
  tmptagpats = g_object_get_data(G_OBJECT(chatline_config_shell),
                                  "tmptagpats");
  
  gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
                                 event->x, event->y,
                                 &path, &col,
                                 NULL, NULL);
  
  i = gtk_tree_path_get_indices(path)[0];
  gtk_tree_model_get_iter(model, &iter, path);
  gtk_tree_path_free(path);
  ptagpat = tag_pattern_list_get(tmptagpats, i);
  
  if (tag_pattern_is_control_only(ptagpat)) {
    notify_user_error(chatline_config_shell,
                       _("This type of pattern may not have colors."));
    return FALSE;
  }

  rendlist = gtk_tree_view_column_get_cell_renderers(col);
  if (!rendlist)
    return FALSE;

  renderer = (GtkCellRenderer *) rendlist->data;
  g_list_free(rendlist);

  column = GPOINTER_TO_INT(
      g_object_get_data(G_OBJECT(renderer), "column"));

  if (column != COLUMN_FOREGROUND && column != COLUMN_BACKGROUND)
    return FALSE;

  dialog = create_my_color_selection_dialog("Select Color");
  colorsel = GTK_COLOR_SELECTION(g_object_get_data(G_OBJECT
    (dialog), "colorsel"));
  gtk_window_set_transient_for (GTK_WINDOW(dialog),
                                GTK_WINDOW(chatline_config_shell));

/*xxxMYDEBUG  printf("  column=%d, i=%d, ptagpat=%p colorsel=%p\n", column, i,*/
/*xxxMYDEBUG          ptagpat, colorsel);*/

  color.red = 0;
  color.green = 0;
  color.blue = 0;
  
  if (column == COLUMN_FOREGROUND && ptagpat->foreground_color)
    color = *ptagpat->foreground_color;

  if (column == COLUMN_BACKGROUND && ptagpat->background_color)
    color = *ptagpat->background_color;
  
  gtk_color_selection_set_previous_color(colorsel, &color);
  gtk_color_selection_set_current_color(colorsel, &color);
  
  response = gtk_dialog_run(GTK_DIALOG(dialog));
  
  if (response == GTK_RESPONSE_OK) {
    gtk_color_selection_get_current_color(colorsel,
                                           &color);

    if (column == COLUMN_FOREGROUND) {
      if (ptagpat->foreground_color)
        gdk_color_free(ptagpat->foreground_color);
      ptagpat->foreground_color = gdk_color_copy(&color);
      gtk_list_store_set(GTK_LIST_STORE(model), &iter, column,
                          ptagpat->foreground_color, -1);
    } else if (column == COLUMN_BACKGROUND) {
      if (ptagpat->background_color)
        gdk_color_free(ptagpat->background_color);
      ptagpat->background_color = gdk_color_copy(&color);
      gtk_list_store_set(GTK_LIST_STORE(model), &iter, column,
                          ptagpat->background_color, -1);
    }

  } else if (response == GTK_RESPONSE_REJECT) {
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, column,
                        NULL, -1);
    
    if (column == COLUMN_FOREGROUND) {
      if (ptagpat->foreground_color)
        gdk_color_free(ptagpat->foreground_color);
      ptagpat->foreground_color = NULL;
    } else if (column == COLUMN_BACKGROUND) {
      if (ptagpat->background_color)
        gdk_color_free(ptagpat->background_color);
      ptagpat->background_color = NULL;
    }
  }
  gtk_widget_destroy(dialog);

  return TRUE;
}
/**************************************************************************
  ...
**************************************************************************/
static GtkWidget *create_chatline_config_shell(void)
{
  GtkWidget *dialog, *hbox, *sw, *treeview, *button, *vbox, *top_vbox,
    *sep, *entry, *label;
  GtkTreeModel *model;
  GtkTreeSelection *sel;
  struct tag_pattern_list *tmptagpats;
  GHashTable *tags_to_delete;
  
  tags_to_delete = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
  tmptagpats = tag_pattern_list_clone(tagpats);
  

  /* create the window */
  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(dialog),
                        _("Chatline Color Configuration"));
  gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 600);
  gtk_window_set_position(GTK_WINDOW(dialog),
                           GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_decorated(GTK_WINDOW(dialog), TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

  gtk_widget_set_name(dialog, "Freeciv");
  setup_dialog(dialog, toplevel);

  g_signal_connect(dialog, "destroy",
      G_CALLBACK(destroy_callback), dialog);
  

  /* layout boxes */
  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);

  top_vbox = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), top_vbox, TRUE, TRUE, 0);

  sep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);


  /* apply/close buttons */
  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  
  button = gtk_stockbutton_new(GTK_STOCK_CANCEL, _("_Cancel"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
      G_CALLBACK(cancel_callback), dialog);
  gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  
  button = gtk_stockbutton_new(GTK_STOCK_OK, _("_Ok"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
      G_CALLBACK(ok_callback), dialog);
  gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  
  button = gtk_stockbutton_new(GTK_STOCK_APPLY, _("_Apply"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
      G_CALLBACK(apply_callback), dialog);
  gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  

  /* save/reset/help buttons */
  button = gtk_stockbutton_new(GTK_STOCK_SAVE, _("_Save"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
      G_CALLBACK(save_callback), dialog);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  button = gtk_button_new_with_mnemonic(_("_Reset"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
      G_CALLBACK(reset_callback), dialog);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  button = gtk_stockbutton_new(GTK_STOCK_HELP, _("_Help"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
      G_CALLBACK(help_callback), dialog);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  

  /* create the scrolled window which will hold the treeview */
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(top_vbox),
                      hbox, TRUE, TRUE, 0);
  
  gtk_box_pack_start(GTK_BOX(hbox), sw, TRUE, TRUE, 0);
  

  /* create the treeview */
  model = create_model(tmptagpats);
  
  treeview = gtk_tree_view_new_with_model(model);
  g_object_unref(model);

  gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(treeview), FALSE);
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);
  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);

  add_columns(GTK_TREE_VIEW(treeview), model);
  g_signal_connect(G_OBJECT(treeview), "button-press-event",
                    G_CALLBACK(treeview_button_press_callback), NULL);
  
  gtk_container_add(GTK_CONTAINER(sw), treeview);
 

  /* feedback label */
  label = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(top_vbox), label, FALSE, FALSE, 0);
  g_object_set_data(G_OBJECT(dialog), "msglabel", label);

  /* input entry */
  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(top_vbox), hbox, FALSE, FALSE, 0);
  
  label = gtk_label_new(_("Input:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  
  entry = gtk_entry_new();
  gtk_widget_set_size_request(entry, 100, -1);
  g_signal_connect(G_OBJECT(entry), "activate",
                    G_CALLBACK(add_callback),
                    dialog);
  g_object_set_data(G_OBJECT(dialog), "entry", entry);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  

  /* add/delete etc. controls */
  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(top_vbox), hbox, FALSE, FALSE, 0);

  button = gtk_stockbutton_new(GTK_STOCK_CLEAR, _("Clear"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
      G_CALLBACK(clear_callback), dialog);
  gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  
  button = gtk_stockbutton_new(GTK_STOCK_DELETE, _("D_elete"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
      G_CALLBACK(delete_callback), dialog);
  gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  
  button = gtk_stockbutton_new(GTK_STOCK_ADD, _("A_dd"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
      G_CALLBACK(add_callback), dialog);
  gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  
  button = gtk_stockbutton_new(GTK_STOCK_ADD, _("Add Con_trol Pattern"));
  gtk_widget_set_size_request(button, -1, 30); /* Too long */
  g_signal_connect(button, "clicked",
      G_CALLBACK(add_control_callback), dialog);
  gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  

  /* up/down buttons */
  button = gtk_stockbutton_new(GTK_STOCK_GO_UP, _("Move _Up"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
      G_CALLBACK(up_callback), dialog);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  button = gtk_stockbutton_new(GTK_STOCK_GO_DOWN, _("Move Do_wn"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
      G_CALLBACK(down_callback), dialog);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  


  /* for easy access */
  g_object_set_data(G_OBJECT(dialog), "model", model);
  g_object_set_data(G_OBJECT(dialog), "tmptagpats", tmptagpats);
  g_object_set_data(G_OBJECT(dialog), "tags_to_delete", tags_to_delete);
  g_object_set_data(G_OBJECT(dialog), "treeview", treeview);

  gtk_widget_show_all(dialog);

  return dialog;
}
/**************************************************************************
  ...
**************************************************************************/
void popup_chatline_config_dialog(void)
{
  if (!chatline_config_shell)  {
    chatline_config_shell = create_chatline_config_shell();
  }
  
  gtk_window_present(GTK_WINDOW(chatline_config_shell));
}
/**************************************************************************
  ...
**************************************************************************/
void chatline_scroll_to_bottom(void)
{
  GtkTextIter end;

  if (!message_buffer) {
    return;
  }
  gtk_text_buffer_get_end_iter(message_buffer, &end);

  if (main_message_area) {
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(main_message_area),
                                 &end, 0.0, TRUE, 1.0, 0.0);
  }
  if (start_message_area) {
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(start_message_area),
                                 &end, 0.0, TRUE, 1.0, 0.0);
  }
}
