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
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "support.h"

#include "game.h"
#include "government.h"
#include "map.h"
#include "packets.h"
#include "player.h"

#include "civclient.h"
#include "climisc.h"
#include "clinet.h"
#include "connectdlg_common.h"
#include "control.h"
#include "goto.h"
#include "multiselect.h"
#include "options.h"
#include "packhand.h"
#include "tilespec.h"
#include "trade.h"

#include "chatline.h"
#include "citydlg.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "menu.h"
#include "optiondlg.h"
#include "wldlg.h"

#include "dialogs.h"

/******************************************************************/
GtkWidget *message_dialog_start(GtkWindow *parent, const gchar *name,
				const gchar *text);
void message_dialog_add(GtkWidget *dshell, const gchar *label,
			GCallback handler, gpointer data);
void message_dialog_end(GtkWidget *dshell);

void message_dialog_set_hide(GtkWidget *dshell, gboolean setting);

/******************************************************************/
static GtkWidget  *races_shell;
static GtkWidget  *races_nation_list;
static GtkWidget  *races_leader;
static int        num_races_leader = -1;
static GtkWidget  *races_sex[2];
static GtkWidget  *races_city_style_list;
static GtkTextBuffer *races_text;

/******************************************************************/
static GtkWidget  *spy_tech_shell;
static int         steal_advance;

/******************************************************************/
static GtkWidget  *spy_sabotage_shell;
static int         sabotage_improvement;

/******************************************************************/
enum {
  SELECT_UNIT_READY = 1,
  SELECT_UNIT_SENTRY,
  SELECT_UNIT_SELECT,
  SELECT_UNIT_ADD_TO_FOCUS
};

static GtkWidget *unit_select_dialog_shell;
static GtkTreeStore *unit_select_store;
static GtkWidget *unit_select_view;
static GtkTreePath *unit_select_path;
static struct tile *unit_select_ptile;
static GtkTreeSelection *unit_select_selection;

static void select_random_race(void);
  
static void create_races_dialog(void);
static void races_destroy_callback(GtkWidget *w, gpointer data);
static void races_response(GtkWidget *w, gint response, gpointer data);
static void races_nation_callback(GtkTreeSelection *select, gpointer data);
static void races_leader_callback(void);
static void races_sex_callback(GtkWidget *w, gpointer data);
static void races_city_style_callback(GtkTreeSelection *select, gpointer data);
static gboolean races_selection_func(GtkTreeSelection *select,
				     GtkTreeModel *model, GtkTreePath *path,
				     gboolean selected, gpointer data);

static int selected_nation;
static int selected_sex;
static int selected_city_style;

static int is_showing_pillage_dialog = FALSE;
static int unit_to_use_to_pillage;

static int caravan_city_id;
static int caravan_unit_id;

static GtkWidget *diplomat_dialog;
static int diplomat_id;
static int diplomat_target_id;

static GtkWidget *caravan_dialog;

/**************************************************************************
  Popup a generic dialog to display some generic information.
**************************************************************************/
void popup_notify_dialog(const char *caption, const char *headline,
			 const char *lines)
{
  static struct gui_dialog *shell;
  GtkWidget *vbox, *label, *headline_label, *sw;

  if (enable_tabs && prevent_duplicate_notify_tabs) {
    while (NULL != (shell = find_gui_dialog_by_title(caption))) {
      gui_dialog_destroy(shell);
    }
  }

  gui_dialog_new(&shell, GTK_NOTEBOOK(bottom_notebook));
  gui_dialog_set_title(shell, caption);

  gui_dialog_add_button(shell, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE); 
  gui_dialog_set_default_response(shell, GTK_RESPONSE_CLOSE);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(shell->vbox), vbox, TRUE, TRUE, 0);

  headline_label = gtk_label_new(headline);   
  gtk_box_pack_start(GTK_BOX(vbox), headline_label, FALSE, FALSE, 0);
  gtk_widget_set_name(headline_label, "notify label");

  gtk_label_set_justify(GTK_LABEL(headline_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment(GTK_MISC(headline_label), 0.0, 0.0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  label = gtk_label_new(lines);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), label);

  gtk_widget_set_name(label, "notify label");
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);

  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

  gui_dialog_show_all(shell);

  gui_dialog_set_default_size(shell, -1, 265);
  gui_dialog_present(shell);

  shell = NULL;
}

/****************************************************************
...
*****************************************************************/
static void notify_goto_response(GtkWidget *w, gint response)
{
  struct city *pcity = NULL;
  struct tile *ptile = g_object_get_data(G_OBJECT(w), "tile");

  switch (response) {
  case 1:
    center_tile_mapcanvas(ptile);
    break;
  case 2:
    pcity = map_get_city(ptile);

    if (center_when_popup_city) {
      center_tile_mapcanvas(ptile);
    }

    if (pcity) {
      popup_city_dialog(pcity, 0);
    }
    break;
  }
  gtk_widget_destroy(w);
}

/**************************************************************************
  Popup a dialog to display information about an event that has a
  specific location.  The user should be given the option to goto that
  location.
**************************************************************************/
void popup_notify_goto_dialog(const char *headline, const char *lines,
			      struct tile *ptile)
{
  GtkWidget *shell, *label, *goto_command, *popcity_command;
  
  shell = gtk_dialog_new_with_buttons(headline,
        NULL,
        0,
        NULL);
  setup_dialog(shell, toplevel);
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_CLOSE);
  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_CENTER_ON_PARENT);

  label = gtk_label_new(lines);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(shell)->vbox), label);
  gtk_widget_show(label);
  
  goto_command = gtk_stockbutton_new(GTK_STOCK_JUMP_TO,
	_("_Goto location"));
  gtk_dialog_add_action_widget(GTK_DIALOG(shell), goto_command, 1);
  gtk_widget_show(goto_command);

  popcity_command = gtk_stockbutton_new(GTK_STOCK_ZOOM_IN,
	_("_Popup City"));
  gtk_dialog_add_action_widget(GTK_DIALOG(shell), popcity_command, 2);
  gtk_widget_show(popcity_command);

  gtk_dialog_add_button(GTK_DIALOG(shell), GTK_STOCK_CLOSE,
			GTK_RESPONSE_CLOSE);

  if (!ptile) {
    gtk_widget_set_sensitive(goto_command, FALSE);
    gtk_widget_set_sensitive(popcity_command, FALSE);
  } else {
    struct city *pcity;

    pcity = map_get_city(ptile);
    gtk_widget_set_sensitive(popcity_command,
      (pcity && pcity->owner == get_player_idx()));
  }

  g_object_set_data(G_OBJECT(shell), "tile", ptile);

  g_signal_connect(shell, "response", G_CALLBACK(notify_goto_response), NULL);
  gtk_widget_show(shell);
}


