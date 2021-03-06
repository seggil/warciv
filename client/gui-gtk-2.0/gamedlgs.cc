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
#include <gdk/gdkkeysyms.h>

#include "wc_intl.hh"
#include "iterator.hh"
#include "registry.hh"
#include "shared.hh"
#include "support.hh"

#include "events.hh"
#include "game.hh"
#include "government.hh"
#include "packets.hh"
#include "player.hh"

#include "chatline.hh"
#include "cityrep.hh"
#include "../civclient.hh"
#include "../clinet.hh"
#include "dialogs.hh"
#include "gui_main.hh"
#include "gui_stuff.hh"
#include "mapview.hh"
#include "../mapview_common.hh"
#include "../include/menu_g.hh"
#include "messagedlg.hh"
#include "messagewin.hh"
#include "../options.hh"
#include "../include/plrdlg_g.hh"
#include "wldlg.hh"

#include "ratesdlg.hh"
#include "optiondlg.hh"

/******************************************************************/
static GtkWidget *rates_dialog_shell;
static GtkWidget *rates_gov_label;
static GtkWidget *rates_tax_toggle, *rates_lux_toggle, *rates_sci_toggle;
static GtkWidget *rates_tax_label, *rates_lux_label, *rates_sci_label;
static GtkObject *rates_tax_adj, *rates_lux_adj, *rates_sci_adj;

static gulong     rates_tax_sig, rates_lux_sig, rates_sci_sig;
/******************************************************************/

static int rates_tax_value, rates_lux_value, rates_sci_value;


static void rates_changed_callback(GtkAdjustment *adj);


/**************************************************************************
...
**************************************************************************/
static void rates_set_values(int tax, int no_tax_scroll,
                             int lux, int no_lux_scroll,
                             int sci, int no_sci_scroll)
{
  char buf[64];
  int tax_lock, lux_lock, sci_lock;
  int maxrate;

  tax_lock = GTK_TOGGLE_BUTTON(rates_tax_toggle)->active;
  lux_lock = GTK_TOGGLE_BUTTON(rates_lux_toggle)->active;
  sci_lock = GTK_TOGGLE_BUTTON(rates_sci_toggle)->active;

  maxrate = get_government_max_rate(get_player_ptr()->government);
  /* This's quite a simple-minded "double check".. */
  tax = MIN(tax, maxrate);
  lux = MIN(lux, maxrate);
  sci = MIN(sci, maxrate);

  if (tax + sci + lux != 100) {
    if ((tax != rates_tax_value)) {
      if (!lux_lock) {
        lux = MIN(MAX(100 - tax - sci, 0), maxrate);
      }
      if (!sci_lock) {
        sci = MIN(MAX(100 - tax - lux, 0), maxrate);
      }
    } else if ((lux != rates_lux_value)) {
      if (!tax_lock) {
        tax = MIN(MAX(100 - lux - sci, 0), maxrate);
      }
      if (!sci_lock) {
        sci = MIN(MAX(100 - lux - tax, 0), maxrate);
      }
    } else if ((sci != rates_sci_value)) {
      if (!lux_lock) {
        lux = MIN(MAX(100 - tax - sci, 0), maxrate);
      }
      if (!tax_lock) {
        tax = MIN(MAX(100 - lux - sci, 0), maxrate);
      }
    }

    if (tax + sci + lux != 100) {
      tax = rates_tax_value;
      lux = rates_lux_value;
      sci = rates_sci_value;

      rates_tax_value = -1;
      rates_lux_value = -1;
      rates_sci_value = -1;

      no_tax_scroll = 0;
      no_lux_scroll = 0;
      no_sci_scroll = 0;
    }

  }

  if (tax != rates_tax_value) {
    my_snprintf(buf, sizeof(buf), "%3d%%", tax);
    if (strcmp(buf, GTK_LABEL(rates_tax_label)->label) != 0)
        gtk_label_set_text(GTK_LABEL(rates_tax_label), buf);
    if (!no_tax_scroll) {
        g_signal_handler_block(rates_tax_adj, rates_tax_sig);
        gtk_adjustment_set_value(GTK_ADJUSTMENT(rates_tax_adj), tax/10 );
        g_signal_handler_unblock(rates_tax_adj, rates_tax_sig);
    }
    rates_tax_value=tax;
  }

  if (lux != rates_lux_value) {
    my_snprintf(buf, sizeof(buf), "%3d%%", lux);
    if (strcmp(buf, GTK_LABEL(rates_lux_label)->label) != 0)
        gtk_label_set_text(GTK_LABEL(rates_lux_label), buf);
    if (!no_lux_scroll) {
        g_signal_handler_block(rates_lux_adj, rates_lux_sig);
        gtk_adjustment_set_value(GTK_ADJUSTMENT(rates_lux_adj), lux/10 );
        g_signal_handler_unblock(rates_lux_adj, rates_lux_sig);
    }
    rates_lux_value=lux;
  }

  if (sci != rates_sci_value) {
    my_snprintf(buf, sizeof(buf), "%3d%%", sci);
    if (strcmp(buf, GTK_LABEL(rates_sci_label)->label) != 0)
        gtk_label_set_text(GTK_LABEL(rates_sci_label),buf);
    if (!no_sci_scroll) {
        g_signal_handler_block(rates_sci_adj, rates_sci_sig);
        gtk_adjustment_set_value(GTK_ADJUSTMENT(rates_sci_adj), sci/10 );
        g_signal_handler_unblock(rates_sci_adj, rates_sci_sig);
    }
    rates_sci_value=sci;
  }
}


/**************************************************************************
...
**************************************************************************/
static void rates_changed_callback(GtkAdjustment *adj)
{
  int percent = adj->value;

  if (adj == GTK_ADJUSTMENT(rates_tax_adj)) {
    int tax_value;

    tax_value = 10 * percent;
    tax_value = MIN(tax_value, 100);
    rates_set_values(tax_value, 1, rates_lux_value, 0, rates_sci_value, 0);
  } else if (adj == GTK_ADJUSTMENT(rates_lux_adj)) {
    int lux_value;

    lux_value = 10 * percent;
    lux_value = MIN(lux_value, 100);
    rates_set_values(rates_tax_value, 0, lux_value, 1, rates_sci_value, 0);
  } else {
    int sci_value;

    sci_value = 10 * percent;
    sci_value = MIN(sci_value, 100);
    rates_set_values(rates_tax_value, 0, rates_lux_value, 0, sci_value, 1);
  }
}


/**************************************************************************
...
**************************************************************************/
static void rates_command_callback(GtkWidget *w, gint response_id)
{
  if (response_id == GTK_RESPONSE_OK) {
    dsend_packet_player_rates(&aconnection, rates_tax_value, rates_lux_value,
                              rates_sci_value);
  }
  gtk_widget_destroy(rates_dialog_shell);
}


