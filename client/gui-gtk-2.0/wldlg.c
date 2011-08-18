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
#  include "../../config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "wc_intl.h"
#include "log.h"
#include "mem.h"
#include "registry.h"
#include "support.h"

#include "city.h"
#include "game.h"
#include "packets.h"
#include "worklist.h"

#include "../citydlg_common.h"
#include "../civclient.h"
#include "../climisc.h"
#include "../clinet.h"
#include "../options.h"
#include "../tilespec.h"

#include "citydlg.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "inputdlg.h"

#include "wldlg.h"

enum {
  WLC_NAME,
  WLC_ID,

  WLC_NUM
};

static GtkWidget *worklist_shell = NULL;
static int max_unit_height = -1, max_unit_width = -1;

static void popup_worklists_report(void);

static void popup_worklist(struct worklist *pwl);
static void popdown_worklist(struct worklist *pwl);

/****************************************************************
...
*****************************************************************/
void blank_max_unit_size(void)
{
  freelog(LOG_DEBUG, "blank_max_unit_size");

  max_unit_height = -1;
  max_unit_width = -1;
}

/****************************************************************
...
*****************************************************************/
static void update_max_unit_size(void)
{
  freelog(LOG_DEBUG, "update_max_unit_size");

  max_unit_height = 0;
  max_unit_width = 0;

  unit_type_iterate(i) {
    struct unit_type *type = get_unit_type(i);
    int x1, x2, y1, y2;

    sprite_get_bounding_box(type->sprite, &x1, &y1, &x2, &y2);
    max_unit_width = MAX(max_unit_width, x2 - x1);
    max_unit_height = MAX(max_unit_height, y2 - y1);
  } unit_type_iterate_end;
}

/****************************************************************
  ...
*****************************************************************/
static void cell_edited(GtkCellRendererText *cell,
                        const gchar *spath,
                        const gchar *text, gpointer data)
{
  struct worklist *temp_worklists =
      (struct worklist *) g_object_get_data(G_OBJECT(data), "worklists");
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(
      g_object_get_data(G_OBJECT(data), "tree_view")));
  GtkTreePath *path;
  GtkTreeIter it;
  gint pos;

  path = gtk_tree_path_new_from_string(spath);
  gtk_tree_model_get_iter(model, &it, path);

  gtk_tree_model_get(model, &it, WLC_ID, &pos, -1);

  sz_strlcpy(temp_worklists[pos].name, text);
  gtk_list_store_set(GTK_LIST_STORE(model), &it, WLC_NAME, text, -1);

  gtk_tree_path_free(path);
}

/****************************************************************
  ...
*****************************************************************/
static void update_tree_model(GObject *object)
{
  struct worklist *pwl, *temp_worklists =
      (struct worklist *) g_object_get_data(object, "worklists");
  GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(
      g_object_get_data(object, "tree_view"))));
  GtkTreeIter it;
  int i;


  gtk_list_store_clear(store);

  /* Find an emtpy worklist */
  for (i = 0; i < ARRAY_SIZE(global_worklists); i++) {
    pwl = &temp_worklists[i];
    if (pwl->is_valid && pwl->name[0] != '\0') {
      gtk_list_store_append(store, &it);
      gtk_list_store_set(store, &it, WLC_NAME, pwl->name, WLC_ID, i, -1);
    }
  }
}

/****************************************************************
  ...
*****************************************************************/
void apply_global_worklists(GtkWidget *widget)
{
  memcpy(global_worklists, g_object_get_data(G_OBJECT(widget), "worklists"),
         sizeof(global_worklists));
}

/****************************************************************
  ...
*****************************************************************/
void refresh_global_worklists(GtkWidget *widget)
{
  memcpy(g_object_get_data(G_OBJECT(widget), "worklists"), global_worklists,
         sizeof(global_worklists));
  update_tree_model(G_OBJECT(widget));
}

/****************************************************************
  ...
*****************************************************************/
void reload_global_worklists(GtkWidget *widget, struct section_file *sf)
{
  struct worklist *pwl, *temp_worklists =
      (struct worklist *) g_object_get_data(G_OBJECT(widget), "worklists");
  int i;

  for (i = 0; i < ARRAY_SIZE(global_worklists); i++) {
    pwl = &temp_worklists[i];
    load_global_worklist(sf, "worklists.worklist%d", i, pwl);
    pwl->is_valid = check_global_worklist(pwl);
  }
  update_tree_model(G_OBJECT(widget));
}

/****************************************************************
  ...
*****************************************************************/
void reset_global_worklists(GtkWidget *widget)
{
  struct worklist *temp_worklists =
      (struct worklist *) g_object_get_data(G_OBJECT(widget), "worklists");
  int i;

  for (i = 0; i < ARRAY_SIZE(global_worklists); i++) {
    init_worklist(&temp_worklists[i]);
  }
  update_tree_model(G_OBJECT(widget));
}

/****************************************************************
  ...
*****************************************************************/
static void worklist_new_callback(GtkButton *button, gpointer data)
{
  struct worklist *pwl = NULL, *temp_worklists =
      (struct worklist *) g_object_get_data(G_OBJECT(data), "worklists");
  int i;

  /* Find an emtpy worklist */
  for (i = 0; i < ARRAY_SIZE(global_worklists); i++) {
    if (temp_worklists[i].name[0] == '\0') {
      pwl = &temp_worklists[i];
      break;
    }
  }

  /* No emtpy worklist, try to find an invalid worklist */
  if (!pwl) {
    for (i = 0; i < ARRAY_SIZE(global_worklists); i++) {
      if (worklist_is_empty(&temp_worklists[i])
          || !temp_worklists[i].is_valid) {
        pwl = &temp_worklists[i];
        break;
      }
    }
  }

  /* No more worklist slots free.  (!!!Maybe we should tell the user?) */
  if (!pwl) {
    return;
  }

  /* Validate this slot. */
  init_worklist(pwl);
  pwl->is_valid = TRUE;
  strcpy(pwl->name, _("new"));

  update_tree_model(G_OBJECT(data));
}