/****************************************************************
...
*****************************************************************/
static void bribe_response(GtkWidget *w, gint response)
{
  if (response == GTK_RESPONSE_YES) {
    request_diplomat_action(DIPLOMAT_BRIBE, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(w);
}

/****************************************************************
...  Ask the server how much the bribe is
*****************************************************************/
static void diplomat_bribe_callback(GtkWidget *w, gpointer data)
{
  if (find_unit_by_id(diplomat_id) && find_unit_by_id(diplomat_target_id)) {
    dsend_packet_unit_bribe_inq(&aconnection, diplomat_target_id);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
void popup_bribe_dialog(struct unit *punit, int cost)
{
  GtkWidget *shell;

  if (unit_flag(punit, F_UNBRIBABLE)) {
    shell = popup_message_dialog(GTK_WINDOW(toplevel), _("Ooops..."),
                                 _("This unit cannot be bribed!"),
                                 GTK_STOCK_OK, NULL, NULL, NULL);
    gtk_window_present(GTK_WINDOW(shell));
    return;
  } else if (get_player_ptr()->economic.gold >= cost) {
    shell = gtk_message_dialog_new(NULL, 0,
      GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
      _("Bribe unit for %d gold?\nTreasury contains %d gold."),
      cost, get_player_ptr()->economic.gold);
    gtk_window_set_title(GTK_WINDOW(shell), _("Bribe Enemy Unit"));
    setup_dialog(shell, toplevel);
  } else {
    shell = gtk_message_dialog_new(NULL,
      0,
      GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
      _("Bribing the unit costs %d gold.\nTreasury contains %d gold."),
      cost, get_player_ptr()->economic.gold);
    gtk_window_set_title(GTK_WINDOW(shell), _("Traitors Demand Too Much!"));
    setup_dialog(shell, toplevel);
  }
  gtk_window_present(GTK_WINDOW(shell));
  
  g_signal_connect(shell, "response", G_CALLBACK(bribe_response), NULL);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_sabotage_callback(GtkWidget *w, gpointer data)
{
  if(find_unit_by_id(diplomat_id) && 
     find_city_by_id(diplomat_target_id)) { 
    request_diplomat_action(DIPLOMAT_SABOTAGE, diplomat_id,
			    diplomat_target_id, -1);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_investigate_callback(GtkWidget *w, gpointer data)
{
  if(find_unit_by_id(diplomat_id) && 
     (find_city_by_id(diplomat_target_id))) { 
    request_diplomat_action(DIPLOMAT_INVESTIGATE, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void spy_sabotage_unit_callback(GtkWidget *w, gpointer data)
{
  request_diplomat_action(SPY_SABOTAGE_UNIT, diplomat_id,
			  diplomat_target_id, 0);
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_embassy_callback(GtkWidget *w, gpointer data)
{
  if(find_unit_by_id(diplomat_id) && 
     (find_city_by_id(diplomat_target_id))) { 
    request_diplomat_action(DIPLOMAT_EMBASSY, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void spy_poison_callback(GtkWidget *w, gpointer data)
{
  if(find_unit_by_id(diplomat_id) &&
     (find_city_by_id(diplomat_target_id))) {
    request_diplomat_action(SPY_POISON, diplomat_id, diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_steal_callback(GtkWidget *w, gpointer data)
{
  if(find_unit_by_id(diplomat_id) && 
     find_city_by_id(diplomat_target_id)) { 
    request_diplomat_action(DIPLOMAT_STEAL, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void spy_advances_response(GtkWidget *w, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_ACCEPT && steal_advance > 0) {
    if (find_unit_by_id(diplomat_id) && 
        find_city_by_id(diplomat_target_id)) { 
      request_diplomat_action(DIPLOMAT_STEAL, diplomat_id,
			      diplomat_target_id, steal_advance);
    }
  }
  gtk_widget_destroy(spy_tech_shell);
  spy_tech_shell = NULL;
}

/****************************************************************
...
*****************************************************************/
static void spy_advances_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;

  if (gtk_tree_selection_get_selected(select, &model, &it)) {
    gtk_tree_model_get(model, &it, 1, &steal_advance, -1);
    
    gtk_dialog_set_response_sensitive(GTK_DIALOG(spy_tech_shell),
      GTK_RESPONSE_ACCEPT, TRUE);
  } else {
    steal_advance = 0;
	  
    gtk_dialog_set_response_sensitive(GTK_DIALOG(spy_tech_shell),
      GTK_RESPONSE_ACCEPT, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
static void create_advances_list(struct player *pplayer,
				 struct player *pvictim)
{  
  GtkWidget *sw, *label, *vbox, *view;
  int i;
  GtkListStore *store;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;

  spy_tech_shell = gtk_dialog_new_with_buttons(_("Steal Technology"),
    NULL,
    0,
    GTK_STOCK_CANCEL,
    GTK_RESPONSE_CANCEL,
    _("_Steal"),
    GTK_RESPONSE_ACCEPT,
    NULL);
  setup_dialog(spy_tech_shell, toplevel);
  gtk_window_set_position(GTK_WINDOW(spy_tech_shell), GTK_WIN_POS_MOUSE);

  gtk_dialog_set_default_response(GTK_DIALOG(spy_tech_shell),
				  GTK_RESPONSE_ACCEPT);

  label = gtk_frame_new(_("Select Advance to Steal"));
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(spy_tech_shell)->vbox), label);

  vbox = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(label), vbox);
      
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(NULL, rend,
						 "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  label = g_object_new(GTK_TYPE_LABEL,
    "use-underline", TRUE,
    "mnemonic-widget", view,
    "label", _("_Advances:"),
    "xalign", 0.0,
    "yalign", 0.5,
    NULL);
  gtk_container_add(GTK_CONTAINER(vbox), label);
  
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(sw), view);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_widget_set_size_request(sw, -1, 200);
  
  gtk_container_add(GTK_CONTAINER(vbox), sw);

  /* Now populate the list */
  if (pvictim) { /* you don't want to know what lag can do -- Syela */
    GtkTreeIter it;
    GValue value = { 0, };

    for(i=A_FIRST; i<game.ruleset_control.num_tech_types; i++) {
      if(get_invention(pvictim, i)==TECH_KNOWN && 
	 (get_invention(pplayer, i)==TECH_UNKNOWN || 
	  get_invention(pplayer, i)==TECH_REACHABLE)) {
	gtk_list_store_append(store, &it);

	g_value_init(&value, G_TYPE_STRING);
	g_value_set_static_string(&value,
				  get_tech_name(get_player_ptr(), i));
	gtk_list_store_set_value(store, &it, 0, &value);
	g_value_unset(&value);
	gtk_list_store_set(store, &it, 1, i, -1);
      }
    }

    gtk_list_store_append(store, &it);

    g_value_init(&value, G_TYPE_STRING);
    g_value_set_static_string(&value, _("At Spy's Discretion"));
    gtk_list_store_set_value(store, &it, 0, &value);
    g_value_unset(&value);
    gtk_list_store_set(store, &it, 1, game.ruleset_control.num_tech_types, -1);
  }

  gtk_dialog_set_response_sensitive(GTK_DIALOG(spy_tech_shell),
    GTK_RESPONSE_ACCEPT, FALSE);
  
  gtk_widget_show_all(GTK_DIALOG(spy_tech_shell)->vbox);

  g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), "changed",
		   G_CALLBACK(spy_advances_callback), NULL);
  g_signal_connect(spy_tech_shell, "response",
		   G_CALLBACK(spy_advances_response), NULL);
  
  steal_advance = 0;

  gtk_tree_view_focus(GTK_TREE_VIEW(view));
}

/****************************************************************
...
*****************************************************************/
static void spy_improvements_response(GtkWidget *w, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_ACCEPT && sabotage_improvement > -2) {
    if (find_unit_by_id(diplomat_id) && 
        find_city_by_id(diplomat_target_id)) { 
      request_diplomat_action(DIPLOMAT_SABOTAGE, diplomat_id,
			      diplomat_target_id, sabotage_improvement + 1);
    }
  }
  gtk_widget_destroy(spy_sabotage_shell);
  spy_sabotage_shell = NULL;
}

/****************************************************************
...
*****************************************************************/
static void spy_improvements_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;

  if (gtk_tree_selection_get_selected(select, &model, &it)) {
    gtk_tree_model_get(model, &it, 1, &sabotage_improvement, -1);
    
    gtk_dialog_set_response_sensitive(GTK_DIALOG(spy_sabotage_shell),
      GTK_RESPONSE_ACCEPT, TRUE);
  } else {
    sabotage_improvement = -2;
	  
    gtk_dialog_set_response_sensitive(GTK_DIALOG(spy_sabotage_shell),
      GTK_RESPONSE_ACCEPT, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
static void create_improvements_list(struct player *pplayer,
				     struct city *pcity)
{  
  GtkWidget *sw, *label, *vbox, *view;
  GtkListStore *store;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;
  GtkTreeIter it;
  
  spy_sabotage_shell = gtk_dialog_new_with_buttons(_("Sabotage Improvements"),
    NULL,
    0,
    GTK_STOCK_CANCEL,
    GTK_RESPONSE_CANCEL,
    _("_Sabotage"), 
    GTK_RESPONSE_ACCEPT,
    NULL);
  setup_dialog(spy_sabotage_shell, toplevel);
  gtk_window_set_position(GTK_WINDOW(spy_sabotage_shell), GTK_WIN_POS_MOUSE);

  gtk_dialog_set_default_response(GTK_DIALOG(spy_sabotage_shell),
				  GTK_RESPONSE_ACCEPT);

  label = gtk_frame_new(_("Select Improvement to Sabotage"));
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(spy_sabotage_shell)->vbox), label);

  vbox = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(label), vbox);
      
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(NULL, rend,
						 "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  label = g_object_new(GTK_TYPE_LABEL,
    "use-underline", TRUE,
    "mnemonic-widget", view,
    "label", _("_Improvements:"),
    "xalign", 0.0,
    "yalign", 0.5,
    NULL);
  gtk_container_add(GTK_CONTAINER(vbox), label);
  
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(sw), view);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_widget_set_size_request(sw, -1, 200);
  
  gtk_container_add(GTK_CONTAINER(vbox), sw);

  /* Now populate the list */
  gtk_list_store_append(store, &it);
  gtk_list_store_set(store, &it, 0, _("City Production"), 1, -1, -1);

  built_impr_iterate(pcity, i) {
    if (get_improvement_type(i)->sabotage > 0) {
      gtk_list_store_append(store, &it);
      gtk_list_store_set(store, &it, 0, get_impr_name_ex(pcity, i), 1, i, -1);
    }  
  } built_impr_iterate_end;

  gtk_list_store_append(store, &it);
  gtk_list_store_set(store, &it, 0, _("At Spy's Discretion"), 1, B_LAST, -1);

  gtk_dialog_set_response_sensitive(GTK_DIALOG(spy_sabotage_shell),
    GTK_RESPONSE_ACCEPT, FALSE);
  
  gtk_widget_show_all(GTK_DIALOG(spy_sabotage_shell)->vbox);

  g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), "changed",
		   G_CALLBACK(spy_improvements_callback), NULL);
  g_signal_connect(spy_sabotage_shell, "response",
		   G_CALLBACK(spy_improvements_response), NULL);

  sabotage_improvement = -2;
	  
  gtk_tree_view_focus(GTK_TREE_VIEW(view));
}

/****************************************************************
...
*****************************************************************/
static void spy_steal_popup(GtkWidget *w, gpointer data)
{
  struct city *pvcity = find_city_by_id(diplomat_target_id);
  struct player *pvictim = NULL;

  if(pvcity)
    pvictim = city_owner(pvcity);

/* it is concievable that pvcity will not be found, because something
has happened to the city during latency.  Therefore we must initialize
pvictim to NULL and account for !pvictim in create_advances_list. -- Syela */
  
  if(!spy_tech_shell){
    create_advances_list(get_player_ptr(), pvictim);
    gtk_window_present(GTK_WINDOW(spy_tech_shell));
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
 Requests up-to-date list of improvements, the return of
 which will trigger the popup_sabotage_dialog() function.
*****************************************************************/
static void spy_request_sabotage_list(GtkWidget *w, gpointer data)
{
  if(find_unit_by_id(diplomat_id) &&
     (find_city_by_id(diplomat_target_id))) {
    request_diplomat_action(SPY_GET_SABOTAGE_LIST, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
 Pops-up the Spy sabotage dialog, upon return of list of
 available improvements requested by the above function.
*****************************************************************/
void popup_sabotage_dialog(struct city *pcity)
{
  if(!spy_sabotage_shell){
    create_improvements_list(get_player_ptr(), pcity);
    gtk_window_present(GTK_WINDOW(spy_sabotage_shell));
  }
}

/****************************************************************
...  Ask the server how much the revolt is going to cost us
*****************************************************************/
static void diplomat_incite_callback(GtkWidget *w, gpointer data)
{
  if (find_unit_by_id(diplomat_id) && find_city_by_id(diplomat_target_id)) {
    dsend_packet_city_incite_inq(&aconnection, diplomat_target_id);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void incite_response(GtkWidget *w, gint response)
{
  if (response == GTK_RESPONSE_YES) {
    request_diplomat_action(DIPLOMAT_INCITE, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(w);
}

/****************************************************************
Popup the yes/no dialog for inciting, since we know the cost now
*****************************************************************/
void popup_incite_dialog(struct city *pcity, int cost)
{
  int gold = get_player_ptr()->economic.gold;
  GtkWidget *shell;

  if (cost == INCITE_IMPOSSIBLE_COST) {
    shell = gtk_message_dialog_new(NULL, 0,
				   GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
				   _("You can't incite a revolt in %s."),
				   pcity->name);
    gtk_window_set_title(GTK_WINDOW(shell), _("City can't be incited!"));
    setup_dialog(shell, toplevel);
  } else if (gold >= cost) {
    shell = gtk_message_dialog_new(NULL, 0,
				   GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
				   _("Incite a revolt for %d gold?\n"
				     "Treasury contains %d gold."),
				   cost, gold);
    gtk_window_set_title(GTK_WINDOW(shell), _("Incite a Revolt!"));
    setup_dialog(shell, toplevel);
  } else {
    shell = gtk_message_dialog_new(NULL, 0,
				   GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
				   _("Inciting a revolt costs %d gold.\n"
				     "Treasury contains %d gold."),
				   cost, gold);
    gtk_window_set_title(GTK_WINDOW(shell), _("Traitors Demand Too Much!"));
    setup_dialog(shell, toplevel);
  }
  gtk_window_present(GTK_WINDOW(shell));
  
  g_signal_connect(shell, "response", G_CALLBACK(incite_response), NULL);
}


/****************************************************************
  Callback from diplomat/spy dialog for "keep moving".
  (This should only occur when entering allied city.)
*****************************************************************/
static void diplomat_keep_moving_callback(GtkWidget *w, gpointer data)
{
  struct unit *punit;
  struct city *pcity;
  
  if( (punit=find_unit_by_id(diplomat_id))
      && (pcity=find_city_by_id(diplomat_target_id))
      && !same_pos(punit->tile, pcity->tile)) {
    request_diplomat_action(DIPLOMAT_MOVE, diplomat_id,
			    diplomat_target_id, 0);
  }
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_destroy_callback(GtkWidget *w, gpointer data)
{
  diplomat_dialog = NULL;
  process_diplomat_arrival(NULL, 0);
}


/****************************************************************
...
*****************************************************************/
static void diplomat_cancel_callback(GtkWidget *w, gpointer data)
{
  gtk_widget_destroy(diplomat_dialog);
}

/****************************************************************
...
*****************************************************************/
void popup_diplomat_dialog(struct unit *punit, struct tile *dest_tile)
{
  struct city *pcity;
  struct unit *ptunit;
  GtkWidget *shl;
  char buf[128];

  diplomat_id = punit->id;

  if ((pcity = map_get_city(dest_tile))) {
    /* Spy/Diplomat acting against a city */

    diplomat_target_id = pcity->id;
    my_snprintf(buf, sizeof(buf),
		_("Your %s has arrived at %s.\nWhat is your command?"),
		unit_name(punit->type), pcity->name);

    if (!unit_flag(punit, F_SPY)){
      shl = popup_message_dialog(GTK_WINDOW(toplevel),
	_(" Choose Your Diplomat's Strategy"), buf,
	_("Establish _Embassy"), diplomat_embassy_callback, NULL,
	_("_Investigate City"), diplomat_investigate_callback, NULL,
	_("_Sabotage City"), diplomat_sabotage_callback, NULL,
	_("Steal _Technology"), diplomat_steal_callback, NULL,
	_("Incite a _Revolt"), diplomat_incite_callback, NULL,
	_("_Keep moving"), diplomat_keep_moving_callback, NULL,
	GTK_STOCK_CANCEL, diplomat_cancel_callback, NULL,
	NULL);

      if (!diplomat_can_do_action(punit, DIPLOMAT_EMBASSY, dest_tile))
	message_dialog_button_set_sensitive(shl, 0, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_INVESTIGATE, dest_tile))
	message_dialog_button_set_sensitive(shl, 1, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_SABOTAGE, dest_tile))
	message_dialog_button_set_sensitive(shl, 2, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_STEAL, dest_tile))
	message_dialog_button_set_sensitive(shl, 3, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_INCITE, dest_tile))
	message_dialog_button_set_sensitive(shl, 4, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_MOVE, dest_tile))
	message_dialog_button_set_sensitive(shl, 5, FALSE);
    } else {
       shl = popup_message_dialog(GTK_WINDOW(toplevel),
	_("Choose Your Spy's Strategy"), buf,
	_("Establish _Embassy"), diplomat_embassy_callback, NULL,
	_("_Investigate City"), diplomat_investigate_callback, NULL,
	_("_Poison City"), spy_poison_callback, NULL,
	_("Industrial _Sabotage"), spy_request_sabotage_list, NULL,
	_("Steal _Technology"), spy_steal_popup, NULL,
	_("Incite a _Revolt"), diplomat_incite_callback, NULL,
	_("_Keep moving"), diplomat_keep_moving_callback, NULL,
	GTK_STOCK_CANCEL, diplomat_cancel_callback, NULL,
	NULL);

      if (!diplomat_can_do_action(punit, DIPLOMAT_EMBASSY, dest_tile))
	message_dialog_button_set_sensitive(shl, 0, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_INVESTIGATE, dest_tile))
	message_dialog_button_set_sensitive(shl, 1, FALSE);
      if (!diplomat_can_do_action(punit, SPY_POISON, dest_tile))
	message_dialog_button_set_sensitive(shl, 2, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_SABOTAGE, dest_tile))
	message_dialog_button_set_sensitive(shl, 3, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_STEAL, dest_tile))
	message_dialog_button_set_sensitive(shl, 4, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_INCITE, dest_tile))
	message_dialog_button_set_sensitive(shl, 5, FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_MOVE, dest_tile))
	message_dialog_button_set_sensitive(shl, 6, FALSE);
     }

    diplomat_dialog = shl;

    message_dialog_set_hide(shl, TRUE);
    g_signal_connect(shl, "destroy",
		     G_CALLBACK(diplomat_destroy_callback), NULL);
    g_signal_connect(shl, "delete_event",
		     G_CALLBACK(diplomat_cancel_callback), NULL);
  } else { 
    if ((ptunit = unit_list_get(dest_tile->units, 0))){
      /* Spy/Diplomat acting against a unit */ 
       
      diplomat_target_id = ptunit->id;
 
      shl = popup_message_dialog(GTK_WINDOW(toplevel),
	_("Subvert Enemy Unit"),
	(!unit_flag(punit, F_SPY))?
	_("Sir, the diplomat is waiting for your command"):
	_("Sir, the spy is waiting for your command"),
	_("_Bribe Enemy Unit"), diplomat_bribe_callback, NULL,
	_("_Sabotage Enemy Unit"), spy_sabotage_unit_callback, NULL,
	GTK_STOCK_CANCEL, diplomat_cancel_callback, NULL,
	NULL);

      if (!diplomat_can_do_action(punit, DIPLOMAT_BRIBE, dest_tile)) {
	message_dialog_button_set_sensitive(shl, 0, FALSE);
      }
      if (!diplomat_can_do_action(punit, SPY_SABOTAGE_UNIT, dest_tile)) {
	message_dialog_button_set_sensitive(shl, 1, FALSE);
      }

      diplomat_dialog = shl;

      message_dialog_set_hide(shl, TRUE);
      g_signal_connect(shl, "destroy",
		       G_CALLBACK(diplomat_destroy_callback), NULL);
      g_signal_connect(shl, "delete_event",
		       G_CALLBACK(diplomat_cancel_callback), NULL);
    }
  }
}

/****************************************************************
...
*****************************************************************/
bool diplomat_dialog_is_open(void)
{
  return diplomat_dialog != NULL;
}

/****************************************************************
...
*****************************************************************/
static void caravan_establish_trade_callback(GtkWidget *w, gpointer data)
{
  dsend_packet_unit_establish_trade(&aconnection, caravan_unit_id);
}


/****************************************************************
...
*****************************************************************/
static void caravan_help_build_wonder_callback(GtkWidget *w, gpointer data)
{
  dsend_packet_unit_help_build_wonder(&aconnection, caravan_unit_id);
}


/****************************************************************
...
*****************************************************************/
static void caravan_destroy_callback(GtkWidget *w, gpointer data)
{
  caravan_dialog = NULL;
  process_caravan_arrival(NULL);
}



/****************************************************************
...
*****************************************************************/
void popup_caravan_dialog(struct unit *punit,
			  struct city *phomecity, struct city *pdestcity)
{
  char buf[128];
  bool can_establish, can_trade;
  
  my_snprintf(buf, sizeof(buf),
	      _("Your caravan from %s reaches the city of %s.\nWhat now?"),
	      phomecity->name, pdestcity->name);
  
  caravan_city_id=pdestcity->id; /* callbacks need these */
  caravan_unit_id=punit->id;
  
  can_trade = can_cities_trade(phomecity, pdestcity);
  can_establish = can_trade
  		  && can_establish_trade_route(phomecity, pdestcity);
  
  caravan_dialog = popup_message_dialog(GTK_WINDOW(toplevel),
    _("Your Caravan Has Arrived"), 
    buf,
    (can_establish ? _("Establish _Traderoute") :
    _("Enter Marketplace")),caravan_establish_trade_callback, NULL,
    _("Help build _Wonder"),caravan_help_build_wonder_callback, NULL,
    _("_Keep moving"), NULL, NULL,
    NULL);

  g_signal_connect(caravan_dialog, "destroy",
		   G_CALLBACK(caravan_destroy_callback), NULL);
  
  if (!can_trade) {
    message_dialog_button_set_sensitive(caravan_dialog, 0, FALSE);
  }
  
  if (!unit_can_help_build_wonder(punit, pdestcity)) {
    message_dialog_button_set_sensitive(caravan_dialog, 1, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
bool caravan_dialog_is_open(void)
{
  return caravan_dialog != NULL;
}

/****************************************************************
...
*****************************************************************/
static void revolution_response(GtkWidget *w, gint response, gpointer data)
{
  int government = GPOINTER_TO_INT(data);

  if (response == GTK_RESPONSE_YES) {
    if (government == -1) {
      start_revolution();
    } else {
      set_government_choice(government);
    }
  }
  if (w) {
    gtk_widget_destroy(w);
  }
}

/****************************************************************
...
*****************************************************************/
void popup_revolution_dialog(int government)
{
  static GtkWidget *shell = NULL;

  if (get_player_ptr()->revolution_finishes == -1) {
    if (!shell) {
      shell = gtk_message_dialog_new(NULL,
	  0,
	  GTK_MESSAGE_WARNING,
	  GTK_BUTTONS_YES_NO,
	  _("You say you wanna revolution?"));
      gtk_window_set_title(GTK_WINDOW(shell), _("Revolution!"));
      setup_dialog(shell, toplevel);

      g_signal_connect(shell, "destroy",
	  G_CALLBACK(gtk_widget_destroyed), &shell);
    }
    g_signal_connect(shell, "response",
	G_CALLBACK(revolution_response), GINT_TO_POINTER(government));

    gtk_window_present(GTK_WINDOW(shell));
  } else {
    revolution_response(shell, GTK_RESPONSE_YES, GINT_TO_POINTER(government));
  }
}


/****************************************************************
...
*****************************************************************/
static void pillage_callback(GtkWidget *w, gpointer data)
{
  if (data) {
    struct unit *punit = find_unit_by_id(unit_to_use_to_pillage);
    if (punit) {
      request_new_unit_activity_targeted(punit,
					 ACTIVITY_PILLAGE,
					 GPOINTER_TO_INT(data));
    }
  }
}

/****************************************************************
...
*****************************************************************/
static void pillage_destroy_callback(GtkWidget *w, gpointer data)
{
  is_showing_pillage_dialog = FALSE;
}

/****************************************************************
...
*****************************************************************/
void popup_pillage_dialog(struct unit *punit,
			  enum tile_special_type may_pillage)
{
  GtkWidget *shl;

  if (!is_showing_pillage_dialog) {
    is_showing_pillage_dialog = TRUE;
    unit_to_use_to_pillage = punit->id;

    shl = message_dialog_start(GTK_WINDOW(toplevel),
			       _("What To Pillage"),
			       _("Select what to pillage:"));

    while (may_pillage != S_NO_SPECIAL) {
      enum tile_special_type what = get_preferred_pillage(may_pillage);

      message_dialog_add(shl, map_get_infrastructure_text(what),
			 G_CALLBACK(pillage_callback), GINT_TO_POINTER(what));

      may_pillage &= (~(what | map_get_infrastructure_prerequisite(what)));
    }

    message_dialog_add(shl, GTK_STOCK_CANCEL, 0, 0);

    message_dialog_end(shl);

    g_signal_connect(shl, "destroy", G_CALLBACK(pillage_destroy_callback),
		     NULL);   
  }
}

/****************************************************************
...
*****************************************************************/
void message_dialog_button_set_sensitive(GtkWidget *shl, int button,
					 gboolean state)
{
  char button_name[512];
  GtkWidget *b;

  my_snprintf(button_name, sizeof(button_name), "button%d", button);

  b = g_object_get_data(G_OBJECT(shl), button_name);
  gtk_widget_set_sensitive(b, state);
}

/****************************************************************
...
*****************************************************************/
GtkWidget *message_dialog_start(GtkWindow *parent, const gchar *name,
				const gchar *text)
{
  GtkWidget *dshell, *dlabel, *vbox, *bbox;

  dshell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  setup_dialog(dshell, toplevel);
  gtk_window_set_position (GTK_WINDOW(dshell), GTK_WIN_POS_MOUSE);

  gtk_window_set_title(GTK_WINDOW(dshell), name);

  gtk_window_set_transient_for(GTK_WINDOW(dshell), parent);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(dshell), TRUE);

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(dshell),vbox);

  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

  dlabel = gtk_label_new(text);
  gtk_container_add(GTK_CONTAINER(vbox), dlabel);

  bbox = gtk_vbutton_box_new();
  gtk_box_set_spacing(GTK_BOX(bbox), 2);
  gtk_container_add(GTK_CONTAINER(vbox), bbox);
  
  g_object_set_data(G_OBJECT(dshell), "bbox", bbox);
  g_object_set_data(G_OBJECT(dshell), "nbuttons", GINT_TO_POINTER(0));
  g_object_set_data(G_OBJECT(dshell), "hide", GINT_TO_POINTER(FALSE));
  
  gtk_widget_show(vbox);
  gtk_widget_show(dlabel);
  
  return dshell;
}

/****************************************************************
...
*****************************************************************/
static void message_dialog_clicked(GtkWidget *w, gpointer data)
{
  if (g_object_get_data(G_OBJECT(data), "hide")) {
    gtk_widget_hide(GTK_WIDGET(data));
  } else {
    gtk_widget_destroy(GTK_WIDGET(data));
  }
}

/****************************************************************
...
*****************************************************************/
void message_dialog_add(GtkWidget *dshell, const gchar *label,
			GCallback handler, gpointer data)
{
  GtkWidget *button, *bbox;
  char name[512];
  int nbuttons;

  bbox = g_object_get_data(G_OBJECT(dshell), "bbox");
  nbuttons = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dshell), "nbuttons"));
  g_object_set_data(G_OBJECT(dshell), "nbuttons", GINT_TO_POINTER(nbuttons+1));

  my_snprintf(name, sizeof(name), "button%d", nbuttons);

  button = gtk_button_new_from_stock(label);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_object_set_data(G_OBJECT(dshell), name, button);

  if (handler) {
    g_signal_connect(button, "clicked", handler, data);
  }

  g_signal_connect_after(button, "clicked",
			 G_CALLBACK(message_dialog_clicked), dshell);
}

/****************************************************************
...
*****************************************************************/
void message_dialog_end(GtkWidget *dshell)
{
  GtkWidget *bbox;
  
  bbox = g_object_get_data(G_OBJECT(dshell), "bbox");
  
  gtk_widget_show_all(bbox);
  gtk_widget_show(dshell);  
}

/****************************************************************
...
*****************************************************************/
void message_dialog_set_hide(GtkWidget *dshell, gboolean setting)
{
  g_object_set_data(G_OBJECT(dshell), "hide", GINT_TO_POINTER(setting));
}

/****************************************************************
...
*****************************************************************/
GtkWidget *popup_message_dialog(GtkWindow *parent, const gchar *dialogname,
				const gchar *text, ...)
{
  GtkWidget *dshell;
  va_list args;
  gchar *name;
  int i;

  dshell = message_dialog_start(parent, dialogname, text);
  
  i = 0;
  va_start(args, text);

  while ((name = va_arg(args, gchar *))) {
    GCallback handler;
    gpointer data;

    handler = va_arg(args, GCallback);
    data = va_arg(args, gpointer);

    message_dialog_add(dshell, name, handler, data);
  }

  va_end(args);

  message_dialog_end(dshell);

  return dshell;
}

/**************************************************************************
...
**************************************************************************/
static void unit_select_row_activated(GtkTreeView *view, GtkTreePath *path)
{
  GtkTreeIter it;
  struct unit *punit;
  gint id;

  gtk_tree_model_get_iter(GTK_TREE_MODEL(unit_select_store), &it, path);
  gtk_tree_model_get(GTK_TREE_MODEL(unit_select_store), &it, 0, &id, -1);
 
  if ((punit = player_find_unit_by_id(get_player_ptr(), id))) {
    set_unit_focus_and_active(punit);
  }

  gtk_widget_destroy(unit_select_dialog_shell);
}

/****************************************************************
  ...
*****************************************************************/
static int unit_list_tile_sort(const struct unit * const *ppa, 
			       const struct unit * const *ppb)
{
  return (*ppa)->owner != get_player_idx() && (*ppb)->owner == get_player_idx();
}

/**************************************************************************
...
**************************************************************************/
static void unit_select_append(struct unit *punit, GtkTreeIter *it,
    			       GtkTreeIter *parent)
{
  GdkPixbuf *pix;
  struct unit_type *ptype = unit_type(punit);
  struct city *pcity = player_find_city_by_id(get_player_ptr(), punit->homecity);

  pix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
      UNIT_TILE_WIDTH, UNIT_TILE_HEIGHT);

  {
    struct canvas canvas_store;

    canvas_store.type = CANVAS_PIXBUF;
    canvas_store.v.pixbuf = pix;   

    gdk_pixbuf_fill(pix, 0x00000000);
    put_unit(punit, &canvas_store, 0, 0);
  }

  gtk_tree_store_append(unit_select_store, it, parent);
  gtk_tree_store_set(unit_select_store, it,
      0, punit->id,
      1, pix,
      2, _(ptype->name),
  	  3, (pcity?pcity->name:""),
  	  4, _(ptype->veteran[punit->veteran].name),
      -1);
  g_object_unref(pix);

  if (punit == get_unit_in_focus()) {
    unit_select_path =
      gtk_tree_model_get_path(GTK_TREE_MODEL(unit_select_store), it);
  }
}

/**************************************************************************
...
**************************************************************************/
static void unit_select_recurse(int root_id, GtkTreeIter *it_root)
{
  unit_list_sort(unit_select_ptile->units, unit_list_tile_sort);
  unit_list_iterate(unit_select_ptile->units, pleaf) {
    GtkTreeIter it_leaf;

    if (pleaf->transported_by == root_id) {
      unit_select_append(pleaf, &it_leaf, it_root);
      if (pleaf->occupy > 0) {
	unit_select_recurse(pleaf->id, &it_leaf);
      }
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
static void refresh_unit_select_dialog(void)
{
  if (unit_select_dialog_shell) {
    gtk_tree_store_clear(unit_select_store);

    unit_select_recurse(-1, NULL);
    gtk_tree_view_expand_all(GTK_TREE_VIEW(unit_select_view));

    if (unit_select_path) {
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(unit_select_view),
	  unit_select_path, NULL, FALSE);
      gtk_tree_path_free(unit_select_path);
      unit_select_path = NULL;
    }
  }
}

/****************************************************************
  ...
*****************************************************************/
static void unit_select_destroy_callback(GtkObject *object, gpointer data)
{
  unit_select_dialog_shell = NULL;
  gtk_tree_selection_unselect_all(unit_select_selection);
}

/****************************************************************
  ...
*****************************************************************/
static void add_unit_iterate(GtkTreeModel *model, GtkTreePath *path,
			     GtkTreeIter *it, gpointer data)
{
  gint id;
  struct unit *punit;
  bool *focus_change = (bool *)data;

  gtk_tree_model_get(GTK_TREE_MODEL(unit_select_store), it, 0, &id, -1);
  if (!(punit = player_find_unit_by_id(get_player_ptr(), id))) {
    return;
  }
  if (*focus_change) {
    set_unit_focus(punit);
    *focus_change = FALSE;
  } else {
    multi_select_add_or_remove_unit(punit);
  }
}

/****************************************************************
  ...
*****************************************************************/
static void unit_select_cmd_callback(GtkWidget *w, gint rid, gpointer data)
{
  struct tile *ptile = unit_select_ptile;

  switch (rid) {
  case SELECT_UNIT_READY:
  {
    struct unit *pmyunit = NULL;

    unit_list_iterate(ptile->units, punit) {
      if (get_player_idx() == punit->owner) {
	pmyunit = punit;

	/* Activate this unit. */
	request_active_unit(punit);
      }
    } unit_list_iterate_end;

    if (pmyunit) {
      /* Put the focus on one of the activated units. */
      set_unit_focus(pmyunit);
    }
    break;
  }
  case SELECT_UNIT_SENTRY:
  {
    unit_list_iterate(ptile->units, punit) {
      if (get_player_idx() == punit->owner) {
	if ((punit->activity == ACTIVITY_IDLE) &&
	    !punit->ai.control &&
	    can_unit_do_activity(punit, ACTIVITY_SENTRY)) {
	  request_new_unit_activity(punit, ACTIVITY_SENTRY);
	}
      }
    } unit_list_iterate_end;
    break;
  }
  case SELECT_UNIT_SELECT:
  {
    if (gtk_tree_selection_count_selected_rows(unit_select_selection) < 1) {
      break;
    }

    bool focus_change = TRUE;
    multi_select_clear(0);
    gtk_tree_selection_selected_foreach(unit_select_selection,
					add_unit_iterate,
					(gpointer)&focus_change);
    update_unit_info_label(get_unit_in_focus());
    update_menus();
    break;
  }
  case SELECT_UNIT_ADD_TO_FOCUS:
  {
    if (gtk_tree_selection_count_selected_rows(unit_select_selection) < 1) {
      break;
    }

    bool focus_change = FALSE;
    gtk_tree_selection_selected_foreach(unit_select_selection,
					add_unit_iterate,
					(gpointer)&focus_change);
    update_unit_info_label(get_unit_in_focus());
    update_menus();
    break;
  }

  default:
    break;
  }

  gtk_widget_destroy(unit_select_dialog_shell);
}

/****************************************************************
...
*****************************************************************/
#define NUM_UNIT_SELECT_COLUMNS 4

void popup_unit_select_dialog(struct tile *ptile)
{
  if (!unit_select_dialog_shell) {
    GtkTreeStore *store;
    GtkWidget *shell, *view, *sw, *hbox;
    GtkWidget *ready_cmd, *sentry_cmd, *close_cmd;
    GtkWidget *select_cmd, *add_to_focus_cmd;

    static const char *titles[NUM_UNIT_SELECT_COLUMNS] = {
      N_("Unit"),
      N_("Name"),
      N_("Homecity"),
      N_("Veteran level")
    };
    static bool titles_done;

    GType types[NUM_UNIT_SELECT_COLUMNS+1] = {
      G_TYPE_INT,
      GDK_TYPE_PIXBUF,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING
    };
    int i;


    shell = gtk_dialog_new_with_buttons(_("Unit selection"),
      NULL,
      0,
      NULL);
    unit_select_dialog_shell = shell;
    setup_dialog(shell, toplevel);
    g_signal_connect(shell, "destroy",
      G_CALLBACK(unit_select_destroy_callback), NULL);
    gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_MOUSE);
    g_signal_connect(shell, "response",
      G_CALLBACK(unit_select_cmd_callback), NULL);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(shell)->vbox), hbox);

    intl_slist(ARRAY_SIZE(titles), titles, &titles_done);

    store = gtk_tree_store_newv(ARRAY_SIZE(types), types);
    unit_select_store = store;

    view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    unit_select_view = view;
    g_object_unref(store);
 
    for (i = 1; i < ARRAY_SIZE(types); i++) {
      GtkTreeViewColumn *column;
      GtkCellRenderer *render;

      column = gtk_tree_view_column_new();
      gtk_tree_view_column_set_title(column, titles[i-1]);

      switch (types[i]) {
	case G_TYPE_STRING:
	  render = gtk_cell_renderer_text_new();
	  gtk_tree_view_column_pack_start(column, render, TRUE);
	  gtk_tree_view_column_set_attributes(column, render, "text", i, NULL);
	  gtk_tree_view_column_set_sort_column_id(column, i);
	  break;
	default:
	  render = gtk_cell_renderer_pixbuf_new();
	  gtk_tree_view_column_pack_start(column, render, FALSE);
	  gtk_tree_view_column_set_attributes(column, render,
	      "pixbuf", i, NULL);
	  break;
      }
      gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    }

    g_signal_connect(view, "row_activated",
    G_CALLBACK(unit_select_row_activated), NULL);
    unit_select_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    gtk_tree_selection_set_mode(unit_select_selection, GTK_SELECTION_MULTIPLE);


    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(sw, -1, 300);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
	GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
	GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(sw), view);
    gtk_box_pack_start(GTK_BOX(hbox), sw, TRUE, TRUE, 0);


    ready_cmd =
    gtk_dialog_add_button(GTK_DIALOG(shell),
      _("_Ready all"), SELECT_UNIT_READY);

    gtk_button_box_set_child_secondary(
      GTK_BUTTON_BOX(GTK_DIALOG(shell)->action_area),
      ready_cmd, TRUE);

    sentry_cmd =
    gtk_dialog_add_button(GTK_DIALOG(shell),
      _("_Sentry idle"), SELECT_UNIT_SENTRY);

    gtk_button_box_set_child_secondary(
      GTK_BUTTON_BOX(GTK_DIALOG(shell)->action_area),
      sentry_cmd, TRUE);

    select_cmd =
    gtk_dialog_add_button(GTK_DIALOG(shell),
      _("Select"), SELECT_UNIT_SELECT);

    gtk_button_box_set_child_secondary(
      GTK_BUTTON_BOX(GTK_DIALOG(shell)->action_area),
      select_cmd, TRUE);

    add_to_focus_cmd =
    gtk_dialog_add_button(GTK_DIALOG(shell),
      _("Add units to focus"), SELECT_UNIT_ADD_TO_FOCUS);

    gtk_button_box_set_child_secondary(
      GTK_BUTTON_BOX(GTK_DIALOG(shell)->action_area),
      add_to_focus_cmd, TRUE);

    close_cmd =
    gtk_dialog_add_button(GTK_DIALOG(shell),
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

    gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_CLOSE);

    gtk_widget_show_all(GTK_DIALOG(shell)->vbox);
    gtk_widget_show_all(GTK_DIALOG(shell)->action_area);
  }

  unit_select_ptile = ptile;
  refresh_unit_select_dialog();

  gtk_window_present(GTK_WINDOW(unit_select_dialog_shell));
}

/****************************************************************
...
*****************************************************************/
static void create_races_dialog(void)
{
  GtkWidget *shell;
  GtkWidget *cmd;
  GtkWidget *vbox, *hbox, *table;
  GtkWidget *frame, *label;
  GtkWidget *notebook, *text;
  
  GtkWidget *list, *sw;
  GtkTreeSelection *select;
  
  GtkListStore *store;
  GtkCellRenderer *render;
  GtkTreeViewColumn *column;
  int i;
  
  shell =
    gtk_dialog_new_with_buttons(_("What Nation Will You Be?"),
				NULL,
				GTK_DIALOG_MODAL,
				_("_Disconnect"),
				GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK,
				GTK_RESPONSE_ACCEPT,
				NULL);
  races_shell = shell;
  setup_dialog(shell, toplevel);

  gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_default_size(GTK_WINDOW(shell), -1, 310);

  cmd = gtk_dialog_add_button(GTK_DIALOG(shell),
      GTK_STOCK_QUIT, GTK_RESPONSE_CLOSE);
  gtk_button_box_set_child_secondary(
      GTK_BUTTON_BOX(GTK_DIALOG(shell)->action_area), cmd, TRUE);
  gtk_widget_show(cmd);

  frame = gtk_frame_new(_("Select a nation"));
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(shell)->vbox), frame);

  hbox = gtk_hbox_new(FALSE, 18);
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(hbox), vbox);

  /* Nation list. */
  store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_BOOLEAN,
      GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
      3, GTK_SORT_ASCENDING);

  list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  races_nation_list = list;
  g_object_unref(store);

  select = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
  g_signal_connect(select, "changed", G_CALLBACK(races_nation_callback), NULL);
  gtk_tree_selection_set_select_function(select, races_selection_func,
      NULL, NULL);
  label = g_object_new(GTK_TYPE_LABEL,
      "use-underline", TRUE,
      "mnemonic-widget", list,
      "label", _("_Nations:"),
      "xalign", 0.0,
      "yalign", 0.5,
      NULL);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
      GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(sw), list);
  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

  render = gtk_cell_renderer_pixbuf_new();
  column = gtk_tree_view_column_new_with_attributes(_("Flag"), render,
      "pixbuf", 2, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
  render = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(_("Nation"), render,
      "text", 3, "strikethrough", 1, NULL);
  gtk_tree_view_column_set_sort_column_id(column, 3);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
  render = gtk_cell_renderer_text_new();
  g_object_set(render, "style", PANGO_STYLE_ITALIC, NULL);
  column = gtk_tree_view_column_new_with_attributes(_("Class"), render,
      "text", 4, "strikethrough", 1, NULL);
  gtk_tree_view_column_set_sort_column_id(column, 4);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

  /* Populate nation list store. */
  for (i = 0; i < game.ruleset_control.nation_count; i++) {
    struct nation_type *nation;
    SPRITE *s;
    GdkPixbuf *img;
    GtkTreeIter it;
    GValue value = { 0, };

    nation = get_nation_by_idx(i);

    gtk_list_store_append(store, &it);

    s = crop_blankspace(nation->flag_sprite);
    img = gdk_pixbuf_new_from_sprite(s);
    free_sprite(s);
    gtk_list_store_set(store, &it, 0, i, 1, FALSE, 2, img, -1);
    g_object_unref(img);

    g_value_init(&value, G_TYPE_STRING);
    g_value_set_static_string(&value, nation->name);
    gtk_list_store_set_value(store, &it, 3, &value);
    g_value_unset(&value);

    g_value_init(&value, G_TYPE_STRING);
    g_value_set_static_string(&value, Q_(nation->class));
    gtk_list_store_set_value(store, &it, 4, &value);
    g_value_unset(&value);
  }


  /* Right side. */
  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_BOTTOM);
  gtk_container_add(GTK_CONTAINER(hbox), notebook);

  /* Properties pane. */
  label = gtk_label_new_with_mnemonic(_("_Properties"));

  vbox = gtk_vbox_new(FALSE, 6);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

  table = gtk_table_new(3, 4, FALSE); 
  gtk_table_set_row_spacings(GTK_TABLE(table), 2);
  gtk_table_set_col_spacing(GTK_TABLE(table), 0, 12);
  gtk_table_set_col_spacing(GTK_TABLE(table), 1, 12);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

  /* Leader. */ 
  races_leader = gtk_combo_box_entry_new_text();

  label = g_object_new(GTK_TYPE_LABEL,
      "use-underline", TRUE,
      "mnemonic-widget", races_leader,
      "label", _("_Leader:"),
      "xalign", 0.0,
      "yalign", 0.5,
      NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 2);
  gtk_table_attach_defaults(GTK_TABLE(table), races_leader, 1, 3, 0, 1);

  cmd = gtk_radio_button_new_with_mnemonic(NULL, _("_Male"));
  races_sex[1] = cmd;
  gtk_table_attach_defaults(GTK_TABLE(table), cmd, 1, 2, 1, 2);

  cmd = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(cmd),
      _("_Female"));
  races_sex[0] = cmd;
  gtk_table_attach_defaults(GTK_TABLE(table), cmd, 2, 3, 1, 2);

  /* City style. */
  store = gtk_list_store_new(3, G_TYPE_INT,
      GDK_TYPE_PIXBUF, G_TYPE_STRING);

  list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  races_city_style_list = list;
  g_object_unref(store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);
  g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(list)), "changed",
      G_CALLBACK(races_city_style_callback), NULL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
      GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_container_add(GTK_CONTAINER(sw), list);
  gtk_table_attach(GTK_TABLE(table), sw, 1, 3, 2, 4,
      GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);

  label = g_object_new(GTK_TYPE_LABEL,
      "use-underline", TRUE,
      "mnemonic-widget", list,
      "label", _("_City Styles:"),
      "xalign", 0.0,
      "yalign", 0.5,
      NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);

  render = gtk_cell_renderer_pixbuf_new();
  column = gtk_tree_view_column_new_with_attributes(NULL, render,
      "pixbuf", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
  render = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(NULL, render,
      "text", 2, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

  gtk_table_set_row_spacing(GTK_TABLE(table), 1, 12);

  /* Populate city style store. */
  for (i = 0; i < game.ruleset_control.style_count; i++) {
    GdkPixbuf *img;
    SPRITE *s;
    int last;
    GtkTreeIter it;

    if (city_styles[i].techreq != A_NONE) {
      continue;
    }

    gtk_list_store_append(store, &it);

    last = city_styles[i].tiles_num-1;

    s = crop_blankspace(sprites.city.tile[i][last]);
    img = gdk_pixbuf_new_from_sprite(s);
    free_sprite(s);
    gtk_list_store_set(store, &it, 0, i, 1, img, 2,
                       get_city_style_name(i), -1);
    g_object_unref(img);
  }

  /* Legend pane. */
  label = gtk_label_new_with_mnemonic(_("L_egend"));

  vbox = gtk_vbox_new(FALSE, 6);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

  text = gtk_text_view_new();
  races_text = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 6);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text), 6);

  gtk_box_pack_start(GTK_BOX(vbox), text, TRUE, TRUE, 0);

  /* Signals. */
  g_signal_connect(shell, "destroy",
      G_CALLBACK(races_destroy_callback), NULL);
  g_signal_connect(shell, "response",
      G_CALLBACK(races_response), NULL);

  g_signal_connect(races_leader, "changed",
      G_CALLBACK(races_leader_callback), NULL);

  g_signal_connect(races_sex[0], "toggled",
      G_CALLBACK(races_sex_callback), GINT_TO_POINTER(0));
  g_signal_connect(races_sex[1], "toggled",
      G_CALLBACK(races_sex_callback), GINT_TO_POINTER(1));

  /* Init. */
  selected_nation = -1;

  /* Finish up. */
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_ACCEPT);

  gtk_widget_show_all(GTK_DIALOG(shell)->vbox);
}