/**************************************************************************
...
**************************************************************************/
static void rates_destroy_callback(GtkWidget *widget, gpointer data)
{
  rates_dialog_shell = NULL;
}



/****************************************************************
...
*****************************************************************/
static GtkWidget *create_rates_dialog(void)
{
  player_t *pplayer = get_player_ptr();
  GtkWidget *shell;
  GtkWidget *frame, *hbox;
  GtkWidget *scale;

  shell = gtk_dialog_new_with_buttons(_("Select tax, luxury and science rates"),
                                      NULL, (GtkDialogFlags)0,
                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                      GTK_STOCK_OK, GTK_RESPONSE_OK,
                                      NULL);
  setup_dialog(shell, toplevel);
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_OK);
  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_MOUSE);

  rates_gov_label = gtk_label_new("");
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( shell )->vbox ), rates_gov_label, TRUE, TRUE, 5 );

  frame = gtk_frame_new( _("Tax") );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( shell )->vbox ), frame, TRUE, TRUE, 5 );

  hbox = gtk_hbox_new( FALSE, 10 );
  gtk_container_add( GTK_CONTAINER( frame ), hbox );

  rates_tax_adj = gtk_adjustment_new( 0.0, 0.0, 11.0, 1.0, 1.0, 1.0 );
  scale = gtk_hscale_new( GTK_ADJUSTMENT( rates_tax_adj ) );
  gtk_widget_set_size_request(scale, 300, 40);
  gtk_scale_set_digits( GTK_SCALE( scale ), 0 );
  gtk_scale_set_draw_value( GTK_SCALE( scale ), FALSE );
  gtk_box_pack_start( GTK_BOX( hbox ), scale, TRUE, TRUE, 0 );

  rates_tax_label = gtk_label_new("  0%");
  gtk_box_pack_start( GTK_BOX( hbox ), rates_tax_label, TRUE, TRUE, 0 );
  gtk_widget_set_size_request(rates_tax_label, 40, -1);

  rates_tax_toggle = gtk_check_button_new_with_label( _("Lock") );
  gtk_box_pack_start( GTK_BOX( hbox ), rates_tax_toggle, TRUE, TRUE, 0 );

  frame = gtk_frame_new( _("Luxury") );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( shell )->vbox ), frame, TRUE, TRUE, 5 );

  hbox = gtk_hbox_new( FALSE, 10 );
  gtk_container_add( GTK_CONTAINER( frame ), hbox );

  rates_lux_adj = gtk_adjustment_new( 0.0, 0.0, 11.0, 1.0, 1.0, 1.0 );
  scale = gtk_hscale_new( GTK_ADJUSTMENT( rates_lux_adj ) );
  gtk_widget_set_size_request(scale, 300, 40);
  gtk_scale_set_digits( GTK_SCALE( scale ), 0 );
  gtk_scale_set_draw_value( GTK_SCALE( scale ), FALSE );
  gtk_box_pack_start( GTK_BOX( hbox ), scale, TRUE, TRUE, 0 );

  rates_lux_label = gtk_label_new("  0%");
  gtk_box_pack_start( GTK_BOX( hbox ), rates_lux_label, TRUE, TRUE, 0 );
  gtk_widget_set_size_request(rates_lux_label, 40, -1);

  rates_lux_toggle = gtk_check_button_new_with_label( _("Lock") );
  gtk_box_pack_start( GTK_BOX( hbox ), rates_lux_toggle, TRUE, TRUE, 0 );

  frame = gtk_frame_new( _("Science") );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( shell )->vbox ), frame, TRUE, TRUE, 5 );

  hbox = gtk_hbox_new( FALSE, 10 );
  gtk_container_add( GTK_CONTAINER( frame ), hbox );

  rates_sci_adj = gtk_adjustment_new( 0.0, 0.0, 11.0, 1.0, 1.0, 1.0 );
  scale = gtk_hscale_new( GTK_ADJUSTMENT( rates_sci_adj ) );
  gtk_widget_set_size_request(scale, 300, 40);
  gtk_scale_set_digits( GTK_SCALE( scale ), 0 );
  gtk_scale_set_draw_value( GTK_SCALE( scale ), FALSE );
  gtk_box_pack_start( GTK_BOX( hbox ), scale, TRUE, TRUE, 0 );

  rates_sci_label = gtk_label_new("  0%");
  gtk_box_pack_start( GTK_BOX( hbox ), rates_sci_label, TRUE, TRUE, 0 );
  gtk_widget_set_size_request(rates_sci_label, 40, -1);

  rates_sci_toggle = gtk_check_button_new_with_label( _("Lock") );
  gtk_box_pack_start( GTK_BOX( hbox ), rates_sci_toggle, TRUE, TRUE, 0 );


  g_signal_connect(shell, "response",
                   G_CALLBACK(rates_command_callback), NULL);
  g_signal_connect(shell, "destroy",
                   G_CALLBACK(rates_destroy_callback), NULL);

  gtk_widget_show_all( GTK_DIALOG( shell )->vbox );
  gtk_widget_show_all( GTK_DIALOG( shell )->action_area );

  rates_tax_value = -1;
  rates_lux_value = -1;
  rates_sci_value = -1;

  rates_tax_sig =
    g_signal_connect_after(rates_tax_adj, "value_changed",
                           G_CALLBACK(rates_changed_callback), NULL);

  rates_lux_sig =
    g_signal_connect_after(rates_lux_adj, "value_changed",
                           G_CALLBACK(rates_changed_callback), NULL);

  rates_sci_sig =
    g_signal_connect_after(rates_sci_adj, "value_changed",
                           G_CALLBACK(rates_changed_callback), NULL);

  rates_set_values(pplayer->economic.tax, 0,
                   pplayer->economic.luxury, 0,
                   pplayer->economic.science, 0);
  return shell;
}




/****************************************************************
...
*****************************************************************/
void popup_rates_dialog(void)
{
  char buf[64];
  int gov = get_player_ptr()->government;

  if (!rates_dialog_shell) {
    rates_dialog_shell = create_rates_dialog();
  }

  my_snprintf(buf, sizeof(buf), _("%s max rate: %d%%"),
      get_government_name(gov), get_government_max_rate(gov));
  gtk_label_set_text(GTK_LABEL(rates_gov_label), buf);

  gtk_window_present(GTK_WINDOW(rates_dialog_shell));
}




/**************************************************************************
  Option dialog
**************************************************************************/
enum {
  LC_INDEX,
  LC_TEXT_ORIGIN,
  LC_TEXT_TRANSLATED,
  LC_NUM
};

