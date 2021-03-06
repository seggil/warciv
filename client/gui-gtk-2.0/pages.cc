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
#include <time.h>

#include <sys/stat.h>

#include <gtk/gtk.h>

#include "dataio.hh"
#include "wc_intl.hh"
#include "game.hh"
#include "log.hh"
#include "mem.hh"
#include "netintf.hh"            /* adns_is_available */
#include "shared.hh"
#include "support.hh"
#include "version.hh"

#include "chatline.hh"
#include "../civclient.hh"
#include "../climisc.hh"
#include "../clinet.hh"
#include "colors.hh"
#include "connectdlg.hh"
#include "../connectdlg_common.hh"
#include "dialogs.hh"
#include "graphics.hh"
#include "gui_main.hh"
#include "gui_stuff.hh"
#include "optiondlg.hh"
#include "../packhand.hh"
#include "pages.hh"
#include "style.hh"


GtkWidget *start_message_area;
GtkListStore *conn_model;

static GtkWidget *start_options_table;

static GtkListStore *load_store, *scenario_store,
  *nation_store, *meta_store, *lan_store, *variables_store;
static GtkTreeStore *meta_player_tree_store;

enum metaplayerlist_column_ids {
  METAPLAYER_LIST_COL_NAME,
  METAPLAYER_LIST_COL_USER,
  METAPLAYER_LIST_COL_HOST,
  METAPLAYER_LIST_COL_TYPE,
  METAPLAYER_LIST_COL_NATION,

  METAPLAYER_LIST_NUM_COLUMNS,
};

enum metavariableslist_column_ids {
  MVAR_COL_NAME,
  MVAR_COL_VALUE,

  MVAR_NUM_COLUMNS,
};

static GtkTreeSelection *load_selection, *scenario_selection,
  *nation_selection, *meta_selection, *lan_selection;

static enum client_pages current_page;

static void set_page_callback(GtkWidget *w, gpointer data);
static void update_nation_page(struct packet_game_load *packet);

static guint lan_timer = 0;
static int num_lanservers_timer = 0;

static GtkWidget *statusbar, *statusbar_frame;
static GtkWidget *network_message_dialog = NULL;
static GtkWidget *start_page_entry = NULL;

static struct server_list *internet_server_list = NULL;
static int server_list_request_id = -1;

/**************************************************************************
  spawn a server, if there isn't one, using the default settings.
**************************************************************************/
static void start_new_game_callback(GtkWidget *w, gpointer data)
{
  if (is_server_running()) {
    return;
  }

  if (!client_start_server()) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new(GTK_WINDOW(toplevel),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
        _("Failed to start the server!\n"
          "You will have to start it separately."));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  }
}

/**************************************************************************
  go to the scenario page, spawning a server,
**************************************************************************/
static void start_scenario_callback(GtkWidget *w, gpointer data)
{
  set_client_page(PAGE_SCENARIO);

  start_new_game_callback(NULL, NULL);
}

/**************************************************************************
  go to the load page, spawning a server.
**************************************************************************/
static void load_saved_game_callback(GtkWidget *w, gpointer data)
{
  set_client_page(PAGE_LOAD);

  start_new_game_callback(NULL, NULL);
}

/**************************************************************************
  cancel, by terminating the connection and going back to main page.
**************************************************************************/
static void main_callback(GtkWidget *w, gpointer data)
{
  if (server_list_request_id > 0) {
    cancel_async_server_list_request(server_list_request_id);
    append_network_statusbar(_("Server list request cancelled."));
    server_list_request_id = -1;
  }
  if (aconnection.used) {
    disconnect_from_server();
  } else {
    set_client_page(PAGE_MAIN);
  }
}

/**************************************************************************
  this is called whenever the intro graphic needs a graphics refresh.
**************************************************************************/
static gboolean intro_expose(GtkWidget *w, GdkEventExpose *ev)
{
  static PangoLayout *layout;
  static int width, height;
  GdkScreen *screen;

  screen = gdk_screen_get_default();

  if (!layout) {
    char msgbuf[128];

    layout = pango_layout_new(gdk_pango_context_get_for_screen(screen));
    pango_layout_set_font_description(layout, main_font);

    my_snprintf(msgbuf, sizeof(msgbuf), "%s%s",
        word_version(), VERSION_STRING);
    pango_layout_set_text(layout, msgbuf, -1);

    pango_layout_get_pixel_size(layout, &width, &height);
  }

  gtk_draw_shadowed_string(w->window,
                           w->style->black_gc,
                           w->style->white_gc,
                           w->allocation.x + w->allocation.width - width - 4,
                           w->allocation.y + w->allocation.height - height -
                           4, layout);
  return TRUE;
}

/**************************************************************************
  create the main page.
**************************************************************************/
GtkWidget *create_main_page(void)
{
  GtkWidget *align, *box, *sbox, *bbox, *frame, *image;

  GtkWidget *button;
  GtkSizeGroup *size;

  size = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);

  box = gtk_vbox_new(FALSE, 6);
  gtk_container_set_border_width(GTK_CONTAINER(box), 4);

  align = gtk_alignment_new(0.5, 0.0, 0.0, 0.0);
  gtk_container_set_border_width(GTK_CONTAINER(align), 18);
  gtk_box_pack_start(GTK_BOX(box), align, FALSE, FALSE, 0);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
  gtk_container_add(GTK_CONTAINER(align), frame);

  image = gtk_image_new_from_file(main_intro_filename);
  g_signal_connect_after(image, "expose_event",
      G_CALLBACK(intro_expose), NULL);
  gtk_container_add(GTK_CONTAINER(frame), image);

  align = gtk_alignment_new(0.5, 0.0, 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(box), align, FALSE, FALSE, 0);

  sbox = gtk_vbox_new(FALSE, 18);
  gtk_container_add(GTK_CONTAINER(align), sbox);

  bbox = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(sbox), bbox);

  button = gtk_button_new_with_mnemonic(_("Start _New Game"));
  gtk_size_group_add_widget(size, button);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
      G_CALLBACK(start_new_game_callback), NULL);

  button = gtk_button_new_with_mnemonic(_("Start _Scenario Game"));
  gtk_size_group_add_widget(size, button);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
      G_CALLBACK(start_scenario_callback), NULL);

  button = gtk_button_new_with_mnemonic(_("_Load Saved Game"));
  gtk_size_group_add_widget(size, button);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
      G_CALLBACK(load_saved_game_callback), NULL);

  bbox = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(sbox), bbox);

  button = gtk_button_new_with_mnemonic(_("C_onnect to Network Game"));
  gtk_size_group_add_widget(size, button);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(set_page_callback),
                   GUINT_TO_POINTER(PAGE_NETWORK));

  bbox = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(sbox), bbox);

  button = gtk_button_new_from_stock(GTK_STOCK_QUIT);
  gtk_size_group_add_widget(size, button);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked", G_CALLBACK(gtk_main_quit), NULL);

#if IS_BETA_VERSION
  {
    GtkWidget *label;

    label = gtk_label_new(beta_message());
    gtk_widget_set_name(label, "beta label");
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_container_add(GTK_CONTAINER(sbox), label);
  }
#endif

  return box;
}


/**************************************************************************
                                 NETWORK PAGE
**************************************************************************/
static GtkWidget *network_login_label, *network_login;
static GtkWidget *network_host_label, *network_host;
static GtkWidget *network_port_label, *network_port;
static GtkWidget *network_password_label, *network_password;
static GtkWidget *network_confirm_password_label, *network_confirm_password;

/**************************************************************************
  ...
**************************************************************************/
static int get_real_player_number(struct server *pserver)
{
  struct server::server_players *pplayer;
  int i;
  int nplayers = 0;

  if (!pserver->players) {
    /* We don't have the list to verify */
    return pserver->nplayers;
  }

  for (i = 0, pplayer = pserver->players; i < pserver->nplayers;
       i++, pplayer++) {
    /* All 'player' cases */
    if (0 == mystrcasecmp("Human", pplayer->type)
        || 0 == mystrcasecmp("A.I.", pplayer->type)
        || 0 == mystrcasecmp("Dead", pplayer->type)
        || 0 == mystrcasecmp("Barbarian", pplayer->type)) {
      nplayers++;
    }
  }

  return nplayers;
}

/**************************************************************************
  update a server list.
**************************************************************************/
static void update_server_list(GtkTreeSelection *selection,
                               GtkListStore *store,
                               struct server_list *list)
{
  const gchar *host, *port;

  if (get_client_page() != PAGE_NETWORK){
    return;
  }

  host = gtk_entry_get_text(GTK_ENTRY(network_host));
  port = gtk_entry_get_text(GTK_ENTRY(network_port));

  gtk_list_store_clear(store);

  if (!list) {
    return;
  }

  server_list_iterate(list, pserver) {
    GtkTreeIter it;

    gtk_list_store_append(store, &it);
    gtk_list_store_set(store, &it,
                       0, pserver->host,
                       1, pserver->port,
                       2, pserver->version,
                       3, pserver->patches,
                       4, _(pserver->state),
                       5, get_real_player_number(pserver),
                       6, pserver->message, -1);

    if (strcmp(host, pserver->host) == 0 && strcmp(port, pserver->port) == 0) {
      gtk_tree_selection_select_iter(selection, &it);
    }
  } server_list_iterate_end;
}

