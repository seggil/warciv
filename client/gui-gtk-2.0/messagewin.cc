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
#  include "../../config.hh"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "events.hh"
#include "wc_intl.hh"
#include "game.hh"
#include "map.hh"
#include "mem.hh"
#include "packets.hh"
#include "player.hh"
#include "chatline.hh"
#include "citydlg.hh"
#include "../clinet.hh"
#include "colors.hh"
#include "gui_main.hh"
#include "gui_stuff.hh"
#include "mapview.hh"
#include "../options.hh"

#include "messagewin.hh"

static struct gui_dialog *meswin_shell;
static GtkListStore *meswin_store;
static GtkTreeSelection *meswin_selection;

static void create_meswin_dialog(void);
static void meswin_selection_callback(GtkTreeSelection *selection,
                                      gpointer data);
static void meswin_row_activated_callback(GtkTreeView *view,
                                          GtkTreePath *path,
                                          GtkTreeViewColumn *col,
                                          gpointer data);
static void meswin_response_callback(struct gui_dialog *dlg, int response);

static GtkWidget *split_message_window = NULL;
static GtkWidget *split_message_view = NULL;

enum {
  CMD_GOTO = 1, CMD_POPCITY
};

#define N_MSG_VIEW 24          /* max before scrolling happens */

/****************************************************************
popup the dialog 10% inside the main-window
*****************************************************************/
void popup_meswin_dialog(void)
{
  if (show_split_message_window) {
    update_meswin_dialog();
  } else {
  if (!meswin_shell) {
    create_meswin_dialog();
  }

  update_meswin_dialog();

  gui_dialog_present(meswin_shell);
}
}

/****************************************************************
 Raises the message window dialog.
****************************************************************/
void raise_meswin_dialog(void)
{
  if (!show_split_message_window) {
  popup_meswin_dialog();
  gui_dialog_raise(meswin_shell);
}
}

/**************************************************************************
 Closes the message window dialog.
**************************************************************************/
void popdown_meswin_dialog(void)
{
  if (meswin_shell) {
    gui_dialog_destroy(meswin_shell);
  }
}

/****************************************************************
...
*****************************************************************/
bool is_meswin_open(void)
{
  return meswin_shell != NULL || show_split_message_window;
}

/****************************************************************
...
*****************************************************************/
static void meswin_visited_item(gint n)
{
  GtkTreeIter it;

  if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(meswin_store),&it,NULL,n)) {
    gtk_list_store_set(meswin_store, &it, 1, (gint)TRUE, -1);
    set_message_visited_state(n, TRUE);
  }
}

