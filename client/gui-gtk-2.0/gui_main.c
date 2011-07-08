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
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "dataio.h"
#include "fciconv.h"
#include "fcintl.h"
#include "game.h"
#include "government.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "shared.h"
#include "support.h"
#include "version.h"

#include "chatline.h"
#include "civclient.h"
#include "climisc.h"
#include "clinet.h"
#include "colors.h"
#include "connectdlg.h"
#include "control.h"
#include "dialogs.h"
#include "gotodlg.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdata.h"                   /* boot_help_texts() */
#include "mapctrl.h"
#include "mapview.h"
#include "menu.h"
#include "messagewin.h"
#include "multiselect.h"
#include "netintf.h"
#include "optiondlg.h"
#include "options.h"
#include "pages.h"
#include "spaceshipdlg.h"
#include "resources.h"
#include "tilespec.h"


#include "freeciv.ico"

const char *client_string = "gui-gtk-2.0";

GtkWidget *map_canvas;                  /* GtkDrawingArea */
GtkWidget *map_horizontal_scrollbar;
GtkWidget *map_vertical_scrollbar;

GtkWidget *overview_canvas;             /* GtkDrawingArea */
GdkPixmap *overview_canvas_store;       /* this pixmap acts as a backing store
                                         * for the overview_canvas widget */
GtkWidget *player_colors_mode_label;
int overview_canvas_store_width = 2 * 80;
int overview_canvas_store_height = 2 * 50;

bool allied_chat_only = FALSE;
GtkWidget *allied_chat_toggle_button = NULL;

GtkWidget *toplevel;
GdkWindow *root_window;
GtkWidget *toplevel_tabs;
GtkWidget *top_vbox;
GtkWidget *top_notebook, *bottom_notebook;

PangoFontDescription *main_font;
PangoFontDescription *city_productions_font;

GdkGC *civ_gc;
GdkGC *mask_fg_gc;
GdkGC *mask_bg_gc;
GdkGC *fill_bg_gc;
GdkGC *fill_tile_gc;
GdkGC *thin_line_gc;
GdkGC *thick_line_gc;
GdkGC *border_line_gc;
GdkPixmap *gray50, *gray25, *black50;
GdkPixmap *mask_bitmap;

GtkWidget *main_frame_civ_name;
GtkWidget *main_label_info;

GtkWidget *conn_box;
GtkWidget *scroll_panel;

static GtkWidget *info_table;
GtkWidget *econ_label[10];
GtkWidget *bulb_label;
GtkWidget *sun_label;
GtkWidget *flake_label;
GtkWidget *government_label;
GtkWidget *timeout_label;
GtkWidget *turn_done_button;

GtkWidget *unit_info_label;
GtkWidget *unit_info_frame;

GtkTooltips *main_tips;
GtkWidget *econ_ebox;
GtkWidget *bulb_ebox;
GtkWidget *sun_ebox;
GtkWidget *flake_ebox;
GtkWidget *government_ebox;

/* For global observers */
static GtkWidget *go_table;
GtkWidget *go_sun_label;
GtkWidget *go_flake_label;
GtkWidget *go_timeout_label;
GtkWidget *go_sun_ebox;
GtkWidget *go_flake_ebox;

static GtkWidget *main_menubar;
static GtkWidget *unit_pixmap_table;
static GtkWidget *unit_pixmap;
static GtkWidget *unit_pixmap_button;
static GtkWidget *unit_below_pixmap[MAX_NUM_UNITS_BELOW];
static GtkWidget *unit_below_pixmap_button[MAX_NUM_UNITS_BELOW];
static GtkWidget *more_arrow_pixmap;
static GtkWidget *more_time_button;
static GtkWidget *pause_button;

static int unit_ids[MAX_NUM_UNITS_BELOW];  /* ids of the units icons in
                                            * information display: (or 0) */
GtkTextView *main_message_area;
GtkTextBuffer *message_buffer, *network_message_buffer;
GtkWidget *inputline;

struct voteinfo_bar *pregame_votebar = NULL;
struct voteinfo_bar *ingame_votebar = NULL;

static enum Display_color_type display_color_type;  /* practically unused */
static gint timer_id;                               /*       ditto        */
static guint input_id;


static gboolean show_info_button_release(GtkWidget *w, GdkEventButton *ev,
                                         gpointer data);
static gboolean show_info_popup(GtkWidget *w, GdkEventButton *ev,
                                gpointer data);
static gboolean player_colors_mode_label_click(GtkWidget *w,
					       GdkEventButton *ev,
					       gpointer data);

static void end_turn_callback(GtkWidget *w, gpointer data);
static gboolean get_net_input(GIOChannel *source, GIOCondition cond,
                              gpointer data);
static void set_wait_for_writable_socket(struct connection *pc,
                                         bool socket_writable);

static void print_usage(const char *argv0);
static void parse_options(int argc, char **argv);
static gboolean keyboard_handler(GtkWidget *w, GdkEventKey *ev, gpointer data);

static void tearoff_callback(GtkWidget *b, gpointer data);
static GtkWidget *detached_widget_new(void);
static GtkWidget *detached_widget_fill(GtkWidget *ahbox,
                                       gboolean propagate_keypress);

static gboolean select_unit_pixmap_callback(GtkWidget *w, GdkEvent *ev,
                                            gpointer data);
static gint timer_callback(gpointer data);
gboolean show_conn_popup(GtkWidget *view, GdkEventButton *ev, gpointer data);
static gboolean quit_dialog_callback(void);

static guint chatline_scroll_callback_id = 0;

#define NET_INPUT_CTX_MEMORY_GUARD 0xfece5ace

/* used by add_net_input_callback and related functions */
struct net_input_ctx {
  int guard;
  input_ready_callback_t callback;
  int sock;
  void *userdata;
  data_free_func_t datafree;
};

/**************************************************************************
...
**************************************************************************/
void init_chat_buttons(void)
{
  allied_chat_only = TRUE;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(allied_chat_toggle_button),
                               TRUE);
  if (can_client_issue_orders()) {
    gtk_widget_show(more_time_button);
    gtk_widget_show(pause_button);
  } else {
    gtk_widget_hide(more_time_button);
    gtk_widget_hide(pause_button);
  }
}

/**************************************************************************
...
**************************************************************************/
static void log_callback_utf8(int level, const char *message)
{
  fc_fprintf(stderr, "%d: %s\n", level, message);
}

/**************************************************************************
  Print extra usage information, including one line help on each option,
  to stderr.
**************************************************************************/
static void print_usage(const char *argv0)
{
  /* add client-specific usage information here */
  fc_fprintf(stderr, _("Report bugs at %s.\n"), BUG_URL);
}

/**************************************************************************
 search for command line options. right now, it's just help
 semi-useless until we have options that aren't the same across all clients.
**************************************************************************/
static void parse_options(int argc, char **argv)
{
  int i = 1;

  while (i < argc) {
    if (is_option("--help", argv[i])) {
      print_usage(argv[0]);
      exit(EXIT_SUCCESS);
    }
    i++;
  }
}