/****************************************************************
  popup the dialog 10% inside the main-window 
 *****************************************************************/
void popup_races_dialog(void)
{
  create_races_dialog();
  gtk_window_present(GTK_WINDOW(races_shell));

  select_random_race();
}

/****************************************************************
  ...
 *****************************************************************/
void popdown_races_dialog(void)
{
  if (races_shell) {
    gtk_widget_destroy(races_shell);
  }

  /* We're probably starting a new game, maybe with a new ruleset.
     So we warn the worklist dialog. */
  blank_max_unit_size();
}


/****************************************************************
  ...
 *****************************************************************/
static void races_destroy_callback(GtkWidget *w, gpointer data)
{
  races_shell = NULL;
}

/****************************************************************
  Selects a leader.
  Updates the gui elements.
 *****************************************************************/
static void select_random_leader(void)
{
  struct leader *leaders;
  int i;

  for (i = 0 ; i < num_races_leader + 1  ; i++) {
    gtk_combo_box_remove_text(GTK_COMBO_BOX(races_leader), 0);
  }

  gtk_combo_box_append_text(GTK_COMBO_BOX(races_leader), default_user_name);
  leaders = get_nation_leaders(selected_nation, &num_races_leader);
  for (i = 0 ; i < num_races_leader ; i++) {
    gtk_combo_box_append_text(GTK_COMBO_BOX(races_leader), leaders[i].name);
  }

  /* Select random leader or username */
  if (random_leader){
    gtk_combo_box_set_active(GTK_COMBO_BOX(races_leader), 
			     (int) myrand(num_races_leader)+1);
  } else {
    gtk_combo_box_set_active(GTK_COMBO_BOX(races_leader), 0);
  }

}