/****************************************************************
...
*****************************************************************/
static void meswin_not_visited_item(gint n)
{
  GtkTreeIter it;

  if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(meswin_store),&it,NULL,n)) {
    gtk_list_store_set(meswin_store, &it, 1, (gint)FALSE, -1);
    set_message_visited_state(n, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
static void meswin_cell_data_func(GtkTreeViewColumn *col,
                                  GtkCellRenderer *cell,
                                  GtkTreeModel *model, GtkTreeIter *it,
                                  gpointer data)
{
  gboolean b;

  gtk_tree_model_get(model, it, 1, &b, -1);

  if (b) {
    g_object_set(G_OBJECT(cell), "style", PANGO_STYLE_ITALIC,
                 "weight", PANGO_WEIGHT_NORMAL, NULL);
  } else {
    g_object_set(G_OBJECT(cell), "style", PANGO_STYLE_NORMAL,
                 "weight", PANGO_WEIGHT_BOLD, NULL);
  }
}

/****************************************************************
...
*****************************************************************/
static void create_meswin_store(void)
{
  meswin_store = gtk_list_store_new(2,
                                    G_TYPE_STRING,
                                    G_TYPE_BOOLEAN);
}

/****************************************************************
...
*****************************************************************/
static void create_meswin_dialog(void)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  GtkWidget *view, *sw, *cmd;

  gui_dialog_new(&meswin_shell, GTK_NOTEBOOK(bottom_notebook));
  gui_dialog_set_title(meswin_shell, _("Messages"));

  if (!meswin_store)
    create_meswin_store();

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                          GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_box_pack_start(GTK_BOX(meswin_shell->vbox), sw, TRUE, TRUE, 0);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(meswin_store));
  meswin_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(NULL, renderer,
        "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  gtk_tree_view_column_set_cell_data_func(col, renderer,
        meswin_cell_data_func, NULL, NULL);
  gtk_container_add(GTK_CONTAINER(sw), view);

  g_signal_connect(meswin_selection, "changed",
                   G_CALLBACK(meswin_selection_callback), NULL);
  g_signal_connect(view, "row_activated",
                   G_CALLBACK(meswin_row_activated_callback), NULL);

  cmd = gui_dialog_add_stockbutton(meswin_shell, GTK_STOCK_JUMP_TO,
      _("Goto _location"), CMD_GOTO);
  gtk_widget_set_sensitive(cmd, FALSE);

  cmd = gui_dialog_add_stockbutton(meswin_shell, GTK_STOCK_ZOOM_IN,
      _("_Popup City"), CMD_POPCITY);
  gtk_widget_set_sensitive(cmd, FALSE);

  gui_dialog_add_button(meswin_shell, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  gui_dialog_response_set_callback(meswin_shell, meswin_response_callback);
  gui_dialog_set_default_size(meswin_shell, 520, 300);

  gui_dialog_show_all(meswin_shell);

  gtk_tree_view_focus(GTK_TREE_VIEW(view));
}

/**************************************************************************
...
**************************************************************************/
void real_update_meswin_dialog(void)
{
  int i, num = get_num_messages(), num_not_visited = 0;
  GtkTreeIter it;

  gtk_list_store_clear(meswin_store);

  for (i = 0; i < num; i++) {
    GValue value = { 0, };

    gtk_list_store_append(meswin_store, &it);

    g_value_init(&value, G_TYPE_STRING);
    g_value_set_static_string(&value, get_message(i)->descr);
    gtk_list_store_set_value(meswin_store, &it, 0, &value);
    g_value_unset(&value);

    if (get_message(i)->visited) {
      meswin_visited_item(i);
    } else {
      meswin_not_visited_item(i);
      num_not_visited++;
    }
  }

  if (!show_split_message_window) {
  gui_dialog_set_response_sensitive(meswin_shell, CMD_GOTO, FALSE);
  gui_dialog_set_response_sensitive(meswin_shell, CMD_POPCITY, FALSE);

  if (num_not_visited > 0) {
    gui_dialog_alert(meswin_shell);
  }
  } else {
    if (split_message_view && num > 0) {
      GtkTreePath *path = gtk_tree_path_new_from_indices(num - 1, -1);
      gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(split_message_view),
                                   path, NULL, FALSE, 0, 0);
      gtk_tree_path_free(path);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
static void meswin_selection_callback(GtkTreeSelection *selection,
                                      gpointer data)
{
  gint row = gtk_tree_selection_get_row(selection);

  if (row != -1) {
    struct message_s *message = get_message(row);

    gui_dialog_set_response_sensitive(meswin_shell, CMD_GOTO,
        message->location_ok);
    gui_dialog_set_response_sensitive(meswin_shell, CMD_POPCITY,
        message->city_ok);
  }
}

/**************************************************************************
...
**************************************************************************/
static void meswin_row_activated_callback(GtkTreeView *view,
                                          GtkTreePath *path,
                                          GtkTreeViewColumn *col,
                                          gpointer data)
{
  gint row = gtk_tree_path_get_indices(path)[0];
  struct message_s *message = get_message(row);

  meswin_double_click(row);
  meswin_visited_item(row);

  gui_dialog_set_response_sensitive(meswin_shell, CMD_GOTO,
      message->location_ok);
  gui_dialog_set_response_sensitive(meswin_shell, CMD_POPCITY,
      message->city_ok);
}

/**************************************************************************
...
**************************************************************************/
static void meswin_response_callback(struct gui_dialog *dlg, int response)
{
  switch (response) {
  case CMD_GOTO:
    {
      gint row = gtk_tree_selection_get_row(meswin_selection);

      if (row == -1) {
        return;
      }

      meswin_goto(row);
      meswin_visited_item(row);
    }
    break;
  case CMD_POPCITY:
    {
      gint row = gtk_tree_selection_get_row(meswin_selection);

      if (row == -1) {
        return;
      }
      meswin_popup_city(row);
      meswin_visited_item(row);
    }
    break;
  default:
    gui_dialog_destroy(dlg);
    break;
  }
}

/**************************************************************************
...
**************************************************************************/
static gboolean split_message_button_press(GtkWidget *treeview,
                                           GdkEventButton *event,
                                           gpointer data)
{
  GtkTreePath *path = NULL;
  gint row;

  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  if (event->button != 1 && event->button != 3)
    return FALSE;

  if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
                                     (gint) event->x,
                                     (gint) event->y,
                                     &path, NULL, NULL, NULL))
  {
    return TRUE;
  }

  row = gtk_tree_path_get_indices(path)[0];
  gtk_tree_path_free(path);

  if (event->button == 1)
    meswin_goto(row);
  else
    meswin_popup_city(row);

  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
GtkWidget *get_split_message_window(void)
{
  GtkWidget *sw, *vbox, *label, *view;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;

  if (split_message_window)
    return split_message_window;

  vbox = gtk_vbox_new(FALSE, 4);

  label = gtk_label_new(_("Messages"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_ALWAYS,
                                 GTK_POLICY_ALWAYS);

  if (!meswin_store)
    create_meswin_store();

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(meswin_store));
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
  split_message_view = view;

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(NULL, renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  gtk_container_add(GTK_CONTAINER(sw), view);

  g_signal_connect(view, "button-press-event",
                   G_CALLBACK(split_message_button_press), NULL);

  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
  split_message_window = vbox;

  return split_message_window;
}