/**************************************************************************
  new_server_list must be freed when no longer needed.
**************************************************************************/
static void server_list_created_callback(struct server_list *new_server_list,
                                         const char *error, void *data)
{
  char msg[256];

  freelog(LOG_DEBUG,
          "server_list_created_callback new_server_list=%p"
          " error=\"%s\" data=%p", new_server_list, error, data);
  /*if (internet_server_list)
    delete_server_list(internet_server_list);*/

  if (!new_server_list) {
    if (server_list_request_id > 0) {
      cancel_async_server_list_request(server_list_request_id);
      my_snprintf(msg, sizeof(msg),
                  _("Could not get server list: %s"), error);
      append_network_statusbar(msg);
      server_list_request_id = -1;
     return;
    }
  } else {
    my_snprintf(msg, sizeof(msg),
                _("Received %d server(s) from the meta server."),
                server_list_size(new_server_list));
    freelog(LOG_DEBUG, "slcc   list size %d",
            server_list_size(new_server_list));
  }

  server_list_request_id = -1;
  append_network_statusbar(msg);

  internet_server_list = new_server_list;

  update_server_list(meta_selection, meta_store, internet_server_list);
}

/**************************************************************************
  Get the list of servers from the metaserver.
**************************************************************************/
static bool get_meta_list(char *errbuf, int n_errbuf)
{
  int id;
  char buf[1024];

  if (!adns_is_available()) {
    /* if (internet_server_list)
      delete_server_list(internet_server_list);*/

    internet_server_list = create_server_list(errbuf, n_errbuf);

    update_server_list(meta_selection, meta_store, internet_server_list);

    return internet_server_list != NULL;
  }

  freelog(LOG_DEBUG, "gml get_meta_list server_list_request_id=%d",
          server_list_request_id);
  if (server_list_request_id != -1) {
    freelog(LOG_DEBUG, "gml   request in progress...");

    my_snprintf(errbuf, n_errbuf, _("Server list request in progress..."));
    return FALSE;
  }

  id = create_server_list_async(errbuf, n_errbuf,
                                server_list_created_callback, NULL, NULL);
  freelog(LOG_DEBUG, "gml   got server_list_request_id=%d", id);
  if (id == -1) {
    return FALSE;               /* we got an error :( */
  }
  if (id == 0) {
    return TRUE;                /* server_list_created_callback called directly */
  }

  /* request in progress! */
  server_list_request_id = id;
  my_snprintf(buf, sizeof(buf),
              _("Requesting server list from %s..."), default_metaserver);
  append_network_statusbar(buf);
  return TRUE;
}

/**************************************************************************
  this function frees the list of LAN servers on timeout destruction.
**************************************************************************/
static void get_lan_destroy(gpointer data)
{
  finish_lanserver_scan();
  num_lanservers_timer = 0;
  lan_timer = 0;
}

/**************************************************************************
  this function updates the list of LAN servers every 1000 ms for 5 secs.
**************************************************************************/
static gboolean get_lan_list(gpointer data)
{
  struct server_list *server_list = get_lan_server_list();

  update_server_list(lan_selection, lan_store, server_list);
  num_lanservers_timer++;
  if (num_lanservers_timer == 5) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**************************************************************************
  update the metaserver and lan server lists.
**************************************************************************/
static void update_network_lists(void)
{
  char errbuf[128];

  if (!get_meta_list(errbuf, sizeof(errbuf)))  {
    append_network_statusbar(errbuf);
  }

  if (lan_timer == 0) {
    if (begin_lanserver_scan()) {
      lan_timer = g_timeout_add_full(G_PRIORITY_DEFAULT, 1000,
          get_lan_list, NULL, get_lan_destroy);
    }
  }
}

/**************************************************************************
  network connection state defines.
**************************************************************************/
enum connection_state {
  LOGIN_TYPE,
  NEW_PASSWORD_TYPE,
  ENTER_PASSWORD_TYPE,
  WAITING_TYPE
};

static enum connection_state connection_status;

/**************************************************************************
  clear statusbar.
**************************************************************************/
static void clear_network_statusbar(void)
{
  GtkTextIter start, end;

  gtk_text_buffer_get_start_iter(network_message_buffer, &start);
  gtk_text_buffer_get_end_iter(network_message_buffer, &end);
  gtk_text_buffer_delete(network_message_buffer, &start, &end);

  gtk_label_set_text(GTK_LABEL(statusbar), "");
  append_network_statusbar(_("Network messages has been cleared."));
}

/**************************************************************************
  Add a message to the status bar.
**************************************************************************/
void append_network_statusbar(const char *text)
{
  GtkTextIter iter;
  char buf[64];
  struct tm *nowtm;
  time_t now;

  if (!text[0]) {
    gtk_label_set_text(GTK_LABEL(statusbar), "");
    return;
  }

  now = time(NULL);
  nowtm = localtime(&now);
  strftime(buf, sizeof(buf), "[%H:%M:%S] ", nowtm);
  gtk_text_buffer_get_end_iter(network_message_buffer, &iter);
  gtk_text_buffer_insert(network_message_buffer, &iter, buf, -1);
  gtk_text_buffer_insert(network_message_buffer, &iter, text, -1);
  gtk_text_buffer_insert(network_message_buffer, &iter, "\n", -1);

  if (GTK_WIDGET_VISIBLE(statusbar_frame)) {
    gtk_label_set_text(GTK_LABEL(statusbar), text);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static GtkWidget *create_network_message_dialog(void)
{
  GtkWidget *dialog, *vbox, *hbox, *sw, *button, *text, *clearbutton;

  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(dialog), _("Network Messages"));
  gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 200);
  gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

  gtk_widget_set_name(dialog, "Warciv");
  setup_dialog(dialog, toplevel);

  g_signal_connect_swapped(dialog, "destroy",
                           G_CALLBACK(gtk_widget_hide), dialog);

  vbox = gtk_vbox_new(FALSE, 8);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
  text = gtk_text_view_new_with_buffer(network_message_buffer);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
  gtk_container_add(GTK_CONTAINER(sw), text);

  button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  g_signal_connect_swapped(button, "clicked",
                           G_CALLBACK(gtk_widget_hide), dialog);
  gtk_widget_set_size_request(button, 100, 30);

  clearbutton = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
  g_signal_connect_swapped(clearbutton, "clicked",
                           G_CALLBACK(clear_network_statusbar), NULL);
  gtk_widget_set_size_request(clearbutton, 100, 30);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 4);
  gtk_box_pack_end(GTK_BOX(hbox), clearbutton, FALSE, FALSE, 0);

  gtk_widget_show_all(dialog);

  return dialog;
}

/**************************************************************************
  ...
**************************************************************************/
static void popup_network_messages_callback(GtkWidget * w, gpointer data)
{
  if (!network_message_dialog) {
    network_message_dialog = create_network_message_dialog();
  }
  gtk_window_present(GTK_WINDOW(network_message_dialog));
}

/**************************************************************************
  create statusbar.
**************************************************************************/
GtkWidget *create_statusbar(void)
{
  GtkWidget *button;

  statusbar_frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(statusbar_frame), GTK_SHADOW_IN);

  button = gtk_button_new();
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(popup_network_messages_callback), NULL);
  statusbar = gtk_label_new("");
  gtk_container_add(GTK_CONTAINER(button), statusbar);
  gtk_container_add(GTK_CONTAINER(statusbar_frame), button);

  return statusbar_frame;
}

/**************************************************************************
  update network page connection state.
**************************************************************************/
static void set_connection_state(enum connection_state state)
{
  switch (state) {
  case LOGIN_TYPE:
    append_network_statusbar("");

    gtk_entry_set_text(GTK_ENTRY(network_password), "");
    gtk_entry_set_text(GTK_ENTRY(network_confirm_password), "");

    gtk_widget_set_sensitive(network_host, TRUE);
    gtk_widget_set_sensitive(network_port, TRUE);
    gtk_widget_set_sensitive(network_login, TRUE);
    gtk_widget_set_sensitive(network_password_label, FALSE);
    gtk_widget_set_sensitive(network_password, FALSE);
    gtk_widget_set_sensitive(network_confirm_password_label, FALSE);
    gtk_widget_set_sensitive(network_confirm_password, FALSE);
    break;
  case NEW_PASSWORD_TYPE:
    gtk_entry_set_text(GTK_ENTRY(network_password), "");
    gtk_entry_set_text(GTK_ENTRY(network_confirm_password), "");

    gtk_widget_set_sensitive(network_host, FALSE);
    gtk_widget_set_sensitive(network_port, FALSE);
    gtk_widget_set_sensitive(network_login, FALSE);
    gtk_widget_set_sensitive(network_password_label, TRUE);
    gtk_widget_set_sensitive(network_password, TRUE);
    gtk_widget_set_sensitive(network_confirm_password_label, TRUE);
    gtk_widget_set_sensitive(network_confirm_password, TRUE);

    gtk_widget_grab_focus(network_password);
    break;
  case ENTER_PASSWORD_TYPE:
    set_client_page(PAGE_NETWORK);
    gtk_entry_set_text(GTK_ENTRY(network_password), "");
    gtk_entry_set_text(GTK_ENTRY(network_confirm_password), "");

    gtk_widget_set_sensitive(network_host, FALSE);
    gtk_widget_set_sensitive(network_port, FALSE);
    gtk_widget_set_sensitive(network_login, FALSE);
    gtk_widget_set_sensitive(network_password_label, TRUE);
    gtk_widget_set_sensitive(network_password, TRUE);
    gtk_widget_set_sensitive(network_confirm_password_label, FALSE);
    gtk_widget_set_sensitive(network_confirm_password, FALSE);

    gtk_widget_grab_focus(network_password);
    break;
  case WAITING_TYPE:
    append_network_statusbar("");

    gtk_widget_set_sensitive(network_login, FALSE);
    gtk_widget_set_sensitive(network_password_label, FALSE);
    gtk_widget_set_sensitive(network_password, FALSE);
    gtk_widget_set_sensitive(network_confirm_password_label, FALSE);
    gtk_widget_set_sensitive(network_confirm_password, FALSE);
    break;
  }

  connection_status = state;
}

