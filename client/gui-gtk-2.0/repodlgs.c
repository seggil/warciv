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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "fcintl.h"
#include "game.h"
#include "government.h"
#include "packets.h"
#include "shared.h"
#include "support.h"

#include "chatline_common.h"
#include "cityrep.h"
#include "civclient.h"
#include "climisc.h"
#include "clinet.h"
#include "control.h"
#include "dialogs.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "mapview_common.h"
#include "options.h"
#include "packhand_gen.h"
#include "control.h"
#include "text.h"

#include "repodlgs_common.h"
#include "repodlgs.h"

/******************************************************************/

static void create_science_dialog(bool make_modal);
static void science_help_callback(GtkTreeView *view,
      				  GtkTreePath *arg1,
				  GtkTreeViewColumn *arg2,
				  gpointer data);
static void science_change_callback(GtkComboBox *combo, gpointer data);
static void science_goal_callback(GtkComboBox *combo, gpointer data);
static void science_help_button_callback(GtkWidget *widget, gpointer data);
static void science_change_from_goal_callback(GtkWidget *widget, gpointer data);

GtkListStore *science_list_new(void);
void science_list_add(GtkListStore *list, GtkTreeIter *iter, Tech_Type_id tech);
/******************************************************************/
static struct gui_dialog *science_dialog_shell = NULL;
static GtkWidget *science_label;
static GtkWidget *science_current_label, *science_goal_label;
static GtkWidget *science_change_combo, *science_goal_combo;
static GtkWidget *science_help_button, *science_help_combo = NULL;
static GtkWidget *science_change_from_goal_button;
static GtkListStore *science_model[3], *reachable_techs, *reachable_goals;
static int science_dialog_shell_is_modal;
static bool no_science_callback = FALSE;

enum {TECH_ID, TECH_NAME, TECH_STEPS, TECH_NUM};
/******************************************************************/
enum {
  ECONOMY_SELL_OBSOLETE = 1, ECONOMY_SELL_ALL
};

static void create_economy_report_dialog(bool make_modal);
static void economy_command_callback(struct gui_dialog *dlg, int response);
static void economy_selection_callback(GtkTreeSelection *selection,
				       gpointer data);
struct economy_row {
  int is_impr;
  int type;
};
static struct economy_row economy_row_type[U_LAST + B_LAST];

static struct gui_dialog *economy_dialog_shell = NULL;
static GtkWidget *economy_label2;
static GtkListStore *economy_store;
static GtkTreeSelection *economy_selection;
static GtkWidget *sellall_command, *sellobsolete_command;
static int economy_dialog_shell_is_modal;

/******************************************************************/
static void create_activeunits_report_dialog(bool make_modal);
static void activeunits_command_callback(struct gui_dialog *dlg, int response);
static void activeunits_selection_callback(GtkTreeSelection *selection,
					   gpointer data);
static struct gui_dialog *activeunits_dialog_shell = NULL;
static GtkListStore *activeunits_store;
static GtkTreeSelection *activeunits_selection;

enum {
  ACTIVEUNITS_NEAREST = 1, ACTIVEUNITS_UPGRADE
};

static int activeunits_dialog_shell_is_modal;

/******************************************************************/
static void create_endgame_report(struct packet_endgame_report *packet);

static GtkListStore *scores_store;
static struct gui_dialog *endgame_report_shell;
static GtkWidget *scores_list;
static GtkWidget *sw;

#define NUM_SCORE_COLS 14                

/******************************************************************/
static GtkWidget *settable_options_dialog_shell;

/******************************************************************/

/******************************************************************
...
*******************************************************************/
void update_report_dialogs(void)
{
  if(is_report_dialogs_frozen()) return;
  activeunits_report_dialog_update();
  economy_report_dialog_update();
  city_report_dialog_update(); 
  science_dialog_update();
}


/****************************************************************
...
*****************************************************************/
void popup_science_dialog(bool make_modal)
{
  if(!science_dialog_shell) {
    science_dialog_shell_is_modal = make_modal;
    
    create_science_dialog(make_modal);
  }

  gui_dialog_present(science_dialog_shell);
}


/****************************************************************
 Raises the science dialog.
****************************************************************/
void raise_science_dialog(void)
{
  popup_science_dialog(FALSE);
  gui_dialog_raise(science_dialog_shell);
}


/****************************************************************
 Closes the science dialog.
*****************************************************************/
void popdown_science_dialog(void)
{
  if (science_dialog_shell) {
    gui_dialog_destroy(science_dialog_shell);
  }
}

/****************************************************************
  ...
*****************************************************************/
GtkListStore *science_list_new(void)
{
  return gtk_list_store_new(TECH_NUM, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);
}

/****************************************************************
  ...
*****************************************************************/
void science_list_add(GtkListStore *list, GtkTreeIter *iter, Tech_Type_id tech)
{
  char buf[256];
  const gchar *tech_name;

  if (tech < game.ruleset_control.num_tech_types || tech == A_UNSET) {
    tech_name = get_tech_name(get_player_ptr(), tech);
  } else {
    my_snprintf(buf, sizeof(buf), _("Future Tech. %d"),
		tech - game.ruleset_control.num_tech_types);
    tech_name = buf;
  }

  gtk_list_store_append(list, iter);
  gtk_list_store_set(list, iter,
		     TECH_ID, tech,
		     TECH_NAME, tech_name,
		     TECH_STEPS, num_unknown_techs_for_goal(get_player_ptr(),
							    tech), -1);
}