/****************************************************************
  ...
*****************************************************************/
static void worklist_delete_callback(GtkButton *button, gpointer data)
{
  struct worklist *pwl, *temp_worklists =
      (struct worklist *) g_object_get_data(G_OBJECT(data), "worklists");
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter it;
  gint pos;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_object_get_data(
                  G_OBJECT(data), "tree_view")));

  if (!gtk_tree_selection_get_selected(selection, &model, &it)) {
    return;
  }
  gtk_tree_model_get(model, &it, WLC_ID, &pos, -1);

  pwl = &temp_worklists[pos];
  popdown_worklist(pwl);

  init_worklist(pwl);
  pwl->is_valid = TRUE;

  update_tree_model(G_OBJECT(data));
}

/****************************************************************
  ...
*****************************************************************/
static void worklist_edit_callback(GtkButton *button, gpointer data)
{
  struct worklist *temp_worklists =
      (struct worklist *) g_object_get_data(G_OBJECT(data), "worklists");
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter it;
  gint pos;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_object_get_data(
                  G_OBJECT(data), "tree_view")));

  if (!gtk_tree_selection_get_selected(selection, &model, &it)) {
    return;
  }
  gtk_tree_model_get(model, &it, WLC_ID, &pos, -1);

  popup_worklist(&temp_worklists[pos]);
}

/****************************************************************
  ...
*****************************************************************/
static void destroy_callback(GtkWidget *widget, gpointer data)
{
  struct worklist *temp_worklists = (struct worklist *) data;
  int i;

  for (i = 0; i < ARRAY_SIZE(global_worklists); i++) {
    popdown_worklist(&temp_worklists[i]);
  }

  free((struct worklist *) data);
}

/****************************************************************
  Bring up the global worklist report.
*****************************************************************/
GtkWidget *create_worklists_report(void)
{
  struct worklist *temp_worklists;
  GtkWidget *vbox, *view, *sw, *label, *hbox, *button;
  GtkListStore *store;
  GtkCellRenderer *rend;

  vbox = gtk_vbox_new(FALSE, 0);

  /* Create the temporary worklists */
  temp_worklists = wc_malloc(sizeof(global_worklists));
  memcpy(temp_worklists, global_worklists, sizeof(global_worklists));
  g_object_set_data(G_OBJECT(vbox), "worklists", temp_worklists);
  g_signal_connect(vbox, "destroy",
                   G_CALLBACK(destroy_callback), temp_worklists);

  /* Tree view */
  store = gtk_list_store_new(WLC_NUM, G_TYPE_STRING,    /* WLC_NAME */
                                      G_TYPE_INT);      /* WLC_ID */

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);
  g_object_set_data(G_OBJECT(vbox), "tree_view", view);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  rend = gtk_cell_renderer_text_new();
  g_object_set(rend, "editable", TRUE, NULL);
  g_signal_connect(rend, "edited", G_CALLBACK(cell_edited), vbox);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, NULL,
                                              rend, "text", WLC_NAME, NULL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(sw), view);

  gtk_widget_set_size_request(sw, -1, 100);

  label = g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", view,
                       "label", _("_Worklists:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);

  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

  /* Button bar */
  hbox = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 8);

  button = gtk_button_new_from_stock(GTK_STOCK_NEW);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 5);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(worklist_new_callback), vbox);

  button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 5);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(worklist_delete_callback), vbox);

  button = gtk_button_new_from_stock(GTK_STOCK_EDIT);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 5);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(worklist_edit_callback), vbox);

  gtk_widget_show_all(vbox);
  return vbox;
}

/****************************************************************
  ...
*****************************************************************/
static void worklists_report_destroy(GtkWidget *widget, gpointer data)
{
  apply_global_worklists(GTK_WIDGET(data));
  worklist_shell = NULL;
}

/****************************************************************
  ...
*****************************************************************/
static void popup_worklists_report(void)
{
  if (worklist_shell) {
    return;
  }

  GtkWidget *vbox;

  worklist_shell = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(worklist_shell), _("Edit worklists"));
  gtk_window_set_position(GTK_WINDOW(worklist_shell), GTK_WIN_POS_MOUSE);

  vbox = create_worklists_report();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(worklist_shell)->vbox),
                     vbox, TRUE, TRUE, 0);
  g_signal_connect(worklist_shell, "destroy",
                   G_CALLBACK(worklists_report_destroy), vbox);

  update_tree_model(G_OBJECT(vbox));

  gtk_widget_show(GTK_DIALOG(worklist_shell)->vbox);
  gtk_widget_show(worklist_shell);
}


/****************************************************************
  ...
*****************************************************************/
struct worklist_data {
  struct worklist *pwl;
  struct city *pcity;

  GtkWidget *editor;

  GtkListStore *src, *dst;
  GtkWidget *src_view, *dst_view;
  GtkTreeSelection *src_selection, *dst_selection;

  GtkTreeViewColumn *src_col, *dst_col;

  GtkWidget *add_cmd, *clear_cmd, *change_cmd, *help_cmd;
  GtkWidget *up_cmd, *down_cmd, *prepend_cmd, *append_cmd;

  bool future;
};

static GHashTable *hash;

static void commit_worklist(struct worklist_data *ptr);


enum {
  TARGET_GTK_TREE_MODEL_ROW
};