/**************************************************************************
 configure the dialog depending on what type of authentication request the
 server is making.
 from client/packhand_gen.c
**************************************************************************/
void handle_authentication_req(enum authentication_type type, char *message) /* 6 sc */
{
# if REPLAY
  printf("AUTHENTICATION_REQ\n"); /* done */
  printf("type=%d message=%s\n", type, message);
# endif
  append_network_statusbar(message);

  switch (type) {
  case AUTH_NEWUSER_FIRST:
  case AUTH_NEWUSER_RETRY:
    set_connection_state(NEW_PASSWORD_TYPE);
    break;
  case AUTH_LOGIN_FIRST:
    /* if we magically have a password already present in 'password'
     * then, use that and skip the password entry dialog */
    if (default_password[0] != '\0') {
      struct packet_authentication_reply reply;

      sz_strlcpy(reply.password, default_password);
      send_packet_authentication_reply(&aconnection, &reply);
      return;
    } else {
      set_connection_state(ENTER_PASSWORD_TYPE);
    }
    break;
  case AUTH_LOGIN_RETRY:
    set_connection_state(ENTER_PASSWORD_TYPE);
    break;
  default:
    assert(0);
  }
}

/**************************************************************************
 if on the network page, switch page to the login page (with new server
 and port). if on the login page, send connect and/or authentication
 requests to the server.
**************************************************************************/
static void connect_callback(GtkWidget *w, gpointer data)
{
  char errbuf [512];
  struct packet_authentication_reply reply;

  if (server_list_request_id > 0) {
    cancel_async_server_list_request(server_list_request_id);
    append_network_statusbar(_("Server list request cancelled."));
    server_list_request_id = -1;
  }

  switch (connection_status) {
  case LOGIN_TYPE:
    sz_strlcpy(default_user_name,
               gtk_entry_get_text(GTK_ENTRY(network_login)));
    sz_strlcpy(server_host,
               gtk_entry_get_text(GTK_ENTRY(network_host)));
    server_port = atoi(gtk_entry_get_text(GTK_ENTRY(network_port)));

    if (-1 == connect_to_server(default_user_name, server_host,
                                server_port, errbuf, sizeof(errbuf))) {
      append_network_statusbar(errbuf);
    }
    break;
  case NEW_PASSWORD_TYPE:
    if (w != network_password) {
      sz_strlcpy(default_password,
                 gtk_entry_get_text(GTK_ENTRY(network_password)));
      sz_strlcpy(reply.password,
          gtk_entry_get_text(GTK_ENTRY(network_confirm_password)));
      if (strncmp(reply.password, default_password, MAX_LEN_NAME) == 0) {
        default_password[0] = '\0';
        send_packet_authentication_reply(&aconnection, &reply);

        set_connection_state(WAITING_TYPE);
      } else {
        append_network_statusbar(_("Passwords don't match, enter password."));
        set_connection_state(NEW_PASSWORD_TYPE);
      }
    }
    break;
  case ENTER_PASSWORD_TYPE:
    sz_strlcpy(reply.password,
        gtk_entry_get_text(GTK_ENTRY(network_password)));
    send_packet_authentication_reply(&aconnection, &reply);

    set_connection_state(WAITING_TYPE);
    break;
  case WAITING_TYPE:
    break;
  default:
    assert(0);
  }
}

/**************************************************************************
  connect on list item double-click.
***************************************************************************/
static void network_activate_callback(GtkTreeView *view,
                                      GtkTreePath *arg1,
                                      GtkTreeViewColumn *arg2,
                                      gpointer data)
{
  connect_callback(NULL, data);
}

/**************************************************************************
  ...
**************************************************************************/
static void meta_player_tree_store_append(struct server *pserver,
                                          const char *type)
{
  struct server::server_players *pplayer;
  struct server::server_players *pobserver;
  GtkTreeIter parent, iter;
  int i, j;
  size_t name_len;

  for (i = 0, pplayer = pserver->players; i < pserver->nplayers;
       i++, pplayer++) {
    if (0 == mystrcasecmp(type, pplayer->type)) {
      gtk_tree_store_append(meta_player_tree_store, &parent, NULL);
      gtk_tree_store_set(meta_player_tree_store, &parent,
                         METAPLAYER_LIST_COL_NAME, pplayer->name,
                         METAPLAYER_LIST_COL_USER, pplayer->user,
                         METAPLAYER_LIST_COL_HOST, pplayer->host,
                         METAPLAYER_LIST_COL_TYPE, pplayer->type,
                         METAPLAYER_LIST_COL_NATION, pplayer->nation, -1);
      name_len = strlen(pplayer->name);
      /* Check for observers */
      for (j = 0, pobserver = pserver->players; j < pserver->nplayers;
           j++, pobserver++) {
        if (pobserver->name[0] == '*'
            && pobserver->name[1] == '('
            && 0 == strncmp(pobserver->name + 2, pplayer->name, name_len)
            && pobserver->name[2 + name_len] == ')') {
          gtk_tree_store_append(meta_player_tree_store, &iter, &parent);
          gtk_tree_store_set(meta_player_tree_store, &iter,
                             METAPLAYER_LIST_COL_USER, pobserver->user,
                             METAPLAYER_LIST_COL_HOST, pobserver->host,
                             METAPLAYER_LIST_COL_TYPE, pobserver->type, -1);
        }
      }
    }
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void
meta_player_tree_store_append_global_observers(struct server *pserver)
{
  struct server::server_players *pobserver;
  GtkTreeIter parent, iter;
  int i;
  bool first = TRUE;

  for (i = 0, pobserver = pserver->players; i < pserver->nplayers;
       i++, pobserver++) {
    if (0 == mystrcasecmp("Observer", pobserver->type)
        && 0 == mystrncasecmp("*global", pobserver->name, 7)) {
      if (first) {
        gtk_tree_store_append(meta_player_tree_store, &parent, NULL);
        gtk_tree_store_set(meta_player_tree_store, &parent,
                           METAPLAYER_LIST_COL_NAME, "Global observers", -1);
        first = FALSE;
      }

      gtk_tree_store_append(meta_player_tree_store, &iter, &parent);
      gtk_tree_store_set(meta_player_tree_store, &iter,
                         METAPLAYER_LIST_COL_USER, pobserver->user,
                         METAPLAYER_LIST_COL_HOST, pobserver->host,
                         METAPLAYER_LIST_COL_TYPE, pobserver->type, -1);
    }
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void meta_player_tree_store_append_detached_conn(struct server *pserver)
{
  struct server::server_players *pdetached;
  GtkTreeIter parent, iter;
  int i;
  bool first = TRUE;

  for (i = 0, pdetached = pserver->players; i < pserver->nplayers;
       i++, pdetached++) {
    if (0 == mystrcasecmp("Detached", pdetached->type)) {
      if (first) {
        gtk_tree_store_append(meta_player_tree_store, &parent, NULL);
        gtk_tree_store_set(meta_player_tree_store, &parent,
                           METAPLAYER_LIST_COL_NAME, "Detached", -1);
        first = FALSE;
      }

      gtk_tree_store_append(meta_player_tree_store, &iter, &parent);
      gtk_tree_store_set(meta_player_tree_store, &iter,
                         METAPLAYER_LIST_COL_USER, pdetached->user,
                         METAPLAYER_LIST_COL_HOST, pdetached->host,
                         METAPLAYER_LIST_COL_TYPE, pdetached->type, -1);
    }
  }
}

/**************************************************************************
  update the player list for the server given by path
**************************************************************************/
static void update_metaplayerlist(GtkTreePath * path)
{
  struct server *pserver;
  int i;

  gtk_tree_store_clear(meta_player_tree_store);

  if (!internet_server_list || !path) {
    return;
  }

  i = gtk_tree_path_get_indices(path)[0];
  pserver = server_list_get(internet_server_list, i);

  if (!pserver) {
    return;
  }

  meta_player_tree_store_append(pserver, "Human");
  meta_player_tree_store_append(pserver, "A.I.");
  meta_player_tree_store_append(pserver, "Dead");
  meta_player_tree_store_append(pserver, "Barbarian");
  meta_player_tree_store_append_global_observers(pserver);
  meta_player_tree_store_append_detached_conn(pserver);
}

/**************************************************************************
  update the variable list for the server given by path
**************************************************************************/
static void update_variableslist(GtkTreePath * path)
{
  int i;
  struct server *pserver;
  GtkTreeIter iter;

  gtk_list_store_clear(variables_store);

  if (!internet_server_list || !path) {
    return;
  }

  i = gtk_tree_path_get_indices(path)[0];
  pserver = server_list_get(internet_server_list, i);

  if (!pserver) {
    return;
  }

  for (i = 0; i < pserver->nvars; i++) {
    gtk_list_store_append(variables_store, &iter);
    gtk_list_store_set(variables_store, &iter,
                       MVAR_COL_NAME, pserver->vars[i].name,
                       MVAR_COL_VALUE, pserver->vars[i].value, -1);
  }
}

/**************************************************************************
  sets the host, port of the selected server.
**************************************************************************/
static void network_list_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  char *host, *port;

  if (!gtk_tree_selection_get_selected(select, &model, &it)) {
    return;
  }

  /* only one server can be selected in either list. */
  if (select == meta_selection) {
    GtkTreePath *path;
    gtk_tree_selection_unselect_all(lan_selection);
    path = gtk_tree_model_get_path(model, &it);
    update_metaplayerlist(path);
    update_variableslist(path);
    gtk_tree_path_free(path);
  } else {
    gtk_tree_selection_unselect_all(meta_selection);
    update_metaplayerlist(NULL);
    update_variableslist(NULL);
  }

  gtk_tree_model_get(model, &it, 0, &host, 1, &port, -1);

  gtk_entry_set_text(GTK_ENTRY(network_host), host);
  gtk_entry_set_text(GTK_ENTRY(network_port), port);
}

/**************************************************************************
  update the network page.
**************************************************************************/
static void update_network_page(void)
{
  char buf[256];

  gtk_tree_selection_unselect_all(lan_selection);
  gtk_tree_selection_unselect_all(meta_selection);

  gtk_entry_set_text(GTK_ENTRY(network_login), default_user_name);
  gtk_entry_set_text(GTK_ENTRY(network_host), server_host);
  my_snprintf(buf, sizeof(buf), "%d", server_port);
  gtk_entry_set_text(GTK_ENTRY(network_port), buf);

  set_connection_state(LOGIN_TYPE);
}

/**************************************************************************
  create the player list widget (scroll window, view and model)
**************************************************************************/
static GtkWidget *create_metaplayerlist_view()
{
  GtkWidget *view, *sw;
  GtkCellRenderer *rend;
  GtkTreeSelection *selection;

  meta_player_tree_store = gtk_tree_store_new(METAPLAYER_LIST_NUM_COLUMNS,
                                              G_TYPE_STRING,    /* name */
                                              G_TYPE_STRING,    /* user */
                                              G_TYPE_STRING,    /* host */
                                              G_TYPE_STRING,    /* type */
                                              G_TYPE_STRING);   /* nation */

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(meta_player_tree_store));
  g_object_unref(meta_player_tree_store);
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1,
                                              _("Name"), rend,
                                              "text",
                                              METAPLAYER_LIST_COL_NAME,
                                              NULL);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1,
                                              _("User"), rend,
                                              "text",
                                              METAPLAYER_LIST_COL_USER,
                                              NULL);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1,
                                              _("Host"), rend,
                                              "text",
                                              METAPLAYER_LIST_COL_HOST, NULL);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1,
                                              _("Type"), rend,
                                              "text",
                                              METAPLAYER_LIST_COL_TYPE, NULL);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1,
                                              _("Nation"), rend,
                                              "text",
                                              METAPLAYER_LIST_COL_NATION, NULL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(sw), 0);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), view);

  return sw;
}