/**************************************************************************
...
**************************************************************************/
static gboolean toplevel_focus(GtkWidget *w, GtkDirectionType arg)
{
  switch (arg) {
  case GTK_DIR_TAB_FORWARD:
  case GTK_DIR_TAB_BACKWARD:

    if (!GTK_WIDGET_CAN_FOCUS(w)) {
      return FALSE;
    }

    if (!gtk_widget_is_focus(w)) {
      gtk_widget_grab_focus(w);
      return TRUE;
    }
    break;

  default:
    break;
  }
  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
gboolean inputline_handler(GtkWidget *w, GdkEventKey *ev)
{
  void *data = NULL;
  gint keypress = FALSE;

  if (ev->keyval == GDK_Tab && chatline_autocomplementation) {
    return chatline_autocomplement(GTK_EDITABLE(w));
  }

  if (ev->keyval == GDK_Up) {
    keypress = TRUE;

    if (history_pos < genlist_size(history_list) - 1)
      history_pos++;

    data = genlist_get(history_list, history_pos);
  }

  if (ev->keyval == GDK_Down) {
    keypress = TRUE;

    if (history_pos >= 0)
      history_pos--;

    if (history_pos >= 0) {
      data = genlist_get(history_list, history_pos);
    } else {
      data = "";
    }
  }

  if (data) {
    gtk_entry_set_text(GTK_ENTRY(w), data);
    gtk_editable_set_position(GTK_EDITABLE(w), -1);
  }
  return keypress;
}

/**************************************************************************
  In GTK+ keyboard events are recursively propagated from the hierarchy
  parent down to its children. Sometimes this is not what we want.
  E.g. The inputline is active, the user presses the 's' key, we want it
  to be sent to the inputline, but because the main menu is further up
  the hierarchy, it wins and the inputline never gets anything!
  This function ensures an entry widget (like the inputline) always gets
  first dibs at handling a keyboard event.
**************************************************************************/
static gboolean toplevel_handler(GtkWidget *w, GdkEventKey *ev,
                                 gpointer data)
{
  GtkWidget *focus;

  focus = gtk_window_get_focus(GTK_WINDOW(toplevel));
  if (focus) {
    if (GTK_IS_ENTRY(focus)) {
      /* Propagate event to currently focused entry widget. */
      if (gtk_widget_event(focus, (GdkEvent *) ev)) {
        /* Do not propagate event to our children. */
        return TRUE;
      }
    }
  }

  /* Continue propagating event to our children. */
  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
static gboolean chatline_scroll_callback(gpointer data)
{
  /* Why do we do it in such a convuluted fasion rather than calling
   * chatline_scroll_to_bottom directly from toplevel_configure?
   * Because the widget is not at its final size yet when the configure
   * event occurs. */
  chatline_scroll_to_bottom();
  chatline_scroll_callback_id = 0;
  return FALSE; /* Remove this idle function. */
}

/**************************************************************************
  ...
**************************************************************************/
void queue_chatline_scroll_to_bottom(void)
{
  guint id;

  if (chatline_scroll_callback_id != 0) {
    return;
  }

  id = g_idle_add(chatline_scroll_callback, NULL);
  chatline_scroll_callback_id = id;
}

/**************************************************************************
  Called whenever the toplevel window is resized or moved (we primarily
  care about the resize only).
**************************************************************************/
static gboolean toplevel_configure(GtkWidget *w,
                                   GdkEventConfigure *event,
                                   gpointer user_data)
{
  static int old_width = 0, old_height = 0;

  if (!disable_chatline_scroll_on_window_resize
      && (event->width != old_width || event->height != old_height)) {

    /* Often it happens that by resizing the window the scrollbar for the
     * chat window gets pushed up, causing the chatline to not scroll
     * automatically to the bottom when new messages arrives.
     * This rectifies that situation. */
    queue_chatline_scroll_to_bottom();

    old_width = event->width;
    old_height = event->height;
  }

  return FALSE; /* Continue propagating. */
}

/**************************************************************************
...
**************************************************************************/
static gboolean keyboard_handler(GtkWidget *w, GdkEventKey *ev,
                                 gpointer data)
{
  /* inputline history code */
  if (!GTK_WIDGET_MAPPED(top_vbox) || GTK_WIDGET_HAS_FOCUS(inputline)) {
    return FALSE;
  }

  if (ev->keyval == GDK_Tab) {
    if (ev->state & GDK_CONTROL_MASK) {
      key_cycle_player_colors_modes();
    } else if (!GTK_WIDGET_HAS_FOCUS(inputline)) {
      gtk_widget_grab_focus(inputline);
    }
    return TRUE;
  }

  if (ev->keyval == GDK_Page_Up) {
    g_signal_emit_by_name(main_message_area, "move_cursor",
                          GTK_MOVEMENT_PAGES, -1, FALSE);
    return TRUE;
  }
  if (ev->keyval == GDK_Page_Down) {
    g_signal_emit_by_name(main_message_area, "move_cursor",
                          GTK_MOVEMENT_PAGES, 1, FALSE);
    return TRUE;
  }

  if (ev->keyval == GDK_t
      && !(ev->state & GDK_CONTROL_MASK)
      && !(ev->state & GDK_SHIFT_MASK)) {
    /* Workaround for conflict with menu shortcuts. If you add other
       key handlers here be sure to check menu.c for conflicts otherwise
       you may get unexpected behavior. */
    key_city_workers(w, ev);
    return TRUE;
  }

  if (!client_is_observer()) {
    if (use_digits_short_cuts) {
      /* If this option is enabled, the keys GDK_1-GDK_9 are used 
       * for Warclient features instead of units moves. This is
       * useful for laptop users. */
      guint keyval;

      if (ev->keyval < GDK_1 || ev->keyval > GDK_9) {
        /* Try with the shift mode */
        gdk_keymap_translate_keyboard_state(NULL, ev->hardware_keycode,
                                            GDK_SHIFT_MASK, ev->group,
                                            &keyval, NULL, NULL, NULL);
      } else {
        keyval = ev->keyval;
      }

      if ((ev->state & GDK_SHIFT_MASK)) {
        if ((ev->state & GDK_CONTROL_MASK)) {
          if ((ev->state & GDK_MOD1_MASK)) {
            /* <alt><shift><ctrl> */
            switch (keyval) {
            case GDK_1:
              request_unit_execute_delayed_goto(1);
              return TRUE;

            case GDK_2:
              request_unit_execute_delayed_goto(2);
              return TRUE;

            case GDK_3:
              request_unit_execute_delayed_goto(3);
              return TRUE;

            case GDK_4:
              do_airlift_for(1, NULL);
              return TRUE;

            case GDK_5:
              do_airlift_for(2, NULL);
              return TRUE;

            case GDK_6:
              do_airlift_for(3, NULL);
              return TRUE;

            case GDK_7:
              do_airlift_for(4, NULL);
              return TRUE;

            case GDK_8:
              do_airlift_for(5, NULL);
              return TRUE;

            case GDK_9:
              do_airlift_for(6, NULL);
              return TRUE;

            default:
              break;
            }
          }
          /* <shift><ctrl> */
          switch (keyval) {
          case GDK_1:
            multi_select_clear(1);
            return TRUE;

          case GDK_2:
            multi_select_clear(2);
            return TRUE;

          case GDK_3:
            multi_select_clear(3);
            return TRUE;

          case GDK_4:
            multi_select_clear(4);
            return TRUE;

          case GDK_5:
            multi_select_clear(5);
            return TRUE;

          case GDK_6:
            multi_select_clear(6);
            return TRUE;

          case GDK_7:
            multi_select_clear(7);
            return TRUE;

          case GDK_8:
            multi_select_clear(8);
            return TRUE;

          case GDK_9:
            multi_select_clear(9);
            return TRUE;

          default:
            break;
          }
        }
        if ((ev->state & GDK_MOD1_MASK)) {
          /* <alt><shift> */
          switch (keyval) {
          case GDK_1:
            delayed_goto_cat(0, 1);
            return TRUE;

          case GDK_2:
            delayed_goto_cat(0, 2);
            return TRUE;

          case GDK_3:
            delayed_goto_cat(0, 3);
            return TRUE;

          case GDK_4:
            airlift_queue_cat(0, 1);
            return TRUE;

          case GDK_5:
            airlift_queue_cat(0, 2);
            return TRUE;

            break;
          case GDK_6:
            airlift_queue_cat(0, 3);
            return TRUE;

          case GDK_7:
            airlift_queue_cat(0, 4);
            return TRUE;

          case GDK_8:
            airlift_queue_cat(0, 5);
            return TRUE;

          case GDK_9:
            airlift_queue_cat(0, 6);
            return TRUE;

          default:
            break;
          }
        }
        /* <shift> */
        switch (keyval) {
        case GDK_1:
          multi_select_cat(0, 1);
          return TRUE;

        case GDK_2:
          multi_select_cat(0, 2);
          return TRUE;

        case GDK_3:
          multi_select_cat(0, 3);
          return TRUE;

        case GDK_4:
          multi_select_cat(0, 4);
          return TRUE;

        case GDK_5:
          multi_select_cat(0, 5);
          return TRUE;

        case GDK_6:
          multi_select_cat(0, 6);
          return TRUE;

        case GDK_7:
          multi_select_cat(0, 7);
          return TRUE;

        case GDK_8:
          multi_select_cat(0, 8);
          return TRUE;

        case GDK_9:
          multi_select_cat(0, 9);
          return TRUE;

        default:
          break;
        }
      }
      if ((ev->state & GDK_CONTROL_MASK)) {
        if ((ev->state & GDK_MOD1_MASK)) {
          /* <alt><ctrl> */
          switch (keyval) {
          case GDK_1:
            delayed_goto_move(1, 0);
            return TRUE;

          case GDK_2:
            delayed_goto_move(2, 0);
            return TRUE;

          case GDK_3:
            delayed_goto_move(3, 0);
            return TRUE;

          case GDK_4:
            airlift_queue_move(1, 0);
            return TRUE;

          case GDK_5:
            airlift_queue_move(2, 0);
            return TRUE;

          case GDK_6:
            airlift_queue_move(3, 0);
            return TRUE;

          case GDK_7:
            airlift_queue_move(4, 0);
            return TRUE;

          case GDK_8:
            airlift_queue_move(5, 0);
            return TRUE;

          case GDK_9:
            airlift_queue_move(6, 0);
            return TRUE;

          default:
            break;
          }
        }
        /* <ctrl> */
        switch (keyval) {
        case GDK_1:
          multi_select_copy(1, 0);
          return TRUE;

        case GDK_2:
          multi_select_copy(2, 0);
          return TRUE;

        case GDK_3:
          multi_select_copy(3, 0);
          return TRUE;

        case GDK_4:
          multi_select_copy(4, 0);
          return TRUE;

        case GDK_5:
          multi_select_copy(5, 0);
          return TRUE;

        case GDK_6:
          multi_select_copy(6, 0);
          return TRUE;

        case GDK_7:
          multi_select_copy(7, 0);
          return TRUE;

        case GDK_8:
          multi_select_copy(8, 0);
          return TRUE;

        case GDK_9:
          multi_select_copy(9, 0);
          return TRUE;

        default:
          break;
        }
      }
      /* <alt> */
      if ((ev->state & GDK_MOD1_MASK)) {
        switch (keyval) {
        case GDK_1:
          delayed_goto_copy(0, 1);
          return TRUE;

        case GDK_2:
          delayed_goto_copy(0, 2);
          return TRUE;

        case GDK_3:
          delayed_goto_copy(0, 3);
          return TRUE;

        case GDK_4:
          airlift_queue_copy(0, 1);
          return TRUE;

        case GDK_5:
          airlift_queue_copy(0, 2);
          return TRUE;

        case GDK_6:
          airlift_queue_copy(0, 3);
          return TRUE;

        case GDK_7:
          airlift_queue_copy(0, 4);
          return TRUE;

        case GDK_8:
          airlift_queue_copy(0, 5);
          return TRUE;

        case GDK_9:
          airlift_queue_copy(0, 6);
          return TRUE;

        default:
          break;
        }
      }
      /* Plain */
      if (hover_state == HOVER_AIRLIFT_DEST
          || hover_state == HOVER_DELAYED_AIRLIFT) {
        switch (keyval) {
        case GDK_4:
          airlift_queue_need_city_for = 1;
          return TRUE;

        case GDK_5:
          airlift_queue_need_city_for = 2;
          return TRUE;

        case GDK_6:
          airlift_queue_need_city_for = 3;
          return TRUE;

        case GDK_7:
          airlift_queue_need_city_for = 4;
          return TRUE;

        case GDK_8:
          airlift_queue_need_city_for = 5;
          return TRUE;

        case GDK_9:
          airlift_queue_need_city_for = 6;
          return TRUE;

        default:
          break;
        }
      }
      switch (keyval) {
      case GDK_1:
        multi_select_copy(0, 1);
        return TRUE;

      case GDK_2:
        multi_select_copy(0, 2);
        return TRUE;

      case GDK_3:
        multi_select_copy(0, 3);
        return TRUE;

      case GDK_4:
        multi_select_copy(0, 4);
        return TRUE;

      case GDK_5:
        multi_select_copy(0, 5);
        return TRUE;

      case GDK_6:
        multi_select_copy(0, 6);
        return TRUE;

      case GDK_7:
        multi_select_copy(0, 7);
        return TRUE;

      case GDK_8:
        multi_select_copy(0, 8);
        return TRUE;

      case GDK_9:
        multi_select_copy(0, 9);
        return TRUE;

      default:
        break;
      }
    }

    if ((ev->state & GDK_SHIFT_MASK)) {
      switch (ev->keyval) {
      case GDK_Left:
        scroll_mapview(DIR8_WEST);
        return TRUE;

      case GDK_Right:
        scroll_mapview(DIR8_EAST);
        return TRUE;

      case GDK_Up:
        scroll_mapview(DIR8_NORTH);
        return TRUE;

      case GDK_Down:
        scroll_mapview(DIR8_SOUTH);
        return TRUE;

      case GDK_Home:
        key_center_capital();
        return TRUE;

      case GDK_Return:
      case GDK_KP_Enter:
        key_end_turn();
        return TRUE;

      default:
        break;
      }
    }

    switch (ev->keyval) {
    case GDK_KP_Up:
    case GDK_KP_8:
    case GDK_8:
      key_unit_move(DIR8_NORTH);
      return TRUE;

    case GDK_KP_Page_Up:
    case GDK_KP_9:
    case GDK_9:
      key_unit_move(DIR8_NORTHEAST);
      return TRUE;

    case GDK_KP_Right:
    case GDK_KP_6:
    case GDK_6:
      key_unit_move(DIR8_EAST);
      return TRUE;

    case GDK_KP_Page_Down:
    case GDK_KP_3:
    case GDK_3:
      key_unit_move(DIR8_SOUTHEAST);
      return TRUE;

    case GDK_KP_Down:
    case GDK_KP_2:
    case GDK_2:
      key_unit_move(DIR8_SOUTH);
      return TRUE;

    case GDK_KP_End:
    case GDK_KP_1:
    case GDK_1:
      key_unit_move(DIR8_SOUTHWEST);
      return TRUE;

    case GDK_KP_Left:
    case GDK_KP_4:
    case GDK_4:
      key_unit_move(DIR8_WEST);
      return TRUE;

    case GDK_KP_Home:
    case GDK_KP_7:
    case GDK_7:
      key_unit_move(DIR8_NORTHWEST);
      return TRUE;

    case GDK_KP_5:
    case GDK_KP_Begin:
    case GDK_5:
      key_recall_previous_focus_unit();
      return TRUE;

    case GDK_Escape:
      key_cancel_action();
      return TRUE;

    case GDK_n: /* shared by MENU_VIEW_SHOW_CITY_NAMES */
      if (tiles_hilited_cities && (ev->state & GDK_CONTROL_MASK)) {
        printf( "keyboard_handler(), GDK_n\n");
        normalize_names_in_selected_cities();
        return TRUE;
      } else {
        return FALSE;
      }
      break;

    case GDK_KP_Divide:
      key_quickselect(SELECT_SEA);
      return TRUE;

    case GDK_KP_Multiply:
      key_quickselect(SELECT_LAND);
      return TRUE;

    case GDK_b:
      if (tiles_hilited_cities) {
        buy_production_in_selected_cities();
        return TRUE;
      } else {
        return FALSE;
      }
      break;

    default:
      break;
    }
  }
  return FALSE;
}

/**************************************************************************
 reattaches the detached widget when the user destroys it.
**************************************************************************/
static void tearoff_destroy(GtkWidget *w, gpointer data)
{
  GtkWidget *p, *b, *box;

  box = GTK_WIDGET(data);
  p = g_object_get_data(G_OBJECT(w), "parent");
  b = g_object_get_data(G_OBJECT(w), "toggle");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), FALSE);

  gtk_widget_hide(w);
  gtk_widget_reparent(box, p);
}