/****************************************************************
  ...
*****************************************************************/
void create_science_dialog(bool make_modal)
{
  GtkWidget *frame, *hbox;
  GtkCellRenderer *renderer;
  int i;

  gui_dialog_new_full(&science_dialog_shell, GTK_NOTEBOOK(top_notebook), 5);
  gui_dialog_set_title(science_dialog_shell, _("Science"));

  gui_dialog_add_button(science_dialog_shell,
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
  gui_dialog_set_default_response(science_dialog_shell,
      GTK_RESPONSE_CLOSE);

  science_label = gtk_label_new("no text set yet");

  gtk_box_pack_start(GTK_BOX(science_dialog_shell->vbox),
        science_label, FALSE, FALSE, 0);

  frame = gtk_frame_new(_("Researching"));
  gtk_box_pack_start(GTK_BOX(science_dialog_shell->vbox),
        frame, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(TRUE, 4);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

  reachable_techs = science_list_new();
  science_change_combo =
    gtk_combo_box_new_with_model(GTK_TREE_MODEL(reachable_techs));
  g_object_unref(reachable_techs);
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(science_change_combo),
			     renderer, FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(science_change_combo),
				 renderer, "text", TECH_NAME, NULL);
  g_signal_connect(science_change_combo, "changed",
		   G_CALLBACK(science_change_callback), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), science_change_combo,
                     TRUE, TRUE, 0);

  science_current_label=gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(hbox), science_current_label, TRUE, TRUE, 0);
  gtk_widget_set_size_request(science_current_label, -1, 25);

  science_help_button = gtk_stockbutton_new(GTK_STOCK_HELP, _("Help"));
  g_signal_connect(science_help_button, "clicked",
		   G_CALLBACK(science_help_button_callback), hbox);
  gtk_box_pack_end(GTK_BOX(hbox), science_help_button, TRUE, TRUE, 4);

  frame = gtk_frame_new( _("Goal"));
  gtk_box_pack_start(GTK_BOX(science_dialog_shell->vbox),
        frame, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(TRUE, 4);
  gtk_container_add(GTK_CONTAINER(frame),hbox);

  reachable_goals = science_list_new();
  science_goal_combo =
    gtk_combo_box_new_with_model(GTK_TREE_MODEL(reachable_goals));
  g_object_unref(reachable_goals);
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(science_goal_combo),
			     renderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(science_goal_combo),
				 renderer, "text", TECH_NAME, NULL);
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(science_goal_combo),
			   renderer, FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(science_goal_combo),
				 renderer, "text", TECH_STEPS, NULL);
  g_signal_connect(science_goal_combo, "changed",
		   G_CALLBACK(science_goal_callback), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), science_goal_combo,
                     TRUE, TRUE, 2);

  science_goal_label = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox), science_goal_label, TRUE, TRUE, 0);
  gtk_widget_set_size_request(science_goal_label, -1, 25);

  science_change_from_goal_button =
    gtk_button_new_with_label(_("Change current technology"));
  gtk_box_pack_start(GTK_BOX(hbox), science_change_from_goal_button,
		     TRUE, TRUE, 4);
  g_signal_connect(science_change_from_goal_button, "clicked",
		   G_CALLBACK(science_change_from_goal_callback), NULL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_box_pack_start(GTK_BOX(science_dialog_shell->vbox), sw, TRUE, TRUE, 5);

  hbox = gtk_hbox_new(TRUE, 0);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), hbox);


  for (i = 0; i<ARRAY_SIZE(science_model); i++) {
    GtkWidget *view;
    GtkTreeSelection *selection;
    GtkTreeViewColumn *column;

    science_model[i] = gtk_list_store_new(1, G_TYPE_STRING);
    view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(science_model[i]));
    gtk_box_pack_start(GTK_BOX(hbox), view, TRUE, TRUE, 0);
    gtk_widget_set_name(view, "small font");
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    g_object_unref(science_model[i]);
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
	"text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

    g_signal_connect(view, "row_activated",
	G_CALLBACK(science_help_callback), NULL);
  }

  gui_dialog_show_all(science_dialog_shell);

  science_dialog_update();
}

/****************************************************************
  ...
*****************************************************************/
void science_change_callback(GtkComboBox *combo, gpointer data)
{
  GtkTreeIter iter;
  gint tech;

  if (no_science_callback || !gtk_combo_box_get_active_iter(combo, &iter)) {
    return;
  }
  gtk_tree_model_get(gtk_combo_box_get_model(combo), &iter, TECH_ID, &tech, -1);
  dsend_packet_player_research(&aconnection, tech);
}

/****************************************************************
  ...
*****************************************************************/
void science_goal_callback(GtkComboBox *combo, gpointer data)
{
  GtkTreeIter iter;
  gint tech;

  if (no_science_callback || !gtk_combo_box_get_active_iter(combo, &iter)) {
    return;
  }
  gtk_tree_model_get(gtk_combo_box_get_model(combo), &iter, TECH_ID, &tech, -1);
  dsend_packet_player_tech_goal(&aconnection, tech);
}

/****************************************************************
  ...
*****************************************************************/
static gboolean remove_combo_box(gpointer data)
{
  if (science_help_combo) {
    gtk_combo_box_popdown(GTK_COMBO_BOX(science_help_combo));
    gtk_widget_destroy(science_help_combo);
    gtk_widget_show(science_help_button);
    science_help_combo = NULL;
  }
  return FALSE;
}

/****************************************************************
  ...
*****************************************************************/
static void tech_help_callback(GtkComboBox *combo, gpointer data)
{
  GtkTreeIter iter;
  gint tech;
  gchar *tech_name;

  if (gtk_combo_box_get_active_iter(combo, &iter)) {
    gtk_tree_model_get(gtk_combo_box_get_model(combo), &iter,
		       TECH_ID, &tech,
		       TECH_NAME, &tech_name, -1);
    if (tech_exists(tech)) {
      popup_help_dialog_typed(tech_name, HELP_TECH);
    }
  }
  g_idle_add(remove_combo_box, NULL);
}

/****************************************************************
  ...
*****************************************************************/
void science_help_button_callback(GtkWidget *widget, gpointer data)
{
  GtkCellRenderer *renderer;

  gtk_widget_hide(science_help_button);
  science_help_combo =
    gtk_combo_box_new_with_model(GTK_TREE_MODEL(reachable_goals));
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(science_help_combo),
			     renderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(science_help_combo),
				 renderer, "text", TECH_NAME, NULL);
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(science_help_combo),
			   renderer, FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(science_help_combo),
				 renderer, "text", TECH_STEPS, NULL);
  g_signal_connect(science_help_combo, "changed",
		   G_CALLBACK(tech_help_callback), NULL);
  gtk_box_pack_end(GTK_BOX(data), science_help_combo, TRUE, TRUE, 2);
  
  gtk_widget_show(science_help_combo);
  gtk_combo_box_popup(GTK_COMBO_BOX(science_help_combo));
}

/****************************************************************
  ...
*****************************************************************/
static void science_help_callback(GtkTreeView *view,
      				  GtkTreePath *arg1,
				  GtkTreeViewColumn *arg2,
				  gpointer data)
{
  GtkTreeModel *model = gtk_tree_view_get_model(view);
  GtkTreeIter it;
  char *s;

  gtk_tree_model_get_iter(model, &it, arg1);
  gtk_tree_model_get(model, &it, 0, &s, -1);
  if (*s != '\0') {
    popup_help_dialog_typed(s, HELP_TECH);
  } else {
    popup_help_dialog_string(HELP_TECHS_ITEM);
  }
}