/**************************************************************************
  create the variable list widget (scroll window, view and model)
**************************************************************************/
static GtkWidget *create_variables_view()
{
  GtkWidget *view, *sw;
  GtkCellRenderer *rend;
  GtkTreeSelection *selection;

  variables_store = gtk_list_store_new(MVAR_NUM_COLUMNS,
                                       G_TYPE_STRING,   /* setting */
                                       G_TYPE_STRING);  /* value */

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(variables_store));
  g_object_unref(variables_store);
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1,
                                              _("Setting"), rend,
                                              "text", MVAR_COL_NAME,
                                              NULL);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1,
                                              _("Value"), rend,
                                              "text", MVAR_COL_VALUE,
                                              NULL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(sw), 0);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), view);

  return sw;
}

/**************************************************************************
  create the network page.
**************************************************************************/
GtkWidget *create_network_page(void)
{
  GtkWidget *box, *sbox, *bbox, *notebook, *hbox;

  GtkWidget *button, *label, *view, *sw, *metaplayerlist, *variables;
  GtkCellRenderer *rend;
  GtkTreeSelection *selection;

  GtkWidget *table;

  static const char *titles[] = {
    N_("Server Name"),
    N_("Port"),
    N_("Version"),
    N_("Patches"),
    N_("Status"),
    N_("Players"),
    N_("Comment")
  };
  static bool titles_done;

  unsigned int i;

  intl_slist(ARRAY_SIZE(titles), titles, &titles_done);

  box = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(box), 4);

  notebook = gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(box), notebook, TRUE, TRUE, 8);

  /* LAN pane. */
  lan_store = gtk_list_store_new(7, G_TYPE_STRING, G_TYPE_STRING,
                                 G_TYPE_STRING, G_TYPE_STRING,
                                 G_TYPE_STRING, G_TYPE_INT,
                                 G_TYPE_STRING);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(lan_store));
  g_object_unref(lan_store);
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  lan_selection = selection;
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  g_signal_connect(view, "focus", G_CALLBACK(gtk_true), NULL);
  g_signal_connect(view, "row_activated",
                   G_CALLBACK(network_activate_callback), NULL);
  g_signal_connect(selection, "changed",
                   G_CALLBACK(network_list_callback), NULL);

  rend = gtk_cell_renderer_text_new();
  for (i = 0; i < ARRAY_SIZE(titles); i++) {
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                                -1, titles[i], rend, "text",
                                                i, NULL);
  }

  label = gtk_label_new_with_mnemonic(_("Local _Area Network"));

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(sw), 4);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw, label);


  /* Metaserver pane. */
  meta_store = gtk_list_store_new(7, G_TYPE_STRING, G_TYPE_STRING,
                                  G_TYPE_STRING, G_TYPE_STRING,
                                  G_TYPE_STRING, G_TYPE_INT,
                                  G_TYPE_STRING);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(meta_store));
  g_object_unref(meta_store);
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  meta_selection = selection;
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  g_signal_connect(view, "focus", G_CALLBACK(gtk_true), NULL);
  g_signal_connect(view, "row_activated",
                   G_CALLBACK(network_activate_callback), NULL);
  g_signal_connect(selection, "changed",
                   G_CALLBACK(network_list_callback), NULL);

  rend = gtk_cell_renderer_text_new();
  for (i = 0; i < ARRAY_SIZE(titles); i++) {
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                                -1, titles[i], rend, "text",
                                                i, NULL);
  }

  label = gtk_label_new_with_mnemonic(_("Internet _Metaserver"));

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(sw), 4);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_notebook_prepend_page(GTK_NOTEBOOK(notebook), sw, label);

  /* Bottom part of the page, outside the inner notebook. */
  sbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), sbox, FALSE, FALSE, 8);

  hbox = gtk_hbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(sbox), hbox, FALSE, FALSE, 8);

  table = gtk_table_new(6, 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 2);
  gtk_table_set_col_spacings(GTK_TABLE(table), 12);
  gtk_table_set_row_spacing(GTK_TABLE(table), 2, 12);
  gtk_widget_set_size_request(table, 340, -1);
  gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 4);

  network_host = gtk_entry_new();
  g_signal_connect((GtkWidget*)network_host, "activate",
      G_CALLBACK(connect_callback), NULL);
  gtk_table_attach(GTK_TABLE(table), network_host, 1, 2, 0, 1,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   0, 0);

  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", network_host,
                       "label", _("_Host:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  network_host_label = label;
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);

  network_port = gtk_entry_new();
  g_signal_connect(network_port, "activate",
      G_CALLBACK(connect_callback), NULL);
  gtk_table_attach(GTK_TABLE(table), network_port, 1, 2, 1, 2,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   0, 0);

  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", network_port,
                       "label", _("_Port:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  network_port_label = label;
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                   GTK_FILL, GTK_FILL, 0, 0);

  network_login = gtk_entry_new();
  g_signal_connect(network_login, "activate",
      G_CALLBACK(connect_callback), NULL);
  gtk_table_attach(GTK_TABLE(table), network_login, 1, 2, 3, 4,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   0, 0);

  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", network_login,
                       "label", _("_Login:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  network_login_label = label;
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
                   GTK_FILL, GTK_FILL, 0, 0);

  network_password = gtk_entry_new();
  g_signal_connect(network_password, "activate",
      G_CALLBACK(connect_callback), NULL);
  gtk_entry_set_visibility(GTK_ENTRY(network_password), FALSE);
  gtk_table_attach(GTK_TABLE(table), network_password, 1, 2, 4, 5,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);

  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", network_password,
                       "label", _("Pass_word:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  network_password_label = label;
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
                   GTK_FILL, GTK_FILL, 0, 0);

  network_confirm_password = gtk_entry_new();
  g_signal_connect(network_confirm_password, "activate",
      G_CALLBACK(connect_callback), NULL);
  gtk_entry_set_visibility(GTK_ENTRY(network_confirm_password), FALSE);
  gtk_table_attach(GTK_TABLE(table), network_confirm_password, 1, 2, 5, 6,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);

  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", network_confirm_password,
                       "label", _("Conf_irm Password:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  network_confirm_password_label = label;
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6,
                   GTK_FILL, GTK_FILL, 0, 0);

  variables = create_variables_view();
  gtk_widget_set_size_request(variables, 200, -1);
  gtk_box_pack_start(GTK_BOX(hbox), variables, FALSE, TRUE, 4);

  metaplayerlist = create_metaplayerlist_view();
  gtk_box_pack_start(GTK_BOX(hbox), metaplayerlist, TRUE, TRUE, 4);


  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(bbox), 12);
  gtk_box_pack_start(GTK_BOX(sbox), bbox, FALSE, FALSE, 2);

  button = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(bbox), button, TRUE);
  g_signal_connect(button, "clicked",
      G_CALLBACK(update_network_lists), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked", G_CALLBACK(main_callback), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked", G_CALLBACK(connect_callback), NULL);

  return box;
}