/****************************************************************
  Selectes a random race and the appropriate city style.
  Updates the gui elements and the selected_* variables.
 *****************************************************************/
static void select_random_race(void)
{
  int i, n;
  const int MAX_DEFAULT_NATIONS = 4;
  const gchar *nn;
  char *nations[MAX_DEFAULT_NATIONS];
  Nation_Type_id id;
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(races_nation_list));

  /* Allow only ',' and ';' as separators (not ' ') so that
   * nation names with spaces are parsed correctly. */
  n = get_tokens(default_user_nation, nations,
                 MAX_DEFAULT_NATIONS, ",;");

  for (i = 0, id = NO_NATION_SELECTED; i < n; i++) {
    if (id == NO_NATION_SELECTED) {
      id = find_nation_by_name(nations[i]);
    }
    if (id == NO_NATION_SELECTED) {
      id = find_nation_by_name_orig(nations[i]);
    }
    free(nations[i]);
  }

  if(id != NO_NATION_SELECTED) {
    for(i = 0; i < game.ruleset_control.playable_nation_count; i++) {
      GtkTreePath *path;
      GtkTreeIter it;
      GValue val = { 0, };

      path = gtk_tree_path_new();
      gtk_tree_path_append_index(path, i);
      if (gtk_tree_model_get_iter(model, &it, path)) {
        gtk_tree_model_get_value(model, &it, 3, &val);
        nn = g_value_get_string(&val);
        if (find_nation_by_name(nn) == id) {
          gboolean chosen;

          g_value_unset(&val);
          gtk_tree_model_get(model, &it, 1, &chosen, -1);
          if (!chosen) {
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(races_nation_list),
                                     path, NULL, FALSE);
            gtk_tree_path_free(path);
            return;
          }
          g_value_unset(&val);
        }
        gtk_tree_path_free(path);
      }
    }
  }
	
  /* This has a possibility of infinite loop in case
   * game.ruleset_control.playable_nation_count < game.info.nplayers. */
  while (TRUE) {
    GtkTreePath *path;
    GtkTreeIter it;
    int nation;

    nation = myrand(game.ruleset_control.playable_nation_count);

    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, nation);

    if (gtk_tree_model_get_iter(model, &it, path)) {
      gboolean chosen;

      gtk_tree_model_get(model, &it, 1, &chosen, -1);

      if (!chosen) {
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(races_nation_list), path,
	    NULL, FALSE);
	gtk_tree_path_free(path);
	return;
      }
    }

    gtk_tree_path_free(path);
  }
}