/****************************************************************
...
*****************************************************************/
static void science_change_from_goal_callback(GtkWidget * widget, gpointer data)
{
  force_tech_goal(get_player_ptr()->ai.tech_goal);
}

/****************************************************************
...
*****************************************************************/
static gint cmp_func(gconstpointer a_p, gconstpointer b_p)
{
  const gchar *a_str, *b_str;
  gchar text_a[512], text_b[512];
  gint a = GPOINTER_TO_INT(a_p), b = GPOINTER_TO_INT(b_p);

  /* FIXME: future techs aren't counted this way but are handled by
   * get_tech_name() when given a player parameter. */
  if (!is_future_tech(a)) {
    a_str = get_tech_name(get_player_ptr(), a);
  } else {
    my_snprintf(text_a,sizeof(text_a), _("Future Tech. %d"),
		a - game.ruleset_control.num_tech_types);
    a_str=text_a;
  }

  if(!is_future_tech(b)) {
    b_str = get_tech_name(get_player_ptr(), b);
  } else {
    my_snprintf(text_b,sizeof(text_b), _("Future Tech. %d"),
		b - game.ruleset_control.num_tech_types);
    b_str=text_b;
  }

  return strcmp(a_str,b_str);
}

/****************************************************************
...
*****************************************************************/
void science_dialog_update(void)
{
  if (!science_dialog_shell) {
    return;
  }

  int i, j;
  char format[512], text[512];
  GList *sorting_list = NULL, *it;
  gdouble pct;
  int steps, bulbs;
  GtkSizeGroup *group1, *group2;
  GtkTreeIter iter;
  struct player *pplayer = get_player_ptr();

  if (is_report_dialogs_frozen()) {
    return;
  }

  no_science_callback = TRUE;
  gtk_label_set_text(GTK_LABEL(science_label), science_dialog_text());

  (void) remove_combo_box(NULL);

  for (i=0; i<ARRAY_SIZE(science_model); i++) {
    gtk_list_store_clear(science_model[i]);
  }

  /* collect all researched techs in sorting_list */
  for (i = A_FIRST; i < game.ruleset_control.num_tech_types; i++) {
    if ((get_invention(pplayer, i) == TECH_KNOWN)) {
      sorting_list = g_list_append(sorting_list, GINT_TO_POINTER(i));
    }
  }

  /* sort them, and install them in the list */
  sorting_list = g_list_sort(sorting_list, cmp_func);
  for (i = 0; i < g_list_length(sorting_list); i++) {
    GtkTreeIter it;
    GValue value = { 0, };

    j = GPOINTER_TO_INT(g_list_nth_data(sorting_list, i));
    gtk_list_store_append(science_model[i%ARRAY_SIZE(science_model)], &it);

    g_value_init(&value, G_TYPE_STRING);
    g_value_set_static_string(&value, get_tech_name(pplayer, j));
    gtk_list_store_set_value(science_model[i%ARRAY_SIZE(science_model)], &it,
	0, &value);
    g_value_unset(&value);
  }
  g_list_free(sorting_list);
  sorting_list = NULL;

  gtk_list_store_clear(reachable_techs);
  gtk_widget_set_sensitive(science_change_combo,
			   can_client_issue_orders());

  my_snprintf(text, sizeof(text), "%d/%d",
	      pplayer->research.bulbs_researched,
	      pplayer->research.researching_cost);

  pct = CLAMP((gdouble) pplayer->research.bulbs_researched /
	      pplayer->research.researching_cost, 0.0, 1.0);

  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(science_current_label), text);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(science_current_label), pct);

  /* work around GTK+ refresh bug. */
  gtk_widget_queue_resize(science_current_label);
 
  if (pplayer->research.researching == A_UNSET) {
    science_list_add(reachable_techs, &iter, A_UNSET);
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(science_change_combo),
				  &iter);
  }

  /* collect all techs which are reachable in the next step
   * hist will hold afterwards the techid of the current choice
   */
  if (!is_future_tech(pplayer->research.researching)) {
    for (i = A_FIRST; i < game.ruleset_control.num_tech_types; i++) {
      if (get_invention(pplayer, i) == TECH_REACHABLE) {
        sorting_list = g_list_append(sorting_list, GINT_TO_POINTER(i));
      }
    }
  } else {
    sorting_list = g_list_append(sorting_list,
				 GINT_TO_POINTER(game.ruleset_control.num_tech_types + 1 +
						 pplayer->future_tech));
  }

  /* sort the list and build from it the menu */
  sorting_list = g_list_sort(sorting_list, cmp_func);
  for (i = 0; i < g_list_length(sorting_list); i++) {
    gint tech = GPOINTER_TO_INT(g_list_nth_data(sorting_list, i));

    science_list_add(reachable_techs, &iter, tech);
    if (tech == pplayer->research.researching
	|| (is_future_tech(pplayer->research.researching)
	    && tech > game.ruleset_control.num_tech_types)) {
      gtk_combo_box_set_active_iter(GTK_COMBO_BOX(science_change_combo), &iter);
    }
  }

  g_list_free(sorting_list);
  sorting_list = NULL;

  gtk_list_store_clear(reachable_goals);
  gtk_widget_set_sensitive(science_goal_combo,
			   can_client_issue_orders());

  steps = num_unknown_techs_for_goal(pplayer, pplayer->ai.tech_goal);
  bulbs = total_bulbs_required_for_goal(pplayer, pplayer->ai.tech_goal);
  my_snprintf(format, sizeof(format), "%s -- %s",
	      PL_("(%d step)", "(%d steps)", steps),
	      PL_("(%d bulb)", "(%d bulbs)", bulbs));
  my_snprintf(text, sizeof(text), format, steps, bulbs);
  gtk_label_set_text(GTK_LABEL(science_goal_label), text);

  gtk_widget_set_sensitive(science_change_from_goal_button, can_client_issue_orders());

  if (pplayer->ai.tech_goal == A_UNSET) {
    science_list_add(reachable_goals, &iter, A_UNSET);
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(science_goal_combo),
				  &iter);
  }

  /* collect all techs which are reachable in under 11 steps
   * hist will hold afterwards the techid of the current choice
   */
  for (i = A_FIRST; i < game.ruleset_control.num_tech_types; i++) {
    if (tech_is_available(pplayer, i)
        && get_invention(pplayer, i) != TECH_KNOWN
        && advances[i].req[0] != A_LAST && advances[i].req[1] != A_LAST
        && (num_unknown_techs_for_goal(pplayer, i) < 11
	    || i == pplayer->ai.tech_goal)) {
      sorting_list = g_list_append(sorting_list, GINT_TO_POINTER(i));
    }
  }

  group1 = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
  group2 = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

  /* sort the list and build from it the menu */
  sorting_list = g_list_sort(sorting_list, cmp_func);
  for (it = g_list_first(sorting_list); it; it = g_list_next(it)) {
    gint tech = GPOINTER_TO_INT(g_list_nth_data(it, 0));

    science_list_add(reachable_goals, &iter, tech);
    if (tech == pplayer->ai.tech_goal) {
      gtk_combo_box_set_active_iter(GTK_COMBO_BOX(science_goal_combo),
				    &iter);
    }
  }

  g_list_free(sorting_list);
  sorting_list = NULL;
  no_science_callback = FALSE;
}