static GtkTargetEntry wl_dnd_targets[] = {
  { "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_APP, TARGET_GTK_TREE_MODEL_ROW },
};



/****************************************************************
...
*****************************************************************/
void add_worklist_dnd_target(GtkWidget *w)
{
  gtk_drag_dest_set(w, GTK_DEST_DEFAULT_ALL,
                    wl_dnd_targets, G_N_ELEMENTS(wl_dnd_targets),
                    GDK_ACTION_COPY);
}

/****************************************************************
...
*****************************************************************/
static GtkWidget *get_worklist(struct worklist *pwl)
{
  if (hash) {
    gpointer ret;

    ret = g_hash_table_lookup(hash, pwl);
    return ret;
  } else {
    return NULL;
  }
}

/****************************************************************
  ...
*****************************************************************/
static void insert_worklist(struct worklist *pwl, GtkWidget *editor)
{
  if (!hash) {
    hash = g_hash_table_new(g_direct_hash, g_direct_equal);
  }
  g_hash_table_insert(hash, pwl, editor);
}

/****************************************************************
  ...
*****************************************************************/
static void delete_worklist(struct worklist *pwl)
{
  if (hash) {
    g_hash_table_remove(hash, pwl);
  }
}

/****************************************************************
  ...
*****************************************************************/
static void worklist_response(GtkWidget *shell, gint response)
{
  gtk_widget_destroy(shell);
}

/****************************************************************
  Worklist editor window used by the global worklist report.
*****************************************************************/
static void popup_worklist(struct worklist *pwl)
{
  GtkWidget *shell;

  if (!(shell = get_worklist(pwl))) {
    GtkWidget *editor;

    shell = gtk_dialog_new_with_buttons(pwl->name, NULL,
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_CLOSE,
                                        GTK_RESPONSE_CLOSE,
                                        NULL);
    gtk_window_set_role(GTK_WINDOW(shell), "worklist");
    gtk_window_set_position(GTK_WINDOW(shell), GTK_WIN_POS_MOUSE);
    g_signal_connect(shell, "response", G_CALLBACK(worklist_response), NULL);
    gtk_window_set_default_size(GTK_WINDOW(shell), 500, 400);

    editor = create_worklist();
    reset_worklist(editor, pwl, NULL);
    insert_worklist(pwl, editor);

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(shell)->vbox), editor);
    gtk_widget_show(editor);

    refresh_worklist(editor);
  }

  gtk_window_present(GTK_WINDOW(shell));
}

/****************************************************************
...
*****************************************************************/
static void popdown_worklist(struct worklist *pwl)
{
  GtkWidget *shell;

  if ((shell = get_worklist(pwl))) {
    GtkWidget *parent;

    parent = gtk_widget_get_toplevel(shell);
    gtk_widget_destroy(parent);
  }
}

/****************************************************************
...
*****************************************************************/
static void worklist_destroy(GtkWidget *editor, gpointer data)
{
  struct worklist_data *ptr;

  ptr = data;

  if (ptr->pwl) {
    delete_worklist(ptr->pwl);
  }

  free(ptr);
}

/****************************************************************
...
*****************************************************************/
static void menu_item_callback(GtkMenuItem *item, struct worklist_data *ptr)
{
  gint pos;
  struct worklist *pwl;

  pos = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "pos"));

  pwl = &global_worklists[pos];

  if (pwl->is_valid && !worklist_is_empty(pwl)) {
    int i;

    for (i = 0; i < MAX_LEN_WORKLIST; i++) {
      GtkTreeIter it;
      cid cid;

      if (pwl->wlefs[i] == WEF_END) {
        break;
      }

      cid = cid_encode(pwl->wlefs[i] == WEF_UNIT, pwl->wlids[i]);

      gtk_list_store_append(ptr->dst, &it);
      gtk_list_store_set(ptr->dst, &it, 0, (gint) cid, -1);
    }
  }

  commit_worklist(ptr);
}

/****************************************************************
...
*****************************************************************/
static void popup_add_menu(GtkMenuShell *menu, gpointer data)
{
  struct worklist *pwl;
  int i;
  GtkWidget *item;

  gtk_container_foreach(GTK_CONTAINER(menu),
                        (GtkCallback) gtk_widget_destroy, NULL);

  for (i = 0; i < ARRAY_SIZE(global_worklists); i++) {
    pwl = &global_worklists[i];
    if (pwl->is_valid && !worklist_is_empty(pwl)) {
      item = gtk_menu_item_new_with_label(pwl->name);
      g_object_set_data(G_OBJECT(item), "pos", GINT_TO_POINTER(i));
      gtk_widget_show(item);

      gtk_container_add(GTK_CONTAINER(menu), item);
      g_signal_connect(item, "activate",
                       G_CALLBACK(menu_item_callback), data);
    }
  }

  item = gtk_separator_menu_item_new();
  gtk_widget_show(item);

  gtk_container_add(GTK_CONTAINER(menu), item);

  item = gtk_menu_item_new_with_mnemonic(_("Edit Global _Worklists"));
  gtk_widget_show(item);

  gtk_container_add(GTK_CONTAINER(menu), item);
  g_signal_connect(item, "activate",
                   G_CALLBACK(popup_worklists_report), NULL);
}

