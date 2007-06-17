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
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "netintf.h"
#include "shared.h"
#include "support.h"
#include "version.h"

#include "chatline.h"
#include "civclient.h"
#include "climisc.h"
#include "clinet.h"
#include "colors.h"
#include "connectdlg_common.h"
#include "connectdlg.h"
#include "control.h"
#include "dialogs.h"
#include "gotodlg.h"
#include "graphics.h"
#include "gui_stuff.h"
#include "helpdata.h"           /* boot_help_texts() */
#include "inputdlg.h"
#include "mapctrl.h"
#include "mapview.h"
#include "menu.h"
#include "optiondlg.h"
#include "options.h"
#include "packhand_gen.h"
#include "spaceshipdlg.h"
#include "tilespec.h"


#include "gui_main.h"

static enum {
  LOGIN_TYPE, 
  NEW_PASSWORD_TYPE, 
  VERIFY_PASSWORD_TYPE,
  ENTER_PASSWORD_TYPE
} dialog_config;

static HWND connect_dlg;
static HWND start_dlg;
static HWND newgame_dlg;
static HWND players_dlg;
static HWND players_listview;
static HWND network_dlg;
static HWND network_tabs[3];
static HWND tab_ctrl;
static HWND networkdlg_msg;
static HWND networkdlg_name;
static HWND server_listview;
static HWND lan_listview;

static int autoconnect_timer_id;
struct t_server_button {
  HWND button;
  char *button_string;
  char *command;
};

enum new_game_dlg_ids {
  ID_NAME=100,
  ID_NEWGAMEDLG_AISKILL,
  ID_NEWGAMEDLG_AIFILL,
  ID_NEWGAMEDLG_OPTIONS,
  ID_OK=IDOK,
  ID_CANCEL=IDCANCEL
};

static char saved_games_dirname[MAX_PATH+1]=".";

static void load_game_callback(void);

static int get_lanservers(HWND list);

static int num_lanservers_timer = 0;

extern void socket_timer(void);

/*************************************************************************
 configure the dialog depending on what type of authentication request the
 server is making.
*************************************************************************/
void handle_authentication_req(enum authentication_type type, char *message)
{
  SetFocus(GetDlgItem(network_tabs[0], ID_CONNECTDLG_NAME));
  SetWindowText(GetDlgItem(network_tabs[0], ID_CONNECTDLG_NAME), "");
  EnableWindow(GetDlgItem(network_dlg, ID_CONNECTDLG_CONNECT), TRUE);
  SetWindowText(networkdlg_msg, message);

  switch (type) {
  case AUTH_NEWUSER_FIRST:
    dialog_config = NEW_PASSWORD_TYPE;
    break;
  case AUTH_NEWUSER_RETRY:
    dialog_config = NEW_PASSWORD_TYPE;
    break;
  case AUTH_LOGIN_FIRST:
    /* if we magically have a password already present in 'password'
     * then, use that and skip the password entry dialog */
    if (password[0] != '\0') {
      struct packet_authentication_reply reply;

      sz_strlcpy(reply.password, password);
      send_packet_authentication_reply(&aconnection, &reply);
      return;
    } else {
      dialog_config = ENTER_PASSWORD_TYPE;
    }
    break;
  case AUTH_LOGIN_RETRY:
    dialog_config = ENTER_PASSWORD_TYPE;
    break;
  default:
    assert(0);
  }

  SetWindowText(networkdlg_name, _("Password:"));
  Edit_SetPasswordChar(GetDlgItem(network_tabs[0], ID_CONNECTDLG_NAME), '*');
  fcwin_redo_layout(network_dlg);
  InvalidateRect(network_dlg, NULL, TRUE);
}