/**************************************************************************
 propagates a keypress in a tearoff back to the toplevel window.
**************************************************************************/
static gboolean tearoff_keypress_callback (GtkWidget *w,
                                           GdkEventKey *ev)
{
  gtk_widget_event(toplevel, (GdkEvent *)ev);
  return FALSE;
}

/**************************************************************************
  Toggles the tearoff window for box depending on the state of toggle.
  If propagate_keypress is TRUE, key press events are propagated to the
  toplevel window.
**************************************************************************/
static void toggle_tearoff_window (GtkWidget *box,
                                   GtkWidget *toggle,
                                   gboolean propagate_keypress)
{
  GtkWidget *w;
  gboolean active;

  active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle));

  if (active) {
    w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setup_dialog(w, toplevel);
    gtk_widget_set_name(w, "Freeciv");
    gtk_window_set_title(GTK_WINDOW(w), _("Freeciv"));
    gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_MOUSE);
    g_signal_connect(w, "destroy", G_CALLBACK(tearoff_destroy), box);

    if (propagate_keypress) {
      g_signal_connect (w, "key_press_event",
                        G_CALLBACK (tearoff_keypress_callback), NULL);
    }

    g_object_set_data(G_OBJECT(w), "parent", box->parent);
    g_object_set_data (G_OBJECT (w), "toggle", toggle);
    gtk_widget_reparent(box, w);
    gtk_widget_show(w);
  } else {
    gtk_widget_destroy(box->parent);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void tearoff_callback_no_propagate (GtkWidget *b, gpointer data)
{
  toggle_tearoff_window (GTK_WIDGET (data), b, FALSE);
}

/**************************************************************************
 callback for the toggle button in the detachable widget: causes the
 widget to detach or reattach.
**************************************************************************/
static void tearoff_callback (GtkWidget *b, gpointer data)
{
  toggle_tearoff_window (GTK_WIDGET (data), b, TRUE);
}

/**************************************************************************
 create the container for the widget that's able to be detached
**************************************************************************/
static GtkWidget *detached_widget_new(void)
{
  return gtk_hbox_new(FALSE, 2);
}

/**************************************************************************
 Creates the toggle button necessary to detach and reattach the widget
 and returns a vbox in which you fill your goodies. If propagate_keypress
 is true, the "torn off" widget will propagate keypress events to the
 main window.
**************************************************************************/
static GtkWidget *detached_widget_fill (GtkWidget *ahbox,
                                        gboolean propagate_keypress)
{
  GtkWidget *b, *avbox;

  b = gtk_toggle_button_new();
  gtk_box_pack_start(GTK_BOX(ahbox), b, FALSE, FALSE, 0);
  if (propagate_keypress) {
    g_signal_connect (b, "toggled",
                      G_CALLBACK (tearoff_callback), ahbox);
  } else {
    g_signal_connect (b, "toggled",
                      G_CALLBACK (tearoff_callback_no_propagate), ahbox);
  }

  avbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(ahbox), avbox, TRUE, TRUE, 0);
  return avbox;
}

/**************************************************************************
  Called to build the unit_below pixmap table.  This is the table on the
  left of the screen that shows all of the inactive units in the current
  tile.

  It may be called again if the tileset changes.
**************************************************************************/
static void populate_unit_pixmap_table(void)
{
  int i;
  GtkWidget *table = unit_pixmap_table;

  /* 135 below is rough value (could be more intelligent) --dwp */
  num_units_below = 135 / (int) NORMAL_TILE_WIDTH;
  num_units_below = CLIP(1, num_units_below, MAX_NUM_UNITS_BELOW);

  gtk_table_resize(GTK_TABLE(table), 2, num_units_below);

  /* Note, we ref this and other widgets here so that we can unref them
   * in reset_unit_table. */
  unit_pixmap = gtk_pixcomm_new(UNIT_TILE_WIDTH, UNIT_TILE_HEIGHT);
  gtk_widget_ref(unit_pixmap);
  gtk_pixcomm_clear(GTK_PIXCOMM(unit_pixmap));
  unit_pixmap_button = gtk_event_box_new();
  gtk_widget_ref(unit_pixmap_button);
  gtk_container_add(GTK_CONTAINER(unit_pixmap_button), unit_pixmap);
  gtk_table_attach_defaults(GTK_TABLE(table), unit_pixmap_button, 0, 1, 0, 1);
  g_signal_connect(unit_pixmap_button, "button_press_event",
                   G_CALLBACK(select_unit_pixmap_callback),
                   GINT_TO_POINTER(-1));

  for (i = 0; i < num_units_below; i++) {
    unit_below_pixmap[i] = gtk_pixcomm_new(UNIT_TILE_WIDTH,
                                           UNIT_TILE_HEIGHT);
    gtk_widget_ref(unit_below_pixmap[i]);
    unit_below_pixmap_button[i] = gtk_event_box_new();
    gtk_widget_ref(unit_below_pixmap_button[i]);
    gtk_container_add(GTK_CONTAINER(unit_below_pixmap_button[i]),
                      unit_below_pixmap[i]);
    g_signal_connect(unit_below_pixmap_button[i],
                     "button_press_event",
                     G_CALLBACK(select_unit_pixmap_callback),
                     GINT_TO_POINTER(i));

    gtk_table_attach_defaults(GTK_TABLE(table), unit_below_pixmap_button[i],
                              i, i + 1, 1, 2);
    gtk_widget_set_size_request(unit_below_pixmap[i],
                                UNIT_TILE_WIDTH, UNIT_TILE_HEIGHT);
    gtk_pixcomm_clear(GTK_PIXCOMM(unit_below_pixmap[i]));
  }

  more_arrow_pixmap = gtk_image_new_from_pixmap(sprites.right_arrow->pixmap,
                                                NULL);
  gtk_widget_ref(more_arrow_pixmap);
  gtk_table_attach_defaults(GTK_TABLE(table), more_arrow_pixmap, 4, 5, 1, 2);

  gtk_widget_show_all(table);
}