enum {
  RESPONSE_REFRESH,
  RESPONSE_RESET,
  RESPONSE_RELOAD,
  RESPONSE_SAVE,
  RESPONSE_APPLY,
  RESPONSE_CANCEL,
  RESPONSE_OK
};

/* Used for chat line */
struct extra_option {
  GtkWidget *widget;
  char *name;
  void (*apply_callback)(GtkWidget *);
  void (*refresh_callback)(GtkWidget *);
  void (*reset_callback)(GtkWidget *);
  void (*reload_callback)(GtkWidget *, struct section_file *);
};

#define SPECLIST_TAG extra_option
#define SPECLIST_TYPE struct extra_option
#include "speclist.hh"
#define extra_options_iterate(poption) \
  TYPED_LIST_ITERATE(struct extra_option, extra_options, poption)
#define extra_options_iterate_end  LIST_ITERATE_END

static GtkWidget *option_dialog_shell;
static struct extra_option_list *extra_options;

/**************************************************************************
  Callback for the full screen option.
**************************************************************************/
void fullscreen_mode_callback(struct client_option *poption)
{
  assert(poption->type == CLIENT_OPTION_TYPE_BOOLEAN);
  if (*poption->u.boolean_o.pvalue) {
    gtk_window_fullscreen(GTK_WINDOW(toplevel));
  } else {
    gtk_window_unfullscreen(GTK_WINDOW(toplevel));
  }
}

/**************************************************************************
  Callback for the map scroll bars option.
**************************************************************************/
void map_scrollbars_callback(struct client_option *poption)
{
  assert(poption->type == CLIENT_OPTION_TYPE_BOOLEAN);
  if (*poption->u.boolean_o.pvalue) {
    gtk_widget_show(map_horizontal_scrollbar);
    gtk_widget_show(map_vertical_scrollbar);
  } else {
    gtk_widget_hide(map_horizontal_scrollbar);
    gtk_widget_hide(map_vertical_scrollbar);
  }
}

/**************************************************************************
  Common callback to dedraw the map entierly.
**************************************************************************/
void mapview_redraw_callback(struct client_option *poption)
{
  update_map_canvas_visible(MAP_UPDATE_WRITE);
}

/**************************************************************************
  Common callback to dedraw the map entierly.
**************************************************************************/
void split_message_window_callback(struct client_option *poption)
{
  assert(poption->type == CLIENT_OPTION_TYPE_BOOLEAN);
  if (*poption->u.boolean_o.pvalue) {
    gtk_widget_show(get_split_message_window());
  } else {
    gtk_widget_hide(get_split_message_window());
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void refresh_extra_option_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct extra_option *op = (struct extra_option *) data;

  op->refresh_callback(op->widget);
}

/*************************************************************************
  ...
*************************************************************************/
static void reset_extra_option_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct extra_option *op = (struct extra_option *) data;

  op->reset_callback(op->widget);
}

/*************************************************************************
  ...
*************************************************************************/
static void reload_extra_option_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct section_file sf;

  if (section_file_load(&sf, option_file_name())) {
    struct extra_option *op = (struct extra_option *) data;

    op->reload_callback(op->widget, &sf);
    section_file_free(&sf);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void apply_extra_option_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct extra_option *op = (struct extra_option *) data;

  op->apply_callback(op->widget);
}

/*************************************************************************
  Called when a button is pressed.
*************************************************************************/
static gboolean extra_option_callback(GtkWidget *widget, GdkEventButton *event,
                                      gpointer data)
{
  /* Only right button please! */
  if (event->button != 3) {
    return FALSE;
  }

  GtkWidget *menu, *item;
  struct extra_option *op = (struct extra_option *) data;
  char buf[256];

  menu = gtk_menu_new();

  my_snprintf(buf, sizeof(buf), _("Refresh %s"), op->name);
  item = gtk_image_menu_item_new_with_label(buf);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(refresh_extra_option_callback), data);

  my_snprintf(buf, sizeof(buf), _("Reset %s"), op->name);
  item = gtk_image_menu_item_new_with_label(buf);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(reset_extra_option_callback), data);

  my_snprintf(buf, sizeof(buf), _("Reload %s"), op->name);
  item = gtk_image_menu_item_new_with_label(buf);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(reload_extra_option_callback), data);

  my_snprintf(buf, sizeof(buf), _("Apply changes for %s"), op->name);
  item = gtk_image_menu_item_new_with_label(buf);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(apply_extra_option_callback), data);

  gtk_widget_show_all(menu);
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, 0);

  return TRUE;
}

/*************************************************************************
  Create a new extra option structure.
*************************************************************************/
static void extra_option_new(GtkWidget *widget, GtkWidget *parent_box,
                             const char *name,
                             void (*apply_callback)(GtkWidget *),
                             void (*refresh_callback)(GtkWidget *),
                             void (*reset_callback)(GtkWidget *),
                             void (*reload_callback)(GtkWidget *,
                                                     struct section_file *))
{
  struct extra_option *op = (struct extra_option *)
      wc_malloc(sizeof(struct extra_option));
  GtkWidget *ebox, *frame;

  assert(NULL != widget);
  assert(NULL != parent_box);
  assert(NULL != name);
  assert(NULL != apply_callback);
  assert(NULL != refresh_callback);
  assert(NULL != reset_callback);
  assert(NULL != reload_callback);

  op->widget = widget;
  op->name = mystrdup(name);
  op->apply_callback = apply_callback;
  op->refresh_callback = refresh_callback;
  op->reset_callback = reset_callback;
  op->reload_callback = reload_callback;

  ebox = gtk_event_box_new();
  gtk_box_pack_end(GTK_BOX(parent_box), ebox, TRUE, TRUE, 0);
  frame = gtk_frame_new(op->name);
  gtk_container_add(GTK_CONTAINER(ebox), frame);
  gtk_container_add(GTK_CONTAINER(frame), widget);
  g_signal_connect(ebox, "button_press_event",
                   G_CALLBACK(extra_option_callback), op);

  extra_option_list_append(extra_options, op);
}

/*************************************************************************
  Delete an extra option structure.
*************************************************************************/
static void extra_option_destroy(struct extra_option *op)
{
  free(op->name);
  extra_option_list_unlink(extra_options, op);
}


/****************************************************************
  ...
*****************************************************************/
static void cell_edited(GtkCellRendererText *cell,
                        const gchar *spath,
                        const gchar *text,
                        gpointer data)
{
  struct client_option *op = (struct client_option *) data;
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(op->gui_data));
  GtkTreePath *path;
  GtkTreeIter it;
  gchar *edited;

  path = gtk_tree_path_new_from_string(spath);
  gtk_tree_model_get_iter(model, &it, path);
  gtk_tree_path_free(path);

  gtk_tree_model_get(model, &it, 0, &edited, -1);
  gtk_list_store_set(GTK_LIST_STORE(model), &it, 0, text, -1);
  g_free(edited);
}

