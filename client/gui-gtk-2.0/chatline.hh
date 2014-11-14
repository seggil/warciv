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
#ifndef WC_CLIENT_GUI_CHATLINE_H
#define WC_CLIENT_GUI_CHATLINE_H

#include <gtk/gtk.h>

#include "../include/chatline_g.hh"
#include "map.hh"

#define MAX_CHATLINE_HISTORY 20

extern genlist *history_list;
extern int history_pos;

void inputline_return(GtkEntry *w, gpointer data);
void set_output_window_text(const char *text);

enum tag_pattern_flags {
  TPF_MATCH_AT_START            = 0x001,
  TPF_APPLY_TO_MATCH            = 0x002,
  TPF_IGNORE_IF_MATCHED         = 0x004,
  TPF_STOP_IF_MATCHED           = 0x008,
  TPF_REQUIRE_PREVIOUS_MATCH    = 0x010,
  TPF_NEGATE                    = 0x020,
  TPF_INACTIVE                  = 0x040,
  TPF_SWITCH_NEXT               = 0x080,
  TPF_IS_CONTROL_ONLY           = 0x100,
  TPF_INSENSITIVE               = 0x200
};

struct tag_pattern {
  /* unique name used to lookup in tag_table, may
     be "" if this pattern does not correspond
     to a GtkTextTag (e.g. just used for control flow) */
  char *tag_name;

  /* text to match, if "", matches anything */
  char *pattern;

  int flags; /* TPF_* above */

  /* tag_name of the pattern to jump to, i.e. skip
     all subsequent patterns until we reach one whose
     tag_name matches jump_target */
  char *jump_target;

  /* Sound file tag (defined in a soundspec file,
   * e.g. stdsounds.soundspec) to be played when
   * this pattern matches. "" for no sound. */
  char *sound_tag;

  GdkColor *foreground_color;
  GdkColor *background_color;
};

#define SPECLIST_TAG tag_pattern
#define SPECLIST_TYPE struct tag_pattern
#include "speclist.hh"
#define tag_pattern_list_iterate(alist, pitem)\
  TYPED_LIST_ITERATE(struct tag_pattern, alist, pitem)
#define tag_pattern_list_iterate_end  LIST_ITERATE_END

struct tag_pattern_list *tag_pattern_list_clone(struct tag_pattern_list *old);
void tag_pattern_list_free_all(struct tag_pattern_list *tpl);
void tag_pattern_list_free_data(struct tag_pattern_list *tpl);
struct tag_pattern *tag_pattern_clone(struct tag_pattern *old);


void init_message_buffer_tag_patterns(void);
void free_message_buffer_tag_patterns(void);
void refresh_message_buffer_tag_patterns(GHashTable *tags_to_delete);
struct tag_pattern *tag_pattern_new(const char *name, const char *pattern,
                                    int flags, const char *jump_target,
                                    const char *sound_tag, const char *fgcolor,
                                    const char *bgcolor);
void tag_pattern_free(struct tag_pattern *ptagpat);

void secfile_save_message_buffer_tag_patterns(struct section_file *file);
struct tag_pattern_list *
    secfile_load_message_buffer_tag_patterns(struct section_file *file);

GtkWidget *create_chatline_config(void);
void apply_chatline_config(GtkWidget *widget);
void refresh_chatline_config(GtkWidget *widget);
void reset_chatline_config(GtkWidget *widget);
void reload_chatline_config(GtkWidget *widget, struct section_file *sf);

void set_message_buffer_view_link_handlers(GtkTextView *view);
void insert_chat_link(tile_t *ptile, bool unit);
void chatline_scroll_to_bottom(void);
bool chatline_is_scrolled_to_bottom(void);

bool chatline_autocomplement(GtkEditable *editable);

#endif  /* WC_CLIENT_GUI_CHATLINE_H*/