/****************************************************************
...
*****************************************************************/
static void clear_callback (GtkWidget *w, gpointer data)
{
  struct worklist_data *ptr;
  GtkTreeModel *dst_model;
  GtkTreeIter it;

  ptr = data;
  dst_model = GTK_TREE_MODEL (ptr->dst);

  /* If the worklist is empty, do nothing. */
  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ptr->dst), &it))
    return;

  /* Advance past the first work list entry. */
  if (!gtk_tree_model_iter_next (GTK_TREE_MODEL (ptr->dst), &it))
    return;

  /* Remove all subsequent work list entries. */
  while (gtk_list_store_remove (GTK_LIST_STORE(dst_model), &it))
    ;

  /* Propagate changes to client and server. */
  commit_worklist (ptr);
}
/****************************************************************
...
*****************************************************************/
static void help_callback(GtkWidget *w, gpointer data)
{
  struct worklist_data *ptr;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter it;

  ptr = data;
  selection = ptr->src_selection;

  if (gtk_tree_selection_get_selected(selection, &model, &it)) {
    gint cid;
    int id;
    bool is_unit;

    gtk_tree_model_get(model, &it, 0, &cid, -1);
    is_unit = cid_is_unit(cid);
    id = cid_id(cid);

    if (is_unit) {
      popup_help_dialog_typed(get_unit_type(id)->name, HELP_UNIT);
    } else if (is_wonder(id)) {
      popup_help_dialog_typed(get_improvement_name(id), HELP_WONDER);
    } else {
      popup_help_dialog_typed(get_improvement_name(id), HELP_IMPROVEMENT);
    }
  } else {
    popup_help_dialog_string(HELP_WORKLIST_EDITOR_ITEM);
  }
}

/****************************************************************
...
*****************************************************************/
static void change_callback(GtkWidget *w, gpointer data)
{
  struct worklist_data *ptr;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter it;

  ptr = data;
  selection = ptr->src_selection;

  if (gtk_tree_selection_get_selected(selection, &model, &it)) {
    gint cid;

    gtk_tree_model_get(model, &it, 0, &cid, -1);

    city_change_production(ptr->pcity, cid_is_unit(cid), cid_id(cid));
  }
}

/****************************************************************
...
*****************************************************************/
static void future_callback(GtkToggleButton *toggle, gpointer data)
{
  struct worklist_data *ptr;

  ptr = data;
  ptr->future = !ptr->future;

  refresh_worklist(ptr->editor);
}

/****************************************************************
...
*****************************************************************/
static void queue_bubble_up(struct worklist_data *ptr)
{
  GtkTreePath *path;
  GtkTreeViewColumn *col;
  GtkTreeModel *model;

  if (!GTK_WIDGET_IS_SENSITIVE(ptr->dst_view)) {
    return;
  }

  model = GTK_TREE_MODEL(ptr->dst);
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(ptr->dst_view), &path, &col);
  if (path) {
    GtkTreeIter it, it_prev;

    if (gtk_tree_path_prev(path)) {
      gtk_tree_model_get_iter(model, &it_prev, path);
      it = it_prev;
      gtk_tree_model_iter_next(model, &it);

      gtk_list_store_swap(GTK_LIST_STORE(model), &it, &it_prev);

      gtk_tree_view_set_cursor(GTK_TREE_VIEW(ptr->dst_view), path, col, FALSE);
      commit_worklist(ptr);
    }
  }
  gtk_tree_path_free(path);
}

/****************************************************************
...
*****************************************************************/
static void queue_bubble_down(struct worklist_data *ptr)
{
  GtkTreePath *path;
  GtkTreeViewColumn *col;
  GtkTreeModel *model;

  if (!GTK_WIDGET_IS_SENSITIVE(ptr->dst_view)) {
    return;
  }

  model = GTK_TREE_MODEL(ptr->dst);
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(ptr->dst_view), &path, &col);
  if (path) {
    GtkTreeIter it, it_next;

    gtk_tree_model_get_iter(model, &it, path);
    it_next = it;
    if (gtk_tree_model_iter_next(model, &it_next)) {
      gtk_list_store_swap(GTK_LIST_STORE(model), &it, &it_next);

      gtk_tree_path_next(path);
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(ptr->dst_view), path, col, FALSE);
      commit_worklist(ptr);
    }
  }
  gtk_tree_path_free(path);
}

/****************************************************************
...
*****************************************************************/
static void queue_insert(struct worklist_data *ptr, bool prepend)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  GtkTreePath *path;

  GtkTreeModel *src_model, *dst_model;
  GtkTreeIter src_it, dst_it;
  gint i, ncols;

  if (!GTK_WIDGET_IS_SENSITIVE(ptr->dst_view)) {
    return;
  }

  if (!gtk_tree_selection_get_selected(ptr->src_selection, &model, &it)) {
    return;
  }

  path = gtk_tree_model_get_path(model, &it);

  src_model = GTK_TREE_MODEL(ptr->src);
  dst_model = GTK_TREE_MODEL(ptr->dst);

  gtk_tree_model_get_iter(src_model, &src_it, path);
  if (prepend) {
    gtk_list_store_prepend(GTK_LIST_STORE(dst_model), &dst_it);
  } else {
    gtk_list_store_append(GTK_LIST_STORE(dst_model), &dst_it);
  }

  ncols = gtk_tree_model_get_n_columns(src_model);

  for (i = 0; i < ncols; i++) {
    GValue value = { 0, };

    gtk_tree_model_get_value(src_model, &src_it, i, &value);
    gtk_list_store_set_value(GTK_LIST_STORE(dst_model), &dst_it, i, &value);
  }
  commit_worklist(ptr);

  gtk_tree_path_free(path);
}

/****************************************************************
...
*****************************************************************/
static void queue_prepend(struct worklist_data *ptr)
{
  queue_insert(ptr, TRUE);
}

/****************************************************************
...
*****************************************************************/
static void queue_append(struct worklist_data *ptr)
{
  queue_insert(ptr, FALSE);
}