/*************************************************************************
  ...
*************************************************************************/
static void append_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct client_option *op = (struct client_option *) data;
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(op->gui_data));
  GtkListStore *store = GTK_LIST_STORE(model);
  GtkTreePath *path = (GtkTreePath *)g_object_get_data(G_OBJECT(op->gui_data), "path");
  GtkTreeIter iter;

  if (path) {
    /* Clear the old path. */
    gtk_tree_path_free(path);
  }

  gtk_list_store_append(store, &iter);
  path = gtk_tree_model_get_path(model, &iter);
  gtk_tree_view_set_cursor(GTK_TREE_VIEW(op->gui_data),
                           path,
                           gtk_tree_view_get_column(GTK_TREE_VIEW(op->gui_data), 0),
                           TRUE);
  gtk_tree_path_free(path);
}

/*************************************************************************
  ...
*************************************************************************/
static void insert_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct client_option *op = (struct client_option *) data;
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(op->gui_data));
  GtkListStore *store = GTK_LIST_STORE(model);
  GtkTreePath *path = (GtkTreePath *)g_object_get_data(G_OBJECT(op->gui_data), "path");
  GtkTreeIter iter, niter;

  if (!path) {
    return;
  }

  gtk_tree_model_get_iter(model, &iter, path);
  gtk_tree_path_free(path);
  gtk_list_store_insert_before(store, &niter, &iter);
  path = gtk_tree_model_get_path(model, &niter);
  gtk_tree_view_set_cursor(GTK_TREE_VIEW(op->gui_data),
                           path,
                           gtk_tree_view_get_column(GTK_TREE_VIEW(op->gui_data), 0),
                           TRUE);
  gtk_tree_path_free(path);
}

/*************************************************************************
  ...
*************************************************************************/
static void remove_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct client_option *op = (struct client_option *) data;
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(op->gui_data));
  GtkListStore *store = GTK_LIST_STORE(model);
  GtkTreePath *path = (GtkTreePath*)g_object_get_data((GObject*)op->gui_data, "path");
  GtkTreeIter iter;

  if (!path) {
    return;
  }

  gtk_tree_model_get_iter(model, &iter, path);
  gtk_list_store_remove(store, &iter);
  gtk_tree_path_free(path);
}

/*************************************************************************
  ...
*************************************************************************/
static void edit_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct client_option *op = (struct client_option *) data;
  GtkTreePath *path = (GtkTreePath*)g_object_get_data((GObject*)op->gui_data, "path");

  if (!path) {
    return;
  }

  gtk_tree_view_set_cursor(GTK_TREE_VIEW(op->gui_data),
                           path,
                           gtk_tree_view_get_column(GTK_TREE_VIEW(op->gui_data), 0),
                           TRUE);
  gtk_tree_path_free(path);
}

/*************************************************************************
  ...
*************************************************************************/
static void move_up_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct client_option *op = (struct client_option *) data;
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(op->gui_data));
  GtkListStore *store = GTK_LIST_STORE(model);
  GtkTreePath *path = (GtkTreePath*)g_object_get_data((GObject*)op->gui_data, "path");
  GtkTreeIter iter, prev;

  if (!path) {
    return;
  }

  gtk_tree_model_get_iter(model, &iter, path);
  if (gtk_tree_path_prev(path)) {
    gtk_tree_model_get_iter(model, &prev, path);
    gtk_list_store_swap(store, &iter, &prev);
  }
  gtk_tree_path_free(path);
}

/*************************************************************************
  ...
*************************************************************************/
static void move_down_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct client_option *op = (struct client_option *) data;
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(op->gui_data));
  GtkListStore *store = GTK_LIST_STORE(model);
  GtkTreePath *path = (GtkTreePath*)g_object_get_data((GObject*)op->gui_data, "path");
  GtkTreeIter iter, next;

  if (!path) {
    return;
  }

  gtk_tree_model_get_iter(model, &iter, path);
  gtk_tree_path_next(path);
  if (gtk_tree_model_get_iter(model, &next, path)) {
    gtk_list_store_swap(store, &iter, &next);
  }
  gtk_tree_path_free(path);
}

/*************************************************************************
  ...
*************************************************************************/
static gboolean tree_view_callback(GtkWidget *view, GdkEventButton *event,
                                   gpointer data)
{
  GtkWidget *menu, *item;
  GtkTreePath *path;

  if (event->button != 3) {
    /* Right click only. */
    return FALSE;
  }

  if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(view),
                                    (gint) event->x, (gint) event->y,
                                    &path, NULL, NULL, NULL)) {
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(view), path, NULL, FALSE);
  } else {
    path = NULL;
  }
  g_object_set_data(G_OBJECT(view), "path", path);

  menu = gtk_menu_new();

  item = gtk_image_menu_item_new_with_label(_("Append"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item,"activate",
                   G_CALLBACK(append_callback), data);

  item = gtk_image_menu_item_new_with_label(_("Insert"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(insert_callback), data);

  item = gtk_image_menu_item_new_with_label(_("Remove"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(remove_callback), data);

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

  item = gtk_image_menu_item_new_with_label(_("Edit"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(edit_callback), data);

  item = gtk_image_menu_item_new_with_label(_("Move up"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(move_up_callback), data);

  item = gtk_image_menu_item_new_with_label(_("Move down"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(move_down_callback), data);

  gtk_widget_show_all(menu);
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, 0);

  return TRUE;
}

/*************************************************************************
  Attempt to set the string as active iterator.
*************************************************************************/
static void set_combo_to(GtkComboBox *combo, const char *untranslated)
{
  GtkTreeModel *model = gtk_combo_box_get_model(combo);
  GtkTreeIter iter;
  const char *str;

  if (gtk_tree_model_get_iter_first(model, &iter)) {
    do {
      gtk_tree_model_get(model, &iter, LC_TEXT_ORIGIN, &str, -1);
      if (0 == strcmp(untranslated, str)) {
        gtk_combo_box_set_active_iter(combo, &iter);
        return;
      }
    } while (gtk_tree_model_iter_next(model, &iter));
  }
}

/*************************************************************************
  Reset the check button to their right values.
*************************************************************************/
static void update_filter(struct client_option *op)
{
  assert(op->type == CLIENT_OPTION_TYPE_FILTER);

  GList *list;

  for (list = GTK_BOX(gtk_bin_get_child(GTK_BIN(op->gui_data)))->children;
       list; list = g_list_next(list)) {
    GtkWidget *widget = ((GtkBoxChild *) list->data)->widget;
    filter value;

    value = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),
                                              "filter_value"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                                 op->u.filter.temp & value);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static bool string_empty(const char *str)
{
  if (!str) {
    return TRUE;
  }

  for (; *str != '\0'; str++) {
    if (!my_isspace(*str)) {
      return FALSE;
    }
  }
  return TRUE;
}

/*************************************************************************
  Returns TRUE iff the tree view doesn't correspond to the current set of
  values of the string vector.
*************************************************************************/
static bool string_vec_changed(struct client_option *op)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  size_t i;
  const char *str, *n;

  assert(op != NULL && op->type == CLIENT_OPTION_TYPE_STRING_VEC);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(op->gui_data));

  if (!gtk_tree_model_get_iter_first(model, &iter)
      && string_vector_size(*op->u.string_vec.pvector) == 0) {
    /* Both seems empty */
    return FALSE;
  }

  i = 0;
  do {
    str = string_vector_get(*op->u.string_vec.pvector, i);
    gtk_tree_model_get(model, &iter, 0, &n, -1);
    if (!string_empty(str)) {
      /* Not empty */
      if (n && 0 != strcmp(str, n)) {
        return TRUE;
      }
    } else if (!string_empty(n)) {
      return TRUE;
    }
    i++;
  } while (gtk_tree_model_iter_next(model, &iter));

  return i != string_vector_size(*op->u.string_vec.pvector);
}

/*************************************************************************
  Stores the string vector to the values in the tree view.
*************************************************************************/
static void string_vec_store(struct client_option *op)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  size_t size, i;
  const char *str;

  assert(op != NULL && op->type == CLIENT_OPTION_TYPE_STRING_VEC);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(op->gui_data));
  size = gtk_tree_model_iter_n_children(model, NULL);

  string_vector_reserve(*op->u.string_vec.pvector, size);

  if (!gtk_tree_model_get_iter_first(model, &iter)) {
    return;
  }

  i = 0;
  do {
    gtk_tree_model_get(model, &iter, 0, &str, -1);
    string_vector_set(*op->u.string_vec.pvector, i++, str);
  } while (gtk_tree_model_iter_next(model, &iter));

  string_vector_remove_empty(*op->u.string_vec.pvector);
}