/**************************************************************************
  Called when the tileset is changed to reset the unit pixmap table.
**************************************************************************/
void reset_unit_table(void)
{
  int i;

  /* Unreference all of the widgets that we're about to reallocate, thus
   * avoiding a memory leak. Remove them from the container first, just
   * to be safe. Note, the widgets are ref'd in
   * populatate_unit_pixmap_table. */
  gtk_container_remove(GTK_CONTAINER(unit_pixmap_table),
                       unit_pixmap_button);
  gtk_widget_unref(unit_pixmap);
  gtk_widget_unref(unit_pixmap_button);
  for (i = 0; i < num_units_below; i++) {
    gtk_container_remove(GTK_CONTAINER(unit_pixmap_table),
                         unit_below_pixmap_button[i]);
    gtk_widget_unref(unit_below_pixmap[i]);
    gtk_widget_unref(unit_below_pixmap_button[i]);
  }
  gtk_container_remove(GTK_CONTAINER(unit_pixmap_table),
                       more_arrow_pixmap);
  gtk_widget_unref(more_arrow_pixmap);

  populate_unit_pixmap_table();

  /* We have to force a redraw of the units.  And we explicitly have
   * to force a redraw of the focus unit, which is normally only
   * redrawn when the focus changes. We also have to force the 'more'
   * arrow to go away, both by expicitly hiding it and telling it to
   * do so (this will be reset immediately afterwards if necessary,
   * but we have to make the *internal* state consistent). */
  gtk_widget_hide(more_arrow_pixmap);
  set_unit_icons_more_arrow(FALSE);
  set_unit_icon(-1, get_unit_in_focus());
  update_unit_pix_label(get_unit_in_focus());
}