/****************************************************************
...
*****************************************************************/
static void src_row_callback(GtkTreeView *view, GtkTreePath *path,
                             GtkTreeViewColumn *col, gpointer data)
{
  struct worklist_data *ptr;
  GtkTreeModel *src_model, *dst_model;
  GtkTreeIter src_it, dst_it;
  gint i, ncols;

  ptr = data;

  if (!GTK_WIDGET_IS_SENSITIVE(ptr->dst_view)) {
    return;
  }

  src_model = GTK_TREE_MODEL(ptr->src);
  dst_model = GTK_TREE_MODEL(ptr->dst);

  gtk_tree_model_get_iter(src_model, &src_it, path);
  gtk_list_store_append(GTK_LIST_STORE(dst_model), &dst_it);

  ncols = gtk_tree_model_get_n_columns(src_model);

  for (i = 0; i < ncols; i++) {
    GValue value = { 0, };

    gtk_tree_model_get_value(src_model, &src_it, i, &value);
    gtk_list_store_set_value(GTK_LIST_STORE(dst_model), &dst_it, i, &value);
  }
  commit_worklist(ptr);
}

/****************************************************************
...
*****************************************************************/
static void dst_row_callback(GtkTreeView *view, GtkTreePath *path,
                             GtkTreeViewColumn *col, gpointer data)
{
  struct worklist_data *ptr;
  GtkTreeModel *dst_model;
  GtkTreeIter it;

  ptr = data;
  dst_model = GTK_TREE_MODEL(ptr->dst);

  gtk_tree_model_get_iter(dst_model, &it, path);

  gtk_list_store_remove(GTK_LIST_STORE(dst_model), &it);
  commit_worklist(ptr);
}

/****************************************************************
...
*****************************************************************/
static gboolean src_key_press_callback(GtkWidget *w, GdkEventKey *ev,
                                       gpointer data)
{
  struct worklist_data *ptr;

  ptr = data;

  if (!GTK_WIDGET_IS_SENSITIVE(ptr->dst_view)) {
    return FALSE;
  }

  if ((ev->state & GDK_SHIFT_MASK) && ev->keyval == GDK_Insert) {
    queue_prepend(ptr);
    return TRUE;
  } else if (ev->keyval == GDK_Insert) {
    queue_append(ptr);
    return TRUE;
  } else {
    return FALSE;
  }
}

/****************************************************************
...
*****************************************************************/
static gboolean dst_key_press_callback(GtkWidget *w, GdkEventKey *ev,
                                       gpointer data)
{
  GtkTreeModel *model;
  struct worklist_data *ptr;

  ptr = data;
  model = GTK_TREE_MODEL(ptr->dst);

  if (ev->keyval == GDK_Delete) {
    GtkTreeIter it, it_next;
    bool deleted = FALSE;

    if (gtk_tree_model_get_iter_first(model, &it)) {
      bool more;

      do {
        it_next = it;
        more = gtk_tree_model_iter_next(model, &it_next);

        if (gtk_tree_selection_iter_is_selected(ptr->dst_selection, &it)) {
          gtk_list_store_remove(GTK_LIST_STORE(model), &it);
          deleted = TRUE;
        }
        it = it_next;

      } while (more);
    }

    if (deleted) {
      commit_worklist(ptr);
    }
    return TRUE;

  } else if ((ev->state & GDK_MOD1_MASK) && ev->keyval == GDK_Up) {
    queue_bubble_up(ptr);
    return TRUE;

  } else if ((ev->state & GDK_MOD1_MASK) && ev->keyval == GDK_Down) {
    queue_bubble_down(ptr);
    return TRUE;

  } else {
    return FALSE;
  }
}

/****************************************************************
...
*****************************************************************/
static void src_selection_callback(GtkTreeSelection *selection, gpointer data)
{
  struct worklist_data *ptr;

  ptr = data;

  /* update widget sensitivity. */
  if (gtk_tree_selection_get_selected(selection, NULL, NULL)) {
    if (can_client_issue_orders()
        && ptr->pcity
        && ptr->pcity->owner == get_player_idx()) {
      gtk_widget_set_sensitive(ptr->change_cmd, TRUE);
      gtk_widget_set_sensitive(ptr->prepend_cmd, TRUE);
      gtk_widget_set_sensitive(ptr->append_cmd, TRUE);
    } else {
      gtk_widget_set_sensitive(ptr->change_cmd, FALSE);
      gtk_widget_set_sensitive(ptr->prepend_cmd, FALSE);
      gtk_widget_set_sensitive(ptr->append_cmd, FALSE);
    }
    gtk_widget_set_sensitive(ptr->help_cmd, TRUE);
  } else {
    gtk_widget_set_sensitive(ptr->change_cmd, FALSE);
    gtk_widget_set_sensitive(ptr->help_cmd, FALSE);
    gtk_widget_set_sensitive(ptr->prepend_cmd, FALSE);
    gtk_widget_set_sensitive(ptr->append_cmd, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
static void dst_selection_callback(GtkTreeSelection *selection, gpointer data)
{
  struct worklist_data *ptr;

  ptr = data;

  /* update widget sensitivity. */
  if (gtk_tree_selection_count_selected_rows(selection) > 0) {
    gtk_widget_set_sensitive(ptr->up_cmd, TRUE);
    gtk_widget_set_sensitive(ptr->down_cmd, TRUE);
  } else {
    gtk_widget_set_sensitive(ptr->up_cmd, FALSE);
    gtk_widget_set_sensitive(ptr->down_cmd, FALSE);
  }
}

/****************************************************************
...
*****************************************************************/
static gboolean dst_dnd_callback(GtkWidget *w, GdkDragContext *context,
                                 struct worklist_data *ptr)
{
  commit_worklist(ptr);
  return FALSE;
}

/****************************************************************
...
*****************************************************************/
static void cell_render_func(GtkTreeViewColumn *col, GtkCellRenderer *rend,
                             GtkTreeModel *model, GtkTreeIter *it,
                             gpointer data)
{
  gint cid, id;
  bool is_unit, on_icon_column;

  gtk_tree_model_get(model, it, 0, &cid, -1);
  is_unit = cid_is_unit(cid);
  id = cid_id(cid);
  on_icon_column = GTK_IS_CELL_RENDERER_PIXBUF(rend);

  if (on_icon_column && show_task_icons) {
    GdkPixbuf *pix;

    if (is_unit) {
      struct canvas store;

      if (max_unit_width == -1 || max_unit_height == -1) {
        update_max_unit_size();
      }

      freelog(LOG_DEBUG, "cell_render_func creating pixbuf w=%d h=%d",
              max_unit_width, max_unit_height);

      pix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                           max_unit_width, max_unit_height);

      freelog(LOG_DEBUG, "cell_render_func created pix=%p unit id=%d", pix, id);

      if (pix) {
        store.type = CANVAS_PIXBUF;
        store.v.pixbuf = pix;
        create_overlay_unit(&store, id);

        g_object_set(rend, "pixbuf", pix, NULL);
        g_object_unref(pix);
      }
    } else {
      struct impr_type *impr = get_improvement_type(id);

      pix = sprite_get_pixbuf(impr->sprite);
      g_object_set(rend, "pixbuf", pix, NULL);
    }
  }

  if (!on_icon_column) {
    struct city **pcity;
    struct player *plr;
    gint column;
    char *row[4];
    char  buf[4][64];
    int   i;
    gboolean useless;

    pcity = (struct city **) data;

    for (i = 0; i < ARRAY_SIZE(row); i++) {
      row[i] = buf[i];
    }
    column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(rend), "column"));

    get_city_dialog_production_row(row, sizeof(buf[0]), id, is_unit, *pcity);
    g_object_set(rend, "text", row[column], NULL);

    if (!is_unit && *pcity) {
      plr = city_owner(*pcity);
      useless = improvement_obsolete(plr, id)
        || improvement_redundant(plr, *pcity, id, FALSE);
      g_object_set(rend, "strikethrough", useless, NULL);
    } else {
      g_object_set(rend, "strikethrough", FALSE, NULL);
    }
  }
}