/**************************************************************************
  ...
 **************************************************************************/
void races_toggles_set_sensitive(bool *nations_used)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  GtkTreePath *path;
  gboolean chosen;

  if (!races_shell) {
    return;
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(races_nation_list));

  if (gtk_tree_model_get_iter_first(model, &it)) {
    do {
      int nation;

      gtk_tree_model_get(model, &it, 0, &nation, -1);

      chosen = nations_used[nation];
      gtk_list_store_set(GTK_LIST_STORE(model), &it, 1, chosen, -1);

    } while (gtk_tree_model_iter_next(model, &it));
  }

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(races_nation_list), &path, NULL);
  if (path) {
    gtk_tree_model_get_iter(model, &it, path);
    gtk_tree_model_get(model, &it, 1, &chosen, -1);

    if (chosen) {
      select_random_race();
    }

    gtk_tree_path_free(path);
  }
}

/**************************************************************************
  ...
 **************************************************************************/
static void races_nation_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;

  if (gtk_tree_selection_get_selected(select, &model, &it)) {
    gboolean chosen;
    struct nation_type *nation;

    gtk_tree_model_get(model, &it, 0, &selected_nation, 1, &chosen, -1);
    nation = get_nation_by_idx(selected_nation);

    if (chosen) {
      select_random_race();
    } else {
      int cs, i, j;
      GtkTreePath *path;
      
      select_random_leader();
      
      /* Select city style for chosen nation. */
      cs = get_nation_city_style(selected_nation);
      for (i = 0, j = 0; i < game.ruleset_control.style_count; i++) {
        if (city_styles[i].techreq != A_NONE) {
	  continue;
	}

	if (i < cs) {
	  j++;
	} else {
	  break;
	}
      }

      path = gtk_tree_path_new();
      gtk_tree_path_append_index(path, j);
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(races_city_style_list), path,
			       NULL, FALSE);
      gtk_tree_path_free(path);

      /* Update nation legend text. */
      gtk_text_buffer_set_text(races_text, nation->legend , -1);
    }

  } else {
    selected_nation = -1;
  }
}