/**************************************************************************
  Enable/Disable the game page menu bar.
**************************************************************************/
void enable_menus(bool enable)
{
  if (enable) {
    setup_menus(toplevel, &main_menubar);
    gtk_box_pack_start(GTK_BOX(top_vbox), main_menubar, FALSE, FALSE, 0);
    gtk_widget_show_all(main_menubar);
  } else {
    gtk_widget_destroy(main_menubar);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void allied_chat_button_toggled(GtkToggleButton *button,
                                       gpointer user_data)
{
  allied_chat_only
    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
}

/**************************************************************************
  ...
**************************************************************************/
static void request_more_time_callback(void)
{
  int new_timeout = game.info.timeout + 20;
  char buf[256];
  my_snprintf(buf, sizeof(buf), "/set timeout %d --- I need more time!",
              new_timeout);
  send_chat(buf);
}

/**************************************************************************
  ...
**************************************************************************/
static void request_pause_callback(void)
{
  send_chat("/set timeout 600 --- I need a pause!");
}

/**************************************************************************
 do the heavy lifting for the widget setup.
**************************************************************************/
static void setup_widgets(void)
{
  GtkWidget *box, *ebox, *hbox, *hbox2, *sbox, *vbox, *avbox, *ahbox;
  GtkWidget *align, *label, *button;
  GtkWidget *frame, *table, *table2, *paned, *sw, *text, *splitmsgs;
  GtkStyle *style;
  GList *focus_chain = NULL;
  int i;
  char buf[256];
  struct Sprite *sprite;

  GtkWidget *notebook, *statusbar, *toplevel_vpaned, *overview_box;

  message_buffer = gtk_text_buffer_new(NULL);
  refresh_message_buffer_tag_patterns (NULL);
  network_message_buffer = gtk_text_buffer_new (NULL);

  notebook = gtk_notebook_new();

  /* stop mouse wheel notebook page switching. */
  g_signal_connect(notebook, "scroll_event",
                   G_CALLBACK(gtk_true), NULL);

  toplevel_tabs = notebook;
  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
  gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
  box = gtk_vbox_new(FALSE, 4);
  gtk_container_add(GTK_CONTAINER(toplevel), box);
  gtk_box_pack_start(GTK_BOX(box), notebook, TRUE, TRUE, 0);
  statusbar = create_statusbar();
  gtk_box_pack_start(GTK_BOX(box), statusbar, FALSE, FALSE, 0);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           create_main_page(), NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           create_start_page(), NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           create_scenario_page(), NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           create_load_page(), NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           create_network_page(), NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           create_nation_page(), NULL);

  main_tips = gtk_tooltips_new();

  /* *** everything in the top *** */

  top_vbox = gtk_vbox_new(FALSE, 5);
  hbox = gtk_hbox_new(FALSE, 0);
  paned = gtk_vpaned_new();
  toplevel_vpaned = paned;

  if (small_display_layout) {
    /* The window is divided into two horizontal panels: overview +
     * civinfo + unitinfo, main view + message window. */
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), top_vbox, NULL);
    gtk_box_pack_end(GTK_BOX(top_vbox), hbox, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), paned, TRUE, TRUE, 0);
  } else {
    /* The window is divided into two vertical panes: overview +
     * + civinfo + unitinfo + main view, message window. */
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), paned, NULL);
    gtk_paned_pack1(GTK_PANED(paned), top_vbox, TRUE, FALSE);
    gtk_box_pack_end(GTK_BOX(top_vbox), hbox, TRUE, TRUE, 0);
  }

  /* this holds the overview canvas, production info, etc. */
  vbox = gtk_vbox_new(FALSE, 3);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

  /* overview canvas */
  ahbox = detached_widget_new();
  gtk_box_pack_start(GTK_BOX(vbox), ahbox, FALSE, FALSE, 0);
  avbox = detached_widget_fill(ahbox, TRUE);

  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(avbox), align, FALSE, FALSE, 0);

  overview_box = gtk_vbox_new(FALSE, 4);
  gtk_container_add(GTK_CONTAINER(align), overview_box);

  ebox = gtk_event_box_new();
  gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(ebox, "button_press_event",
                   G_CALLBACK(player_colors_mode_label_click), NULL);
  gtk_box_pack_start(GTK_BOX(overview_box), ebox, FALSE, FALSE, 0);

  hbox2 = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(ebox), hbox2);

  label = gtk_label_new(_("Colors:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(hbox2), label, TRUE, TRUE, 0);
  player_colors_mode_label = label;
  update_player_colors_mode_label();

  overview_canvas = gtk_drawing_area_new();
  gtk_widget_set_size_request(overview_canvas, 160, 100);
  gtk_widget_add_events(overview_canvas, GDK_EXPOSURE_MASK
                        |GDK_BUTTON_PRESS_MASK
                        |GDK_POINTER_MOTION_MASK);
  g_signal_connect(overview_canvas, "expose_event",
                   G_CALLBACK(overview_canvas_expose), NULL);
  g_signal_connect(overview_canvas, "motion_notify_event",
                   G_CALLBACK(move_overviewcanvas), NULL);
  g_signal_connect(overview_canvas, "button_press_event",
                   G_CALLBACK(butt_down_overviewcanvas), NULL);
  gtk_box_pack_start(GTK_BOX(overview_box), overview_canvas,
                     TRUE, TRUE, 0);

  /* The rest */

  ahbox = detached_widget_new();
  gtk_box_pack_start(GTK_BOX(vbox), ahbox, TRUE, TRUE, 0);
  avbox = detached_widget_fill(ahbox, TRUE);

  /* Info on player's civilization, when game is running. */
  frame = gtk_frame_new("");
  gtk_box_pack_start(GTK_BOX(avbox), frame, FALSE, FALSE, 0);

  main_frame_civ_name = frame;

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  ebox = gtk_event_box_new();
  gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);

  gtk_box_pack_start(GTK_BOX(vbox), ebox, FALSE, FALSE, 0);

  main_label_info = gtk_label_new("\n\n\n\n");
  gtk_container_add(GTK_CONTAINER(ebox), main_label_info);
  g_signal_connect(ebox, "button_press_event",
                   G_CALLBACK(show_info_popup), NULL);

  /* Production status */

  /* make a box so the table will be centered */
  box = gtk_hbox_new(FALSE, 0);

  gtk_box_pack_start(GTK_BOX(avbox), box, FALSE, FALSE, 0);

  table = gtk_table_new(3, 10, TRUE);
  gtk_table_set_row_spacing(GTK_TABLE(table), 0, 0);
  gtk_table_set_col_spacing(GTK_TABLE(table), 0, 0);
  gtk_box_pack_start(GTK_BOX(box), table, TRUE, FALSE, 0);
  info_table = table;

  /* citizens for taxrates */
  ebox = gtk_event_box_new();
  gtk_table_attach_defaults(GTK_TABLE(table), ebox, 0, 10, 0, 1);
  econ_ebox = ebox;

  table2 = gtk_table_new(1, 10, TRUE);
  gtk_table_set_row_spacing(GTK_TABLE(table2), 0, 0);
  gtk_table_set_col_spacing(GTK_TABLE(table2), 0, 0);
  gtk_container_add(GTK_CONTAINER(ebox), table2);

  for (i = 0; i < 10; i++) {
    ebox = gtk_event_box_new();
    gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);

    gtk_table_attach_defaults(GTK_TABLE(table2), ebox, i, i + 1, 0, 1);

    g_signal_connect(ebox, "button_press_event",
                     G_CALLBACK(taxrates_callback), GINT_TO_POINTER(i));

    sprite = i < 5 ? sprites.tax_science : sprites.tax_gold;
    econ_label[i] = gtk_image_new_from_pixmap(sprite->pixmap, sprite->mask);
    gtk_container_add(GTK_CONTAINER(ebox), econ_label[i]);
  }

  /* science, environmental, govt, timeout */
  bulb_label = gtk_image_new_from_pixmap(sprites.bulb[0]->pixmap, NULL);
  sun_label = gtk_image_new_from_pixmap(sprites.warming[0]->pixmap, NULL);
  flake_label = gtk_image_new_from_pixmap(sprites.cooling[0]->pixmap, NULL);
  {
    /* HACK: the UNHAPPY citizen is used for the government
     * when we don't know any better. */
    struct citizen_type c = {.type = CITIZEN_UNHAPPY};

    sprite = get_citizen_sprite(c, 0, NULL);
  }
  government_label = gtk_image_new_from_pixmap(sprite->pixmap, sprite->mask);

  for (i = 0; i < 4; i++) {
    GtkWidget *w;

    ebox = gtk_event_box_new();

    switch (i) {
    case 0:
      w = bulb_label;
      bulb_ebox = ebox;
      break;
    case 1:
      w = sun_label;
      sun_ebox = ebox;
      break;
    case 2:
      w = flake_label;
      flake_ebox = ebox;
      break;
    default:
    case 3:
      w = government_label;
      government_ebox = ebox;
      break;
    }

    gtk_misc_set_alignment(GTK_MISC(w), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(w), 0, 0);
    gtk_container_add(GTK_CONTAINER(ebox), w);
    gtk_table_attach_defaults(GTK_TABLE(table), ebox, i, i + 1, 1, 2);
  }

  timeout_label = gtk_label_new("");

  frame = gtk_frame_new(NULL);
  gtk_widget_set_size_request(frame, SMALL_TILE_WIDTH, SMALL_TILE_HEIGHT);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 4, 10, 1, 2);
  gtk_container_add(GTK_CONTAINER(frame), timeout_label);


  /* turn done */
  turn_done_button = gtk_button_new_with_label(_("Turn Done"));
  gtk_button_set_focus_on_click(GTK_BUTTON(turn_done_button),
                                FALSE);

  /* the turn done button must have its own style. otherwise when we flash
     the turn done button other widgets may flash too. */
  if (!(style = gtk_rc_get_style(turn_done_button))) {
    style = turn_done_button->style;
  }
  gtk_widget_set_style(turn_done_button, gtk_style_copy(style));

  gtk_table_attach_defaults(GTK_TABLE(table), turn_done_button, 0, 10, 2, 3);

  g_signal_connect(turn_done_button, "clicked",
                   G_CALLBACK(end_turn_callback), NULL);

  my_snprintf(buf, sizeof(buf), "%s:\n%s", _("Turn Done"), _("Shift+Return"));
  gtk_tooltips_set_tip(main_tips, turn_done_button, buf, "");

  /* Make the global observer version of the info table. */
  box = gtk_hbox_new(FALSE, 0);

  gtk_box_pack_start(GTK_BOX(avbox), box, FALSE, FALSE, 0);

  table = gtk_table_new(1, 10, TRUE);
  gtk_table_set_row_spacing(GTK_TABLE(table), 0, 0);
  gtk_table_set_col_spacing(GTK_TABLE(table), 0, 0);
  gtk_box_pack_start(GTK_BOX(box), table, TRUE, FALSE, 0);
  go_table = table;

  /* environmental, timeout */
  ebox = gtk_event_box_new();
  go_sun_label = gtk_image_new_from_pixmap(sprites.warming[0]->pixmap, NULL);
  gtk_misc_set_alignment(GTK_MISC(go_sun_label), 0.0, 0.0);
  gtk_misc_set_padding(GTK_MISC(go_sun_label), 0, 0);
  gtk_container_add(GTK_CONTAINER(ebox), go_sun_label);
  gtk_table_attach_defaults(GTK_TABLE(table), ebox, 0, 1, 0, 1);
  go_sun_ebox = ebox;

  ebox = gtk_event_box_new();
  go_flake_label = gtk_image_new_from_pixmap(sprites.cooling[0]->pixmap, NULL);
  gtk_misc_set_alignment(GTK_MISC(go_flake_label), 0.0, 0.0);
  gtk_misc_set_padding(GTK_MISC(go_flake_label), 0, 0);
  gtk_container_add(GTK_CONTAINER(ebox), go_flake_label);
  gtk_table_attach_defaults(GTK_TABLE(table), ebox, 1, 2, 0, 1);
  go_flake_ebox = ebox;

  go_timeout_label = gtk_label_new("");

  frame = gtk_frame_new(NULL);
  gtk_widget_set_size_request(frame, SMALL_TILE_WIDTH, SMALL_TILE_HEIGHT);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 2, 10, 0, 1);
  gtk_container_add(GTK_CONTAINER(frame), go_timeout_label);

  /* Selected unit status */
  unit_info_frame = gtk_frame_new("");
  gtk_box_pack_start(GTK_BOX(avbox), unit_info_frame, FALSE, FALSE, 0);

  unit_info_label = gtk_label_new("\n\n\n\n\n");
  gtk_container_add(GTK_CONTAINER(unit_info_frame), unit_info_label);

  box = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(avbox), box, FALSE, FALSE, 0);

  table = gtk_table_new(0, 0, FALSE);
  gtk_box_pack_start(GTK_BOX(box), table, FALSE, FALSE, 5);

  gtk_table_set_row_spacings(GTK_TABLE(table), 2);
  gtk_table_set_col_spacings(GTK_TABLE(table), 2);

  unit_pixmap_table = table;
  populate_unit_pixmap_table();

  /* The top notebook containing the map view and dialogs. */

  top_notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(top_notebook), GTK_POS_BOTTOM);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(top_notebook), TRUE);
  if (small_display_layout) {
    gtk_paned_pack1(GTK_PANED(paned), top_notebook, TRUE, TRUE);
  } else {
    gtk_box_pack_start(GTK_BOX(hbox), top_notebook, TRUE, TRUE, 0);
  }

  /* Map canvas and scrollbars */

  table = gtk_table_new(2, 2, FALSE);
  label = gtk_label_new_with_mnemonic(_("_Map"));

  /* NB: callback_report_map in menu.c relies on
   * the map tab being at position 0. */
  gtk_notebook_insert_page(GTK_NOTEBOOK(top_notebook), table, label, 0);
  focus_chain = g_list_append(focus_chain, top_notebook);

  frame = gtk_frame_new(NULL);
  gtk_table_attach(GTK_TABLE(table), frame, 0, 1, 0, 1,
                   GTK_EXPAND|GTK_SHRINK|GTK_FILL,
                   GTK_EXPAND|GTK_SHRINK|GTK_FILL, 0, 0);

  map_canvas = gtk_drawing_area_new();
  GTK_WIDGET_SET_FLAGS(map_canvas, GTK_CAN_FOCUS);

  for (i = 0; i < 5; i++) {
    gtk_widget_modify_bg(GTK_WIDGET(overview_canvas), i,
                         colors_standard[COLOR_STD_BLACK]);
    gtk_widget_modify_bg(GTK_WIDGET(map_canvas), i,
                         colors_standard[COLOR_STD_BLACK]);
  }

  gtk_widget_add_events(map_canvas, GDK_EXPOSURE_MASK
                        | GDK_BUTTON_PRESS_MASK
                        | GDK_BUTTON_RELEASE_MASK
                        | GDK_KEY_PRESS_MASK
                        | GDK_POINTER_MOTION_MASK);

  gtk_widget_set_size_request(map_canvas, 510, 300);
  gtk_container_add(GTK_CONTAINER(frame), map_canvas);

  map_horizontal_scrollbar = gtk_hscrollbar_new(NULL);
  gtk_table_attach(GTK_TABLE(table), map_horizontal_scrollbar, 0, 1, 1, 2,
                   GTK_EXPAND|GTK_SHRINK|GTK_FILL, 0, 0, 0);

  map_vertical_scrollbar = gtk_vscrollbar_new(NULL);
  gtk_table_attach(GTK_TABLE(table), map_vertical_scrollbar, 1, 2, 0, 1,
                   0, GTK_EXPAND|GTK_SHRINK|GTK_FILL, 0, 0);

  g_signal_connect(map_canvas, "expose_event",
                   G_CALLBACK(map_canvas_expose), NULL);

  g_signal_connect(map_canvas, "configure_event",
                   G_CALLBACK(map_canvas_configure), NULL);

  g_signal_connect(map_canvas, "motion_notify_event",
                   G_CALLBACK(move_mapcanvas), NULL);

  g_signal_connect(map_canvas, "button_press_event",
                   G_CALLBACK(butt_down_mapcanvas), NULL);

  g_signal_connect(map_canvas, "button_release_event",
                   G_CALLBACK(butt_release_mapcanvas), NULL);

  g_signal_connect(toplevel, "key_press_event",
                   G_CALLBACK(keyboard_handler), NULL);

  /* *** The message window -- this is a detachable widget *** */

  sbox = detached_widget_new();
  gtk_paned_pack2(GTK_PANED(paned), sbox, TRUE, TRUE);
  avbox = detached_widget_fill(sbox, FALSE);

  vbox = gtk_vbox_new(FALSE, 0);
  if (ingame_votebar == NULL) {
    ingame_votebar = create_voteinfo_bar();
  }
  gtk_box_pack_start(GTK_BOX(vbox), ingame_votebar->box,
                     FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(avbox), vbox, TRUE, TRUE, 0);

  paned = gtk_hpaned_new();
  gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 4);

  /* split message window */
  splitmsgs = get_split_message_window();
  gtk_paned_pack2(GTK_PANED(paned), splitmsgs, TRUE, TRUE);

  /* botton notebook */
  bottom_notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(bottom_notebook), GTK_POS_TOP);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(bottom_notebook), TRUE);
  gtk_paned_pack1(GTK_PANED(paned), bottom_notebook, TRUE, TRUE);

  vbox = gtk_vbox_new(FALSE, 0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_ALWAYS);
  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

  label = gtk_label_new_with_mnemonic(_("_Chat"));
  gtk_notebook_append_page(GTK_NOTEBOOK(bottom_notebook), vbox, label);

  text = gtk_text_view_new_with_buffer(message_buffer);
  set_message_buffer_view_link_handlers (GTK_TEXT_VIEW (text));
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD_CHAR);
  gtk_widget_set_size_request(text, 100, 60);
  gtk_container_add(GTK_CONTAINER(sw), text);

  gtk_widget_set_name(text, "chatline");

  gtk_widget_realize(text);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 5);

  main_message_area = GTK_TEXT_VIEW(text);

  set_output_window_text(_("Freeciv is free software and you are welcome"
                           " to distribute copies of it\nunder certain"
                           " conditions; See the \"Copying\" item on"
                           " the Help menu.\nNow.. Go give'em hell!"));

  /* the chat line */
  hbox = gtk_hbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

  inputline = gtk_entry_new();
  focus_chain = g_list_append(focus_chain, inputline);
  g_signal_connect(inputline, "activate",
                   G_CALLBACK(inputline_return), NULL);
  g_signal_connect(inputline, "key_press_event",
                   G_CALLBACK(inputline_handler), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), inputline, TRUE, TRUE, 0);

  button = gtk_toggle_button_new_with_label(_("Allies Only"));
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  allied_chat_toggle_button = button;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                               allied_chat_only);
  g_signal_connect(button, "toggled",
                   G_CALLBACK(allied_chat_button_toggled), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  button = gtk_button_new_with_label(_("More Time!"));
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(request_more_time_callback), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  more_time_button = button;

  button = gtk_button_new_with_label(_("Request Pause"));
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(request_pause_callback), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  pause_button = button;

  button = gtk_button_new_with_label(_("Clear links"));
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  g_signal_connect(button, "clicked", G_CALLBACK(clear_all_link_marks), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  /* Only allow tab-focusing between the chat entry and
   * the main notebook. */
  gtk_container_set_focus_chain(GTK_CONTAINER(toplevel_vpaned),
                                focus_chain);
  g_list_free(focus_chain);

  /* Other things to take care of */

  gtk_widget_show_all(gtk_bin_get_child(GTK_BIN(toplevel)));
  gtk_widget_hide(more_arrow_pixmap);

  if (!show_split_message_window) {
    gtk_widget_hide(splitmsgs);
  }

  if (enable_tabs) {
    popup_meswin_dialog();
  }

  gtk_notebook_set_current_page(GTK_NOTEBOOK(top_notebook), 0);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(bottom_notebook), 0);

  if (!map_scrollbars) {
    gtk_widget_hide(map_horizontal_scrollbar);
    gtk_widget_hide(map_vertical_scrollbar);
  }
}