/****************************************************************

                      ECONOMY REPORT DIALOG
 
****************************************************************/

/****************************************************************
...
****************************************************************/
void popup_economy_report_dialog(bool make_modal)
{
  if(!economy_dialog_shell) {
    economy_dialog_shell_is_modal = make_modal;

    create_economy_report_dialog(make_modal);
  }

  gui_dialog_present(economy_dialog_shell);
}


/****************************************************************
 Raises the economy report dialog.
****************************************************************/
void raise_economy_report_dialog(void)
{
  popup_economy_report_dialog(FALSE);
  gui_dialog_raise(economy_dialog_shell);
}


/****************************************************************
 Close the economy report dialog.
****************************************************************/
void popdown_economy_report_dialog(void)
{
  if (economy_dialog_shell) {
    gui_dialog_destroy(economy_dialog_shell);
  }
}
 
/****************************************************************
...
*****************************************************************/
void create_economy_report_dialog(bool make_modal)
{
  static const char *titles[4] = {
    N_("Building Name"),
    N_("Count"),
    N_("Cost"),
    N_("U Total")
  };
  static bool titles_done;
  int i;

  static GType model_types[4] = {
    G_TYPE_STRING,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT
  };
  GtkWidget *view, *sw, *align;

  intl_slist(ARRAY_SIZE(titles), titles, &titles_done);
  
  gui_dialog_new_full(&economy_dialog_shell, GTK_NOTEBOOK(top_notebook), 4);
  gui_dialog_set_title(economy_dialog_shell, _("Economy"));

  align = gtk_alignment_new(0.5, 0.0, 0.0, 1.0);
  gtk_box_pack_start(GTK_BOX(economy_dialog_shell->vbox), align,
      TRUE, TRUE, 0);

  economy_store = gtk_list_store_newv(ARRAY_SIZE(model_types), model_types);

  sw = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(align), sw);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(economy_store));
  g_object_unref(economy_store);
  gtk_widget_set_name(view, "small font");
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
  economy_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  g_signal_connect(economy_selection, "changed",
		   G_CALLBACK(economy_selection_callback), NULL);

  for (i=0; i<4; i++) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    renderer = gtk_cell_renderer_text_new();
      
    col = gtk_tree_view_column_new_with_attributes(titles[i], renderer,
	"text", i, NULL);

    if (i > 0) {
      GValue value = { 0, };

      g_value_init(&value, G_TYPE_FLOAT);
      g_value_set_float(&value, 1.0);
      g_object_set_property(G_OBJECT(renderer), "xalign", &value);
      g_value_unset(&value);

      gtk_tree_view_column_set_alignment(col, 1.0);
    }

    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  }
  gtk_container_add(GTK_CONTAINER(sw), view);

  economy_label2 = gtk_label_new(_("Total Cost:"));
  gtk_box_pack_start(GTK_BOX(economy_dialog_shell->vbox), economy_label2,
      FALSE, FALSE, 0);
  gtk_misc_set_padding(GTK_MISC(economy_label2), 5, 5);

  sellobsolete_command =
    gui_dialog_add_button(economy_dialog_shell, _("Sell _Obsolete"),
	ECONOMY_SELL_OBSOLETE);
  gtk_widget_set_sensitive(sellobsolete_command, FALSE);

  sellall_command =
    gui_dialog_add_button(economy_dialog_shell, _("Sell _All"),
	ECONOMY_SELL_ALL);
  gtk_widget_set_sensitive(sellall_command, FALSE);

  gui_dialog_add_button(economy_dialog_shell,
      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  gui_dialog_response_set_callback(economy_dialog_shell,
      economy_command_callback);

  economy_report_dialog_update();
  gui_dialog_set_default_size(economy_dialog_shell, -1, 350);

  gui_dialog_set_default_response(economy_dialog_shell, GTK_RESPONSE_CLOSE);

  gui_dialog_show_all(economy_dialog_shell);

  gtk_tree_view_focus(GTK_TREE_VIEW(view));
}