/**************************************************************************
  this regenerates the player information from a loaded game on the server.
**************************************************************************/
void handle_game_load(struct packet_game_load *packet)
{ 
  char *row[3];
  int i;

  if (!connect_dlg) {
    /* Create the window if it isn't there, in case a foreign server
     * loads a game */
    gui_server_connect();
  }

  /* we couldn't load the savegame, we could have gotten the name wrong, etc */
  if (!packet->load_successful) {
    SetWindowText(connect_dlg, _("Couldn't load the savegame"));
    return;
  } else {
    char *buf = strrchr(packet->load_filename, '/');

    if (buf == NULL) {
      buf = packet->load_filename;
    } else {
      buf++;
    }
    SetWindowText(connect_dlg, buf);
  }

  ShowWindow(start_dlg, SW_HIDE);
  ShowWindow(players_dlg, SW_SHOWNORMAL);

  game.nplayers = packet->nplayers;
  ListView_DeleteAllItems(players_listview);

  for (i = 0; i < packet->nplayers; i++) {
    row[0] = packet->name[i];
    row[1] = packet->nation_name[i];
    row[2] = packet->is_alive[i] ? _("Alive") : _("Dead");
    row[3] = packet->is_ai[i] ? _("AI") : _("Human");
    fcwin_listview_add_row(players_listview, 0, 4, row);
  }

  /* if nplayers is zero, we suppose it's a scenario */
  if (packet->load_successful && packet->nplayers == 0) {
    char message[MAX_LEN_MSG];

    my_snprintf(message, sizeof(message), "/create %s", user_name);
    send_chat(message);
    my_snprintf(message, sizeof(message), "/ai %s", user_name);
    send_chat(message);
    my_snprintf(message, sizeof(message), "/take %s", user_name);
    send_chat(message);

    /* create a false entry */
    row[0] = user_name;
    row[1] = "";
    row[2] = _("Alive");
    row[3] = _("Human");
    fcwin_listview_add_row(players_listview, 0, 4, row);
  }

  ListView_SetColumnWidth(players_listview, 0, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(players_listview, 1, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(players_listview, 2, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(players_listview, 3, LVSCW_AUTOSIZE);
}

/**************************************************************************
 really close and destroy the dialog.
**************************************************************************/
void really_close_connection_dialog(void)
{
  if (connect_dlg) {
    DestroyWindow(connect_dlg);
    connect_dlg = NULL;
  }
}

/*************************************************************************
 close and destroy the dialog but only if we don't have a local
 server running (that we started).
*************************************************************************/
void close_connection_dialog()
{
  if (!is_server_running()) {
    really_close_connection_dialog();
  }
}

/**************************************************************************
 if on the metaserver page, switch page to the login page (with new server
 and port). if on the login page, send connect and/or authentication 
 requests to the server.
**************************************************************************/
static void connect_callback()
{
  char errbuf[512];
  char portbuf[10];
  struct packet_authentication_reply reply;

  if (TabCtrl_GetCurSel(tab_ctrl) != 0) {
    TabCtrl_SetCurSel(tab_ctrl, 0);
    ShowWindow(network_tabs[0], SW_SHOWNORMAL);
    ShowWindow(network_tabs[1], SW_HIDE);
    ShowWindow(network_tabs[2], SW_HIDE);
    return;
  }

  switch (dialog_config) {
  case LOGIN_TYPE:
    Edit_GetText(GetDlgItem(network_tabs[0], ID_CONNECTDLG_NAME),
		 user_name, 512);
    Edit_GetText(GetDlgItem(network_tabs[0], ID_CONNECTDLG_HOST),
		 server_host, 512);
    Edit_GetText(GetDlgItem(network_tabs[0], ID_CONNECTDLG_PORT),
		 portbuf, 10);
    server_port = atoi(portbuf);
  
    if (connect_to_server(user_name, server_host, server_port,
                          errbuf, sizeof(errbuf)) != -1) {
    } else {
      append_output_window(errbuf);
    }

    break; 
  case NEW_PASSWORD_TYPE:
    Edit_GetText(GetDlgItem(network_tabs[0], ID_CONNECTDLG_NAME),
		 password, 512);
    SetWindowText(networkdlg_msg, _("Verify Password"));
    SetWindowText(GetDlgItem(network_tabs[0], ID_CONNECTDLG_NAME), "");
    SetFocus(GetDlgItem(network_tabs[0], ID_CONNECTDLG_NAME));
    dialog_config = VERIFY_PASSWORD_TYPE;
    break;
  case VERIFY_PASSWORD_TYPE:
    Edit_GetText(GetDlgItem(network_tabs[0], ID_CONNECTDLG_NAME), 
		 reply.password, 512);
    if (strncmp(reply.password, password, MAX_LEN_NAME) == 0) {
      EnableWindow(GetDlgItem(network_dlg, ID_CONNECTDLG_CONNECT), FALSE);
      password[0] = '\0';
      send_packet_authentication_reply(&aconnection, &reply);
    } else { 
      SetFocus(GetDlgItem(network_tabs[0], ID_CONNECTDLG_NAME));
      SetWindowText(GetDlgItem(network_tabs[0], ID_CONNECTDLG_NAME), "");
      SetWindowText(networkdlg_msg,
		    _("Passwords don't match, enter password."));
      dialog_config = NEW_PASSWORD_TYPE;
    }
    break;
  case ENTER_PASSWORD_TYPE:
    EnableWindow(GetDlgItem(network_dlg, ID_CONNECTDLG_CONNECT), FALSE);
    Edit_GetText(GetDlgItem(network_tabs[0], ID_CONNECTDLG_NAME),
		 reply.password, 512);
    send_packet_authentication_reply(&aconnection, &reply);
    break;
  default:
    assert(0);
  }
}

/**************************************************************************
  Callback function for connect dialog window messages
**************************************************************************/
static LONG CALLBACK connectdlg_proc(HWND hWnd, UINT message, WPARAM wParam,
				     LPARAM lParam)  
{
  switch(message)
    {
    case WM_CREATE:
      break;
    case WM_CLOSE:
      PostQuitMessage(0);
      break;
    case WM_DESTROY:
      break;
    case WM_COMMAND:
      break;
    case WM_NOTIFY:
      break;
    case WM_SIZE:
      break;
    case WM_GETMINMAXINFO:
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam); 
    }
  return FALSE;  
}

/**************************************************************************
  Callback function for network subwindow messages
**************************************************************************/
static LONG CALLBACK networkdlg_proc(HWND hWnd, UINT message, WPARAM wParam,
				     LPARAM lParam)  
{
  LPNMHDR nmhdr;
  switch(message)
    {
    case WM_CREATE:
      break;
    case WM_CLOSE:
      PostQuitMessage(0);
      break;
    case WM_DESTROY:
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case ID_CONNECTDLG_BACK:
	  ShowWindow(network_dlg, SW_HIDE);
	  ShowWindow(start_dlg, SW_SHOWNORMAL);
	  break;
	case ID_CONNECTDLG_QUIT:
	  PostQuitMessage(0);
	  break;
	case ID_CONNECTDLG_CONNECT:
	  connect_callback();
	  break;
	}
      break;
    case WM_NOTIFY:
      nmhdr=(LPNMHDR)lParam;
      if (nmhdr->hwndFrom==tab_ctrl) {
	if (TabCtrl_GetCurSel(tab_ctrl) == 0) {
	  ShowWindow(network_tabs[0], SW_SHOWNORMAL);
	  ShowWindow(network_tabs[1], SW_HIDE);
	  ShowWindow(network_tabs[2], SW_HIDE);
	} else if (TabCtrl_GetCurSel(tab_ctrl) == 1) {
	  ShowWindow(network_tabs[0], SW_HIDE);
	  ShowWindow(network_tabs[1], SW_SHOWNORMAL);
	  ShowWindow(network_tabs[2], SW_HIDE);
	} else {
	  ShowWindow(network_tabs[0], SW_HIDE);
	  ShowWindow(network_tabs[1], SW_HIDE);
	  ShowWindow(network_tabs[2], SW_SHOWNORMAL);
	}
      }
      break;
    case WM_SIZE:
      break;
    case WM_GETMINMAXINFO:
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam); 
    }
  return FALSE;  
}