/**************************************************************************
                                  START PAGE
**************************************************************************/
static GtkWidget *start_aifill_spin;

/**************************************************************************
  request the game options dialog.
**************************************************************************/
static void game_options_callback(GtkWidget *w, gpointer data)
{
  send_report_request(REPORT_SERVER_OPTIONS2);
}

/**************************************************************************
  ...
**************************************************************************/
static void configure_style_callback(GtkWidget * w, gpointer data)
{
  popup_style_config_dialog();
}

/**************************************************************************
  AI skill setting callback.
**************************************************************************/
static void ai_skill_callback(GtkWidget *w, gpointer data)
{
  const char *name;
  char buf[512];

  name = skill_level_names[GPOINTER_TO_UINT(data)];

  my_snprintf(buf, sizeof(buf), "/%s", name);
  dsend_packet_chat_msg_req(&aconnection, buf);
}

/**************************************************************************
  AI fill setting callback.
**************************************************************************/
static void ai_fill_callback(GtkWidget *w, gpointer data)
{
  char buf[512];

  if (!is_server_running() || !can_client_access_hack()) {
    return;
  }

  my_snprintf(buf, sizeof(buf), "/set aifill %d",
              gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w)));
  dsend_packet_chat_msg_req(&aconnection, buf);
}

/**************************************************************************
  Clear logs callback.
**************************************************************************/
static void start_clear_callback(GtkWidget *w, gpointer data)
{
  clear_output_window();
}

/**************************************************************************
  Export logs callback.
**************************************************************************/
static void start_export_callback(GtkWidget *w, gpointer data)
{
  log_output_window();
}

/**************************************************************************
  start game callback.
**************************************************************************/
static void start_start_callback(GtkWidget *w, gpointer data)
{
  really_close_connection_dialog();
  dsend_packet_chat_msg_req(&aconnection, "/start");
}

/**************************************************************************
  update the start page.
**************************************************************************/
static void update_start_page(void)
{
  /* Default to aifill 5. */
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(start_aifill_spin), 5);
}

/**************************************************************************
  create start page.
**************************************************************************/
GtkWidget *create_start_page(void)
{
  GtkWidget *box, *sbox, *bbox, *table, *entry, *align, *vbox, *vbox2;

  GtkWidget *view, *sw, *text, *button, *spin, *option;
  GtkWidget *label, *menu, *item;
  GtkCellRenderer *rend;

  int i;

  box = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(box), 4);

  sbox = gtk_hbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(box), sbox, FALSE, FALSE, 0);

  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width(GTK_CONTAINER(align), 12);
  gtk_box_pack_start(GTK_BOX(sbox), align, FALSE, FALSE, 0);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(align), vbox);

  table = gtk_table_new(2, 2, FALSE);
  start_options_table = table;
  gtk_table_set_row_spacings(GTK_TABLE(table), 2);
  gtk_table_set_col_spacings(GTK_TABLE(table), 12);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

  spin = gtk_spin_button_new_with_range(1, MAX_NUM_PLAYERS, 1);
  start_aifill_spin = spin;
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), 0);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin),
                                    GTK_UPDATE_IF_VALID);
  g_signal_connect_after(spin, "value_changed",
                         G_CALLBACK(ai_fill_callback), NULL);

  gtk_table_attach_defaults(GTK_TABLE(table), spin, 1, 2, 0, 1);

  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", spin,
                       "label", _("Number of Players (including AI):"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

  option = gtk_option_menu_new();

  menu = gtk_menu_new();
  for (i = 0; i < NUM_SKILL_LEVELS; i++) {
    item = gtk_menu_item_new_with_label(_(skill_level_names[i]));
    g_signal_connect(item, "activate",
        G_CALLBACK(ai_skill_callback), GUINT_TO_POINTER(i));

    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
  gtk_table_attach_defaults(GTK_TABLE(table), option, 1, 2, 1, 2);

  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", option,
                       "label", _("AI Skill Level:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

  vbox2 = gtk_vbox_new(FALSE, 2);
  button = gtk_stockbutton_new(GTK_STOCK_PROPERTIES,
      _("More Game _Options..."));
  g_signal_connect(button, "clicked",
      G_CALLBACK(game_options_callback), NULL);
  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 8);

  button = gtk_stockbutton_new(GTK_STOCK_SELECT_FONT,
                               _("Configure _Fonts"));
  g_signal_connect(button, "clicked",
                   G_CALLBACK(configure_style_callback), NULL);
  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);

  button = gtk_stockbutton_new(GTK_STOCK_PREFERENCES, _("_Local Options"));
  g_signal_connect(button, "clicked", G_CALLBACK(popup_option_dialog), NULL);
  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);

  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), vbox2);
  gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, FALSE, 8);

  conn_model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(conn_model));
  g_object_unref(conn_model);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  rend = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1, NULL, rend, "text", 0,
                                              NULL);

  g_signal_connect(view, "button-press-event",
                   G_CALLBACK(show_conn_popup), NULL);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_widget_set_size_request(sw, -1, 200);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_box_pack_start(GTK_BOX(sbox), sw, TRUE, TRUE, 0);


  /* Lower area (chatline, entry, buttons) */

  vbox = gtk_vbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(box), vbox, TRUE, TRUE, 4);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

  text = gtk_text_view_new_with_buffer(message_buffer);
  start_message_area = text;
  gtk_widget_set_name(text, "chatline");
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 5);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_container_add(GTK_CONTAINER(sw), text);


  /* Vote widgets. */

  if (pregame_votebar == NULL) {
    pregame_votebar = create_voteinfo_bar();
  }
  gtk_box_pack_start(GTK_BOX(vbox), pregame_votebar->box,
                     FALSE, FALSE, 0);


  /* Chat entry and buttons. */

  sbox = gtk_hbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(box), sbox, FALSE, FALSE, 0);

  entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(sbox), entry, TRUE, TRUE, 0);
  g_signal_connect(entry, "key_press_event",
      G_CALLBACK(inputline_handler), NULL);
  g_signal_connect(entry, "activate", G_CALLBACK(inputline_return), NULL);
  start_page_entry = entry;

  bbox = gtk_hbutton_box_new();
  gtk_box_set_spacing(GTK_BOX(bbox), 12);
  gtk_box_pack_start(GTK_BOX(sbox), bbox, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  g_signal_connect(button, "clicked", G_CALLBACK(start_clear_callback), NULL);

  button = gtk_stockbutton_new(GTK_STOCK_FILE, _("_Export"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  g_signal_connect(button, "clicked", G_CALLBACK(start_export_callback), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked", G_CALLBACK(main_callback), NULL);

  button = gtk_stockbutton_new(GTK_STOCK_EXECUTE, _("_Start"));
  g_signal_connect(button, "clicked",
      G_CALLBACK(start_start_callback), NULL);
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  gtk_container_add(GTK_CONTAINER(bbox), button);

  return box;
}


/**************************************************************************
  this regenerates the player information from a loaded game on the server.
**************************************************************************/
void handle_game_load(struct packet_game_load *packet) /* 111 sc */
{
# if REPLAY
  int i;
  printf("GAME_LOAD\n"); /* done */
  printf("load_successful=%d ", packet->load_successful);
  printf("nplayers=%d ", packet->nplayers);
  printf("load_filename=%s ", packet->load_filename);
  printf("name[%d]={", MAX_NUM_PLAYERS);
  for (i = 0; i < MAX_NUM_PLAYERS; i++) {
    printf("%s ", packet->name[i]);
  }
  printf("}\n");
  printf("username[%d]={", MAX_NUM_PLAYERS);
  for (i = 0; i < MAX_NUM_PLAYERS; i++) {
    printf("%s ", packet->username[i]);
  }
  printf("}\n");
  printf("nation_name[%d]={", MAX_NUM_PLAYERS);
  for (i = 0; i < MAX_NUM_PLAYERS; i++) {
    printf("%s ", packet->nation_name[i]);
  }
  printf("}\n");
  printf("nation_flag[%d]={", MAX_NUM_PLAYERS);
  for (i = 0; i < MAX_NUM_PLAYERS; i++) {
    printf("%s ", packet->nation_flag[i]);
  }
  printf("}\n");
  printf("is_alive[%d]={", MAX_NUM_PLAYERS);
  for (i = 0; i < MAX_NUM_PLAYERS; i++) {
    printf("%d ", packet->is_alive[i]);
  }
  printf("}\n");
  printf("is_ai[%d]={", MAX_NUM_PLAYERS);
  for (i = 0; i < MAX_NUM_PLAYERS; i++) {
    printf("%d ", packet->is_ai[i]);
  }
  printf("}\n");
# endif

  /* NB Only put up the nation page for local game/scenario loads.
   * This is to avoid the terrible confusion this page causes for
   * reloaded network games. */
  if (can_client_access_hack() && packet->load_successful) {
    update_nation_page(packet);

    set_client_page(PAGE_NATION);
  }
}

/**************************************************************************
  loads the currently selected game.
**************************************************************************/
static void load_callback(void)
{
  GtkTreeIter it;
  char *filename;

  if (!gtk_tree_selection_get_selected(load_selection, NULL, &it)) {
    return;
  }

  gtk_tree_model_get(GTK_TREE_MODEL(load_store), &it, 1, &filename, -1);

  if (is_server_running()) {
    char message[MAX_LEN_MSG];

    my_snprintf(message, sizeof(message), "/load %s", filename);
    dsend_packet_chat_msg_req(&aconnection, message);
  }
}

/**************************************************************************
  call the default GTK+ requester for saved game loading.
**************************************************************************/
static void load_browse_callback(GtkWidget *w, gpointer data)
{
  create_file_selection(_("Choose Saved Game to Load"), FALSE);
}

/**************************************************************************
  update the saved games list store.
**************************************************************************/
static void update_saves_store(GtkListStore *store)
{
  struct datafile_list *files;

  gtk_list_store_clear(store);

  /* search for user saved games. */
  files = datafilelist_infix("saves", ".sav", FALSE);
  datafile_list_iterate(files, pfile) {
    GtkTreeIter it;
    gtk_list_store_append(store, &it);
    gtk_list_store_set(store, &it, 0, pfile->name, 1, pfile->fullname, -1);
  } datafile_list_iterate_end;
  free_datafile_list(files);

  files = datafilelist_infix(NULL, ".sav", FALSE);
  datafile_list_iterate(files, pfile) {
    GtkTreeIter it;
    gtk_list_store_append(store, &it);
    gtk_list_store_set(store, &it, 0, pfile->name, 1, pfile->fullname, -1);
  } datafile_list_iterate_end;
  free_datafile_list(files);
}

/**************************************************************************
 update the load page.
**************************************************************************/
static void update_load_page(void)
{
  update_saves_store(load_store);
}

/**************************************************************************
  create the load page.
**************************************************************************/
GtkWidget *create_load_page(void)
{
  GtkWidget *align, *box, *sbox, *bbox;

  GtkWidget *button, *label, *view, *sw;
  GtkCellRenderer *rend;

  box = gtk_vbox_new(FALSE, 18);
  gtk_container_set_border_width(GTK_CONTAINER(box), 4);

  align = gtk_alignment_new(0.5, 0.5, 0.0, 1.0);
  gtk_box_pack_start(GTK_BOX(box), align, TRUE, TRUE, 0);

  load_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(load_store));
  g_object_unref(load_store);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1, NULL, rend, "text", 0,
                                              NULL);

  load_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  gtk_tree_selection_set_mode(load_selection, GTK_SELECTION_SINGLE);

  g_signal_connect(view, "row_activated", G_CALLBACK(load_callback), NULL);

  sbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(align), sbox);

  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", view,
                       "label", _("Choose Saved Game to _Load:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_box_pack_start(GTK_BOX(sbox), label, FALSE, FALSE, 0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(sw, 300, -1);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_box_pack_start(GTK_BOX(sbox), sw, TRUE, TRUE, 0);

  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(bbox), 12);
  gtk_box_pack_start(GTK_BOX(box), bbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_mnemonic(_("_Browse..."));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(bbox), button, TRUE);
  g_signal_connect(button, "clicked",
      G_CALLBACK(load_browse_callback), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked", G_CALLBACK(main_callback), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked", G_CALLBACK(load_callback), NULL);

  return box;
}