/****************************************************************
  Called when a building type is selected in the economy list.
*****************************************************************/
static void economy_selection_callback(GtkTreeSelection *selection,
				       gpointer data)
{
  gint row = gtk_tree_selection_get_row(selection);
  int i = economy_row_type[row].type;

  if (row >= 0) {
    if (economy_row_type[row].is_impr == TRUE) {
      /* The user has selected an improvement type. */
      bool is_sellable = (i >= 0 && i < game.ruleset_control.num_impr_types && !is_wonder(i));

      gtk_widget_set_sensitive(sellobsolete_command, is_sellable
			       && can_client_issue_orders()
			       && improvement_obsolete(get_player_ptr(), i));
      gtk_widget_set_sensitive(sellall_command, is_sellable
			       && can_client_issue_orders());
    } else {
      /* An unit has been selected */
      gtk_widget_set_sensitive(sellall_command, can_client_issue_orders());
    }
  } else {
    /* No selection has been made. */
    gtk_widget_set_sensitive(sellobsolete_command, FALSE);
    gtk_widget_set_sensitive(sellall_command, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
static void economy_command_callback(struct gui_dialog *dlg, int response)
{
  int i, is_impr;
  gint row;
  GtkWidget *shell;
  char buf[1024];

  if (response != ECONOMY_SELL_OBSOLETE && response != ECONOMY_SELL_ALL) {
    gui_dialog_destroy(dlg);
    return;
  }

  /* sell obsolete and sell all. */
  row = gtk_tree_selection_get_row(economy_selection);
  is_impr = economy_row_type[row].is_impr;
  i = economy_row_type[row].type;

  if (is_impr == TRUE) {
    if (response == ECONOMY_SELL_ALL) {
      shell = gtk_message_dialog_new(
	  NULL,
	  GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
	  GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
	  _("Do you really wish to sell your %s?\n"),
	  get_improvement_name(i));
      setup_dialog(shell, gui_dialog_get_toplevel(dlg));
      gtk_window_set_title(GTK_WINDOW(shell), _("Sell Improvements"));

      if (gtk_dialog_run(GTK_DIALOG(shell)) == GTK_RESPONSE_YES) {
	gtk_widget_destroy(shell);
      } else {
	gtk_widget_destroy(shell);
	return;
      }
    }

    sell_all_improvements(i, response!= ECONOMY_SELL_ALL, buf, sizeof(buf));
  } else {
    if (response== ECONOMY_SELL_OBSOLETE) {
      return;
    }
    disband_all_units(i, FALSE, buf, sizeof(buf));
  }

  shell = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
				 GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
				 "%s", buf);
  setup_dialog(shell, gui_dialog_get_toplevel(dlg));

  g_signal_connect(shell, "response", G_CALLBACK(gtk_widget_destroy), NULL);
  gtk_window_set_title(GTK_WINDOW(shell), _("Sell-Off: Results"));
  gtk_window_present(GTK_WINDOW(shell));
}

/****************************************************************
...
*****************************************************************/
void economy_report_dialog_update(void)
{
  if(!is_report_dialogs_frozen() && economy_dialog_shell) {
    int tax, total, i, entries_used, nbr_impr;
    char economy_total[48];
    struct improvement_entry entries[B_LAST];
    struct unit_entry entries_units[U_LAST];
    GtkTreeIter it;
    GValue value = { 0, };

    gtk_list_store_clear(economy_store);

    get_economy_report_data(entries, &entries_used, &total, &tax);

    for (i = 0; i < entries_used; i++) {
      struct improvement_entry *p = &entries[i];

      gtk_list_store_append(economy_store, &it);
      gtk_list_store_set(economy_store, &it,
	1, p->count,
	2, p->cost,
	3, p->total_cost, -1);
      g_value_init(&value, G_TYPE_STRING);
      g_value_set_static_string(&value, get_improvement_name(p->type));
      gtk_list_store_set_value(economy_store, &it, 0, &value);
      g_value_unset(&value);

      economy_row_type[i].is_impr = TRUE;
      economy_row_type[i].type = p->type;
    }

    nbr_impr = entries_used;
    entries_used = 0;
    get_economy_report_units_data(entries_units, &entries_used, &total);

    for (i = 0; i < entries_used; i++) {
      gtk_list_store_append(economy_store, &it);
      gtk_list_store_set(economy_store, &it,
			 1, entries_units[i].count,
			 2, entries_units[i].cost,
			 3, entries_units[i].total_cost,
			 -1);
      g_value_init(&value, G_TYPE_STRING);
      g_value_set_static_string(&value, unit_name(entries_units[i].type));
      gtk_list_store_set_value(economy_store, &it, 0, &value);
      g_value_unset(&value);
    
      economy_row_type[i + nbr_impr].is_impr = FALSE;
      economy_row_type[i + nbr_impr].type = entries_units[i].type;
    }

    my_snprintf(economy_total, sizeof(economy_total),
		_("Income: %d    Total Costs: %d"), tax, total); 
    gtk_label_set_text(GTK_LABEL(economy_label2), economy_total);
  }  
}

/****************************************************************

                      ACTIVE UNITS REPORT DIALOG
 
****************************************************************/

#define AU_COL 7

/****************************************************************
...
****************************************************************/
void popup_activeunits_report_dialog(bool make_modal)
{
  if(!activeunits_dialog_shell) {
    activeunits_dialog_shell_is_modal = make_modal;
    
    create_activeunits_report_dialog(make_modal);
  }

  gui_dialog_present(activeunits_dialog_shell);
}


/****************************************************************
 Raises the units report dialog.
****************************************************************/
void raise_activeunits_report_dialog(void)
{
  popup_activeunits_report_dialog(FALSE);
  gui_dialog_raise(activeunits_dialog_shell);
}


/****************************************************************
 Closes the units report dialog.
****************************************************************/
void popdown_activeunits_report_dialog(void)
{
  if (activeunits_dialog_shell) {
    gui_dialog_destroy(activeunits_dialog_shell);
  }
}

 
/****************************************************************
...
*****************************************************************/
void create_activeunits_report_dialog(bool make_modal)
{
  static const char *titles[AU_COL] = {
    N_("Unit Type"),
    N_("U"),
    N_("In-Prog"),
    N_("Active"),
    N_("Shield"),
    N_("Food"),
    N_("Gold")
  };
  static bool titles_done;
  int i;

  static GType model_types[AU_COL+2] = {
    G_TYPE_STRING,
    G_TYPE_BOOLEAN,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_BOOLEAN,
    G_TYPE_INT
  };
  GtkWidget *view, *sw, *align;

  intl_slist(ARRAY_SIZE(titles), titles, &titles_done);

  gui_dialog_new_full(&activeunits_dialog_shell, GTK_NOTEBOOK(top_notebook), 1);
  gui_dialog_set_title(activeunits_dialog_shell, _("Units"));

  align = gtk_alignment_new(0.5, 0.0, 0.0, 1.0);
  gtk_box_pack_start(GTK_BOX(activeunits_dialog_shell->vbox), align,
      TRUE, TRUE, 0);

  activeunits_store = gtk_list_store_newv(ARRAY_SIZE(model_types), model_types);

  sw = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(align), sw);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(activeunits_store));
  g_object_unref(activeunits_store);
  gtk_widget_set_name(view, "small font");
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
  activeunits_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  g_signal_connect(activeunits_selection, "changed",
	G_CALLBACK(activeunits_selection_callback), NULL);

  for (i=0; i<AU_COL; i++) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    if (model_types[i] == G_TYPE_BOOLEAN) {
      renderer = gtk_cell_renderer_toggle_new();
      
      col = gtk_tree_view_column_new_with_attributes(titles[i], renderer,
	"active", i, "visible", AU_COL, NULL);
    } else {
      renderer = gtk_cell_renderer_text_new();
      
      col = gtk_tree_view_column_new_with_attributes(titles[i], renderer,
	"text", i, NULL);
    }

    if (i > 0) {
      g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_alignment(col, 1.0);
    }

    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  }
  gtk_container_add(GTK_CONTAINER(sw), view);

  gui_dialog_add_stockbutton(activeunits_dialog_shell, GTK_STOCK_FIND,
      _("Find _Nearest"), ACTIVEUNITS_NEAREST);
  gui_dialog_set_response_sensitive(activeunits_dialog_shell,
      				    ACTIVEUNITS_NEAREST, FALSE);	
  
  gui_dialog_add_button(activeunits_dialog_shell,
      _("_Upgrade"), ACTIVEUNITS_UPGRADE);
  gui_dialog_set_response_sensitive(activeunits_dialog_shell,
      				    ACTIVEUNITS_UPGRADE, FALSE);	

  gui_dialog_add_button(activeunits_dialog_shell, GTK_STOCK_CLOSE,
      GTK_RESPONSE_CLOSE);

  gui_dialog_set_default_response(activeunits_dialog_shell,
      GTK_RESPONSE_CLOSE);

  gui_dialog_response_set_callback(activeunits_dialog_shell,
      activeunits_command_callback);

  activeunits_report_dialog_update();
  gui_dialog_set_default_size(activeunits_dialog_shell, -1, 350);

  gui_dialog_show_all(activeunits_dialog_shell);

  gtk_tree_view_focus(GTK_TREE_VIEW(view));
}