/****************************************************************
...
*****************************************************************/
static void populate_view(GtkTreeView *view, struct city **ppcity,
                          GtkTreeViewColumn **pcol)
{
  static const char *titles[] =
  { N_("Type"), N_("Name"), N_("Info"), N_("Cost"), N_("Turns") };

  static bool titles_done;
  gint i;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;

  intl_slist(ARRAY_SIZE(titles), titles, &titles_done);

  /* Case i == 0 taken out of the loop to workaround gcc-4.2.1 bug
   * http://gcc.gnu.org/PR33381
   * Some values would 'stick' from i == 0 round. */
  i = 0;

  rend = gtk_cell_renderer_pixbuf_new();

  gtk_tree_view_insert_column_with_data_func(view,
                   i, titles[i], rend, cell_render_func, ppcity, NULL);
  col = gtk_tree_view_get_column(view, i);

  if (show_task_icons) {
    if (max_unit_width == -1 || max_unit_height == -1) {
      update_max_unit_size();
    }
  } else {
    g_object_set(col, "visible", FALSE, NULL);
  }
  if (show_task_icons) {
    g_object_set(rend, "height", max_unit_height, NULL);
  }

  for (i = 1; i < ARRAY_SIZE(titles); i++) {

    gint pos = i-1;

    rend = gtk_cell_renderer_text_new();
    g_object_set_data(G_OBJECT(rend), "column", GINT_TO_POINTER(pos));

    gtk_tree_view_insert_column_with_data_func(view,
        i, titles[i], rend, cell_render_func, ppcity, NULL);
    col = gtk_tree_view_get_column(view, i);

    if (pos >= 2) {
      g_object_set(G_OBJECT(rend), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_alignment(col, 1.0);
    }

    if (pos == 3) {
      *pcol = col;
    }
    if (show_task_icons) {
      g_object_set(rend, "height", max_unit_height, NULL);
    }
  }
}

/****************************************************************
  Worklist editor shell.
*****************************************************************/
GtkWidget *create_worklist(void)
{
  GtkWidget *editor, *table, *sw, *bbox;
  GtkWidget *src_view, *dst_view, *label, *button;
  GtkWidget *menubar, *item, *menu, *image;
  GtkWidget *table2, *arrow, *check;
  GtkSizeGroup *group;

  GtkListStore *src_store, *dst_store;

  struct worklist_data *ptr;


  ptr = wc_malloc(sizeof(*ptr));

  src_store = gtk_list_store_new(1, G_TYPE_INT);
  dst_store = gtk_list_store_new(1, G_TYPE_INT);

  ptr->pwl = NULL;
  ptr->pcity = NULL;
  ptr->src = src_store;
  ptr->dst = dst_store;
  ptr->future = FALSE;


  /* create shell. */
  editor = gtk_vbox_new(FALSE, 6);
  g_signal_connect(editor, "destroy", G_CALLBACK(worklist_destroy), ptr);
  g_object_set_data(G_OBJECT(editor), "data", ptr);

  ptr->editor = editor;

  /* add source and target lists.  */
  table = gtk_table_new(2, 5, FALSE);
  gtk_box_pack_start(GTK_BOX(editor), table, TRUE, TRUE, 0);

  group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_table_attach(GTK_TABLE(table), sw, 3, 5, 1, 2,
                   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  src_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(src_store));
  g_object_unref(src_store);
  gtk_size_group_add_widget(group, src_view);
  gtk_widget_set_name(src_view, "small font");

  populate_view(GTK_TREE_VIEW(src_view), &ptr->pcity, &ptr->src_col);
  gtk_container_add(GTK_CONTAINER(sw), src_view);

  label = g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", src_view,
                       "label", _("Source _Tasks:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_table_attach(GTK_TABLE(table), label, 3, 4, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);

  check = gtk_check_button_new_with_mnemonic(_("Show _Future Targets"));
  gtk_table_attach(GTK_TABLE(table), check, 4, 5, 0, 1,
                   0, GTK_FILL, 0, 0);
  g_signal_connect(check, "toggled", G_CALLBACK(future_callback), ptr);


  table2 = gtk_table_new(4, 1, FALSE);
  gtk_table_attach(GTK_TABLE(table), table2, 2, 3, 1, 2,
                   GTK_FILL, GTK_FILL, 0, 0);

  button = gtk_button_new();
  ptr->prepend_cmd = button;
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  gtk_table_attach(GTK_TABLE(table2), button, 0, 1, 0, 1,
      0, GTK_EXPAND|GTK_FILL, 0, 24);

  arrow = gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  g_signal_connect_swapped(button, "clicked",
                           G_CALLBACK(queue_prepend), ptr);
  gtk_widget_set_sensitive(ptr->prepend_cmd, FALSE);

  button = gtk_button_new();
  ptr->up_cmd = button;
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  gtk_table_attach(GTK_TABLE(table2), button, 0, 1, 1, 2, 0, 0, 0, 0);

  arrow = gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  g_signal_connect_swapped(button, "clicked",
                           G_CALLBACK(queue_bubble_up), ptr);
  gtk_widget_set_sensitive(ptr->up_cmd, FALSE);

  button = gtk_button_new();
  ptr->down_cmd = button;
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  gtk_table_attach(GTK_TABLE(table2), button, 0, 1, 2, 3, 0, 0, 0, 0);

  arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  g_signal_connect_swapped(button, "clicked",
                           G_CALLBACK(queue_bubble_down), ptr);
  gtk_widget_set_sensitive(ptr->down_cmd, FALSE);

  button = gtk_button_new();
  ptr->append_cmd = button;
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  gtk_table_attach(GTK_TABLE(table2), button, 0, 1, 3, 4,
      0, GTK_EXPAND|GTK_FILL, 0, 24);

  arrow = gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  g_signal_connect_swapped(button, "clicked",
                           G_CALLBACK(queue_append), ptr);
  gtk_widget_set_sensitive(ptr->append_cmd, FALSE);


  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_table_attach(GTK_TABLE(table), sw, 0, 2, 1, 2,
                   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  dst_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(dst_store));
  g_object_unref(dst_store);
  gtk_size_group_add_widget(group, dst_view);
  gtk_widget_set_name(dst_view, "small font");

  populate_view(GTK_TREE_VIEW(dst_view), &ptr->pcity, &ptr->dst_col);
  gtk_container_add(GTK_CONTAINER(sw), dst_view);

  label = g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", dst_view,
                       "label", _("Target _Worklist:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);

  /* add bottom menu and buttons. */
  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(bbox), 10);
  gtk_box_pack_start(GTK_BOX(editor), bbox, FALSE, FALSE, 0);

  menubar = gtk_menu_bar_new();
  gtk_container_add(GTK_CONTAINER(bbox), menubar);
  gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(bbox), menubar, TRUE);

  menu = gtk_menu_new();

  image = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
  item = gtk_image_menu_item_new_with_mnemonic(_("_Add Global Worklist"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
  g_signal_connect(menu, "show",
                   G_CALLBACK(popup_add_menu), ptr);
  ptr->add_cmd = item;
  gtk_widget_set_sensitive(ptr->add_cmd, FALSE);

  button = gtk_button_new_with_mnemonic (_("Clear _Worklist"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(clear_callback), ptr);
  ptr->clear_cmd = button;
  gtk_widget_set_sensitive(ptr->clear_cmd, FALSE);

  button = gtk_button_new_from_stock(GTK_STOCK_HELP);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(help_callback), ptr);
  ptr->help_cmd = button;
  gtk_widget_set_sensitive(ptr->help_cmd, FALSE);

  button = gtk_button_new_with_mnemonic(_("Chan_ge Production"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(change_callback), ptr);
  ptr->change_cmd = button;
  gtk_widget_set_sensitive(ptr->change_cmd, FALSE);

  ptr->src_view = src_view;
  ptr->dst_view = dst_view;
  ptr->src_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(src_view));
  ptr->dst_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dst_view));
  gtk_tree_selection_set_mode(ptr->dst_selection, GTK_SELECTION_MULTIPLE);

  /* DND and other state changing callbacks. */
  gtk_tree_view_set_reorderable(GTK_TREE_VIEW(dst_view), TRUE);
  g_signal_connect(dst_view, "drag_end",
                   G_CALLBACK(dst_dnd_callback), ptr);

  g_signal_connect(src_view, "row_activated",
                   G_CALLBACK(src_row_callback), ptr);
  g_signal_connect(src_view, "key_press_event",
                   G_CALLBACK(src_key_press_callback), ptr);

  g_signal_connect(dst_view, "row_activated",
                   G_CALLBACK(dst_row_callback), ptr);
  g_signal_connect(dst_view, "key_press_event",
                   G_CALLBACK(dst_key_press_callback), ptr);

  g_signal_connect(ptr->src_selection, "changed",
                   G_CALLBACK(src_selection_callback), ptr);
  g_signal_connect(ptr->dst_selection, "changed",
                   G_CALLBACK(dst_selection_callback), ptr);


  gtk_widget_show_all(table);
  gtk_widget_show_all(bbox);

  return editor;
}


/****************************************************************
...
*****************************************************************/
void reset_worklist(GtkWidget *editor, struct worklist *pwl, struct city *pcity)
{
  struct worklist_data *ptr;

  ptr = g_object_get_data(G_OBJECT(editor), "data");

  ptr->pwl = pwl;
  ptr->pcity = pcity;

  gtk_list_store_clear(ptr->src);
  gtk_list_store_clear(ptr->dst);

  /* DND callbacks and object visibility settings. */
  if (pcity) {
    g_object_set(ptr->src_col, "visible", TRUE, NULL);
    g_object_set(ptr->dst_col, "visible", TRUE, NULL);

    gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(ptr->src_view),
                                           GDK_BUTTON1_MASK,
                                           wl_dnd_targets,
                                           G_N_ELEMENTS(wl_dnd_targets),
                                           GDK_ACTION_COPY);
  } else {
    gtk_widget_hide(ptr->change_cmd);
    g_object_set(ptr->src_col, "visible", FALSE, NULL);
    g_object_set(ptr->dst_col, "visible", FALSE, NULL);

    gtk_tree_view_unset_rows_drag_source(GTK_TREE_VIEW(ptr->src_view));
  }
}