/**************************************************************************
 called from main().
**************************************************************************/
void ui_init(void)
{
  gchar *s;

  init_character_encodings("UTF-8", FALSE);

  log_set_callback(log_callback_utf8);

  /* convert inputs */
  s = g_locale_to_utf8(default_user_name, -1, NULL, NULL, NULL);
  sz_strlcpy(default_user_name, s);
  g_free(s);

  /* this is silly, but i don't want the UI to barf on erroneous input */
  s = g_locale_to_utf8(default_metaserver, -1, NULL, NULL, NULL);
  sz_strlcpy(default_metaserver, s);
  g_free(s);

  s = g_locale_to_utf8(server_host, -1, NULL, NULL, NULL);
  sz_strlcpy(server_host, s);
  g_free(s);
}

/**************************************************************************
 called from main(), is what it's named.
**************************************************************************/
void ui_main(int argc, char **argv)
{
  GdkBitmap *icon_bitmap;
  const gchar *home;
  guint sig;
  GtkStyle *style;
  GtkSettings *gtksettings;
  GdkScreen *screen;

  parse_options(argc, argv);

  /* the locale has already been set in init_nls() and the Win32-specific
   * locale logic in gtk_init() causes problems with zh_CN (see PR#39475) */
  gtk_disable_setlocale();

  /* GTK withdraw gtk options. Process GTK arguments */
  gtk_init(&argc, &argv);

  /* Load resources */
  gtk_rc_parse_string(fallback_resources);

  home = g_get_home_dir();
  if (home) {
    gchar *str;

    str = g_build_filename(home, ".warciv.rc-2.0", NULL);
    gtk_rc_parse(str);
    g_free(str);
  }

  toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(toplevel, "key_press_event",
                   G_CALLBACK(toplevel_handler), NULL);

  g_signal_connect(toplevel, "configure_event",
                   G_CALLBACK(toplevel_configure), NULL);

  gtk_window_set_role(GTK_WINDOW(toplevel), "toplevel");
  gtk_widget_realize(toplevel);
  gtk_widget_set_name(toplevel, "Warciv");
  root_window = toplevel->window;

  if (fullscreen_mode) {
    gtk_window_fullscreen(GTK_WINDOW(toplevel));
  }

  char version[256];
  my_snprintf(version, sizeof(version), "%s %s",
              freeciv_name_version(), warclient_name_version());
  gtk_window_set_title(GTK_WINDOW (toplevel), version);

  g_signal_connect(toplevel, "delete_event",
                   G_CALLBACK(quit_dialog_callback), NULL);

  /* Disable GTK+ cursor key focus movement */
  sig = g_signal_lookup("focus", GTK_TYPE_WIDGET);
  g_signal_handlers_disconnect_matched(toplevel, G_SIGNAL_MATCH_ID, sig,
                                       0, 0, 0, 0);
  g_signal_connect(toplevel, "focus", G_CALLBACK(toplevel_focus), NULL);


  display_color_type = get_visual();
  init_color_system();

  icon_bitmap = gdk_bitmap_create_from_data(root_window, freeciv_bits,
                                            freeciv_width, freeciv_height);
  gdk_window_set_icon(root_window, NULL, icon_bitmap, icon_bitmap);

  civ_gc = gdk_gc_new(root_window);

  screen = gdk_screen_get_default();
  gtksettings = gtk_settings_get_for_screen(screen);
  /* kill gtk's default F10 handler */
  g_object_set(G_OBJECT(gtksettings), "gtk-menu-bar-accel", NULL, NULL);

  /* font names shouldn't be in spec files! */
  style = gtk_rc_get_style_by_paths(gtksettings,
                                    "Freeciv*.city names",
                                    NULL, G_TYPE_NONE);
  if (!style) {
    style = gtk_style_new();
  }
  g_object_ref(style);
  main_font = style->font_desc;

  style = gtk_rc_get_style_by_paths(gtksettings,
                                    "Freeciv*.city productions",
                                    NULL, G_TYPE_NONE);
  if (!style) {
    style = gtk_style_new();
  }
  g_object_ref(style);
  city_productions_font = style->font_desc;

  fill_bg_gc = gdk_gc_new(root_window);

  /* for isometric view. always create. the tileset can change at run time. */
  thin_line_gc = gdk_gc_new(root_window);
  thick_line_gc = gdk_gc_new(root_window);
  border_line_gc = gdk_gc_new(root_window);
  gdk_gc_set_line_attributes(thin_line_gc, 1,
                             GDK_LINE_SOLID,
                             GDK_CAP_NOT_LAST,
                             GDK_JOIN_MITER);
  gdk_gc_set_line_attributes(thick_line_gc, 2,
                             GDK_LINE_SOLID,
                             GDK_CAP_NOT_LAST,
                             GDK_JOIN_MITER);
  gdk_gc_set_line_attributes(border_line_gc, BORDER_WIDTH,
                             GDK_LINE_ON_OFF_DASH,
                             GDK_CAP_NOT_LAST,
                             GDK_JOIN_MITER);

  fill_tile_gc = gdk_gc_new(root_window);
  gdk_gc_set_fill(fill_tile_gc, GDK_STIPPLED);

  {
    char d1[] = {0x03, 0x0c, 0x03, 0x0c};
    char d2[] = {0x08, 0x02, 0x08, 0x02};
    char d3[] = {0xAA, 0x55, 0xAA, 0x55};

    gray50 = gdk_bitmap_create_from_data(root_window, d1, 4, 4);
    gray25 = gdk_bitmap_create_from_data(root_window, d2, 4, 4);
    black50 = gdk_bitmap_create_from_data(root_window, d3, 4, 4);
  }

  {
    GdkColor pixel;

    mask_bitmap = gdk_pixmap_new(root_window, 1, 1, 1);

    mask_fg_gc = gdk_gc_new(mask_bitmap);
    pixel.pixel = 1;
    gdk_gc_set_foreground(mask_fg_gc, &pixel);
    gdk_gc_set_function(mask_fg_gc, GDK_OR);

    mask_bg_gc = gdk_gc_new(mask_bitmap);
    pixel.pixel = 0;
    gdk_gc_set_foreground(mask_bg_gc, &pixel);
  }

  tilespec_load_tiles();

  setup_widgets();
  load_intro_gfx();
  load_cursors();

  history_list = genlist_new();
  history_pos = -1;

  gtk_widget_show(toplevel);

  timer_id = gtk_timeout_add(TIMER_INTERVAL, timer_callback, NULL);

  init_mapcanvas_and_overview();

  set_client_state(CLIENT_PRE_GAME_STATE);

  gtk_main();

  free_color_system();
  tilespec_free_tiles();
}

/**************************************************************************
 Update the connected users list at pregame state.
**************************************************************************/
void update_conn_list_dialog(void)
{
  char buf[256];
  GtkTreeIter it;

  if (get_client_state() != CLIENT_GAME_RUNNING_STATE) {
    gtk_list_store_clear(conn_model);
    conn_list_iterate(game.est_connections, pconn) {
      gtk_list_store_append(conn_model, &it);
      if (pconn->observer) {
        my_snprintf(buf, sizeof(buf), "(%s)", pconn->username);
      } else if (pconn->player) {
        my_snprintf(buf, sizeof(buf), "%s", pconn->username);
      } else {
        my_snprintf(buf, sizeof(buf), "[%s]", pconn->username);
      }
      gtk_list_store_set(conn_model, &it, 0, buf, 1, pconn, -1);
    } conn_list_iterate_end;
  }
}