/****************************************************************
...
*****************************************************************/
static void activeunits_selection_callback(GtkTreeSelection *selection,
					   gpointer data)
{
  int ut;
  GtkTreeModel *model;
  GtkTreeIter it;

  ut = U_LAST;
  if (gtk_tree_selection_get_selected(activeunits_selection, &model, &it)) {
    gtk_tree_model_get(model, &it, AU_COL + 1, &ut, -1);
  }


  if (ut == U_LAST) {
    gui_dialog_set_response_sensitive(activeunits_dialog_shell,
				      ACTIVEUNITS_NEAREST, FALSE);

    gui_dialog_set_response_sensitive(activeunits_dialog_shell,
				      ACTIVEUNITS_UPGRADE, FALSE);
  } else {
    gui_dialog_set_response_sensitive(activeunits_dialog_shell,
				      ACTIVEUNITS_NEAREST,
				      can_client_issue_orders());	
    
    if (can_upgrade_unittype(get_player_ptr(), ut) != -1) {
      gui_dialog_set_response_sensitive(activeunits_dialog_shell,
					ACTIVEUNITS_UPGRADE,
					can_client_issue_orders());	
    } else {
      gui_dialog_set_response_sensitive(activeunits_dialog_shell,
					ACTIVEUNITS_UPGRADE, FALSE);
    }
  }
}

/****************************************************************
...
*****************************************************************/
static struct unit *find_nearest_unit(Unit_Type_id type, struct tile *ptile)
{
  struct unit *best_candidate;
  int best_dist = 99999;

  best_candidate = NULL;
  unit_list_iterate(get_player_ptr()->units, punit) {
    if (punit->type == type) {
      if (punit->focus_status==FOCUS_AVAIL
	  && punit->moves_left > 0
	  && !punit->done_moving
	  && !punit->ai.control) {
	int d;
	d=sq_map_distance(punit->tile, ptile);
	if(d<best_dist) {
	  best_candidate = punit;
	  best_dist = d;
	}
      }
    }
  }
  unit_list_iterate_end;
  return best_candidate;
}

/****************************************************************
...
*****************************************************************/
static void activeunits_command_callback(struct gui_dialog *dlg, int response)
{
  int           ut1, ut2;
  GtkTreeModel *model;
  GtkTreeIter   it;

  switch (response) {
    case ACTIVEUNITS_NEAREST:
    case ACTIVEUNITS_UPGRADE:
      break;
    default:
      gui_dialog_destroy(dlg);
      return;
  }

  /* nearest & upgrade commands. */
  ut1 = U_LAST;
  if (gtk_tree_selection_get_selected(activeunits_selection, &model, &it)) {
    gtk_tree_model_get(model, &it, AU_COL + 1, &ut1, -1);
  }

  if (!unit_type_exists(ut1)) {
    return;
  }

  if (response == ACTIVEUNITS_NEAREST) {
    struct tile *ptile;
    struct unit *punit;

    ptile = get_center_tile_mapcanvas();
    if ((punit = find_nearest_unit(ut1, ptile))) {
      center_tile_mapcanvas(punit->tile);

      if (punit->activity == ACTIVITY_IDLE
	  || punit->activity == ACTIVITY_SENTRY) {
	if (can_unit_do_activity(punit, ACTIVITY_IDLE)) {
	  set_unit_focus_and_select(punit);
	}
      }
    }
  } else {
    GtkWidget *shell;
    struct player *pplayer = get_player_ptr();

    ut2 = can_upgrade_unittype(pplayer, ut1);

    shell = gtk_message_dialog_new(
	  NULL,
	  GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
	  GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
	  _("Upgrade as many %s to %s as possible for %d gold each?\n"
	    "Treasury contains %d gold."),
	  unit_types[ut1].name, unit_types[ut2].name,
	  unit_upgrade_price(pplayer, ut1, ut2),
	  pplayer->economic.gold);
    setup_dialog(shell, gui_dialog_get_toplevel(dlg));

    gtk_window_set_title(GTK_WINDOW(shell), _("Upgrade Obsolete Units"));

    if (gtk_dialog_run(GTK_DIALOG(shell)) == GTK_RESPONSE_YES) {
      dsend_packet_unit_type_upgrade(&aconnection, ut1);
    }

    gtk_widget_destroy(shell);
  }
}