/**************************************************************************
...
**************************************************************************/
static void races_leader_callback(void)
{
  const gchar *name;

  name = gtk_entry_get_text(GTK_ENTRY(GTK_BIN(races_leader)->child));

  if (check_nation_leader_name(selected_nation, name)) {
    selected_sex = get_nation_leader_sex(selected_nation, name);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(races_sex[selected_sex]),
				 TRUE);
  }
}

/**************************************************************************
...
**************************************************************************/
static void races_sex_callback(GtkWidget *w, gpointer data)
{
  selected_sex = GPOINTER_TO_INT(data);
}

/**************************************************************************
...
**************************************************************************/
static gboolean races_selection_func(GtkTreeSelection *select,
				     GtkTreeModel *model, GtkTreePath *path,
				     gboolean selected, gpointer data)
{
  GtkTreeIter it;
  gboolean chosen;

  gtk_tree_model_get_iter(model, &it, path);
  gtk_tree_model_get(model, &it, 1, &chosen, -1);
  return (!chosen || selected);
}

/**************************************************************************
...
**************************************************************************/
static void races_city_style_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;

  if (gtk_tree_selection_get_selected(select, &model, &it)) {
    gtk_tree_model_get(model, &it, 0, &selected_city_style, -1);
  } else {
    selected_city_style = -1;
  }
}