/****************************************************************
...
*****************************************************************/
void refresh_worklist(GtkWidget *editor)
{
  struct worklist_data *ptr;
  struct worklist *pwl, queue;

  cid cids[U_LAST + B_LAST];
  int i, cids_used;
  struct item items[U_LAST + B_LAST];

  bool selected, sens;
  gint id;
  GtkTreeIter it;

  GtkTreePath *path;

  GtkTreeModel *model;
  gboolean exists;

  ptr = g_object_get_data(G_OBJECT(editor), "data");
  pwl = ptr->pwl;

  if (!pwl) {
    return;
  }

  /* refresh source tasks. */
  if (gtk_tree_selection_get_selected(ptr->src_selection, NULL, &it)) {
    gtk_tree_model_get(GTK_TREE_MODEL(ptr->src), &it, 0, &id, -1);
    selected = TRUE;
  } else {
    selected = FALSE;
  }
  gtk_list_store_clear(ptr->src);

  cids_used = collect_cids4(cids, ptr->pcity, ptr->future);
  name_and_sort_items(cids, cids_used, items, FALSE, ptr->pcity);

  path = NULL;
  for (i = 0; i < cids_used; i++) {
    gtk_list_store_append(ptr->src, &it);
    gtk_list_store_set(ptr->src, &it, 0, (gint) items[i].cid, -1);

    if (selected && items[i].cid == id) {
      path = gtk_tree_model_get_path(GTK_TREE_MODEL(ptr->src), &it);
    }
  }
  if (path) {
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(ptr->src_view), path, NULL, FALSE);
    gtk_tree_path_free(path);
  }


  /* refresh target worklist. */
  model = GTK_TREE_MODEL(ptr->dst);
  exists = gtk_tree_model_get_iter_first(model, &it);

  /* dance around worklist braindamage. */
  if (ptr->pcity) {
    city_get_queue(ptr->pcity, &queue);
  } else {
    copy_worklist(&queue, pwl);
  }

  for (i = 0; i < MAX_LEN_WORKLIST; i++) {
    cid cid;

    if (queue.wlefs[i] == WEF_END) {
      break;
    }

    cid = cid_encode(queue.wlefs[i] == WEF_UNIT, queue.wlids[i]);

    if (!exists) {
      gtk_list_store_append(ptr->dst, &it);
    }

    gtk_list_store_set(ptr->dst, &it, 0, (gint) cid, -1);

    if (exists) {
      exists = gtk_tree_model_iter_next(model, &it);
    }
  }

  if (exists) {
    GtkTreeIter it_next;
    bool more;

    do {
      it_next = it;
      more = gtk_tree_model_iter_next(model, &it_next);

      gtk_list_store_remove(ptr->dst, &it);
      it = it_next;
    } while (more);
  }

  /* update widget sensitivity. */
  sens = !ptr->pcity || (can_client_issue_orders()
                         && ptr->pcity->owner == get_player_idx());
  gtk_widget_set_sensitive(ptr->add_cmd, sens);
  gtk_widget_set_sensitive(ptr->clear_cmd, sens);
  gtk_widget_set_sensitive(ptr->dst_view, sens);

  /* Sometimes, the destination widget is not updated for an unknown reason. */
  gtk_widget_queue_draw(ptr->dst_view);
}

