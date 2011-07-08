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

#include <gtk/gtk.h>

#include "events.h"
#include "fcintl.h"
#include "registry.h"

#include "colors.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "options.h"

#include "messagedlg.h"

enum {
  COL_OUT,
  COL_MES,
  COL_POP,
  COL_NAME,
  COL_EVENT,

  COL_NUM
};

/**************************************************************************
  ...
**************************************************************************/
static void apply_message_option(GtkTreeModel *model, GtkTreeIter *iter)
{
  int i, bit, event;

  gtk_tree_model_get(model, iter, COL_EVENT, &event, -1);
  messages_where[event] = 0;
  for (i = COL_OUT; i <= COL_POP; i++) {
    gtk_tree_model_get(model, iter, i, &bit, -1);
    messages_where[event] |= bit << i;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void apply_message_options(GtkWidget *widget)
{
  GtkTreeModel *model = g_object_get_data(G_OBJECT(widget), "model");
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first(model, &iter)) {
    do {
      apply_message_option(model, &iter);
    } while (gtk_tree_model_iter_next(model, &iter));
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void refresh_message_option(GtkTreeModel *model, GtkTreeIter *iter)
{
  int i, event;

  gtk_tree_model_get(model, iter, COL_EVENT, &event, -1);
  for (i = COL_OUT; i <= COL_POP; i++) {
    gtk_list_store_set(GTK_LIST_STORE(model), iter,
		       i, messages_where[event] & (1 << i), -1);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void refresh_message_options(GtkWidget *widget)
{
  GtkTreeModel *model = g_object_get_data(G_OBJECT(widget), "model");
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first(model, &iter)) {
    do {
      refresh_message_option(model, &iter);
    } while (gtk_tree_model_iter_next(model, &iter));
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void reset_message_option(GtkTreeModel *model, GtkTreeIter *iter)
{
  int i, event, value;

  gtk_tree_model_get(model, iter, COL_EVENT, &event, -1);
  value = get_default_messages_where(event);
  for (i = COL_OUT; i <= COL_POP; i++) {
    gtk_list_store_set(GTK_LIST_STORE(model), iter, i, value & (1 << i), -1);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void reset_message_options(GtkWidget *widget)
{
  GtkTreeModel *model = g_object_get_data(G_OBJECT(widget), "model");
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first(model, &iter)) {
    do {
      reset_message_option(model, &iter);
    } while (gtk_tree_model_iter_next(model, &iter));
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void reload_message_option(GtkTreeModel *model, GtkTreeIter *iter,
				  struct section_file *sf)
{
  int i, event, value;

  gtk_tree_model_get(model, iter, COL_EVENT, &event, -1);
  value = secfile_lookup_int_default(sf, -1, "client.message_where_%02d",
				     event);
  if (value != -1) {
    for (i = COL_OUT; i <= COL_POP; i++) {
      gtk_list_store_set(GTK_LIST_STORE(model), iter, i, value & (1 << i), -1);
    }
  }
}

/**************************************************************************
  ...
**************************************************************************/
void reload_message_options(GtkWidget *widget, struct section_file *sf)
{
  GtkTreeModel *model = g_object_get_data(G_OBJECT(widget), "model");
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first(model, &iter)) {
    do {
      reload_message_option(model, &iter, sf);
    } while (gtk_tree_model_iter_next(model, &iter));
  }
}

/**************************************************************************
...
**************************************************************************/
static void item_toggled(GtkCellRendererToggle *cell,
			 gchar *spath, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL(data);
  GtkTreePath *path;
  GtkTreeIter it;
  gboolean toggle;
  gint column;

  path = gtk_tree_path_new_from_string(spath);

  column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "column"));

  gtk_tree_model_get_iter(model, &it, path);
  gtk_tree_model_get(model, &it, column, &toggle, -1);
  toggle ^= 1;
  gtk_list_store_set(GTK_LIST_STORE(model), &it, column, toggle, -1);

  gtk_tree_path_free(path);
}

/*************************************************************************
  Search a given iter in the GtkTreeModel.
*************************************************************************/
static gboolean search_iter(GtkTreeModel *model, GtkTreeIter *iter, int event)
{
  int i;

  if (gtk_tree_model_get_iter_first(model, iter)) {
    do {
      gtk_tree_model_get(model, iter, COL_EVENT, &i, -1);
      if (i == event) {
	return TRUE;
      }
    } while (gtk_tree_model_iter_next(model, iter));
  }
  return FALSE;
}

/*************************************************************************
  ...
*************************************************************************/
static void refresh_message_option_callback(GtkMenuItem *menuitem,
					    gpointer data)
{
  GtkTreeModel *model = g_object_get_data(G_OBJECT(menuitem), "model");
  GtkTreeIter iter;

  if (search_iter(model, &iter, GPOINTER_TO_INT(data))) {
    refresh_message_option(model, &iter);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void reset_message_option_callback(GtkMenuItem *menuitem, gpointer data)
{
  GtkTreeModel *model = g_object_get_data(G_OBJECT(menuitem), "model");
  GtkTreeIter iter;

  if (search_iter(model, &iter, GPOINTER_TO_INT(data))) {
    reset_message_option(model, &iter);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void reload_message_option_callback(GtkMenuItem *menuitem, gpointer data)
{
  GtkTreeModel *model = g_object_get_data(G_OBJECT(menuitem), "model");
  GtkTreeIter iter;
  struct section_file sf;

  if (section_file_load(&sf, option_file_name())) {
    if (search_iter(model, &iter, GPOINTER_TO_INT(data))) {
      reload_message_option(model, &iter, &sf);
    }
    section_file_free(&sf);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void apply_message_option_callback(GtkMenuItem *menuitem, gpointer data)
{
  GtkTreeModel *model = g_object_get_data(G_OBJECT(menuitem), "model");
  GtkTreeIter iter;

  if (search_iter(model, &iter, GPOINTER_TO_INT(data))) {
    apply_message_option(model, &iter);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static gboolean button_press_callback(GtkWidget *view,
				      GdkEventButton *event, gpointer data)
{
  GtkWidget *menu, *item;
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  int ev;

  if (event->button != 3) {
    return FALSE;
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
  if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(view), event->x, event->y,
				     &path, NULL, NULL, NULL)) {
    return FALSE;
  }

  gtk_tree_model_get_iter(model, &iter, path);
  gtk_tree_model_get(model, &iter, COL_EVENT, &ev, -1);
  gtk_tree_path_free(path);

  menu = gtk_menu_new();

  item = gtk_image_menu_item_new_with_label(_("Refresh this message option"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_object_set_data(G_OBJECT(item), "model", model);
  g_signal_connect(item, "activate",
                   G_CALLBACK(refresh_message_option_callback),
		   GINT_TO_POINTER(ev));

  item = gtk_image_menu_item_new_with_label(_("Reset this message option"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_object_set_data(G_OBJECT(item), "model", model);
  g_signal_connect(item, "activate",
                   G_CALLBACK(reset_message_option_callback),
		   GINT_TO_POINTER(ev));

  item = gtk_image_menu_item_new_with_label(_("Reload this message option"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_object_set_data(G_OBJECT(item), "model", model);
  g_signal_connect(item, "activate",
                   G_CALLBACK(reload_message_option_callback),
		   GINT_TO_POINTER(ev));

  item = gtk_image_menu_item_new_with_label(_("Apply the changes for "
					      "this message option"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_object_set_data(G_OBJECT(item), "model", model);
  g_signal_connect(item, "activate",
                   G_CALLBACK(apply_message_option_callback),
		   GINT_TO_POINTER(ev));

  gtk_widget_show_all(menu);
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, 0);

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
GtkWidget *create_messages_configuration(void)
{
  GtkWidget *vbox, *explanation, *view, *sw;
  GtkCellRenderer *renderer;
  GtkListStore *model;
  GtkTreeViewColumn *col;
  int i;

  vbox = gtk_vbox_new(FALSE, 0);

  explanation = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(explanation),
		       _("Where to display messages?\n"
			 "\t<b>Out</b>put window ;\n"
			 "\t<b>Mes</b>sages window ;\n"
			 "\t<b>Pop</b>up individual window"));
  gtk_widget_set_name(explanation, "comment label");
  gtk_box_pack_start(GTK_BOX(vbox), explanation, FALSE, FALSE, 0);

  model = gtk_list_store_new(COL_NUM,
			     G_TYPE_BOOLEAN,	/* COL_OUT */
			     G_TYPE_BOOLEAN,	/* COL_MES */
			     G_TYPE_BOOLEAN,	/* COL_POP */
			     G_TYPE_STRING,	/* COL_NAME */
			     G_TYPE_INT);	/* COL_EVENT */
  g_object_set_data(G_OBJECT(vbox), "model", model);

  for (i = 0; i < E_LAST; i++)  {
    GtkTreeIter it;
    GValue value = { 0, };

    gtk_list_store_append(model, &it);

    g_value_init(&value, G_TYPE_STRING);
    g_value_set_static_string(&value, get_message_text(sorted_events[i]));
    gtk_list_store_set_value(model, &it, COL_NAME, &value);
    g_value_unset(&value);

    gtk_list_store_set(model, &it, COL_EVENT, sorted_events[i], -1);
  }

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
  g_object_unref(model);
  g_signal_connect(G_OBJECT(view), "button-press-event",
		   G_CALLBACK(button_press_callback), NULL);

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(_("Event"), renderer,
						 "text", COL_NAME, NULL);
  gtk_tree_view_column_set_expand(col, TRUE);
  gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);

  renderer = gtk_cell_renderer_toggle_new();
  g_object_set_data(G_OBJECT(renderer), "column", GINT_TO_POINTER(COL_OUT));
  g_signal_connect(renderer, "toggled", G_CALLBACK(item_toggled), model);
  col = gtk_tree_view_column_new_with_attributes(_("Out"), renderer,
						 "active", COL_OUT, NULL);
  gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);

  renderer = gtk_cell_renderer_toggle_new();
  g_object_set_data(G_OBJECT(renderer), "column", GINT_TO_POINTER(COL_MES));
  g_signal_connect(renderer, "toggled", G_CALLBACK(item_toggled), model);
  col = gtk_tree_view_column_new_with_attributes(_("Mes"), renderer,
						 "active", COL_MES, NULL);
  gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);

  renderer = gtk_cell_renderer_toggle_new();
  g_object_set_data(G_OBJECT(renderer), "column", GINT_TO_POINTER(COL_POP));
  g_signal_connect(renderer, "toggled", G_CALLBACK(item_toggled), model);
  col = gtk_tree_view_column_new_with_attributes(_("Pop"), renderer, "active",
						 COL_POP, NULL);
  gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(sw), view);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_end(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

  gtk_tree_view_focus(GTK_TREE_VIEW(view));

  gtk_widget_show_all(vbox);

  return vbox;
}