/**************************************************************************
  loads the currently selected scenario.
**************************************************************************/
static void scenario_callback(void)
{
  GtkTreeIter it;
  char *filename;

  if (!gtk_tree_selection_get_selected(scenario_selection, NULL, &it)) {
    return;
  }

  gtk_tree_model_get(GTK_TREE_MODEL(scenario_store), &it, 1, &filename, -1);

  if (is_server_running()) {
    char message[MAX_LEN_MSG];

    my_snprintf(message, sizeof(message), "/load %s", filename);
    dsend_packet_chat_msg_req(&aconnection, message);
  }
}

/**************************************************************************
  call the default GTK+ requester for scenario loading.
**************************************************************************/
static void scenario_browse_callback(GtkWidget *w, gpointer data)
{
  create_file_selection(_("Choose a Scenario"), FALSE);
}

/**************************************************************************
  update the scenario page.
**************************************************************************/
static void update_scenario_page(void)
{
  struct datafile_list *files;

  gtk_list_store_clear(scenario_store);

  /* search for scenario files. */
  files = datafilelist_infix("scenario", ".sav", TRUE);
  datafile_list_iterate(files, pfile) {
    GtkTreeIter it;

    gtk_list_store_append(scenario_store, &it);
    gtk_list_store_set(scenario_store, &it,
        0, pfile->name, 1, pfile->fullname, -1);
  } datafile_list_iterate_end;
  free_datafile_list(files);
}

/**************************************************************************
  create the scenario page.
**************************************************************************/
GtkWidget *create_scenario_page(void)
{
  GtkWidget *align, *box, *sbox, *bbox;

  GtkWidget *button, *label, *view, *sw;
  GtkCellRenderer *rend;

  box = gtk_vbox_new(FALSE, 18);
  gtk_container_set_border_width(GTK_CONTAINER(box), 4);

  align = gtk_alignment_new(0.5, 0.5, 0.0, 1.0);
  gtk_box_pack_start(GTK_BOX(box), align, TRUE, TRUE, 0);

  scenario_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(scenario_store));
  g_object_unref(scenario_store);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1, NULL, rend, "text", 0,
                                              NULL);

  scenario_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  gtk_tree_selection_set_mode(scenario_selection, GTK_SELECTION_SINGLE);

  g_signal_connect(view, "row_activated",
                   G_CALLBACK(scenario_callback), NULL);

  sbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(align), sbox);

  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", view,
                       "label", _("Choose a _Scenario:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_box_pack_start(GTK_BOX(sbox), label, FALSE, FALSE, 0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(sw, 300, -1);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_box_pack_start(GTK_BOX(sbox), sw, TRUE, TRUE, 0);

  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(bbox), 12);
  gtk_box_pack_start(GTK_BOX(box), bbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_mnemonic(_("_Browse..."));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(bbox), button, TRUE);
  g_signal_connect(button, "clicked",
      G_CALLBACK(scenario_browse_callback), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked", G_CALLBACK(main_callback), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked", G_CALLBACK(scenario_callback), NULL);

  return box;
}


/**************************************************************************
  change the player name to that of the nation's leader and start the game.
**************************************************************************/
static void nation_start_callback(void)
{
  GtkTreeIter it;
  char *name;

  if (!gtk_tree_selection_get_selected(nation_selection, NULL, &it)) {
    return;
  }

  gtk_tree_model_get(GTK_TREE_MODEL(nation_store), &it, 0, &name, -1);
  sz_strlcpy(player_name, name);

  send_start_saved_game();
}

/**************************************************************************
 ....
**************************************************************************/
static GdkPixbuf *get_flag(char *flag_str)
{
  GdkPixbuf *img;
  wc_Sprite *Sprite3;
  wc_Sprite *s;

  s = load_sprite(flag_str);

  if (!s) {
    return NULL;
  }

  Sprite3 = crop_blankspace(s);
  img = gdk_pixbuf_new_from_sprite(Sprite3);
  free_sprite(Sprite3);

  /* Will call free_sprite(s) if needed. */
  unload_sprite(flag_str);

  return img;
}