/**************************************************************************
  Callback function for start subwindow messages
**************************************************************************/
static LONG CALLBACK startdlg_proc(HWND hWnd, UINT message, WPARAM wParam,
				   LPARAM lParam)  
{
  switch(message)
    {
    case WM_CREATE:
      break;
    case WM_CLOSE:
      PostQuitMessage(0);
      break;
    case WM_DESTROY:
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case ID_STARTDLG_NEWGAME:
	  if (is_server_running() || client_start_server()) {
	    ShowWindow(start_dlg, SW_HIDE);
	    ShowWindow(newgame_dlg, SW_SHOWNORMAL);
	  }
	  break;
	case ID_STARTDLG_LOADGAME:
	  load_game_callback();
	  break;
	case ID_STARTDLG_CONNECTGAME:
	  ShowWindow(start_dlg, SW_HIDE);
	  ShowWindow(network_dlg, SW_SHOWNORMAL);
	  break;
	}
      break;
    case WM_NOTIFY:
      break;
    case WM_SIZE:
      break;
    case WM_GETMINMAXINFO:
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam); 
    }
  return FALSE;  
}

/**************************************************************************
  Callback function for players subwindow messages
**************************************************************************/
static LONG CALLBACK playersdlg_proc(HWND hWnd, UINT message, WPARAM wParam,
				     LPARAM lParam)  
{
  NM_LISTVIEW *nmlv;
  switch(message)
    {
    case WM_CREATE:
      break;
    case WM_CLOSE:
      PostQuitMessage(0);
      break;
    case WM_DESTROY:
      break;
    case WM_COMMAND:
      break;
    case WM_NOTIFY:
      nmlv = (NM_LISTVIEW *)lParam;
      if (nmlv->hdr.hwndFrom == players_listview) {
	char name[512];
	int i, n;
	n = ListView_GetItemCount(players_listview);
	for (i = 0; i < n; i++) {
	  if (ListView_GetItemState(players_listview, i, LVIS_SELECTED)) {
	    LV_ITEM lvi;
	    lvi.iItem = i;
	    lvi.iSubItem = 0;
	    lvi.mask = LVIF_TEXT;
	    lvi.cchTextMax = 512;
	    lvi.pszText = name;
	    ListView_GetItem(players_listview, &lvi);
	  }
	}

	sz_strlcpy(player_name, name);

	if (nmlv->hdr.code == NM_DBLCLK) {
	  really_close_connection_dialog();
	  send_start_saved_game();
	}
      }
      break;
    case WM_SIZE:
      break;
    case WM_GETMINMAXINFO:
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam); 
    }
  return FALSE;  
}