/****************************************************************
...
*****************************************************************/
static void commit_worklist(struct worklist_data *ptr)
{
  struct worklist *pwl, queue;
  GtkTreeModel *model;
  GtkTreeIter it;
  int i;

  pwl = ptr->pwl;

  if (!pwl) {
    return;
  }

  model = GTK_TREE_MODEL(ptr->dst);

  init_worklist(&queue);
  sz_strlcpy(queue.name, pwl->name);

  i = 0;
  if (gtk_tree_model_get_iter_first(model, &it)) {
    do {
      gint cid;

      /* oops, the player has a worklist longer than what we can store. */
      if (i >= MAX_LEN_WORKLIST) {
        break;
      }

      gtk_tree_model_get(model, &it, 0, &cid, -1);
      queue.wlefs[i] = cid_is_unit(cid) ? WEF_UNIT : WEF_IMPR;
      queue.wlids[i] = cid_id(cid);

      i++;
    } while (gtk_tree_model_iter_next(model, &it));
  }

  /* dance around worklist braindamage. */
  if (ptr->pcity) {
    if (!city_set_queue(ptr->pcity, &queue)) {
      /* Failed to change worklist. This means worklist visible
       * on screen is not true. */
      refresh_worklist(ptr->editor);
    }
  } else {
    copy_worklist(pwl, &queue);
  }
}