/**************************************************************************
...
**************************************************************************/
static void update_nation_page(struct packet_game_load *packet)
{
  int i;

  game.info.nplayers = packet->nplayers;

  gtk_list_store_clear(nation_store);

  for (i = 0; i < packet->nplayers; i++) {
    GtkTreeIter iter;
    GdkPixbuf *flag;

    gtk_list_store_append(nation_store, &iter);
    gtk_list_store_set(nation_store, &iter,
        0, packet->name[i],
        2, packet->nation_name[i],
        3, packet->is_alive[i] ? _("Alive") : _("Dead"),
        4, packet->is_ai[i] ? _("AI") : _("Human"), -1);

    /* set flag if we've got one to set. */
    if (strcmp(packet->nation_flag[i], "") != 0) {
      flag = get_flag(packet->nation_flag[i]);
      if (flag) {
        gtk_list_store_set(nation_store, &iter, 1, flag, -1);
        g_object_unref(flag);
      }
    }
  }

  /* if nplayers is zero, we suppose it's a scenario */
  if (packet->nplayers == 0) {
    GtkTreeIter iter;
    char message[MAX_LEN_MSG];

    my_snprintf(message, sizeof(message), "/create %s", default_user_name);
    dsend_packet_chat_msg_req(&aconnection, message);
    my_snprintf(message, sizeof(message), "/ai %s", default_user_name);
    dsend_packet_chat_msg_req(&aconnection, message);
    my_snprintf(message, sizeof(message), "/take \"%s\"", default_user_name);
    dsend_packet_chat_msg_req(&aconnection, message);

    /* create a false entry */
    gtk_list_store_append(nation_store, &iter);
    gtk_list_store_set(nation_store, &iter,
                       0, default_user_name, 3, _("Alive"), 4, _("Human"), -1);
  }
}

/**************************************************************************
  create the nation page.
**************************************************************************/
GtkWidget *create_nation_page(void)
{
  GtkWidget *box, *sbox, *label, *view, *sw, *bbox, *button;
  GtkCellRenderer *trenderer, *prenderer;

  box = gtk_vbox_new(FALSE, 18);
  gtk_container_set_border_width(GTK_CONTAINER(box), 4);

  sbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(box), sbox);

  nation_store = gtk_list_store_new(5, G_TYPE_STRING, GDK_TYPE_PIXBUF,
                                    G_TYPE_STRING, G_TYPE_STRING,
                                    G_TYPE_STRING);

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(nation_store));
  nation_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  g_object_unref(nation_store);
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));

  trenderer = gtk_cell_renderer_text_new();
  prenderer = gtk_cell_renderer_pixbuf_new();

  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1, _("Name"), trenderer,
                                              "text", 0, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1,
                                              _("Flag"), prenderer, "pixbuf",
                                              1, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1,
                                              _("Nation"), trenderer, "text",
                                              2, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1,
                                              _("Status"), trenderer, "text",
                                              3, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1,
                                              _("Type"), trenderer, "text",
                                              4, NULL);

  gtk_tree_selection_set_mode(nation_selection, GTK_SELECTION_SINGLE);

  g_signal_connect(view, "row_activated",
                   G_CALLBACK(nation_start_callback), NULL);

  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", view,
                       "label", _("Choose a _nation to play:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_box_pack_start(GTK_BOX(sbox), label, FALSE, FALSE, 2);

  sw = gtk_scrolled_window_new(NULL,NULL);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(sbox), sw, TRUE, TRUE, 0);

  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(bbox), 12);
  gtk_box_pack_start(GTK_BOX(box), bbox, FALSE, FALSE, 0);

  button = gtk_stockbutton_new(GTK_STOCK_PREFERENCES, _("Game _Options..."));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(bbox), button, TRUE);
  g_signal_connect(button, "clicked",
      G_CALLBACK(game_options_callback), NULL);

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect(button, "clicked", G_CALLBACK(main_callback), NULL);

  button = gtk_stockbutton_new(GTK_STOCK_EXECUTE, _("_Start"));
  g_signal_connect(button, "clicked",
      G_CALLBACK(nation_start_callback), NULL);
  gtk_container_add(GTK_CONTAINER(bbox), button);

  return box;
}


/**************************************************************************
  changes the current page.
  this is basically a state machine. jumps actions are hardcoded.
**************************************************************************/
void set_client_page(enum client_pages new_page)
{
  enum client_pages old_page;
  int entry_pos = -1;

  old_page = current_page;

  /* If the page remains the same, don't do anything. */
  if (old_page == new_page) {
    return;
  }

  switch (old_page) {
  /* We aren't interested in showing a start page on game load or scenario. */
  case PAGE_SCENARIO:
  case PAGE_LOAD:
    if (new_page == PAGE_START) {
      return;
    }
    break;
  case PAGE_NETWORK:
    if (lan_timer != 0) {
      g_source_remove(lan_timer);
    }
    break;
  case PAGE_GAME:
    enable_menus(FALSE);
    break;
  default:
    break;
  }

  /* GUI changes to be done before the new page's
   * widgets are shown. */
  switch (new_page) {
  case PAGE_MAIN:
    break;
  case PAGE_START:
    if (GTK_WIDGET_HAS_FOCUS(inputline)) {
      entry_pos = gtk_editable_get_position(GTK_EDITABLE(inputline));
    }
    if (is_server_running()) {
      gtk_widget_show(start_options_table);
      update_start_page();
    } else {
      gtk_widget_hide(start_options_table);
    }
    break;
  case PAGE_NATION:
    break;
  case PAGE_GAME:
    if (GTK_WIDGET_HAS_FOCUS(start_page_entry)) {
      entry_pos = gtk_editable_get_position(GTK_EDITABLE(start_page_entry));
    }
    enable_menus(TRUE);
    break;
  case PAGE_LOAD:
    update_load_page();
    break;
  case PAGE_SCENARIO:
    update_scenario_page();
    break;
  case PAGE_NETWORK:
    update_network_page();
    break;
  }

  /* hide/show statusbar. */
  if (new_page == PAGE_START || new_page == PAGE_GAME) {
    gtk_widget_hide(statusbar_frame);
  } else {
    gtk_widget_show(statusbar_frame);
  }

  /* Now we actually switch the page. */
  gtk_notebook_set_current_page(GTK_NOTEBOOK(toplevel_tabs), new_page);
  current_page = new_page;

  /* We have to do this here since gtk_notebook_set_current_page
   * seems to do a show-all. */
  voteinfo_gui_update();

  /* Update the GUI. */
  while (gtk_events_pending()) {
    gtk_main_iteration();
  }

  /* GUI changes that are to be done after the
   * widgets for the new page have been shown. */
  switch (new_page) {
  case PAGE_MAIN:
    break;
  case PAGE_START:
    if (start_message_area) {
      gtk_widget_grab_focus(start_message_area);
    }
    if (old_page == PAGE_GAME && inputline && start_page_entry) {
      /* Copy the game page entry to the start page entry */
      gtk_entry_set_text(GTK_ENTRY(start_page_entry),
                         gtk_entry_get_text(GTK_ENTRY(inputline)));
      gtk_entry_set_text(GTK_ENTRY(inputline), "");
      if (entry_pos >= 0) {
        gtk_widget_grab_focus(start_page_entry);
        gtk_editable_set_position(GTK_EDITABLE(start_page_entry), entry_pos);
      }
    }
    chatline_scroll_to_bottom();
    allied_chat_only = FALSE;
    gtk_widget_grab_focus(start_page_entry);
    break;
  case PAGE_NATION:
    gtk_tree_view_focus(gtk_tree_selection_get_tree_view(nation_selection));
    break;
  case PAGE_LOAD:
    gtk_tree_view_focus(gtk_tree_selection_get_tree_view(load_selection));
    break;
  case PAGE_SCENARIO:
    gtk_tree_view_focus(gtk_tree_selection_get_tree_view
                        (scenario_selection));
    break;
  case PAGE_GAME:
    if (old_page == PAGE_START && inputline && start_page_entry) {
      /* Copy the game start entry to the start page entry */
      gtk_entry_set_text(GTK_ENTRY(inputline),
                         gtk_entry_get_text(GTK_ENTRY(start_page_entry)));
      gtk_entry_set_text(GTK_ENTRY(start_page_entry), "");
      if (entry_pos >= 0) {
        gtk_widget_grab_focus(inputline);
        gtk_editable_set_position(GTK_EDITABLE(inputline), entry_pos);
      }
    }
    chatline_scroll_to_bottom();
    init_chat_buttons();
    break;
  case PAGE_NETWORK:
    update_network_lists();
    gtk_widget_grab_focus(network_login);
    gtk_editable_set_position(GTK_EDITABLE(network_login), 0);
    break;
  }
}

/**************************************************************************
  Returns the current page.
**************************************************************************/
enum client_pages get_client_page(void)
{
  return current_page;
}


/**************************************************************************
...
**************************************************************************/
static void set_page_callback(GtkWidget *w, gpointer data)
{
  set_client_page((client_pages)GPOINTER_TO_UINT(data));
}

/**************************************************************************
                                SAVE GAME DIALOG
**************************************************************************/
static GtkWidget *save_dialog_shell, *save_entry;
static GtkListStore *save_store;
static GtkTreeSelection *save_selection;

enum {
  SAVE_BROWSE,
  SAVE_DELETE,
  SAVE_SAVE
};

/**************************************************************************
  update save dialog.
**************************************************************************/
static void update_save_dialog(void)
{
  update_saves_store(save_store);
}