/**************************************************************************
 Show details about a user in the Connected Users dialog in a popup.
**************************************************************************/
gboolean show_conn_popup(GtkWidget *view, GdkEventButton *ev, gpointer data)
{
  GtkTreePath *path;
  GtkTreeIter it;
  GtkWidget *popup, *table, *label;
  gpointer p;
  struct connection *pconn;

  /* Get the current selection in the Connected Users list */
  if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(view),
                                     ev->x, ev->y, &path, NULL, NULL, NULL)) {
    return FALSE;
  }

  gtk_tree_model_get_iter(GTK_TREE_MODEL(conn_model), &it, path);
  gtk_tree_path_free(path);

  gtk_tree_model_get(GTK_TREE_MODEL(conn_model), &it, 1, &p, -1);
  pconn = (struct connection *)p;

  /* Show popup. */
  popup = gtk_window_new(GTK_WINDOW_POPUP);
  gtk_widget_set_app_paintable(popup, TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(popup), 4);
  gtk_window_set_position(GTK_WINDOW(popup), GTK_WIN_POS_MOUSE);

  table = gtk_table_new(2, 2, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(table), 6);
  gtk_container_add(GTK_CONTAINER(popup), table);

  label = gtk_label_new(_("Name:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
  label = gtk_label_new(pconn->username);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 0, 1);

  label = gtk_label_new(_("Host:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
  label = gtk_label_new(pconn->addr);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 1, 2);

  gtk_widget_show_all(table);
  gtk_widget_show(popup);

  gdk_pointer_grab(popup->window, TRUE, GDK_BUTTON_RELEASE_MASK,
                   NULL, NULL, ev->time);
  gtk_grab_add(popup);
  g_signal_connect_after(popup, "button_release_event",
                         G_CALLBACK(show_info_button_release), NULL);
  return FALSE;
}

/**************************************************************************
 obvious...
**************************************************************************/
void sound_bell(void)
{
  gdk_beep();
}

/**************************************************************************
  Set one of the unit icons in information area based on punit.
  Use punit==NULL to clear icon.
  Index 'idx' is -1 for "active unit", or 0 to (num_units_below-1) for
  units below.  Also updates unit_ids[idx] for idx>=0.
**************************************************************************/
void set_unit_icon(int idx, struct unit *punit)
{
  GtkWidget *w;

  assert(idx >= -1 && idx < num_units_below);

  if (idx == -1) {
    w = unit_pixmap;
  } else {
    w = unit_below_pixmap[idx];
    unit_ids[idx] = punit ? punit->id : 0;
  }

  gtk_pixcomm_freeze(GTK_PIXCOMM(w));

  if (punit) {
    put_unit_gpixmap(punit, GTK_PIXCOMM(w));
  } else {
    gtk_pixcomm_clear(GTK_PIXCOMM(w));
  }

  gtk_pixcomm_thaw(GTK_PIXCOMM(w));
}

/**************************************************************************
  Set the "more arrow" for the unit icons to on(1) or off(0).
  Maintains a static record of current state to avoid unnecessary redraws.
  Note initial state should match initial gui setup (off).
**************************************************************************/
void set_unit_icons_more_arrow(bool onoff)
{
  static bool showing = FALSE;

  if (onoff && !showing) {
    gtk_widget_show(more_arrow_pixmap);
    showing = TRUE;
  }
  else if(!onoff && showing) {
    gtk_widget_hide(more_arrow_pixmap);
    showing = FALSE;
  }
}

/**************************************************************************
 callback for clicking a unit icon underneath unit info box.
 these are the units on the same tile as the focus unit.
**************************************************************************/
static gboolean select_unit_pixmap_callback(GtkWidget *w, GdkEvent *ev,
                                            gpointer data)
{
  int i = GPOINTER_TO_INT(data);
  struct unit *punit;

  if (i == -1) {
    if ((punit = get_unit_in_focus())) {
      /* Clicking on the currently selected unit will center it. */
      center_tile_mapcanvas(punit->tile);
    }
    return TRUE;
  }

  if (unit_ids[i] == 0) /* no unit displayed at this place */
    return TRUE;

  punit = find_unit_by_id(unit_ids[i]);
  if(punit) { /* should always be true at this point */
    if (punit->owner == get_player_idx()) {  /* may be non-true if alliance */
      set_unit_focus_and_active(punit);
    }
  }

  return TRUE;
}

/**************************************************************************
 this is called every TIMER_INTERVAL milliseconds whilst we are in
 gtk_main() (which is all of the time) TIMER_INTERVAL needs to be .5s
**************************************************************************/
static gint timer_callback(gpointer data)
{
  real_timer_callback();
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static gboolean show_info_button_release(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
  gtk_grab_remove(w);
  gdk_pointer_ungrab(GDK_CURRENT_TIME);
  gtk_widget_destroy(w);
  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
static gboolean show_info_popup(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
  if (ev->button == 1 && !client_is_global_observer()) {
    GtkWidget *p;
    char buf[512];
    struct player *pplayer = get_player_ptr();

    my_snprintf(buf, sizeof(buf),
		_("%s People\nYear: %s Turn: %d\nGold: %d\nNet Income: %d\n"
		  "Tax:%d Lux:%d Sci:%d\nResearching %s: %d/%d\nGovernment: %s"),
		population_to_text(civ_population(pplayer)),
		textyear(game.info.year), game.info.turn,
		pplayer->economic.gold,
		player_get_expected_income(pplayer),
		pplayer->economic.tax,
		pplayer->economic.luxury,
		pplayer->economic.science,
		get_tech_name(pplayer, pplayer->research.researching),
		pplayer->research.bulbs_researched,
		total_bulbs_required(pplayer),
		get_government_name(pplayer->government));

    p = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_widget_set_app_paintable(p, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(p), 4);
    gtk_window_set_position(GTK_WINDOW(p), GTK_WIN_POS_MOUSE);

    gtk_widget_new(GTK_TYPE_LABEL, "GtkWidget::parent", p,
                   "GtkLabel::label", buf,
                   "GtkWidget::visible", TRUE,
                   NULL);
    gtk_widget_show(p);

    gdk_pointer_grab(p->window, TRUE, GDK_BUTTON_RELEASE_MASK,
                     NULL, NULL, ev->time);
    gtk_grab_add(p);

    g_signal_connect_after(p, "button_release_event",
                           G_CALLBACK(show_info_button_release), NULL);
  }
  return TRUE;
}

/**************************************************************************
 user clicked "Turn Done" button
**************************************************************************/
static void end_turn_callback(GtkWidget *w, gpointer data)
{
  gtk_widget_set_sensitive(turn_done_button, FALSE);
  user_ended_turn();
}

/**************************************************************************
...
**************************************************************************/
static gboolean get_net_input(GIOChannel *source,
                              GIOCondition cond,
                              gpointer data)
{
  input_from_server(GPOINTER_TO_INT(data));
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static void set_wait_for_writable_socket(struct connection *pc,
                                         bool socket_writable)
{
  static bool previous_state = FALSE;
  GIOChannel *gioc;
  GIOCondition cond = 0;

  assert(pc == &aconnection);

  if (previous_state == socket_writable)
    return;

  freelog(LOG_DEBUG, "set_wait_for_writable_socket(%d)", socket_writable);
  g_source_remove(input_id);

#ifdef WIN32_NATIVE
  gioc = g_io_channel_win32_new_socket(aconnection.sock);
#else
  gioc = g_io_channel_unix_new(aconnection.sock);
#endif

  cond = G_IO_IN | G_IO_PRI | (socket_writable ? G_IO_OUT : 0) | G_IO_ERR |
    G_IO_HUP | G_IO_NVAL;
  input_id = g_io_add_watch_full(gioc, G_PRIORITY_DEFAULT, cond,
                                 get_net_input, &aconnection.sock,
                                 NULL);
  previous_state = socket_writable;
}

/**************************************************************************
  ...
**************************************************************************/
void remove_timer_callback(int id)
{
  g_source_remove(id);
}

/**************************************************************************
  ...
**************************************************************************/
int add_timer_callback(int millisecond_interval,
                       timer_callback_t cb,
                       void *data)
{
  return g_timeout_add(millisecond_interval, (GSourceFunc) cb, data);
}

/**************************************************************************
 This function is called after the client succesfully
 has connected to the server
**************************************************************************/
void add_net_input(int sock)
{
  GIOChannel *gioc;
  GIOCondition cond = 0;

#ifdef WIN32_NATIVE
  gioc = g_io_channel_win32_new_socket(sock);
#else
  gioc = g_io_channel_unix_new(sock);
#endif
  cond = G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
  input_id = g_io_add_watch_full(gioc, G_PRIORITY_DEFAULT, cond,
                                 get_net_input, GINT_TO_POINTER(sock), NULL);
  aconnection.client.notify_of_writable_data = set_wait_for_writable_socket;
}

/**************************************************************************
  ...
**************************************************************************/
static gboolean gioc_input_ready(GIOChannel *source,
                                 GIOCondition cond,
                                 gpointer data)
{
  struct net_input_ctx *ctx = data;
  int flags = 0;
  gboolean keep;

  freelog(LOG_DEBUG, "gir gioc_input_ready source=%p cond=%d ctx=%p, "
          "sock=%d", source, cond, ctx, ctx->sock);

  assert(ctx != NULL);

  if (cond & G_IO_IN) {
    flags |= INPUT_READ;
  }
  if (cond & G_IO_OUT) {
    flags |= INPUT_WRITE;
  }
  if (cond & G_IO_ERR) {
    flags |= INPUT_ERROR;
  }
  if (cond & G_IO_HUP) {
    flags |= INPUT_CLOSED;
  }

  freelog(LOG_DEBUG, "gir calling cb=%p userdata=%p with flags=%d",
          ctx->callback, ctx->userdata, flags);
  keep = (*ctx->callback) (ctx->sock, flags, ctx->userdata);
  freelog(LOG_DEBUG, "gir callback returned keep=%d", keep);

  return keep;
}

/**************************************************************************
  ...
**************************************************************************/
static void destroy_net_input_ctx(gpointer data)
{
  struct net_input_ctx *ctx = data;

  freelog(LOG_DEBUG, "dnic destroy_net_input_ctx %p (socket=%d)",
          data, ctx->sock);
  if (!ctx) {
    return;
  }

  if (ctx->guard != NET_INPUT_CTX_MEMORY_GUARD) {
    freelog(LOG_DEBUG, "dnic called more than once on %p", ctx);
    return;
  }
  ctx->guard = 0;

  if (ctx->datafree) {
    freelog(LOG_DEBUG, "dnic calling datafree func %p", ctx->datafree);
    (*ctx->datafree) (ctx->userdata);
    ctx->datafree = NULL;
  }

  freelog(LOG_DEBUG, "dnic freeing %p", ctx);
  free(ctx);
}

/**************************************************************************
  ...
**************************************************************************/
int add_net_input_callback(int sock,
                           int flags,
                           input_ready_callback_t cb,
                           void *data,
                           data_free_func_t datafree)
{
  int id;
  GIOCondition cond = 0;
  struct net_input_ctx *ctx;
  GIOChannel *gioc;

  freelog(LOG_DEBUG, "anic add_net_input_callback sock=%d flags=%d cb=%p"
          " data=%p datafree=%p", sock, flags, cb, data, datafree);

  if (flags & INPUT_READ) {
    cond |= G_IO_IN;
  }
  if (flags & INPUT_WRITE) {
    cond |= G_IO_OUT;
  }

  if (flags & INPUT_ERROR) {
    cond |= G_IO_ERR;
  }
  if (flags & INPUT_CLOSED) {
    cond |= G_IO_HUP;
  }

  ctx = fc_calloc(1, sizeof (struct net_input_ctx));
  ctx->guard = NET_INPUT_CTX_MEMORY_GUARD;

  freelog(LOG_DEBUG, "anic new net_input_ctx %p", ctx);

  ctx->sock = sock;
  ctx->callback = cb;
  ctx->userdata = data;
  ctx->datafree = datafree;

#ifdef WIN32_NATIVE
  gioc = g_io_channel_win32_new_socket(sock);
#else
  gioc = g_io_channel_unix_new(sock);
#endif

  freelog(LOG_DEBUG, "anic gioc=%p", gioc);
  id = g_io_add_watch_full(gioc, G_PRIORITY_DEFAULT, cond,
                           gioc_input_ready, ctx, destroy_net_input_ctx);
  freelog(LOG_DEBUG, "anic g_io_add_watch_full returns id=%d", id);

  return id;
}

/**************************************************************************
  ...
**************************************************************************/
void remove_net_input_callback(int input_id)
{
  freelog(LOG_DEBUG, "rnic remove_net_input_callback input_id=%d",
          input_id);
  g_source_remove(input_id); /* calls destroy_net_input_ctx on ctx */
}

/**************************************************************************
 This function is called if the client disconnects
 from the server
**************************************************************************/
void remove_net_input()
{
  g_source_remove(input_id);
  gdk_window_set_cursor(root_window, NULL);
}

/****************************************************************
  This is the response callback for the dialog with the message:
  Are you sure you want to quit?
****************************************************************/
static void quit_dialog_response(GtkWidget *dialog, gint response)
{
  gtk_widget_destroy(dialog);
  if (response == GTK_RESPONSE_YES) {
    if (aconnection.used) {
      disconnect_from_server();
    }
    exit(EXIT_SUCCESS);
  }
}

/****************************************************************
  Popups the dialog with the message:
  Are you sure you want to quit?
****************************************************************/
void popup_quit_dialog(void)
{
  static GtkWidget *dialog;

  if (!dialog) {
    dialog = gtk_message_dialog_new(NULL,
                                    0,
                                    GTK_MESSAGE_WARNING,
                                    GTK_BUTTONS_YES_NO,
                                    _("Are you sure you want to quit?"));
    setup_dialog(dialog, toplevel);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);

    g_signal_connect(dialog, "response",
                     G_CALLBACK(quit_dialog_response), NULL);
    g_signal_connect(dialog, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &dialog);
  }

  gtk_window_present(GTK_WINDOW(dialog));
}

/****************************************************************
  Popups the quit dialog.
****************************************************************/
static gboolean quit_dialog_callback(void)
{
  popup_quit_dialog();
  /* Stop emission of event. */
  return TRUE;
}

/****************************************************************
  Toggle the main info table for player (and player observer)
  and global observer if needed.
****************************************************************/
void update_info_table(void)
{
  if (client_is_global_observer()) {
    gtk_widget_hide(info_table);
    gtk_widget_hide(unit_info_label);
    gtk_widget_hide(unit_info_frame);
    gtk_widget_show(go_table);
  } else {
    gtk_widget_show(info_table);
    gtk_widget_show(unit_info_label);
    gtk_widget_show(unit_info_frame);
    gtk_widget_hide(go_table);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void voteinfo_bar_do_vote_callback(GtkWidget *w, gpointer userdata)
{
  enum client_vote_type vote;
  struct voteinfo *vi;

  vote = GPOINTER_TO_INT(userdata);
  vi = voteinfo_queue_get_current(NULL);

  if (vi == NULL) {
    return;
  }

  voteinfo_do_vote(vi->vote_no, vote);
}

/**************************************************************************
  ...
**************************************************************************/
static void voteinfo_bar_next_callback(GtkWidget *w, gpointer userdata)
{
  voteinfo_queue_next();
  voteinfo_gui_update();
}

/**************************************************************************
  ...
**************************************************************************/
struct voteinfo_bar *create_voteinfo_bar(void)
{
  GtkWidget *label, *button, *hbox, *evbox, *spacer, *arrow;
  struct voteinfo_bar *vib;
  const int BUTTON_HEIGHT = 12;

  vib = fc_calloc(1, sizeof(struct voteinfo_bar));

  hbox = gtk_hbox_new(FALSE, 4);
  vib->box = hbox;

  label = gtk_label_new("");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_misc_set_padding(GTK_MISC(label), 8, 4);
  gtk_label_set_max_width_chars(GTK_LABEL(label), 80);
  evbox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(evbox), label);
  gtk_box_pack_start(GTK_BOX(hbox), evbox, TRUE, TRUE, 0);
  gtk_widget_set_name(evbox, "vote label");
  vib->label = label;

  arrow = gtk_image_new_from_stock(GTK_STOCK_MEDIA_REWIND,
                                   GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_misc_set_alignment(GTK_MISC(arrow), 0.5, 0.25);

  button = gtk_button_new();
  g_signal_connect(button, "clicked",
                   G_CALLBACK(voteinfo_bar_next_callback), NULL);
  gtk_button_set_image(GTK_BUTTON(button), arrow);
  gtk_widget_set_size_request(button, -1, BUTTON_HEIGHT);
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  vib->next_button = button;

  spacer = gtk_alignment_new(0, 0, 0, 0);
  gtk_widget_set_size_request(spacer, 16, -1);
  gtk_box_pack_start(GTK_BOX(hbox), spacer, FALSE, FALSE, 0);

  button = gtk_button_new_with_mnemonic(_("_YES"));
  g_signal_connect(button, "clicked",
                   G_CALLBACK(voteinfo_bar_do_vote_callback),
                   GINT_TO_POINTER(CVT_YES));
  gtk_widget_set_size_request(button, 70, BUTTON_HEIGHT);
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  evbox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(evbox), button);
  gtk_box_pack_start(GTK_BOX(hbox), evbox, FALSE, FALSE, 0);
  gtk_widget_set_name(button, "vote yes button");
  gtk_widget_set_name(evbox, "vote yes button");
  gtk_widget_set_name(gtk_bin_get_child(GTK_BIN(button)),
                      "vote yes button");
  vib->yes_button = button;

  label = gtk_label_new("0");
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_widget_set_size_request(label, 24, -1);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  vib->yes_count_label = label;

  button = gtk_button_new_with_mnemonic(_("_NO"));
  g_signal_connect(button, "clicked",
                   G_CALLBACK(voteinfo_bar_do_vote_callback),
                   GINT_TO_POINTER(CVT_NO));
  gtk_widget_set_size_request(button, 70, BUTTON_HEIGHT);
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  evbox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(evbox), button);
  gtk_box_pack_start(GTK_BOX(hbox), evbox, FALSE, FALSE, 0);
  gtk_widget_set_name(button, "vote no button");
  gtk_widget_set_name(evbox, "vote no button");
  gtk_widget_set_name(gtk_bin_get_child(GTK_BIN(button)),
                      "vote no button");
  vib->no_button = button;

  label = gtk_label_new("0");
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_widget_set_size_request(label, 24, -1);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  vib->no_count_label = label;

  button = gtk_button_new_with_mnemonic(_("_ABSTAIN"));
  g_signal_connect(button, "clicked",
                   G_CALLBACK(voteinfo_bar_do_vote_callback),
                   GINT_TO_POINTER(CVT_ABSTAIN));
  gtk_widget_set_size_request(button, 70, BUTTON_HEIGHT);
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  evbox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(evbox), button);
  gtk_box_pack_start(GTK_BOX(hbox), evbox, FALSE, FALSE, 0);
  gtk_widget_set_name(button, "vote abstain button");
  gtk_widget_set_name(evbox, "vote abstain button");
  gtk_widget_set_name(gtk_bin_get_child(GTK_BIN(button)),
                      "vote abstain button");
  vib->abstain_button = button;

  label = gtk_label_new("0");
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_widget_set_size_request(label, 24, -1);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  vib->abstain_count_label = label;

  label = gtk_label_new("/0");
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_widget_set_size_request(label, 24, -1);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  vib->voter_count_label = label;

  return vib;
}

/**************************************************************************
  Handle a mouse click on the player colors mode label.
**************************************************************************/
static gboolean player_colors_mode_label_click(GtkWidget *w,
					       GdkEventButton *ev,
					       gpointer data)
{
  key_cycle_player_colors_modes();

  return TRUE;
}