/****************************************************************
...
*****************************************************************/
void activeunits_report_dialog_update(void)
{
  struct repoinfo {
    int active_count;
    int upkeep_shield;
    int upkeep_food;
    int upkeep_gold;
    int building_count;
  };

  if (is_report_dialogs_frozen()) {
    return;
  }

  if (activeunits_dialog_shell) {
    int    k, can;
    struct repoinfo unitarray[U_LAST];
    struct repoinfo unittotals;
    struct player *pplayer = get_player_ptr();
    GtkTreeIter it;
    GValue value = { 0, };

    gtk_list_store_clear(activeunits_store);

    memset(unitarray, '\0', sizeof(unitarray));
    unit_list_iterate(pplayer->units, punit) {
      (unitarray[punit->type].active_count)++;
      if (punit->homecity) {
	unitarray[punit->type].upkeep_shield += punit->upkeep;
	unitarray[punit->type].upkeep_food += punit->upkeep_food;
	unitarray[punit->type].upkeep_gold += punit->upkeep_gold;
      }
    }
    unit_list_iterate_end;
    city_list_iterate(pplayer->cities,pcity) {
      if (pcity->is_building_unit &&
	  (unit_type_exists (pcity->currently_building)))
	(unitarray[pcity->currently_building].building_count)++;
    }
    city_list_iterate_end;

    k = 0;
    memset(&unittotals, '\0', sizeof(unittotals));
    unit_type_iterate(i) {
    
      if ((unitarray[i].active_count > 0) || (unitarray[i].building_count > 0)) {
	can = (can_upgrade_unittype(pplayer, i) != -1);
	
        gtk_list_store_append(activeunits_store, &it);
	gtk_list_store_set(activeunits_store, &it,
		1, can,
		2, unitarray[i].building_count,
		3, unitarray[i].active_count,
		4, unitarray[i].upkeep_shield,
		5, unitarray[i].upkeep_food,
		6, unitarray[i].upkeep_gold,
		7, TRUE,
		8, ((unitarray[i].active_count > 0) ? i : U_LAST), -1);
	g_value_init(&value, G_TYPE_STRING);
	g_value_set_static_string(&value, unit_name(i));
	gtk_list_store_set_value(activeunits_store, &it, 0, &value);
	g_value_unset(&value);

	k++;
	unittotals.active_count += unitarray[i].active_count;
	unittotals.upkeep_shield += unitarray[i].upkeep_shield;
	unittotals.upkeep_food += unitarray[i].upkeep_food;
	unittotals.upkeep_gold += unitarray[i].upkeep_gold;
	unittotals.building_count += unitarray[i].building_count;
      }
    } unit_type_iterate_end;

    gtk_list_store_append(activeunits_store, &it);
    gtk_list_store_set(activeunits_store, &it,
	    1, FALSE,
    	    2, unittotals.building_count,
    	    3, unittotals.active_count,
    	    4, unittotals.upkeep_shield,
    	    5, unittotals.upkeep_food,
	    6, unittotals.upkeep_gold,
	    7, FALSE,
	    8, U_LAST, -1);
    g_value_init(&value, G_TYPE_STRING);
    g_value_set_static_string(&value, _("Totals:"));
    gtk_list_store_set_value(activeunits_store, &it, 0, &value);
    g_value_unset(&value);
  }
}

/****************************************************************

                      FINAL REPORT DIALOG
 
****************************************************************/

/****************************************************************
  Prepare the Final Report dialog, and fill it with 
  statistics for each player.
*****************************************************************/
static void create_endgame_report(struct packet_endgame_report *packet)
{
  int i;
  static bool titles_done;
  GtkTreeIter it;
      
  static const char *titles[NUM_SCORE_COLS] = {
    N_("Player\n"),
    N_("Score\n"),
    N_("Population\n"),
    N_("Trade\n(M goods)"), 
    N_("Production\n(M tons)"), 
    N_("Cities\n"),
    N_("Technologies\n"),
    N_("Military Service\n(months)"), 
    N_("Wonders\n"),
    N_("Research Speed\n(%)"), 
    N_("Land Area\n(sq. mi.)"), 
    N_("Settled Area\n(sq. mi.)"), 
    N_("Literacy\n(%)"), 
    N_("Spaceship\n")
  };

  static GType model_types[NUM_SCORE_COLS] = {
    G_TYPE_STRING,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT,
    G_TYPE_INT
  };

  intl_slist(ARRAY_SIZE(titles), titles, &titles_done);

  gui_dialog_new(&endgame_report_shell, GTK_NOTEBOOK(top_notebook));
  gui_dialog_set_title(endgame_report_shell, _("Score"));
  gui_dialog_add_button(endgame_report_shell, GTK_STOCK_CLOSE,
      GTK_RESPONSE_CLOSE);

  gui_dialog_set_default_size(endgame_report_shell, 700, 420);

  scores_store = gtk_list_store_newv(ARRAY_SIZE(model_types), model_types);
  scores_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(scores_store));
  g_object_unref(scores_store);
  gtk_widget_set_name(scores_list, "small font");
    
  for (i = 0; i < NUM_SCORE_COLS; i++) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;
      
    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes(titles[i], renderer,
                                                   "text", i, NULL);
    gtk_tree_view_column_set_sort_column_id(col, i);
    gtk_tree_view_append_column(GTK_TREE_VIEW(scores_list), col);
  }  

  /* Setup the layout. */
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(sw), scores_list);
  gtk_box_pack_start(GTK_BOX(endgame_report_shell->vbox), sw, TRUE, TRUE, 0);
  gui_dialog_set_default_response(endgame_report_shell, GTK_RESPONSE_CLOSE);
  gui_dialog_show_all(endgame_report_shell);
  
  /* Insert score statistics into table.  */
  gtk_list_store_clear(scores_store);
  for (i = 0; i < packet->nscores; i++) {
    gtk_list_store_append(scores_store, &it);
    gtk_list_store_set(scores_store, &it,
                       0, (gchar *)get_player(packet->id[i])->name,
                       1, packet->score[i],
                       2, packet->pop[i],
                       3, packet->bnp[i],
                       4, packet->mfg[i],
                       5, packet->cities[i],
                       6, packet->techs[i],
                       7, packet->mil_service[i],
                       8, packet->wonders[i],
                       9, packet->research[i],
                       10, packet->landarea[i],
                       11, packet->settledarea[i],
                       12, packet->literacy[i],
                       13, packet->spaceship[i],
                       -1);
  }
}

/**************************************************************************
  Show a dialog with player statistics at endgame.
**************************************************************************/
void popup_endgame_report_dialog(struct packet_endgame_report *packet)
{
  if (!endgame_report_shell){
    create_endgame_report(packet);
  }
  gui_dialog_present(endgame_report_shell);
}