/**************************************************************************
 This function updates the list of LAN servers every 100 ms for 5 seconds. 
**************************************************************************/
static int get_lanservers(HWND list)
{
  struct server_list *server_list = get_lan_server_list();
  char *row[6];

  if (server_list != NULL) {
    ListView_DeleteAllItems(list);

    server_list_iterate(*server_list, pserver) {

      row[0] = pserver->host;
      row[1] = pserver->port;
      row[2] = pserver->version;
      row[3] = _(pserver->state);
      row[4] = pserver->nplayers;
      row[5] = pserver->message;

      fcwin_listview_add_row(list,0,6,row);

    } server_list_iterate_end;
  }

  ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(list, 1, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(list, 2, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(list, 3, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(list, 4, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(list, 5, LVSCW_AUTOSIZE);

  num_lanservers_timer++;
  if (num_lanservers_timer == 50) {
    finish_lanserver_scan();
    num_lanservers_timer = 0;
    return 0;
  }
  return 1;
}

/**************************************************************************

 *************************************************************************/
static int get_meta_list(HWND list, char *errbuf, int n_errbuf)
{
  int i;
  char *row[6];
  char  buf[6][64];

  struct server_list *server_list = create_server_list(errbuf, n_errbuf);

  if (!server_list) {
    return -1;
  }

  ListView_DeleteAllItems(list);

  for (i = 0; i < 6; i++)
    row[i] = buf[i];

  server_list_iterate(*server_list, pserver) {
    sz_strlcpy(buf[0], pserver->host);
    sz_strlcpy(buf[1], pserver->port);
    sz_strlcpy(buf[2], pserver->version);
    sz_strlcpy(buf[3], _(pserver->state));
    sz_strlcpy(buf[4], pserver->nplayers);
    sz_strlcpy(buf[5], pserver->message);
    fcwin_listview_add_row(list, 0, 6, row);
  } server_list_iterate_end;
  
  ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(list, 1, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(list, 2, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(list, 3, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(list, 4, LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(list, 5, LVSCW_AUTOSIZE);

  delete_server_list(server_list);

  return 0;
}

/**************************************************************************
  handle a selection in either the metaserver or LAN tabs
 *************************************************************************/
static void handle_row_click(HWND list)
{
  int i, n;
  n = ListView_GetItemCount(list);
  for(i = 0; i < n; i++) {
    if (ListView_GetItemState(list, i, LVIS_SELECTED)) {
      char portbuf[10];
      LV_ITEM lvi;
      lvi.iItem = i;
      lvi.iSubItem = 0;
      lvi.mask = LVIF_TEXT;
      lvi.cchTextMax = 512;
      lvi.pszText = server_host;
      ListView_GetItem(list, &lvi);
      lvi.iItem = i;
      lvi.iSubItem = 1;
      lvi.mask = LVIF_TEXT;
      lvi.cchTextMax = sizeof(portbuf);
      lvi.pszText = portbuf;
      ListView_GetItem(list, &lvi);
      SetWindowText(GetDlgItem(network_tabs[0], ID_CONNECTDLG_HOST),
		    server_host);
      SetWindowText(GetDlgItem(network_tabs[0], ID_CONNECTDLG_PORT),
		    portbuf);
    }
  }
}

/**************************************************************************

 *************************************************************************/
static LONG CALLBACK tabs_page_proc(HWND dlg, UINT message, WPARAM wParam,
				    LPARAM lParam)
{
  NM_LISTVIEW *nmlv;
  switch(message)
    {
    case WM_CREATE:
      break;
    case WM_COMMAND:
      if (LOWORD(wParam)==IDOK) {
	char errbuf[128];
	if (TabCtrl_GetCurSel(tab_ctrl) == 2) {
	  get_lanservers(lan_listview);
	} else {
	  if (get_meta_list(server_listview, errbuf, sizeof(errbuf)) == -1) {
	    append_output_window(errbuf);
	  }
	}
      }
      break;
    case WM_NOTIFY:
      nmlv=(NM_LISTVIEW *)lParam;
      if (nmlv->hdr.hwndFrom == server_listview) {
	handle_row_click(server_listview);
	if (nmlv->hdr.code == NM_DBLCLK)
	  connect_callback();
      } else if (nmlv->hdr.hwndFrom == lan_listview) {
	handle_row_click(lan_listview);
	if (nmlv->hdr.code == NM_DBLCLK)
	  connect_callback();
      }
      break;
    default:
      return DefWindowProc(dlg,message,wParam,lParam);
    }
  return 0;
}

/**************************************************************************
  Make an attempt to autoconnect to the server.
  (server_autoconnect() gets GTK to call this function every so often.)
**************************************************************************/
static int try_to_autoconnect()
{
  char errbuf[512];
  static int count = 0;

  count++;

  if (count >= MAX_AUTOCONNECT_ATTEMPTS) {
    freelog(LOG_FATAL,
            _("Failed to contact server \"%s\" at port "
              "%d as \"%s\" after %d attempts"),
            server_host, server_port, user_name, count);
    exit(EXIT_FAILURE);
  }

  switch (try_to_connect(user_name, errbuf, sizeof(errbuf))) {
  case 0:                       /* Success! */
    return FALSE;               /* Do not call this
                                   function again */
#if 0
  case ECONNREFUSED:            /* Server not available (yet) */
    return TRUE;                /* Keep calling this function */
#endif
  default:                      /* All other errors are fatal */
    freelog(LOG_FATAL,
            _("Error contacting server \"%s\" at port %d "
              "as \"%s\":\n %s\n"),
            server_host, server_port, user_name, errbuf);
    exit(EXIT_FAILURE);     
  }
}

/**************************************************************************

**************************************************************************/
static void CALLBACK autoconnect_timer(HWND hwnd,UINT uMsg,
				       UINT idEvent,DWORD  dwTime)  
{
  printf("Timer\n");
  if (!try_to_autoconnect())
    KillTimer(NULL, autoconnect_timer_id);
}

/**************************************************************************
  Start trying to autoconnect to civserver.  Calls
  get_server_address(), then arranges for try_to_autoconnect(), which
  calls try_to_connect(), to be called roughly every
  AUTOCONNECT_INTERVAL milliseconds, until success, fatal error or
  user intervention.  
**************************************************************************/
void server_autoconnect()
{
  char buf[512];

  my_snprintf(buf, sizeof(buf),
              _("Auto-connecting to server \"%s\" at port %d "
                "as \"%s\" every %d.%d second(s) for %d times"),
              server_host, server_port, user_name,
              AUTOCONNECT_INTERVAL / 1000,AUTOCONNECT_INTERVAL % 1000, 
              MAX_AUTOCONNECT_ATTEMPTS);
  append_output_window(buf);
  if (get_server_address(server_host, server_port, buf, sizeof(buf)) < 0) {
    freelog(LOG_FATAL,
            _("Error contacting server \"%s\" at port %d "
              "as \"%s\":\n %s\n"),
            server_host, server_port, user_name, buf);
    exit(EXIT_FAILURE);
  }
  printf("server_autoconnect\n");
  if (try_to_autoconnect()) {
    printf("T2\n");
    autoconnect_timer_id = SetTimer(root_window, 3, AUTOCONNECT_INTERVAL,
				    autoconnect_timer);
  }

}

/**************************************************************************
  Handle the saving and loading functions.
**************************************************************************/
void handle_save_load(const char *title, bool is_save)
{
  OPENFILENAME ofn;
  char dirname[MAX_PATH + 1];
  char szfile[MAX_PATH] = "\0";

  strcpy(szfile, "");
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = root_window;
  ofn.hInstance = freecivhinst;
  /* WARNING: Don't translate this next string, it has NULLs in it. */
  ofn.lpstrFilter = "Freeciv saves\0*.sav;*.sav.gz\0All files\0*.*\0\0";
  ofn.lpstrCustomFilter = NULL;
  ofn.nMaxCustFilter = 0;
  ofn.nFilterIndex = 1;
  ofn.lpstrFile = szfile;
  ofn.nMaxFile = sizeof(szfile);
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = title;
  ofn.nFileOffset = 0;
  ofn.nFileExtension = 0;
  ofn.lpstrDefExt = NULL;
  ofn.lCustData = 0;
  ofn.lpfnHook = NULL;
  ofn.lpTemplateName = NULL;
  ofn.Flags = OFN_EXPLORER;
  GetCurrentDirectory(MAX_PATH, dirname);
  SetCurrentDirectory(saved_games_dirname);
  if (is_save) {
    if (GetSaveFileName(&ofn)) {

      if (current_filename) {
	free(current_filename);
      }

      GetCurrentDirectory(MAX_PATH, saved_games_dirname);
      SetCurrentDirectory(dirname);

      current_filename = mystrdup(ofn.lpstrFile);

      send_save_game(current_filename);
    } else {
      SetCurrentDirectory(dirname);
    }
  } else {
    if (GetOpenFileName(&ofn)) {
      char cmd[MAX_LEN_MSG];

      if (current_filename) {
	free(current_filename);
      }

      GetCurrentDirectory(MAX_PATH, saved_games_dirname);
      SetCurrentDirectory(dirname);

      current_filename = mystrdup(ofn.lpstrFile);

      my_snprintf(cmd, sizeof(cmd), "/load %s", ofn.lpstrFile);
      send_chat(cmd);
    } else {
      SetCurrentDirectory(dirname);
    }
  }
}

/**************************************************************************
  Callback function for load game button
**************************************************************************/
static void load_game_callback()
{
  if (is_server_running() || client_start_server()) {
    while(!can_client_access_hack()) {
      socket_timer();
    }
    handle_save_load(_("Load Game"), FALSE);
  }
}

/**************************************************************************

**************************************************************************/
static void set_new_game_params(HWND win)
{
  int aifill;
  int aiskill;

  char buf[512];
  char aifill_str[MAX_LEN_MSG - MAX_LEN_USERNAME + 1];

  if (!is_server_running()) {
    client_start_server();
  }

  aiskill = ComboBox_GetCurSel(GetDlgItem(newgame_dlg,
					  ID_NEWGAMEDLG_AISKILL));

  my_snprintf(buf, sizeof(buf), "/%s", skill_level_names[aiskill]);
  send_chat(buf);

#if 0 
  send_chat("/set autotoggle 1");
#endif
  Edit_GetText(GetDlgItem(newgame_dlg, ID_NEWGAMEDLG_AIFILL), buf, 512);
  aifill = atoi(buf);

  my_snprintf(aifill_str, sizeof(aifill_str), "/set aifill %d", aifill);
  send_chat(aifill_str);

  send_chat("/start");
}

/**************************************************************************
  Callback function for new game subwindow messages
**************************************************************************/
static LONG CALLBACK new_game_proc(HWND win, UINT message,
				   WPARAM wParam, LPARAM lParam)
{
  switch(message)
    {
    case WM_CREATE:
    case WM_DESTROY:
    case WM_SIZE:
    case WM_GETMINMAXINFO:
      break;
    case WM_CLOSE:
      break;
    case WM_COMMAND:
      switch((enum new_game_dlg_ids)LOWORD(wParam))
	{
	case ID_NEWGAMEDLG_OPTIONS:
	  send_report_request(REPORT_SERVER_OPTIONS2);
	  break;
	case ID_CANCEL:
	  client_kill_server(TRUE);
	  ShowWindow(newgame_dlg,SW_HIDE);
	  ShowWindow(start_dlg,SW_SHOWNORMAL);
	  break;
	case ID_OK:
	  set_new_game_params(win);
	  break;
	default:
	  break;
	}
      break;
    default:
      return DefWindowProc(win, message, wParam, lParam);
    }
  return 0;
}

/**************************************************************************
  Callback function for setting connect dialog window size
**************************************************************************/
static void cdlg_setsize(RECT *size, void *data)
{
  MoveWindow(network_dlg, size->left, size->top, size->right - size->left,
	     size->bottom - size->top, TRUE);
  MoveWindow(start_dlg, size->left, size->top, size->right - size->left,
	     size->bottom - size->top, TRUE);
  MoveWindow(players_dlg, size->left, size->top, size->right - size->left,
	     size->bottom - size->top, TRUE);
  MoveWindow(newgame_dlg, size->left, size->top, size->right - size->left,
	     size->bottom - size->top, TRUE);
  fcwin_redo_layout(network_dlg);
  fcwin_redo_layout(start_dlg);
  fcwin_redo_layout(players_dlg);
  fcwin_redo_layout(newgame_dlg);
}

/**************************************************************************
  Returns the minimum connect dialog window size
**************************************************************************/
static void cdlg_minsize(POINT *size, void *data)
{
  size->x = 460;
  size->y = 310;
}

/**************************************************************************
  Function called when connect dialog is destroyed
**************************************************************************/
static void cdlg_del(void *data)
{
  DestroyWindow(network_dlg);
  DestroyWindow(start_dlg);
  DestroyWindow(players_dlg);
  DestroyWindow(newgame_dlg);
}


/**************************************************************************
  Creates the server connection window and all associated subwindows
**************************************************************************/
void gui_server_connect()
{
  int i;
  char buf[20];
  char *titles_[3] = {N_("Freeciv Server Selection"),N_("Metaserver"),
		      N_("Local Area Network")};
  char *server_list_titles_[6] = {N_("Server Name"), N_("Port"),
				  N_("Version"), N_("Status"), N_("Players"),
				  N_("Comment")};
  char *titles[3];
  WNDPROC wndprocs[3] = {tabs_page_proc, tabs_page_proc, tabs_page_proc};
  void *user_data[3] = {NULL, NULL, NULL};
  struct fcwin_box *hbox;
  struct fcwin_box *vbox;
  struct fcwin_box *main_vbox;
  struct fcwin_box *network_vbox;
  struct fcwin_box *start_vbox;
  struct fcwin_box *players_vbox;
  struct fcwin_box *newgame_vbox;
  LV_COLUMN lvc;
  
  titles[0] =_(titles_[0]);
  titles[1] =_(titles_[1]);
  titles[2] =_(titles_[2]);

  dialog_config = LOGIN_TYPE;

  /* If the connect dialog already exists, return it to its initial state
     and return. */
  if (connect_dlg) {
    ShowWindow(start_dlg, SW_SHOWNORMAL);
    ShowWindow(network_dlg, SW_HIDE);
    ShowWindow(players_dlg, SW_HIDE);
    ShowWindow(newgame_dlg, SW_HIDE);
    return;
  }

  /* Create connect dialog, which contains all the other dialogs */
  connect_dlg = fcwin_create_layouted_window(connectdlg_proc,
					     _("Welcome to Freeciv"),
					     WS_OVERLAPPEDWINDOW,
					     CW_USEDEFAULT, CW_USEDEFAULT,
					     root_window, NULL,
					     REAL_CHILD, NULL);
  main_vbox = fcwin_vbox_new(connect_dlg, FALSE);
  fcwin_box_add_generic(main_vbox, cdlg_minsize, cdlg_setsize, cdlg_del,
			NULL, TRUE, TRUE, 0);


  /* Create start dialog */
  start_dlg = fcwin_create_layouted_window(startdlg_proc, NULL, WS_CHILD,
					   0, 0, connect_dlg, NULL, 
					   REAL_CHILD, NULL);
  start_vbox = fcwin_vbox_new(start_dlg, FALSE);

  fcwin_box_add_button_default(start_vbox, _("Start New Game"),
			       ID_STARTDLG_NEWGAME, 0);
  fcwin_box_add_button_default(start_vbox, _("Load Saved Game"),
			       ID_STARTDLG_LOADGAME, 0);
  fcwin_box_add_button_default(start_vbox, _("Connect To Network Game"),
			       ID_STARTDLG_CONNECTGAME, 0);


  /* Create player selection dialog, for starting a loaded game */
  players_dlg = fcwin_create_layouted_window(playersdlg_proc, NULL, WS_CHILD,
					     0, 0, connect_dlg, NULL, 
					     REAL_CHILD, NULL);
  players_vbox = fcwin_vbox_new(players_dlg, FALSE);

  fcwin_box_add_static(players_vbox, _("Choose a nation to play"), 0,
		       SS_LEFT, FALSE, FALSE, 5);

  players_listview = fcwin_box_add_listview(players_vbox, 5, 0, 
					    LVS_REPORT | LVS_SINGLESEL, TRUE,
					    TRUE, 5);
  lvc.pszText = _("Name");
  lvc.mask = LVCF_TEXT;
  ListView_InsertColumn(players_listview, 0, &lvc);
  ListView_SetColumnWidth(players_listview, 0, LVSCW_AUTOSIZE_USEHEADER);

  lvc.pszText = _("Nation");
  lvc.mask = LVCF_TEXT;
  ListView_InsertColumn(players_listview, 1, &lvc);
  ListView_SetColumnWidth(players_listview, 1, LVSCW_AUTOSIZE_USEHEADER);

  lvc.pszText = _("Status");
  lvc.mask = LVCF_TEXT;
  ListView_InsertColumn(players_listview, 2, &lvc);
  ListView_SetColumnWidth(players_listview, 2, LVSCW_AUTOSIZE_USEHEADER);

  lvc.pszText = _("Type");
  lvc.mask = LVCF_TEXT;
  ListView_InsertColumn(players_listview, 3, &lvc);
  ListView_SetColumnWidth(players_listview, 3, LVSCW_AUTOSIZE_USEHEADER);


  /* Create new game dialog */
  newgame_dlg = fcwin_create_layouted_window(new_game_proc, NULL, WS_CHILD,
					     0, 0, connect_dlg, NULL, 
					     REAL_CHILD, NULL);
  newgame_vbox = fcwin_vbox_new(newgame_dlg, FALSE);

  hbox = fcwin_hbox_new(newgame_dlg, TRUE);

  fcwin_box_add_static(hbox, _("Number of players (Including AI):"), 0,
		       SS_LEFT, TRUE, TRUE, 5);
  fcwin_box_add_edit(hbox, "5", 3, ID_NEWGAMEDLG_AIFILL, ES_NUMBER, TRUE,
		     TRUE, 10);
  fcwin_box_add_box(newgame_vbox, hbox, FALSE, FALSE, 5);

  hbox = fcwin_hbox_new(newgame_dlg, TRUE);

  fcwin_box_add_static(hbox, _("AI skill level:"), 0, SS_LEFT, TRUE, TRUE,
		       5);
  fcwin_box_add_combo(hbox, NUM_SKILL_LEVELS, ID_NEWGAMEDLG_AISKILL,
		      CBS_DROPDOWN | WS_VSCROLL, TRUE, TRUE, 5);
  for (i = 0; i < NUM_SKILL_LEVELS; i++) {
    ComboBox_AddString(GetDlgItem(newgame_dlg, ID_NEWGAMEDLG_AISKILL), 
		       _(skill_level_names[i]));
  }
  ComboBox_SetCurSel(GetDlgItem(newgame_dlg, ID_NEWGAMEDLG_AISKILL), 1);

  fcwin_box_add_box(newgame_vbox, hbox, FALSE, FALSE, 5);

  hbox = fcwin_hbox_new(newgame_dlg, TRUE);

  fcwin_box_add_button(hbox, _("Game Options"), ID_NEWGAMEDLG_OPTIONS, 0, 
		       TRUE, TRUE, 5);
  fcwin_box_add_button(hbox, _("OK"), ID_OK, 0, TRUE, TRUE, 5);
  fcwin_box_add_button(hbox, _("Cancel"), ID_CANCEL, 0, TRUE, TRUE, 5);

  fcwin_box_add_box(newgame_vbox, hbox, TRUE, FALSE, 5);


  /* Create network game dialog */
  network_dlg = fcwin_create_layouted_window(networkdlg_proc, NULL, WS_CHILD,
					     0, 0, connect_dlg, NULL, 
					     REAL_CHILD, NULL);
  network_vbox = fcwin_vbox_new(network_dlg,FALSE);

  /* Change 2 to 3 here to enable lan tab */
  tab_ctrl = fcwin_box_add_tab(network_vbox, wndprocs, network_tabs, titles, 
			       user_data, 2, 0, 0, TRUE, TRUE, 5);

  hbox = fcwin_hbox_new(network_tabs[0], FALSE);

  vbox = fcwin_vbox_new(network_tabs[0], FALSE);
  networkdlg_name = fcwin_box_add_static(vbox, _("Login:"), 0, SS_CENTER,
					 TRUE, TRUE, 5);
  fcwin_box_add_static(vbox, _("Host:"), 0, SS_CENTER, TRUE, TRUE, 5);
  fcwin_box_add_static(vbox, _("Port:"), 0, SS_CENTER, TRUE, TRUE, 5);
  fcwin_box_add_box(hbox, vbox, FALSE, FALSE, 5);
  vbox = fcwin_vbox_new(network_tabs[0], FALSE);
  fcwin_box_add_edit(vbox, user_name, 40, ID_CONNECTDLG_NAME, 0,
		     TRUE, TRUE, 10);
  fcwin_box_add_edit(vbox, server_host, 40, ID_CONNECTDLG_HOST, 0,
		     TRUE, TRUE, 10);
  my_snprintf(buf, sizeof(buf), "%d", server_port);
  fcwin_box_add_edit(vbox, buf, 8, ID_CONNECTDLG_PORT, 0, TRUE, TRUE, 15);

  fcwin_box_add_box(hbox, vbox, TRUE, TRUE, 5);

  vbox = fcwin_vbox_new(network_tabs[0], FALSE);
  networkdlg_msg = fcwin_box_add_static(vbox, "", 0, SS_LEFT, TRUE, TRUE,
					5);
  fcwin_box_add_box(vbox, hbox, TRUE, FALSE, 0);
  fcwin_set_box(network_tabs[0], vbox);

  vbox = fcwin_vbox_new(network_tabs[1], FALSE);
  server_listview = fcwin_box_add_listview(vbox, 5, 0, LVS_REPORT
					   | LVS_SINGLESEL, TRUE, TRUE, 5);
  fcwin_box_add_button(vbox, _("Update"), IDOK, 0, FALSE, FALSE, 5);
  fcwin_set_box(network_tabs[1], vbox);

  /* Enable this to enable lan tab */
#if 0
  vbox = fcwin_vbox_new(network_tabs[2], FALSE);
  lan_listview = fcwin_box_add_listview(vbox, 5, 0, LVS_REPORT
					| LVS_SINGLESEL, TRUE, TRUE,5);
  fcwin_box_add_button(vbox, _("Update"), IDOK, 0, FALSE, FALSE, 5);
  fcwin_set_box(network_tabs[2], vbox);
#endif

  hbox = fcwin_hbox_new(network_dlg, TRUE);
  fcwin_box_add_button(hbox, _("Back"), ID_CONNECTDLG_BACK,
		       0, TRUE, TRUE, 5);
  fcwin_box_add_button(hbox, _("Connect"), ID_CONNECTDLG_CONNECT,
		       0, TRUE, TRUE, 5);
  fcwin_box_add_button(hbox, _("Quit"), ID_CONNECTDLG_QUIT,
		       0, TRUE, TRUE, 5);
  fcwin_box_add_box(network_vbox, hbox, FALSE, FALSE, 5);

  for(i = 0; i < ARRAY_SIZE(server_list_titles_); i++) {
    LV_COLUMN lvc;
    lvc.pszText = _(server_list_titles_[i]);
    lvc.mask = LVCF_TEXT;
    ListView_InsertColumn(server_listview, i, &lvc);
    ListView_InsertColumn(lan_listview, i, &lvc);
  }

  for(i = 0; i < ARRAY_SIZE(server_list_titles_); i++) {
    ListView_SetColumnWidth(server_listview, i, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(lan_listview, i, LVSCW_AUTOSIZE_USEHEADER);
  }

  /* Assign boxes to windows */
  fcwin_set_box(start_dlg, start_vbox);
  fcwin_set_box(network_dlg, network_vbox);
  fcwin_set_box(connect_dlg, main_vbox);
  fcwin_set_box(players_dlg, players_vbox);
  fcwin_set_box(newgame_dlg, newgame_vbox);

  /* Redo layouts */
  fcwin_redo_layout(connect_dlg);
  fcwin_redo_layout(network_dlg);
  fcwin_redo_layout(start_dlg);
  fcwin_redo_layout(players_dlg);
  fcwin_redo_layout(newgame_dlg);

  /* Show the first tab, the initial dialog, and the window */
  ShowWindow(network_tabs[0],SW_SHOWNORMAL);
  ShowWindow(start_dlg,SW_SHOWNORMAL);
  ShowWindow(connect_dlg,SW_SHOWNORMAL);
}