/**************************************************************************
...
**************************************************************************/
static void races_response(GtkWidget *w, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_ACCEPT) {
    const char *s;

    if (selected_nation == -1) {
      append_output_window(_("You must select a nation."));
      return;
    }

    if (selected_sex == -1) {
      append_output_window(_("You must select your sex."));
      return;
    }

    if (selected_city_style == -1) {
      append_output_window(_("You must select your city style."));
      return;
    }

    s = gtk_entry_get_text(GTK_ENTRY(GTK_BIN(races_leader)->child));

    /* Perform a minimum of sanity test on the name. */
    /* This could call is_allowed_player_name if it were available. */
    if (strlen(s) == 0) {
      append_output_window(_("You must type a legal name."));
      return;
    }

    dsend_packet_nation_select_req(&aconnection, selected_nation,
				   selected_sex, s, selected_city_style);
  } else if (response == GTK_RESPONSE_CLOSE) {
    exit(EXIT_SUCCESS);

  } else {
    popdown_races_dialog();
    disconnect_from_server();
    client_kill_server(FALSE);
  }
}


/**************************************************************************
  Adjust tax rates from main window
**************************************************************************/
gboolean taxrates_callback(GtkWidget * w, GdkEventButton * ev, gpointer data)
{
  common_taxrates_callback((size_t) data);
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static void nuke_children(gpointer data, gpointer user_data)
{
  if (data != user_data) {
    if (GTK_IS_WINDOW(data) && GTK_WINDOW(data)->type == GTK_WINDOW_TOPLEVEL) {
      gtk_widget_destroy(data);
    }
  }
}

/********************************************************************** 
  This function is called when the tileset is reloaded.
***********************************************************************/
void popdown_all_game_dialogs_except_option_dialog(void)
{
  GList *res;

  gui_dialog_destroy_all();

  res = gtk_window_list_toplevels();
  res = g_list_remove(res, (gconstpointer) get_option_dialog_shell());

  g_list_foreach(res, (GFunc)g_object_ref, NULL);
  g_list_foreach(res, nuke_children, toplevel);
  g_list_foreach(res, (GFunc)g_object_unref, NULL);

  g_list_free(res);
}

/********************************************************************** 
  This function is called when the client disconnects or the game is
  over.  It should close all dialog windows for that game.
***********************************************************************/
void popdown_all_game_dialogs(void)
{
  GList *res;

  gui_dialog_destroy_all();

  res = gtk_window_list_toplevels();

  g_list_foreach(res, (GFunc)g_object_ref, NULL);
  g_list_foreach(res, nuke_children, toplevel);
  g_list_foreach(res, (GFunc)g_object_unref, NULL);

  g_list_free(res);
}



/*************************************************************************
  Trade planning calculation window.
*************************************************************************/

static GtkWidget *trade_planning_calculation_info = NULL;
static guint resume_request = 0;

/*************************************************************************
  ...
*************************************************************************/
static void tpc_callback(GtkWidget *window, gint rid, gpointer data)
{
  struct trade_planning_calculation *pcalc = get_trade_planning_calculation();

  if (pcalc) {
    switch (rid) {
    case GTK_RESPONSE_OK:
      trade_planning_calculation_destroy(pcalc, TRUE);
      break;
    case GTK_RESPONSE_CANCEL:
      trade_planning_calculation_destroy(pcalc, FALSE);
      break;
    default:
      break;
    }
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void tpc_destroy(GtkWidget *window, gpointer data)
{
  struct trade_planning_calculation *pcalc = get_trade_planning_calculation();

  if (pcalc) {
    trade_planning_calculation_destroy(pcalc, FALSE);
  }
}

/*************************************************************************
  ...
*************************************************************************/
void popup_trade_planning_calculation_info(void)
{
  if (trade_planning_calculation_info) {
    return;
  }

  GtkWidget *vbox, *bar;

  trade_planning_calculation_info =
      gtk_dialog_new_with_buttons(_("Trade planning calculation"), NULL, 0,
				  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				  GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
  setup_dialog(trade_planning_calculation_info, toplevel);

  g_signal_connect(trade_planning_calculation_info, "response",
		   G_CALLBACK(tpc_callback), NULL);
  g_signal_connect(trade_planning_calculation_info, "destroy",
		   G_CALLBACK(tpc_destroy), NULL);

  vbox = GTK_DIALOG(trade_planning_calculation_info)->vbox;

  bar = gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(vbox), bar, 0, TRUE, TRUE);
  g_object_set_data(G_OBJECT(trade_planning_calculation_info),
		    "main_bar", bar);

  bar = gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(vbox), bar, 0, TRUE, TRUE);
  g_object_set_data(G_OBJECT(trade_planning_calculation_info),
		    "advance_bar", bar);

  gtk_widget_show_all(vbox);
  gtk_widget_show(trade_planning_calculation_info);

  update_trade_planning_calculation_info();
}

/*************************************************************************
  ...
*************************************************************************/
void popdown_trade_planning_calculation_info(void)
{
  if (trade_planning_calculation_info) {
    gtk_widget_destroy(trade_planning_calculation_info);
    trade_planning_calculation_info = NULL;
  }
}

/* The pulsing progress bar will be update at this time (µs) */
#define PULSE_TIME	200000

/*************************************************************************
  ...
*************************************************************************/
void update_trade_planning_calculation_info(void)
{
  if (!trade_planning_calculation_info) {
    return;
  }

  static clock_t last_time = 0;
  struct trade_planning_calculation *pcalc = get_trade_planning_calculation();
  char buf[256];
  int num, max, moves;
  GtkProgressBar *bar;
  clock_t cur_time = clock();

  if (cur_time > last_time + PULSE_TIME) {
    bar = GTK_PROGRESS_BAR(g_object_get_data(
	      G_OBJECT(trade_planning_calculation_info), "main_bar"));
    gtk_progress_bar_pulse(bar);
    last_time = cur_time;
  }

  bar = GTK_PROGRESS_BAR(g_object_get_data(
	    G_OBJECT(trade_planning_calculation_info), "advance_bar"));
  trade_planning_calculation_get_advancement(pcalc, &num, &max, &moves);
  gtk_progress_bar_set_fraction(bar, (gdouble) num / MAX(max, 1));
  my_snprintf(buf, sizeof(buf), "%d/%d trade routes, %d moves",
	      num, max, moves);
  gtk_progress_bar_set_text(bar, buf);
}

/*************************************************************************
  ...
*************************************************************************/
static gboolean resume_callback(gpointer data)
{
  struct trade_planning_calculation *pcalc = get_trade_planning_calculation();

  if (!pcalc || !trade_planning_calculation_resume(pcalc)) {
    resume_request = 0;
    return FALSE;
  }
  update_trade_planning_calculation_info();
  return TRUE;
}

/*************************************************************************
  ...
*************************************************************************/
void request_trade_planning_calculation_resume(void)
{
  if (resume_request == 0) {
    resume_request = g_idle_add(resume_callback, NULL);
  }
}

/*************************************************************************
  ...
*************************************************************************/
void remove_trade_planning_calculation_resume_request(void)
{
  if (resume_request != 0) {
    g_source_remove(resume_request);
    resume_request = 0;
  }
}



/*************************************************************************
  ...
*************************************************************************/
static void disband_unit(gpointer data, gpointer dump)
{
  struct unit *punit = player_find_unit_by_id(get_player_ptr(),
					      GPOINTER_TO_INT(data));

  if (punit) {
    request_unit_disband(punit);
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void disband_unit_response(GtkWidget *w, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_YES) {
    disband_unit(data, NULL);
  }
  gtk_widget_destroy(w);
}

/*************************************************************************
  ...
*************************************************************************/
void popup_disband_unit(const struct unit *punit)
{
  GtkWidget *dialog;

  if (!punit || !can_client_issue_orders()) {
    return;
  }

  dialog = gtk_message_dialog_new(NULL, 0,
				  GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
				  _("Are you sure to want to disband this %s?"),
				  unit_name(punit->type));
  gtk_window_set_title(GTK_WINDOW(dialog), _("Disband unit"));
  setup_dialog(dialog, toplevel);
  g_signal_connect(dialog, "response", G_CALLBACK(disband_unit_response),
		   GINT_TO_POINTER(punit->id));
  gtk_window_present(GTK_WINDOW(dialog));
}

/*************************************************************************
  ...
*************************************************************************/
static void disband_units_response(GtkWidget *w, gint response, gpointer data)
{
  GList *list = (GList *) data;

  if (response == GTK_RESPONSE_YES) {
    g_list_foreach(list, disband_unit, NULL);
  }
  g_list_free(list);
  gtk_widget_destroy(w);
}

/*************************************************************************
  ...
*************************************************************************/
void popup_disband_units_focus(void)
{
  GtkWidget *dialog;
  GList *list = NULL;

  if (!can_client_issue_orders()) {
    return;
  }

  multi_select_iterate(FALSE, punit) {
    list = g_list_append(list, GINT_TO_POINTER(punit->id));
  } multi_select_iterate_end;

  dialog = gtk_message_dialog_new(NULL, 0,
				  GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
				  _("Are you sure to want to disband "
				    "those %d units?"), g_list_length(list));
  gtk_window_set_title(GTK_WINDOW(dialog), _("Disband units"));
  setup_dialog(dialog, toplevel);
  g_signal_connect(dialog, "response",
		   G_CALLBACK(disband_units_response), list);
  gtk_window_present(GTK_WINDOW(dialog));
}