/*************************************************************************
  helper function for server options dialog
*************************************************************************/
static void option_changed_callback(GtkWidget *widget, gpointer data) 
{
  g_object_set_data(G_OBJECT(widget), "changed", (gpointer)TRUE); 
}

/*************************************************************************
  helper function for server options dialog
*************************************************************************/
static void set_options(GtkWidget *w)
{
  GtkWidget *tmp;

  /* if the entry has been changed, then send the changes to the server */
  if (g_object_get_data(G_OBJECT(w), "changed")) {
    char buffer[MAX_LEN_MSG];

    /* append the name of the option */
    my_snprintf(buffer, MAX_LEN_MSG, "/set %s ", gtk_widget_get_name(w));

    /* append the setting */
    if (GTK_IS_ENTRY(w)) {
      sz_strlcat(buffer, gtk_entry_get_text(GTK_ENTRY(w)));
    } else {
      bool active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
      sz_strlcat(buffer, active ? "1" : "0");
    }
    send_chat(buffer);
  }

  /* using the linked list, work backwards and check the previous widget */
  tmp = (GtkWidget *)g_object_get_data(G_OBJECT(w), "prev");
  if (tmp) {
    set_options(tmp);
  }
}

/****************************************************************
...
*****************************************************************/
static void settable_options_callback(GtkWidget *win, gint rid, GtkWidget *w)
{
  if (rid == GTK_RESPONSE_OK) {
    set_options(w);
  }
  gtk_widget_destroy(win);
}

/*************************************************************************
  Server options dialog.
*************************************************************************/
static void create_settable_options_dialog(void)
{
  GtkWidget *win, *book, *sw, **vbox, *prev_widget = NULL;
  static GtkStyle *style = NULL;
  GtkTooltips *tips;
  int num[num_options_categories];
  int i;

  if (!style) {
    style = gtk_style_new();
    g_object_ref(style);
    style->bg[GTK_STATE_NORMAL] = style->bg[GTK_STATE_INSENSITIVE];
  }

  for(i = 0; i < num_options_categories; i++){
    num[i] = 0;
  }

  tips = gtk_tooltips_new();
  settable_options_dialog_shell = gtk_dialog_new_with_buttons(_("Game Options"),
      NULL, 0,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_OK, GTK_RESPONSE_OK,
      NULL);
  win = settable_options_dialog_shell;

  gtk_dialog_set_has_separator(GTK_DIALOG(win), FALSE);
  g_signal_connect(settable_options_dialog_shell, "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &settable_options_dialog_shell);
  setup_dialog(win, toplevel);

  /* create a notebook for the options */
  book = gtk_notebook_new();
  gtk_widget_set_size_request(book, -1, 500);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(win)->vbox), book, TRUE, TRUE, 2);

  /* create a number of notebook pages for each category */
  vbox = fc_malloc(num_options_categories * sizeof(GtkWidget *));

  for (i = 0; i < num_options_categories; i++) {
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_notebook_append_page(GTK_NOTEBOOK(book), sw,
                             gtk_label_new(_(options_categories[i])));

    vbox[i] = gtk_vbox_new(FALSE, 2);
    gtk_container_set_border_width(GTK_CONTAINER(vbox[i]), 6);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), vbox[i]);
  }

  /* fill each category */
  for (i = 0; i < num_settable_options; i++) {
    GtkWidget *ebox, *hbox, *ent;

    /* create a box for the new option and insert it in the correct page */
    ebox = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX(vbox[settable_options[i].category]), 
                       ebox, FALSE, FALSE, 0);
    if (num[settable_options[i].category]++ & 1) {
      /* color it */
      gtk_widget_set_style(ebox, style);
    }

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ebox), hbox);

    /* insert the option short help as the label into the event box */
    gtk_box_pack_start(GTK_BOX(hbox),
		       gtk_label_new(_(settable_options[i].short_help)),
		       FALSE, FALSE, 5);

    /* if we have extra help, use that as a tooltip */
    if (settable_options[i].extra_help[0] != '\0') {
      char buf[4096];

      my_snprintf(buf, sizeof(buf), "%s\n\n%s",
		  settable_options[i].name,
		  _(settable_options[i].extra_help));
      gtk_tooltips_set_tip(tips, ebox, buf, NULL);
    }

    /* create the proper entry method depending on the type */
    if (settable_options[i].type == 0) {
      /* boolean */
      ent = gtk_check_button_new();
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ent),
                                   settable_options[i].val);

      g_signal_connect(ent, "toggled", 
                       G_CALLBACK(option_changed_callback), NULL);
    } else if (settable_options[i].type == 1) {
      /* integer */
      double step, max, min;

      min = settable_options[i].min;
      max = settable_options[i].max;
 
      /* pick a reasonable step size */
      step = ceil((max - min) / 100.0);
      if (step > 100.0) {
	/* this is ridiculous, the bounds must be meaningless */
	step = 5.0;
      }

      ent = gtk_spin_button_new_with_range(min, max, step);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(ent), settable_options[i].val);

      g_signal_connect(ent, "changed", 
                       G_CALLBACK(option_changed_callback), NULL);
    } else {
      /* string */
      ent = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(ent), settable_options[i].strval);

      g_signal_connect(ent, "changed", 
                       G_CALLBACK(option_changed_callback), NULL);
    }
    gtk_box_pack_end(GTK_BOX(hbox), ent, FALSE, FALSE, 0);

    /* set up a linked list so we can work our way through the widgets */
    gtk_widget_set_name(ent, settable_options[i].name);
    g_object_set_data(G_OBJECT(ent), "prev", prev_widget);
    g_object_set_data(G_OBJECT(ent), "changed", FALSE);
    prev_widget = ent;
  }

  /* remove any unused categories pages */
  for (i = num_options_categories - 1; i >= 0; i--) {
    if (num[i] == 0) {
      gtk_notebook_remove_page(GTK_NOTEBOOK(book), i);
    }
  }

  g_signal_connect(win, "response",
		   G_CALLBACK(settable_options_callback), prev_widget);

  gtk_widget_show_all(GTK_DIALOG(win)->vbox);
}

/**************************************************************************
  Show a dialog with the server options.
**************************************************************************/
void popup_settable_options_dialog(void)
{
  if (!settable_options_dialog_shell) {
    create_settable_options_dialog();
    gtk_window_set_position(GTK_WINDOW(settable_options_dialog_shell), GTK_WIN_POS_MOUSE);
  }
  gtk_window_present(GTK_WINDOW(settable_options_dialog_shell));
}