/*************************************************************************
  Set the client options to its new values.
*************************************************************************/
static void apply_option_change(struct client_option *op)
{
  switch (op->type) {
  case CLIENT_OPTION_TYPE_BOOLEAN:
    {
      bool new_value = GTK_TOGGLE_BUTTON(op->gui_data)->active;
      if (*op->u.boolean_o.pvalue != new_value) {
        *op->u.boolean_o.pvalue = new_value;
        if (op->change_callback) {
          (op->change_callback)(op);
        }
      }
    }
    break;
  case CLIENT_OPTION_TYPE_INTEGER:
    {
      int new_value =
          gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(op->gui_data));
      if (*op->u.integer_o.pvalue != new_value) {
        *op->u.integer_o.pvalue = new_value;
        if (op->change_callback) {
          (op->change_callback)(op);
        }
      }
    }
    break;
  case CLIENT_OPTION_TYPE_STRING:
  case CLIENT_OPTION_TYPE_PASSWORD:
    {
      const char *new_value = op->u.string.val_accessor
          ? gtk_entry_get_text(GTK_ENTRY(GTK_BIN(op->gui_data)->child))
          : gtk_entry_get_text(GTK_ENTRY(op->gui_data));
      if (strcmp(op->u.string.pvalue, new_value)) {
        mystrlcpy(op->u.string.pvalue, new_value, op->u.string.size);
        if (op->change_callback) {
          (op->change_callback)(op);
        }
      }
    }
    break;
  case CLIENT_OPTION_TYPE_STRING_VEC:
    if (string_vec_changed(op)) {
      string_vec_store(op);
      if (op->change_callback) {
        (op->change_callback)(op);
      }
    }
    break;
  case CLIENT_OPTION_TYPE_ENUM_LIST:
    {
      GtkTreeIter iter;
      int new_value;

      if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(op->gui_data), &iter)) {
        break;
      }

      gtk_tree_model_get(gtk_combo_box_get_model(GTK_COMBO_BOX(op->gui_data)),
                         &iter, LC_INDEX, &new_value, -1);

      if (*op->u.enum_list.pvalue != new_value) {
        *op->u.enum_list.pvalue = new_value;
        if (op->change_callback) {
          (op->change_callback)(op);
        }
      }
    }
    break;
  case CLIENT_OPTION_TYPE_FILTER:
    if (*op->u.filter.pvalue != op->u.filter.temp) {
      *op->u.filter.pvalue = op->u.filter.temp;
      if (op->change_callback) {
        (op->change_callback)(op);
      }
    }
    break;
  case CLIENT_OPTION_TYPE_VOLUME:
    {
      int new_value =
#ifdef GTK_SCALE_BUTTON
          gtk_scale_button_get_value(GTK_SCALE_BUTTON(op->gui_data));
#else
          gtk_range_get_value(GTK_RANGE(op->gui_data));
#endif /* GTK_SCALE_BUTTON */
      if (*op->u.integer_o.pvalue != new_value) {
        *op->u.integer_o.pvalue = new_value;
        if (op->change_callback) {
          (op->change_callback)(op);
        }
      }
    }
    break;
  }
}

/*************************************************************************
  Set all client options to their new values.
*************************************************************************/
static void apply_changes(void)
{
  client_options_iterate(op) {
    apply_option_change(op);
  } client_options_iterate_end;

  extra_options_iterate(op) {
    op->apply_callback(op->widget);
  } extra_options_iterate_end;
}

/*************************************************************************
  Set the options to its current value.
*************************************************************************/
static void refresh_option(struct client_option *op)
{
  switch (op->type) {
  case CLIENT_OPTION_TYPE_BOOLEAN:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(op->gui_data),
                                 *op->u.boolean_o.pvalue);
    break;
  case CLIENT_OPTION_TYPE_INTEGER:
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(op->gui_data),
                              *op->u.integer_o.pvalue);
    break;
  case CLIENT_OPTION_TYPE_STRING:
  case CLIENT_OPTION_TYPE_PASSWORD:
    if (op->u.string.val_accessor) {
      gtk_entry_set_text(GTK_ENTRY(GTK_BIN(op->gui_data)->child),
                         op->u.string.pvalue);
    } else {
      gtk_entry_set_text(GTK_ENTRY(op->gui_data), op->u.string.pvalue);
    }
    break;
  case CLIENT_OPTION_TYPE_STRING_VEC:
    {
      GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(
                                           GTK_TREE_VIEW(op->gui_data)));
      GtkTreeIter iter;

      gtk_list_store_clear(store);
      string_vector_iterate(*op->u.string_vec.pvector, str) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, str, -1);
      } string_vector_iterate_end;
    }
    break;
  case CLIENT_OPTION_TYPE_ENUM_LIST:
    set_combo_to(GTK_COMBO_BOX(op->gui_data),
                 op->u.enum_list.str_accessor(*op->u.enum_list.pvalue));
    break;
  case CLIENT_OPTION_TYPE_FILTER:
    op->u.filter.temp = *op->u.filter.pvalue;
    update_filter(op);
    break;
  case CLIENT_OPTION_TYPE_VOLUME:
#ifdef GTK_SCALE_BUTTON
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(op->gui_data),
                               *op->u.integer_o.pvalue);
#else
    gtk_range_set_value(GTK_RANGE(op->gui_data), *op->u.integer.pvalue);
#endif /* GTK_SCALE_BUTTON */
    break;
  }
}

/*************************************************************************
  Set all options to their current values.
*************************************************************************/
static void refresh_option_dialog(void)
{
  client_options_iterate(op) {
    refresh_option(op);
  } client_options_iterate_end;

  extra_options_iterate(op) {
    op->refresh_callback(op->widget);
  } extra_options_iterate_end;
}

/*************************************************************************
  Set the option to its default value and do not apply it.
*************************************************************************/
static void reset_option(struct client_option *op)
{
  switch (op->type) {
  case CLIENT_OPTION_TYPE_BOOLEAN:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(op->gui_data),
                                 op->u.boolean_o.def);
    break;
  case CLIENT_OPTION_TYPE_INTEGER:
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(op->gui_data),
                              op->u.integer_o.def);
    break;
  case CLIENT_OPTION_TYPE_STRING:
  case CLIENT_OPTION_TYPE_PASSWORD:
    if (op->u.string.val_accessor) {
      gtk_entry_set_text(GTK_ENTRY(GTK_BIN(op->gui_data)->child),
                         op->u.string.def);
    } else {
      gtk_entry_set_text(GTK_ENTRY(op->gui_data), op->u.string.def);
    }
    break;
  case CLIENT_OPTION_TYPE_STRING_VEC:
    {
      GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(
                                           GTK_TREE_VIEW(op->gui_data)));
      GtkTreeIter iter;
      const char *const *p;

      gtk_list_store_clear(store);
      for (p = op->u.string_vec.def; p && *p; p++) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, *p, -1);
      }
    }
    break;
  case CLIENT_OPTION_TYPE_ENUM_LIST:
    set_combo_to(GTK_COMBO_BOX(op->gui_data),
                 op->u.enum_list.str_accessor(op->u.enum_list.def));
    break;
  case CLIENT_OPTION_TYPE_FILTER:
    op->u.filter.temp = op->u.filter.def;
    update_filter(op);
    break;
  case CLIENT_OPTION_TYPE_VOLUME:
#ifdef GTK_SCALE_BUTTON
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(op->gui_data),
                               op->u.integer_o.def);
#else
    gtk_range_set_value(GTK_RANGE(op->gui_data), op->u.integer.def);
#endif /* GTK_SCALE_BUTTON */
    break;
  }
}

/*************************************************************************
  Set all options to their default values but do not apply it.
*************************************************************************/
static void reset_options(void)
{
  client_options_iterate(op) {
    reset_option(op);
  } client_options_iterate_end;

  extra_options_iterate(op) {
    op->reset_callback(op->widget);
  } extra_options_iterate_end;
}

/*************************************************************************
  Read the option from the rc file and don't apply.
*************************************************************************/
static void reload_option(struct section_file *sf, struct client_option *op)
{
  switch (op->type) {
  case CLIENT_OPTION_TYPE_BOOLEAN:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(op->gui_data),
                                 load_option_bool(sf, op, *op->u.boolean_o.pvalue));
    break;
  case CLIENT_OPTION_TYPE_INTEGER:
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(op->gui_data),
                              load_option_int(sf, op, *op->u.integer_o.pvalue));
    break;
  case CLIENT_OPTION_TYPE_STRING:
  case CLIENT_OPTION_TYPE_PASSWORD:
    if (op->u.string.val_accessor) {
      gtk_entry_set_text(GTK_ENTRY(GTK_BIN(op->gui_data)->child),
                         load_option_string(sf, op, op->u.string.pvalue));
    } else {
      gtk_entry_set_text(GTK_ENTRY(op->gui_data),
                         load_option_string(sf, op, op->u.string.pvalue));
    }
    break;
  case CLIENT_OPTION_TYPE_STRING_VEC:
    {
      GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(op->gui_data));
      GtkListStore *store = GTK_LIST_STORE(model);
      GtkTreeIter iter;
      size_t i;
      size_t size = gtk_tree_model_iter_n_children(model, NULL);
      struct string_vector *vector = string_vector_new();
      const char *def[MAX(size, 1)];

      if (!gtk_tree_model_get_iter_first(model, &iter)) {
        load_option_string_vec(sf, op, NULL, vector);
      } else {
        i = 0;
        do {
          gtk_tree_model_get(model, &iter, 0, &def[i], -1);
          i++;
        } while (gtk_tree_model_iter_next(model, &iter));
        load_option_string_vec(sf, op, def, vector);
      }

      gtk_list_store_clear(store);
      string_vector_iterate(vector, str) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, str, -1);
      } string_vector_iterate_end;

      string_vector_destroy(vector);
    }
    break;
  case CLIENT_OPTION_TYPE_ENUM_LIST:
    set_combo_to(GTK_COMBO_BOX(op->gui_data),
                 op->u.enum_list.str_accessor(load_option_enum_list(
                 sf, op, *op->u.enum_list.pvalue)));
    break;
  case CLIENT_OPTION_TYPE_FILTER:
    op->u.filter.temp = load_option_filter(sf, op, *op->u.filter.pvalue);
    update_filter(op);
    break;
  case CLIENT_OPTION_TYPE_VOLUME:
#ifdef GTK_SCALE_BUTTON
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(op->gui_data),
                               load_option_int(sf, op, *op->u.integer_o.pvalue));
#else
    gtk_range_set_value(GTK_RANGE(op->gui_data),
                        load_option_int(sf, op, *op->u.integer.pvalue));
#endif /* GTK_SCALE_BUTTON */
    break;
  }
}

/*************************************************************************
  Read all options from the rc file but don't apply.
*************************************************************************/
static void reload_options(void)
{
  struct section_file sf;

  if (!section_file_load(&sf, option_file_name())) {
    /* Cannot access to the option file */
    return;
  }

  client_options_iterate(op) {
    reload_option(&sf, op);
  } client_options_iterate_end;

  extra_options_iterate(op) {
    op->reload_callback(op->widget, &sf);
  } extra_options_iterate_end;

  section_file_free(&sf);
}