/**************************************************************************
  handle save dialog response.
**************************************************************************/
static void save_response_callback(GtkWidget *w, gint arg)
{
  switch (arg) {
  case SAVE_BROWSE:
    create_file_selection(_("Select Location to Save"), TRUE);
    break;
  case SAVE_DELETE:
    {
      char *filename;
      GtkTreeIter it;

      if (!gtk_tree_selection_get_selected(save_selection, NULL, &it)) {
        return;
      }

      gtk_tree_model_get(GTK_TREE_MODEL(save_store), &it, 1, &filename, -1);
      remove(filename);
      update_save_dialog();
    }
    return;
  case SAVE_SAVE:
    {
      const char *text;
      char *filename;

      text = gtk_entry_get_text(GTK_ENTRY(save_entry));
      filename = g_filename_from_utf8(text, -1, NULL, NULL, NULL);
      send_save_game(filename);
      g_free(filename);
    }
    break;
  default:
    break;
  }
  gtk_widget_destroy(save_dialog_shell);
}

/**************************************************************************
  handle save list double click.
**************************************************************************/
static void save_row_callback(void)
{
  save_response_callback(NULL, SAVE_SAVE);
}

/**************************************************************************
  handle save filename entry activation.
**************************************************************************/
static void save_entry_callback(GtkEntry *w, gpointer data)
{
  save_response_callback(NULL, SAVE_SAVE);
}

/**************************************************************************
  handle save list selection change.
**************************************************************************/
static void save_list_callback(GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter it;
  char *name;

  if (!gtk_tree_selection_get_selected(select, &model, &it)) {
    gtk_dialog_set_response_sensitive(GTK_DIALOG(save_dialog_shell),
        SAVE_DELETE, FALSE);
    return;
  }

  gtk_dialog_set_response_sensitive(GTK_DIALOG(save_dialog_shell),
      SAVE_DELETE, TRUE);

  gtk_tree_model_get(model, &it, 0, &name, -1);
  gtk_entry_set_text(GTK_ENTRY(save_entry), name);
}

/**************************************************************************
  create save dialog.
**************************************************************************/
static void create_save_dialog(void)
{
  GtkWidget *shell;

  GtkWidget *sbox, *sw;

  GtkWidget *label, *view, *entry;
  GtkCellRenderer *rend;
  GtkTreeSelection *selection;


  shell = gtk_dialog_new_with_buttons(_("Save Game"),
                                      NULL,
                                      (GtkDialogFlags)0,
                                      _("_Browse..."),
                                      SAVE_BROWSE,
                                      GTK_STOCK_DELETE,
                                      SAVE_DELETE,
                                      GTK_STOCK_CANCEL,
                                      GTK_RESPONSE_CANCEL,
                                      GTK_STOCK_SAVE, SAVE_SAVE, NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(shell), GTK_RESPONSE_CANCEL);
  save_dialog_shell = shell;
  setup_dialog(shell, toplevel);

  save_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(save_store));
  g_object_unref(save_store);

  rend = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
                                              -1, NULL, rend, "text", 0,
                                              NULL);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  save_selection = selection;
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

  g_signal_connect(view, "row_activated",
                   G_CALLBACK(save_row_callback), NULL);
  g_signal_connect(selection, "changed",
                   G_CALLBACK(save_list_callback), NULL);

  sbox = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(shell)->vbox), sbox, TRUE, TRUE, 0);

  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", view,
                       "label", _("Saved _Games:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_box_pack_start(GTK_BOX(sbox), label, FALSE, FALSE, 0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(sw, 300, 300);
  gtk_container_add(GTK_CONTAINER(sw), view);
  gtk_box_pack_start(GTK_BOX(sbox), sw, TRUE, TRUE, 0);


  sbox = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(shell)->vbox), sbox, FALSE, FALSE, 12);

  entry = gtk_entry_new();
  save_entry = entry;
  g_signal_connect(entry, "activate", G_CALLBACK(save_entry_callback), NULL);

  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", entry,
                       "label", _("Save _Filename:"),
                       "xalign", 0.0, "yalign", 0.5, NULL);
  gtk_box_pack_start(GTK_BOX(sbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(sbox), entry, FALSE, FALSE, 0);


  g_signal_connect(shell, "response",
      G_CALLBACK(save_response_callback), NULL);
  g_signal_connect(shell, "destroy",
      G_CALLBACK(gtk_widget_destroyed), &save_dialog_shell);

  gtk_dialog_set_response_sensitive(GTK_DIALOG(save_dialog_shell),
      SAVE_DELETE, FALSE);

  gtk_window_set_focus(GTK_WINDOW(shell), entry);

  gtk_widget_show_all(GTK_DIALOG(shell)->vbox);
}

/**************************************************************************
  popup save dialog.
**************************************************************************/
void popup_save_dialog(void)
{
  if (!save_dialog_shell) {
    create_save_dialog();
  }
  update_save_dialog();

  gtk_window_present(GTK_WINDOW(save_dialog_shell));
}

/**************************************************************************
  ...
**************************************************************************/
void voteinfo_gui_update(void)
{
  int vote_count, index;
  struct voteinfo_bar *vib = NULL;
  struct voteinfo *vi = NULL;
  char buf[512], status[128], ordstr[128], color[32];
  bool running, need_scroll;
  gchar *escaped_desc, *escaped_user;

  if (get_client_page() == PAGE_START) {
    vib = pregame_votebar;
  } else if (get_client_page() == PAGE_GAME) {
    vib = ingame_votebar;
  }

  if (vib == NULL) {
    return;
  }

  if (voteinfo_queue == NULL) {
    return;
  }

  vote_count = voteinfo_list_size(voteinfo_queue);
  vi = voteinfo_queue_get_current(&index);

  if (!use_voteinfo_bar
      || (!always_show_votebar && (vote_count <= 0 || vi == NULL))
      || (!client_is_player() && do_not_show_votebar_if_not_player)) {
    gtk_widget_hide_all(vib->box);
    return;
  }

  if (vi != NULL && vi->resolved && vi->passed) {
    /* TRANS: Describing a vote that passed. */
    my_snprintf(status, sizeof(status), _("[passed]"));
    sz_strlcpy(color, "green");
  } else if (vi != NULL && vi->resolved && !vi->passed) {
    /* TRANS: Describing a vote that failed. */
    my_snprintf(status, sizeof(status), _("[failed]"));
    sz_strlcpy(color, "red");
  } else if (vi != NULL && vi->remove_time > 0) {
    /* TRANS: Describing a vote that was removed. */
    my_snprintf(status, sizeof(status), _("[removed]"));
    sz_strlcpy(color, "grey");
  } else {
    status[0] = '\0';
  }

  if (vote_count > 1) {
    my_snprintf(ordstr, sizeof(ordstr),
                "<span weight=\"bold\">(%d/%d)</span> ",
                index + 1, vote_count);
  } else {
    ordstr[0] = '\0';
  }

  if (status[0] != '\0') {
    my_snprintf(buf, sizeof(buf),
        "<span weight=\"bold\" background=\"%s\">%s</span> ",
        color, status);
    sz_strlcpy(status, buf);
  }

  if (vi != NULL) {
    escaped_desc = g_markup_escape_text(vi->desc, -1);
    escaped_user = g_markup_escape_text(vi->user, -1);
    if (vi->is_poll) {
      my_snprintf(buf, sizeof(buf), _("%sPoll by %s: %s%s"),
                  ordstr, escaped_user, status, escaped_desc);
    } else {
      my_snprintf(buf, sizeof(buf), _("%sVote %d by %s: %s%s"),
                  ordstr, vi->vote_no, escaped_user, status,
                  escaped_desc);
    }
    g_free(escaped_desc);
    g_free(escaped_user);
  } else {
    buf[0] = '\0';
  }
  gtk_label_set_markup(GTK_LABEL(vib->label), buf);

  if (vi != NULL)  {
    my_snprintf(buf, sizeof(buf), "%d", vi->yes);
    gtk_label_set_text(GTK_LABEL(vib->yes_count_label), buf);
    my_snprintf(buf, sizeof(buf), "%d", vi->no);
    gtk_label_set_text(GTK_LABEL(vib->no_count_label), buf);
    my_snprintf(buf, sizeof(buf), "%d", vi->abstain);
    gtk_label_set_text(GTK_LABEL(vib->abstain_count_label), buf);
    my_snprintf(buf, sizeof(buf), "/%d", vi->num_voters);
    gtk_label_set_text(GTK_LABEL(vib->voter_count_label), buf);
  } else {
    gtk_label_set_text(GTK_LABEL(vib->yes_count_label), "-");
    gtk_label_set_text(GTK_LABEL(vib->no_count_label), "-");
    gtk_label_set_text(GTK_LABEL(vib->abstain_count_label), "-");
    gtk_label_set_text(GTK_LABEL(vib->voter_count_label), "/-");
  }

  running = vi != NULL && !vi->resolved && vi->remove_time == 0;

  gtk_widget_set_sensitive(vib->yes_button, running);
  gtk_widget_set_sensitive(vib->no_button, running);
  gtk_widget_set_sensitive(vib->abstain_button, running);

  need_scroll = !GTK_WIDGET_VISIBLE(vib->box)
    && chatline_is_scrolled_to_bottom();

  gtk_widget_show_all(vib->box);

  if (vote_count <= 1) {
    gtk_widget_hide(vib->next_button);
  }

  if (need_scroll) {
    /* Showing the votebar when it was hidden
     * previously makes the chatline scroll up. */
    queue_chatline_scroll_to_bottom();
  }
}