/*************************************************************************
  ...
*************************************************************************/
static void refresh_option_callback(GtkMenuItem *menuitem, gpointer data)
{
  refresh_option((struct client_option *) data);
}

/*************************************************************************
  ...
*************************************************************************/
static void reset_option_callback(GtkMenuItem *menuitem, gpointer data)
{
  reset_option((struct client_option *) data);
}

/*************************************************************************
  ...
*************************************************************************/
static void reload_option_callback(GtkMenuItem *menuitem, gpointer data)
{
  struct section_file sf;

  if (section_file_load(&sf, option_file_name())) {
    reload_option(&sf, (struct client_option *) data);
    section_file_free(&sf);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void apply_option_callback(GtkMenuItem *menuitem, gpointer data)
{
  apply_option_change((struct client_option *) data);
  /* May have changed from the callback */
  refresh_option((struct client_option *) data);
  init_menus();
}

/*************************************************************************
  Called when a button is pressed.
*************************************************************************/
static gboolean option_callback(GtkWidget *widget, GdkEventButton *event,
                                gpointer data)
{
  /* Only right button please! */
  if (event->button != 3) {
    return FALSE;
  }

  GtkWidget *menu, *item;

  menu = gtk_menu_new();

  item = gtk_image_menu_item_new_with_label(_("Refresh this option"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(refresh_option_callback), data);

  item = gtk_image_menu_item_new_with_label(_("Reset this option"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(reset_option_callback), data);

  item = gtk_image_menu_item_new_with_label(_("Reload this option"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(reload_option_callback), data);

  item =
    gtk_image_menu_item_new_with_label(_("Apply the changes for this option"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
      gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(apply_option_callback), data);

  gtk_widget_show_all(menu);
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, 0);

  return TRUE;
}

/*************************************************************************
  ...
*************************************************************************/
static void filter_changed_callback(GtkToggleButton *togglebutton,
                                    gpointer data)
{
  struct client_option *op = (struct client_option *) data;
  filter value;

  assert(op->type == CLIENT_OPTION_TYPE_FILTER);

  value = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(togglebutton),
                                            "filter_value"));
  if (((op->u.filter.temp & value) != 0)
      ^ GTK_TOGGLE_BUTTON(togglebutton)->active
      && op->u.filter.change(&op->u.filter.temp, value)) {
    update_filter(op);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void option_dialog_destroy(GtkWidget *window)
{
  option_dialog_shell = NULL;
  extra_options_iterate(op) {
    extra_option_destroy(op);
  } extra_options_iterate_end;
  extra_option_list_free(extra_options);
  extra_options = NULL;
}

/*************************************************************************
  ...
*************************************************************************/
static void option_dialog_callback(GtkWidget *window, gint rid)
{
  switch (rid) {
    case RESPONSE_REFRESH:
      refresh_option_dialog();
      break;
    case RESPONSE_RESET:
      reset_options();
      break;
    case RESPONSE_RELOAD:
      reload_options();
      break;
    case RESPONSE_SAVE:
      apply_changes();
      save_options();
      init_menus();
      refresh_option_dialog();
      break;
    case RESPONSE_APPLY:
      apply_changes();
      init_menus();
      refresh_option_dialog();
      break;
    case RESPONSE_OK:
      apply_changes();
      init_menus();
      /* break not missing */
    case RESPONSE_CANCEL:
      gtk_widget_destroy(window);
      break;
  }
}

/****************************************************************
  ...
*****************************************************************/
static void create_option_dialog(void)
{
  GtkWidget *notebook, *sw;
  GtkWidget *vbox[CLIENT_OPTION_CATEGORY_NUM];
  GtkWidget *hbox, *ebox;
  static GtkStyle *style = NULL;
  GtkTooltips *tips;
  int i;
  bool even[CLIENT_OPTION_CATEGORY_NUM];

  if (!style) {
    style = gtk_style_new();
    g_object_ref(style);
    style->bg[GTK_STATE_NORMAL] = style->bg[GTK_STATE_INSENSITIVE];
  }

  tips = gtk_tooltips_new();
  option_dialog_shell =
      gtk_dialog_new_with_buttons(_("Set local options"), NULL, GtkDialogFlags(0),
                                  GTK_STOCK_REFRESH, RESPONSE_REFRESH,
                                  _("Reset"), RESPONSE_RESET,
                                  _("Reload"), RESPONSE_RELOAD,
                                  GTK_STOCK_SAVE, RESPONSE_SAVE,
                                  GTK_STOCK_APPLY, RESPONSE_APPLY,
                                  GTK_STOCK_CANCEL, RESPONSE_CANCEL,
                                  GTK_STOCK_OK, RESPONSE_OK, NULL);
  gtk_widget_set_size_request(option_dialog_shell, -1, 600);
  setup_dialog(option_dialog_shell, toplevel);
  g_signal_connect(option_dialog_shell, "response",
                   G_CALLBACK(option_dialog_callback), NULL);
  g_signal_connect(option_dialog_shell, "destroy",
                   G_CALLBACK(option_dialog_destroy), NULL);

  notebook = gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(option_dialog_shell)->vbox),
                     notebook, TRUE, TRUE, 2);

  /* Build the pages */
  for (i = 0; i < CLIENT_OPTION_CATEGORY_NUM; i++) {
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw,
                             gtk_label_new(
                                 get_option_category_name(
                                     (client_option_category)i)));
    vbox[i] = gtk_vbox_new(FALSE, 2);
    gtk_container_set_border_width(GTK_CONTAINER(vbox[i]), 6);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), vbox[i]);
    even[i] = FALSE;
  }

  /* Add client options */
  client_options_iterate(op) {
    ebox = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX(vbox[op->category]), ebox, FALSE, FALSE, 0);
    g_signal_connect(ebox, "button_press_event",
                     G_CALLBACK(option_callback),
                     op);
    if (even[op->category]) {
      gtk_widget_set_style(ebox, style);
    }
    even[op->category] ^= 1;

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ebox), hbox);

    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_(op->description)),
                       FALSE, FALSE, 5);

    if (op->type == CLIENT_OPTION_TYPE_STRING_VEC) {
      /* Add extra help text for editing. */
      const char *string_vec_help =
        _("You can add, remove, edit or move strings with clicking "
          "with the right button on the entry box. You can click left "
          "to start to edit and drag strings with the left button to "
          "move them.");
      const char *help_text = _(op->help_text); /* Translate now! */
      char buf[strlen(string_vec_help) + strlen(help_text) + 16];

      my_snprintf(buf, sizeof(buf), "%s\n\n%s", help_text, string_vec_help);
      gtk_tooltips_set_tip(tips, ebox, buf, NULL);
    } else {
      gtk_tooltips_set_tip(tips, ebox, _(op->help_text), NULL);
    }

    op->gui_data = NULL;
    switch (op->type) {
    case CLIENT_OPTION_TYPE_BOOLEAN:
      op->gui_data = gtk_check_button_new();
      break;
    case CLIENT_OPTION_TYPE_INTEGER:
      op->gui_data = gtk_spin_button_new_with_range(op->u.integer_o.min,
                         op->u.integer_o.max,
                         MAX((op->u.integer_o.max - op->u.integer_o.min) / 50, 1));
      break;
    case CLIENT_OPTION_TYPE_STRING:
    case CLIENT_OPTION_TYPE_PASSWORD:
      if (op->u.string.val_accessor) {
        const char **val;

        op->gui_data = gtk_combo_box_entry_new_text();
        for (val = op->u.string.val_accessor(); *val; val++) {
          gtk_combo_box_append_text(GTK_COMBO_BOX(op->gui_data), *val);
        }
      } else {
        op->gui_data = gtk_entry_new();
        if (op->type == CLIENT_OPTION_TYPE_PASSWORD) {
          gtk_entry_set_visibility(GTK_ENTRY(op->gui_data), FALSE);
        }
        gtk_widget_set_size_request((GtkWidget*)op->gui_data, 300, -1);
      }
      break;
    case CLIENT_OPTION_TYPE_STRING_VEC:
      {
        GtkListStore *model = gtk_list_store_new(1, G_TYPE_STRING);
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

        op->gui_data = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
        g_object_unref(model);

        gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(op->gui_data), FALSE);
        gtk_tree_view_set_reorderable(GTK_TREE_VIEW(op->gui_data), TRUE);
        g_object_set(renderer, "editable", TRUE, NULL);
        g_signal_connect(renderer,
                         "edited",
                         G_CALLBACK(cell_edited),
                         op);
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(op->gui_data),
                                                    -1, NULL, renderer,
                                                    "text", 0, NULL);
        gtk_widget_set_size_request((GtkWidget*)op->gui_data, 300, -1);

        g_signal_connect(G_OBJECT(op->gui_data),
                         "button-press-event",
                         G_CALLBACK(tree_view_callback),
                         op);
      }
      break;
    case CLIENT_OPTION_TYPE_ENUM_LIST:
      {
        int i;
        const char *str;
        /* Use a double string list for translation */
        GtkListStore *model = gtk_list_store_new(LC_NUM, G_TYPE_INT,
                                                 G_TYPE_STRING, G_TYPE_STRING);
        GtkCellRenderer *renderer;
        GtkTreeIter iter;

        op->gui_data = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
        g_object_unref(model);
        renderer = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(op->gui_data),
                                   renderer, FALSE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(op->gui_data),
                                       renderer, "text",
                                       LC_TEXT_TRANSLATED, NULL);
        for (i = 0; (str = op->u.enum_list.str_accessor(i)); i++) {
          gtk_list_store_append(model, &iter);
          gtk_list_store_set(model, &iter,
                             LC_INDEX, i,
                             LC_TEXT_ORIGIN, str,
                             LC_TEXT_TRANSLATED, _(str), -1);
        }
      }
      break;
    case CLIENT_OPTION_TYPE_FILTER:
      {
        GtkWidget *fbox, *cb;
        filter i;
        const char *str;

        op->gui_data = gtk_frame_new(NULL);
        fbox = gtk_vbox_new(FALSE,0);
        gtk_container_add(GTK_CONTAINER(op->gui_data), fbox);

        for (i = 1; (str = op->u.filter.str_accessor(i)); i <<= 1) {
          cb = gtk_check_button_new_with_label(_(str));
          g_object_set_data(G_OBJECT(cb), "filter_value", GINT_TO_POINTER(i));
          g_signal_connect(cb,
                           "toggled",
                           G_CALLBACK(filter_changed_callback),
                           op);
          gtk_box_pack_start(GTK_BOX(fbox), cb, FALSE, FALSE, 0);
        }
      }
      break;
    case CLIENT_OPTION_TYPE_VOLUME:
      {
#ifdef GTK_SCALE_BUTTON
        GtkObject *adjustment;

        op->gui_data = gtk_volume_button_new();
        adjustment = gtk_adjustment_new(*op->u.integer_o.pvalue,
                         op->u.integer_o.min,
                         op->u.integer_o.max,
                         MAX((op->u.integer_o.max - op->u.integer_o.min) / 50, 1),
                         0, 1);
        gtk_scale_button_set_adjustment(GTK_SCALE_BUTTON(op->gui_data),
                                        GTK_ADJUSTMENT(adjustment));
        gtk_object_destroy(adjustment);
#else
        op->gui_data = gtk_hscale_new_with_range(op->u.integer.min,
                           op->u.integer.max,
                           MAX((op->u.integer.max - op->u.integer.min) / 50, 1));
        gtk_widget_set_size_request(GTK_WIDGET(op->gui_data),
                                    op->u.integer.max - op->u.integer.min, -1);
#endif /* GTK_SCALE_BUTTON */
      }
      break;
    }
    if (op->gui_data) {
      gtk_box_pack_end(GTK_BOX(hbox), (GtkWidget*)op->gui_data, FALSE, FALSE, 0);
    } else {
      die("Couldn't create a line for option '%s'.", op->name);
    }
  } client_options_iterate_end;

  /* Extra options (e.g. chat colors) */
  extra_options = extra_option_list_new();

  extra_option_new(create_chatline_config(), vbox[CLIENT_OPTION_CATEGORY_CHAT],
                   _("Chat colors configuration"),
                   apply_chatline_config, refresh_chatline_config,
                   reset_chatline_config, reload_chatline_config);
  extra_option_new(create_messages_configuration(), vbox[CLIENT_OPTION_CATEGORY_MESSAGE],
                   _("Message options"),
                   apply_message_options, refresh_message_options,
                   reset_message_options, reload_message_options);
  if (get_client_state() >= CLIENT_SELECT_RACE_STATE) {
    extra_option_new(create_worklists_report(), vbox[CLIENT_OPTION_CATEGORY_GAMEPLAY],
                     _("Global Worklists"),
                     apply_global_worklists, refresh_global_worklists,
                     reset_global_worklists, reload_global_worklists);
  }

  gtk_widget_show_all(GTK_DIALOG(option_dialog_shell)->vbox);
  gtk_widget_show(option_dialog_shell);
}

/****************************************************************
...
*****************************************************************/
void popup_option_dialog(void)
{
  if (!option_dialog_shell) {
    create_option_dialog();
  }
  refresh_option_dialog();
}

/****************************************************************
  ...
*****************************************************************/
const GtkWidget *get_option_dialog_shell(void)
{
  return option_dialog_shell;
}
