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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "astring.h"
#include "fcintl.h"
#include "log.h"
#include "government.h"
#include "map.h"
#include "mem.h"
#include "support.h"
#include "unit.h"

#include "chatline.h"
#include "cityrep.h"
#include "civclient.h"
#include "clinet.h"
#include "connectdlg_common.h"
#include "connectdlg.h"
#include "control.h"
#include "dialogs.h"
#include "finddlg.h"
#include "gotodlg.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "mapctrl.h"   /* center_on_unit */
#include "mapview.h"
#include "menu_g.h"
#include "messagedlg.h"
#include "messagewin.h"
#include "multiselect.h"
#include "myai.h"
#include "optiondlg.h"
#include "options.h"
#include "packhand.h"
#include "pages.h"
#include "peptool.h"
#include "plrdlg.h"
#include "ratesdlg.h"
#include "repodlgs.h"
#include "spaceshipdlg.h"
#include "wldlg.h"
#include "unittype.h"
#include "style.h"

#include "menu.h"

void create_automatic_processus_menus(void);
void update_automatic_processus_filter_menu(automatic_processus *pap);

static void get_accel_label(GtkWidget *widget,const char *uname);
static bool can_player_unit_type(Unit_Type_id utype);
static void pep_airlift_menu_set_active(void);
static void pep_airlift_menu_callback(GtkWidget *widget,gpointer data);
static void create_pep_airlift_menu(int aq,GtkWidget *widget);

static GtkItemFactory *item_factory = NULL;
static GtkWidget *main_menubar = NULL;
GtkAccelGroup *toplevel_accel = NULL;
static enum unit_activity road_activity;

static void update_delayed_goto_inclusive_filter_menu(void);
static void update_delayed_goto_exclusive_filter_menu(void);
static void update_multi_select_inclusive_filter_menu(void);
static void update_multi_select_exclusive_filter_menu(void);

static void menus_rename(const char *path, const char *s);
static void menus_set_active(const char *path, int active);

/****************************************************************
...
*****************************************************************/
enum MenuID {
  MENU_END_OF_LIST=0,

  MENU_GAME_OPTIONS,
  MENU_GAME_MSG_OPTIONS,
  MENU_GAME_CHATLINE_COLORS,
  MENU_GAME_STYLE,
  MENU_GAME_SAVE_SETTINGS,
  MENU_GAME_PEPSETTINGS,
  MENU_GAME_PEPSETTINGS_LOAD,
  MENU_GAME_PEPSETTINGS_SAVE,
  MENU_GAME_SERVER_OPTIONS1,
  MENU_GAME_SERVER_OPTIONS2,
  MENU_GAME_SAVE_GAME,
  MENU_GAME_SAVE_QUICK, 
  MENU_GAME_OUTPUT_LOG,
  MENU_GAME_CLEAR_OUTPUT,
  MENU_GAME_LEAVE,
  MENU_GAME_QUIT,
  
  MENU_GOVERNMENT_TAX_RATE,
  MENU_GOVERNMENT_FIND_CITY,
  MENU_GOVERNMENT_WORKLISTS,
  MENU_GOVERNMENT_CLEAR_SELECTED_WORKLISTS,
  MENU_GOVERNMENT_REVOLUTION,

  MENU_VIEW_SHOW_MAP_GRID,
  MENU_VIEW_SHOW_NATIONAL_BORDERS,
  MENU_VIEW_SHOW_CITY_NAMES,
  MENU_VIEW_SHOW_CITY_GROWTH_TURNS,
  MENU_VIEW_SHOW_CITY_PRODUCTIONS,
  MENU_VIEW_SHOW_CITY_PRODUCTION_BUY_COST,
  MENU_VIEW_SHOW_CITY_TRADEROUTES,
  MENU_VIEW_SHOW_TERRAIN,
  MENU_VIEW_SHOW_COASTLINE,
  MENU_VIEW_SHOW_ROADS_RAILS,
  MENU_VIEW_SHOW_IRRIGATION,
  MENU_VIEW_SHOW_MINES,
  MENU_VIEW_SHOW_FORTRESS_AIRBASE,
  MENU_VIEW_SHOW_SPECIALS,
  MENU_VIEW_SHOW_POLLUTION,
  MENU_VIEW_SHOW_CITIES,
  MENU_VIEW_SHOW_UNITS,
  MENU_VIEW_SHOW_FOCUS_UNIT,
  MENU_VIEW_SHOW_FOG_OF_WAR,
  MENU_VIEW_CENTER_VIEW,

  MENU_ORDER_BUILD_CITY,     /* shared with BUILD_WONDER */
  MENU_ORDER_ROAD,           /* shared with TRADEROUTE */
  MENU_ORDER_IRRIGATE,
  MENU_ORDER_MINE,
  MENU_ORDER_TRANSFORM,
  MENU_ORDER_FORTRESS,       /* shared with FORTIFY */
  MENU_ORDER_SLEEP,
  MENU_ORDER_AIRBASE,
  MENU_ORDER_POLLUTION,      /* shared with PARADROP */
  MENU_ORDER_FALLOUT,
  MENU_ORDER_SENTRY,
  MENU_ORDER_PILLAGE,
  MENU_ORDER_HOMECITY,
  MENU_ORDER_UNLOAD_TRANSPORTER,
  MENU_ORDER_LOAD,
  MENU_ORDER_UNLOAD,
  MENU_ORDER_WAKEUP_OTHERS,
  MENU_ORDER_AUTO_SETTLER,   /* shared with AUTO_ATTACK */
  MENU_ORDER_AUTO_EXPLORE,
  MENU_ORDER_CONNECT_ROAD,
  MENU_ORDER_CONNECT_RAIL,
  MENU_ORDER_CONNECT_IRRIGATE,
  MENU_ORDER_PATROL,
  MENU_ORDER_GOTO,
  MENU_ORDER_GOTO_CITY,
  MENU_ORDER_RETURN,
  MENU_ORDER_DISBAND,
  MENU_ORDER_DIPLOMAT_DLG,
  MENU_ORDER_NUKE,
  MENU_ORDER_WAIT,
  MENU_ORDER_DONE,

  MENU_DELAYED_GOTO_DELAYED_GOTO,
  MENU_DELAYED_GOTO_DELAYED_PARADROP_OR_NUKE,
  MENU_DELAYED_GOTO_DELAYED_AIRLIFT,
  MENU_DELAYED_GOTO_EXECUTE_DELAYED_GOTO,
  MENU_DELAYED_GOTO_ADD_PAUSE,
  MENU_DELAYED_GOTO_CLEAR_DELAYED_ORDERS,
  MENU_DELAYED_GOTO_GOTO_SINGLE_UNIT,
  MENU_DELAYED_GOTO_GOTO_IN_TRANSPORTER,
  MENU_DELAYED_GOTO_GOTO_ON_TILE,
  MENU_DELAYED_GOTO_GOTO_ON_CONTINENT,
  MENU_DELAYED_GOTO_GOTO_EVERY_WHERE,
  MENU_DELAYED_GOTO_GOTO_SAME_TYPE,
  MENU_DELAYED_GOTO_GOTO_SAME_MOVE_TYPE,
  MENU_DELAYED_GOTO_GOTO_ALL,
  MENU_DELAYED_GOTO_OLD_SINGLE,
  MENU_DELAYED_GOTO_OLD_TILE,
  MENU_DELAYED_GOTO_OLD_CONTINENT,
  MENU_DELAYED_GOTO_OLD_TILE_ALL_TYPES,
  MENU_DELAYED_GOTO_DG1_SELECT,
  MENU_DELAYED_GOTO_DG1_ADD,
  MENU_DELAYED_GOTO_DG1_RECORD,
  MENU_DELAYED_GOTO_DG1_CLEAR,
  MENU_DELAYED_GOTO_DG1_EXECUTE,
  MENU_DELAYED_GOTO_DG2_SELECT,
  MENU_DELAYED_GOTO_DG2_ADD,
  MENU_DELAYED_GOTO_DG2_RECORD,
  MENU_DELAYED_GOTO_DG2_CLEAR,
  MENU_DELAYED_GOTO_DG2_EXECUTE,
  MENU_DELAYED_GOTO_DG3_SELECT,
  MENU_DELAYED_GOTO_DG3_ADD,
  MENU_DELAYED_GOTO_DG3_RECORD,
  MENU_DELAYED_GOTO_DG3_CLEAR,
  MENU_DELAYED_GOTO_DG3_EXECUTE,
  MENU_DELAYED_GOTO_INCLUSIVE_ALL,
  MENU_DELAYED_GOTO_INCLUSIVE_NEW,
  MENU_DELAYED_GOTO_INCLUSIVE_FORTIFIED,
  MENU_DELAYED_GOTO_INCLUSIVE_SENTRIED,
  MENU_DELAYED_GOTO_INCLUSIVE_VETERAN,
  MENU_DELAYED_GOTO_INCLUSIVE_AUTO,
  MENU_DELAYED_GOTO_INCLUSIVE_IDLE,
  MENU_DELAYED_GOTO_INCLUSIVE_ABLE_TO_MOVE,
  MENU_DELAYED_GOTO_INCLUSIVE_MILITARY,
  MENU_DELAYED_GOTO_INCLUSIVE_OFF,
  MENU_DELAYED_GOTO_EXCLUSIVE_ALL,
  MENU_DELAYED_GOTO_EXCLUSIVE_NEW,
  MENU_DELAYED_GOTO_EXCLUSIVE_FORTIFIED,
  MENU_DELAYED_GOTO_EXCLUSIVE_SENTRIED,
  MENU_DELAYED_GOTO_EXCLUSIVE_VETERAN,
  MENU_DELAYED_GOTO_EXCLUSIVE_AUTO,
  MENU_DELAYED_GOTO_EXCLUSIVE_IDLE,
  MENU_DELAYED_GOTO_EXCLUSIVE_ABLE_TO_MOVE,
  MENU_DELAYED_GOTO_EXCLUSIVE_MILITARY,
  MENU_DELAYED_GOTO_EXCLUSIVE_OFF,

  MENU_AIRLIFT_SET_AIRLIFT_DEST,
  MENU_AIRLIFT_SET_AIRLIFT_SRC,
  MENU_AIRLIFT_AIRLIFT_SELECT_AIRPORT_CITIES,
  MENU_AIRLIFT_CLEAR_AIRLIFT_QUEUE,
  MENU_AIRLIFT_SHOW_CITIES_IN_AIRLIFT_QUEUE,
  MENU_AIRLIFT_AQ1_SELECT,
  MENU_AIRLIFT_AQ1_ADD,
  MENU_AIRLIFT_AQ1_RECORD,
  MENU_AIRLIFT_AQ1_CLEAR,
  MENU_AIRLIFT_AQ1_EXECUTE,
  MENU_AIRLIFT_AQ1_SHOW,
  MENU_AIRLIFT_AQ2_SELECT,
  MENU_AIRLIFT_AQ2_ADD,
  MENU_AIRLIFT_AQ2_RECORD,
  MENU_AIRLIFT_AQ2_CLEAR,
  MENU_AIRLIFT_AQ2_EXECUTE,
  MENU_AIRLIFT_AQ2_SHOW,
  MENU_AIRLIFT_AQ3_SELECT,
  MENU_AIRLIFT_AQ3_ADD,
  MENU_AIRLIFT_AQ3_RECORD,
  MENU_AIRLIFT_AQ3_CLEAR,
  MENU_AIRLIFT_AQ3_EXECUTE,
  MENU_AIRLIFT_AQ3_SHOW,
  MENU_AIRLIFT_AQ4_SELECT,
  MENU_AIRLIFT_AQ4_ADD,
  MENU_AIRLIFT_AQ4_RECORD,
  MENU_AIRLIFT_AQ4_CLEAR,
  MENU_AIRLIFT_AQ4_EXECUTE,
  MENU_AIRLIFT_AQ4_SHOW,
  MENU_AIRLIFT_AQ5_SELECT,
  MENU_AIRLIFT_AQ5_ADD,
  MENU_AIRLIFT_AQ5_RECORD,
  MENU_AIRLIFT_AQ5_CLEAR,
  MENU_AIRLIFT_AQ5_EXECUTE,
  MENU_AIRLIFT_AQ5_SHOW,
  MENU_AIRLIFT_AQ6_SELECT,
  MENU_AIRLIFT_AQ6_ADD,
  MENU_AIRLIFT_AQ6_RECORD,
  MENU_AIRLIFT_AQ6_CLEAR,
  MENU_AIRLIFT_AQ6_EXECUTE,
  MENU_AIRLIFT_AQ6_SHOW,

  MENU_AUTO_CARAVAN_MY_AI_TRADE_CITY,
  MENU_AUTO_CARAVAN_MY_AI_CLEAR_TRADE_CITY,
  MENU_AUTO_CARAVAN_MY_AI_SHOW_TRADE_CITIES,
  MENU_AUTO_CARAVAN_MY_AI_SHOW_FREE_SLOTS,
  MENU_AUTO_CARAVAN_MY_AI_ESTIMATE_TRADE,
  MENU_AUTO_CARAVAN_MY_AI_TRADE_RECALCULATE,
  MENU_AUTO_CARAVAN_MY_AI_TRADE_WITH,
  MENU_AUTO_CARAVAN_MY_AI_CARAVAN,
  MENU_AUTO_CARAVAN_MY_AI_TRADE_EXECUTE,

  MENU_MULTI_SELECTION_MS_SELECT,
  MENU_MULTI_SELECTION_MS_ACTIVE_ALL,
  MENU_MULTI_SELECTION_MS_CLEAR,
  MENU_MULTI_SELECTION_INCLUSIVE_ALL,
  MENU_MULTI_SELECTION_INCLUSIVE_NEW,
  MENU_MULTI_SELECTION_INCLUSIVE_FORTIFIED,
  MENU_MULTI_SELECTION_INCLUSIVE_SENTRIED,
  MENU_MULTI_SELECTION_INCLUSIVE_VETERAN,
  MENU_MULTI_SELECTION_INCLUSIVE_AUTO,
  MENU_MULTI_SELECTION_INCLUSIVE_IDLE,
  MENU_MULTI_SELECTION_INCLUSIVE_ABLE_TO_MOVE,
  MENU_MULTI_SELECTION_INCLUSIVE_MILITARY,
  MENU_MULTI_SELECTION_INCLUSIVE_OFF,
  MENU_MULTI_SELECTION_EXCLUSIVE_ALL,
  MENU_MULTI_SELECTION_EXCLUSIVE_NEW,
  MENU_MULTI_SELECTION_EXCLUSIVE_FORTIFIED,
  MENU_MULTI_SELECTION_EXCLUSIVE_SENTRIED,
  MENU_MULTI_SELECTION_EXCLUSIVE_VETERAN,
  MENU_MULTI_SELECTION_EXCLUSIVE_AUTO,
  MENU_MULTI_SELECTION_EXCLUSIVE_IDLE,
  MENU_MULTI_SELECTION_EXCLUSIVE_ABLE_TO_MOVE,
  MENU_MULTI_SELECTION_EXCLUSIVE_MILITARY,
  MENU_MULTI_SELECTION_EXCLUSIVE_OFF,
  MENU_MULTI_SELECTION_MODE_SINGLE_UNIT,
  MENU_MULTI_SELECTION_MODE_IN_TRANSPORTER,
  MENU_MULTI_SELECTION_MODE_ON_TILE,
  MENU_MULTI_SELECTION_MODE_ON_CONTINENT,
  MENU_MULTI_SELECTION_MODE_EVERY_WHERE,
  MENU_MULTI_SELECTION_MODE_SAME_TYPE,
  MENU_MULTI_SELECTION_MODE_SAME_MOVE_TYPE,
  MENU_MULTI_SELECTION_MODE_ALL,
  MENU_MULTI_SELECTION_MY_AI_SPREAD,
  MENU_MULTI_SELECTION_MY_AI_SPREAD_AIRPORT,
  MENU_MULTI_SELECTION_MY_AI_SPREAD_ALLY,
  MENU_MULTI_SELECTION_MS1_SELECT,
  MENU_MULTI_SELECTION_MS1_ADD,
  MENU_MULTI_SELECTION_MS1_RECORD,
  MENU_MULTI_SELECTION_MS1_CLEAR,
  MENU_MULTI_SELECTION_MS2_SELECT,
  MENU_MULTI_SELECTION_MS2_ADD,
  MENU_MULTI_SELECTION_MS2_RECORD,
  MENU_MULTI_SELECTION_MS2_CLEAR,
  MENU_MULTI_SELECTION_MS3_SELECT,
  MENU_MULTI_SELECTION_MS3_ADD,
  MENU_MULTI_SELECTION_MS3_RECORD,
  MENU_MULTI_SELECTION_MS3_CLEAR,
  MENU_MULTI_SELECTION_MS4_SELECT,
  MENU_MULTI_SELECTION_MS4_ADD,
  MENU_MULTI_SELECTION_MS4_RECORD,
  MENU_MULTI_SELECTION_MS4_CLEAR,
  MENU_MULTI_SELECTION_MS5_SELECT,
  MENU_MULTI_SELECTION_MS5_ADD,
  MENU_MULTI_SELECTION_MS5_RECORD,
  MENU_MULTI_SELECTION_MS5_CLEAR,
  MENU_MULTI_SELECTION_MS6_SELECT,
  MENU_MULTI_SELECTION_MS6_ADD,
  MENU_MULTI_SELECTION_MS6_RECORD,
  MENU_MULTI_SELECTION_MS6_CLEAR,
  MENU_MULTI_SELECTION_MS7_SELECT,
  MENU_MULTI_SELECTION_MS7_ADD,
  MENU_MULTI_SELECTION_MS7_RECORD,
  MENU_MULTI_SELECTION_MS7_CLEAR,
  MENU_MULTI_SELECTION_MS8_SELECT,
  MENU_MULTI_SELECTION_MS8_ADD,
  MENU_MULTI_SELECTION_MS8_RECORD,
  MENU_MULTI_SELECTION_MS8_CLEAR,
  MENU_MULTI_SELECTION_MS9_SELECT,
  MENU_MULTI_SELECTION_MS9_ADD,
  MENU_MULTI_SELECTION_MS9_RECORD,
  MENU_MULTI_SELECTION_MS9_CLEAR,

  MENU_MISCELLANEOUS_AIR_PATROL,
  MENU_MISCELLANEOUS_AIR_PATROL_DEST,
  MENU_MISCELLANEOUS_PATROL_EXECUTE,
  MENU_MISCELLANEOUS_SET_RALLIES,
  MENU_MISCELLANEOUS_CLEAR_RALLIES,
  MENU_MISCELLANEOUS_CARAVAN_POPUP,
  MENU_MISCELLANEOUS_CARAVAN_TRADE,
  MENU_MISCELLANEOUS_CARAVAN_BUILD,
  MENU_MISCELLANEOUS_CARAVAN_NOTHING,
  MENU_MISCELLANEOUS_DIPLOMAT_POPUP,
  MENU_MISCELLANEOUS_DIPLOMAT_BRIBE,
  MENU_MISCELLANEOUS_DIPLOMAT_SABOTAGE_UNIT,
  MENU_MISCELLANEOUS_DIPLOMAT_EMBASSY,
  MENU_MISCELLANEOUS_DIPLOMAT_INVESTIGATE,
  MENU_MISCELLANEOUS_DIPLOMAT_SABOTAGE_CITY,
  MENU_MISCELLANEOUS_DIPLOMAT_STEAL,
  MENU_MISCELLANEOUS_DIPLOMAT_INCITE,
  MENU_MISCELLANEOUS_DIPLOMAT_POISON,
  MENU_MISCELLANEOUS_DIPLOMAT_NOTHING,
  MENU_MISCELLANEOUS_DIPLOMAT_IGNORE_ALLIES,
  MENU_MISCELLANEOUS_UNIT_IDLE,
  MENU_MISCELLANEOUS_UNIT_SENTRY,
  MENU_MISCELLANEOUS_UNIT_FORTIFY,
  MENU_MISCELLANEOUS_UNIT_SLEEP,
  MENU_MISCELLANEOUS_UNIT_FORTIFY_OR_SLEEP,
  MENU_MISCELLANEOUS_UNIT_MILITARY,
  MENU_MISCELLANEOUS_UNIT_LOCK,
  MENU_MISCELLANEOUS_TOGGLE_WAKEUP,
  MENU_MISCELLANEOUS_TOGGLE_MOVEANDATTACK,
  MENU_MISCELLANEOUS_MY_AI_EXECUTE,
  MENU_MISCELLANEOUS_MY_AI_FREE,
  MENU_MISCELLANEOUS_MY_AI_EXECUTE_ALL,

  MENU_REPORT_CITIES,
  MENU_REPORT_UNITS,
  MENU_REPORT_PLAYERS,
  MENU_REPORT_ECONOMY,
  MENU_REPORT_SCIENCE,
  MENU_REPORT_WOW,
  MENU_REPORT_TOP_CITIES,
  MENU_REPORT_MESSAGES,
  MENU_REPORT_DEMOGRAPHIC,
  MENU_REPORT_SPACESHIP,

  MENU_HELP_LANGUAGES,
  MENU_HELP_CONNECTING,
  MENU_HELP_CONTROLS,
  MENU_HELP_CHATLINE,
  MENU_HELP_WORKLIST_EDITOR,
  MENU_HELP_CMA,
  MENU_HELP_PLAYING,
  MENU_HELP_IMPROVEMENTS,
  MENU_HELP_UNITS,
  MENU_HELP_COMBAT,
  MENU_HELP_ZOC,
  MENU_HELP_TECH,
  MENU_HELP_TERRAIN,
  MENU_HELP_WONDERS,
  MENU_HELP_GOVERNMENT,
  MENU_HELP_HAPPINESS,
  MENU_HELP_SPACE_RACE,
  MENU_HELP_WARCLIENT,
  MENU_HELP_COPYING,
  MENU_HELP_ABOUT
};


/****************************************************************
  This is the response callback for the dialog with the message:
  Leaving a local game will end it!
****************************************************************/
static void leave_local_game_response(GtkWidget* dialog, gint response)
{
  gtk_widget_destroy(dialog);
  if (response == GTK_RESPONSE_OK) {
    /* It might be killed already */
    if (aconnection.used) {
      /* It will also kill the server */
      disconnect_from_server();
    }
  }
}

/****************************************************************
  ...
 *****************************************************************/
static void game_menu_callback(gpointer callback_data,
    guint callback_action, GtkWidget *widget)
{
  switch(callback_action) {
  case MENU_GAME_OPTIONS:
    popup_option_dialog();
    break;
  case MENU_GAME_MSG_OPTIONS:
    popup_messageopt_dialog();
    break;
  case MENU_GAME_CHATLINE_COLORS:
    popup_chatline_config_dialog();
    break;
  case MENU_GAME_STYLE:
    popup_style_config_dialog();
    break;
  case MENU_GAME_SAVE_SETTINGS:
    save_options();
    break;
  case MENU_GAME_PEPSETTINGS:
    create_pepsetting_dialog();
    break;
  case MENU_GAME_PEPSETTINGS_LOAD:
    load_all_settings();
    init_menus();
    break;
  case MENU_GAME_PEPSETTINGS_SAVE:
    save_all_settings();
    break;
  case MENU_GAME_SERVER_OPTIONS1:
    send_report_request(REPORT_SERVER_OPTIONS1);
    break;
  case MENU_GAME_SERVER_OPTIONS2:
    send_report_request(REPORT_SERVER_OPTIONS2);
    break;
  case MENU_GAME_SAVE_GAME:
    popup_save_dialog();
    break;
  case MENU_GAME_SAVE_QUICK:
    send_save_game(NULL);
    break;
  case MENU_GAME_OUTPUT_LOG:
    log_output_window();
    break;
  case MENU_GAME_CLEAR_OUTPUT:
    clear_output_window();
    break;
  case MENU_GAME_LEAVE:
    if (is_server_running()) {
      GtkWidget* dialog = gtk_message_dialog_new(NULL,
	  0,
	  GTK_MESSAGE_WARNING,
	  GTK_BUTTONS_OK_CANCEL,
	  _("Leaving a local game will end it!"));
      setup_dialog(dialog, toplevel);
      gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
      g_signal_connect(dialog, "response", 
	  G_CALLBACK(leave_local_game_response), NULL);
      gtk_window_present(GTK_WINDOW(dialog));
    } else {
      disconnect_from_server();
    }
    break;
  case MENU_GAME_QUIT:
    popup_quit_dialog();
    break;
  }
}


/****************************************************************
...
*****************************************************************/
static void government_menu_callback(gpointer callback_data,
				  guint callback_action, GtkWidget *widget)
{
  switch(callback_action) {
  case MENU_GOVERNMENT_TAX_RATE:
    popup_rates_dialog();
    break;
  case MENU_GOVERNMENT_FIND_CITY:
    popup_find_dialog();
    break;
  case MENU_GOVERNMENT_WORKLISTS:
  {
    struct unit *punit = get_unit_in_focus();

    if (punit && unit_flag(punit, F_SETTLERS)) {
      key_unit_connect(ACTIVITY_RAILROAD);
    } else {
      popup_worklists_report();
    }
    break;
  }
  case MENU_GOVERNMENT_CLEAR_SELECTED_WORKLISTS:
    clear_worklists_in_selected_cities();
    break;
  case MENU_GOVERNMENT_REVOLUTION:
    popup_revolution_dialog(-1);
    break;
  }
}


static void menus_set_sensitive(const char *, int);
/****************************************************************
...
*****************************************************************/
static void view_menu_callback(gpointer callback_data, guint callback_action,
			       GtkWidget *widget)
{
  switch(callback_action) {
  case MENU_VIEW_SHOW_MAP_GRID:
    if (draw_map_grid ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_map_grid_toggle();
    break;
  case MENU_VIEW_SHOW_NATIONAL_BORDERS:
    if (draw_borders ^ GTK_CHECK_MENU_ITEM(widget)->active) {
      key_map_borders_toggle();
    }
    break;
  case MENU_VIEW_SHOW_CITY_NAMES:
    if (!tiles_hilited_cities) {
    if (draw_city_names ^ GTK_CHECK_MENU_ITEM(widget)->active) {
      key_city_names_toggle();
      menus_set_sensitive("<main>/_View/City G_rowth", draw_city_names);
        menus_set_sensitive("<main>/_View/City _Trade Routes", draw_city_names);
      }
    }
    break;
  case MENU_VIEW_SHOW_CITY_GROWTH_TURNS:
    if (draw_city_growth ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_city_growth_toggle();
    break;
  case MENU_VIEW_SHOW_CITY_PRODUCTIONS:
    if (draw_city_productions ^ GTK_CHECK_MENU_ITEM(widget)->active) {
      key_city_productions_toggle();
      menus_set_sensitive("<main>/_View/City Production Buy _Cost", draw_city_productions);
    }
    break;
  case MENU_VIEW_SHOW_CITY_PRODUCTION_BUY_COST:
    if (draw_city_production_buy_cost ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_city_production_buy_cost_toggle();
    break;
  case MENU_VIEW_SHOW_CITY_TRADEROUTES:
    if (draw_city_traderoutes ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_city_traderoutes_toggle();
    break;
  case MENU_VIEW_SHOW_TERRAIN:
    if (draw_terrain ^ GTK_CHECK_MENU_ITEM(widget)->active) {
      key_terrain_toggle();
      menus_set_sensitive("<main>/View/Coastline", !draw_terrain);
    }
    break;
  case MENU_VIEW_SHOW_COASTLINE:
    if (draw_coastline ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_coastline_toggle();
    break;
  case MENU_VIEW_SHOW_ROADS_RAILS:
    if (draw_roads_rails ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_roads_rails_toggle();
    break;
  case MENU_VIEW_SHOW_IRRIGATION:
    if (draw_irrigation ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_irrigation_toggle();
    break;
  case MENU_VIEW_SHOW_MINES:
    if (draw_mines ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_mines_toggle();
    break;
  case MENU_VIEW_SHOW_FORTRESS_AIRBASE:
    if (draw_fortress_airbase ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_fortress_airbase_toggle();
    break;
  case MENU_VIEW_SHOW_SPECIALS:
    if (draw_specials ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_specials_toggle();
    break;
  case MENU_VIEW_SHOW_POLLUTION:
    if (draw_pollution ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_pollution_toggle();
    break;
  case MENU_VIEW_SHOW_CITIES:
    if (draw_cities ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_cities_toggle();
    break;
  case MENU_VIEW_SHOW_UNITS:
    if (draw_units ^ GTK_CHECK_MENU_ITEM(widget)->active) {
      key_units_toggle();
      menus_set_sensitive("<main>/View/Focus Unit", !draw_units);
    }
    break;
  case MENU_VIEW_SHOW_FOCUS_UNIT:
    if (draw_focus_unit ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_focus_unit_toggle();
    break;
  case MENU_VIEW_SHOW_FOG_OF_WAR:
    if (draw_fog_of_war ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_fog_of_war_toggle();
    break;
  case MENU_VIEW_CENTER_VIEW:
    center_on_unit();
    break;
  }
}


/****************************************************************
...
*****************************************************************/
static void orders_menu_callback(gpointer callback_data,
				 guint callback_action, GtkWidget *widget)
{
  switch(callback_action) {
   case MENU_ORDER_BUILD_CITY:
     /* Also used to build wonder and add to city. */
     key_unit_build(); 
    break;
   case MENU_ORDER_ROAD:
    if (get_unit_in_focus()) {
      if (unit_can_est_traderoute_here(get_unit_in_focus()))
	key_unit_traderoute();
      else
	key_unit_road();
    }
    break;
   case MENU_ORDER_IRRIGATE:
    key_unit_irrigate();
    break;
   case MENU_ORDER_MINE:
    key_unit_mine();
    break;
   case MENU_ORDER_TRANSFORM:
    key_unit_transform();
    break;
   case MENU_ORDER_FORTRESS:
    if (get_unit_in_focus()) {
      if (can_unit_do_activity(get_unit_in_focus(), ACTIVITY_FORTRESS))
	key_unit_fortress();
      else
	key_unit_fortify();
    }
    break;
   case MENU_ORDER_SLEEP:
    key_unit_sleep();
    break;
   case MENU_ORDER_AIRBASE:
    if (get_unit_in_focus()) {
      if (can_unit_do_activity(get_unit_in_focus(), ACTIVITY_AIRBASE))
    key_unit_airbase(); 
      else {
        key_unit_air_patrol();
      }
    }
    break;
   case MENU_ORDER_POLLUTION:
    if (get_unit_in_focus()) {
      if (can_unit_paradrop(get_unit_in_focus()))
	key_unit_paradrop();
      else
	key_unit_pollution();
    }
    break;
   case MENU_ORDER_FALLOUT:
    key_unit_fallout();
    break;
   case MENU_ORDER_SENTRY:
    key_unit_sentry();
    break;
   case MENU_ORDER_PILLAGE:
    key_unit_pillage();
    break;
   case MENU_ORDER_HOMECITY:
    key_unit_homecity();
    break;
   case MENU_ORDER_UNLOAD_TRANSPORTER:
   {
    struct unit *punit = get_unit_in_focus();

    if (punit && get_transporter_occupancy(punit) > 0) {
      key_unit_unload_all();
    } else {
      airlift_queue_clear(0);
    }
    break;
   }
   case MENU_ORDER_LOAD:
	  key_unit_load();
    break;
   case MENU_ORDER_UNLOAD:
	  key_unit_unload();
    break;
   case MENU_ORDER_WAKEUP_OTHERS:
    key_unit_wakeup_others();
    break;
   case MENU_ORDER_AUTO_SETTLER:
    if(get_unit_in_focus())
      enable_auto_mode();
    break;
   case MENU_ORDER_AUTO_EXPLORE:
    key_unit_auto_explore();
    break;
   case MENU_ORDER_CONNECT_ROAD:
    key_unit_connect(ACTIVITY_ROAD);
    break;
   case MENU_ORDER_CONNECT_RAIL:
   {
    struct unit *punit = get_unit_in_focus();

    if (punit && unit_flag(punit, F_SETTLERS)) {
      key_unit_connect(ACTIVITY_RAILROAD);
    } else {
      popup_worklists_report();
    }
    break;
   }
   case MENU_ORDER_CONNECT_IRRIGATE:
    key_unit_connect(ACTIVITY_IRRIGATE);
    break;
   case MENU_ORDER_PATROL:
    key_unit_patrol();
    break;
   case MENU_ORDER_GOTO:
    key_unit_goto();
    break;
   case MENU_ORDER_GOTO_CITY:
    if(get_unit_in_focus())
      popup_goto_dialog();
    break;
   case MENU_ORDER_RETURN:
	   key_unit_return();
    break;
   case MENU_ORDER_DISBAND:
    key_unit_disband();
    break;
   case MENU_ORDER_DIPLOMAT_DLG:
    key_unit_diplomat_actions();
    break;
   case MENU_ORDER_NUKE:
    key_unit_nuke();
    break;
   case MENU_ORDER_WAIT:
    key_unit_wait();
    break;
   case MENU_ORDER_DONE:
    key_unit_done();
    break;
  }
}

/****************************************************************
...
*****************************************************************/
static void delayed_goto_menu_callback(gpointer callback_data,
				 guint callback_action, GtkWidget *widget)
{
  switch(callback_action) {
   case MENU_DELAYED_GOTO_DELAYED_GOTO:
	key_unit_delayed_goto(0);
    break;
   case MENU_DELAYED_GOTO_DELAYED_PARADROP_OR_NUKE:
	key_unit_delayed_goto(1);
    break;
   case MENU_DELAYED_GOTO_DELAYED_AIRLIFT:
    key_unit_delayed_airlift();
    break;
   case MENU_DELAYED_GOTO_EXECUTE_DELAYED_GOTO:
	key_unit_execute_delayed_goto();
    break;
   case MENU_DELAYED_GOTO_ADD_PAUSE:
     key_add_pause_delayed_goto();
    break;
   case MENU_DELAYED_GOTO_CLEAR_DELAYED_ORDERS:
	key_unit_clear_delayed_orders();
    break;
   case MENU_DELAYED_GOTO_GOTO_SINGLE_UNIT:
        delayed_goto_place = PLACE_SINGLE_UNIT;
        break;
   case MENU_DELAYED_GOTO_GOTO_IN_TRANSPORTER:
        delayed_goto_place = PLACE_IN_TRANSPORTER;
        break;
   case MENU_DELAYED_GOTO_GOTO_ON_TILE:
        delayed_goto_place = PLACE_ON_TILE;
        break;
   case MENU_DELAYED_GOTO_GOTO_ON_CONTINENT:
        delayed_goto_place = PLACE_ON_CONTINENT;
        break;
   case MENU_DELAYED_GOTO_GOTO_EVERY_WHERE:
        delayed_goto_place = PLACE_EVERY_WHERE;
        break;
   case MENU_DELAYED_GOTO_GOTO_SAME_TYPE:
        delayed_goto_utype = UTYPE_SAME_TYPE;
        break;
   case MENU_DELAYED_GOTO_GOTO_SAME_MOVE_TYPE:
        delayed_goto_utype = UTYPE_SAME_MOVE_TYPE;
        break;
   case MENU_DELAYED_GOTO_GOTO_ALL:
        delayed_goto_utype = UTYPE_ALL;
        break;
   case MENU_DELAYED_GOTO_OLD_SINGLE:
        menus_set_active("<main>/Delayed Goto/Delayed goto mode/Single unit",
			 TRUE);
        break;
   case MENU_DELAYED_GOTO_OLD_TILE:
        menus_set_active("<main>/Delayed Goto/Delayed goto mode/"
			 "All units on the tile", TRUE);
        menus_set_active("<main>/Delayed Goto/Delayed goto mode/"
			 "Only units with the same type", TRUE);
        break;
   case MENU_DELAYED_GOTO_OLD_CONTINENT:
        menus_set_active("<main>/Delayed Goto/Delayed goto mode/"
			 "All units on the continent", TRUE);
        menus_set_active("<main>/Delayed Goto/Delayed goto mode/"
			 "Only units with the same type", TRUE);
        break;
   case MENU_DELAYED_GOTO_OLD_TILE_ALL_TYPES:
        menus_set_active("<main>/Delayed Goto/Delayed goto mode/"
			 "All units on the tile", TRUE);
        menus_set_active("<main>/Delayed Goto/Delayed goto mode/"
			 "All unit types", TRUE);
        break;
   case MENU_DELAYED_GOTO_INCLUSIVE_ALL:
      if(!!(delayed_goto_inclusive_filter & FILTER_ALL) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_inclusive_filter, FILTER_ALL);
         update_delayed_goto_inclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_INCLUSIVE_NEW:
      if(!!(delayed_goto_inclusive_filter & FILTER_NEW) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_inclusive_filter, FILTER_NEW);
         update_delayed_goto_inclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_INCLUSIVE_FORTIFIED:
      if(!!(delayed_goto_inclusive_filter & FILTER_FORTIFIED) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_inclusive_filter, FILTER_FORTIFIED);
         update_delayed_goto_inclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_INCLUSIVE_SENTRIED:
      if(!!(delayed_goto_inclusive_filter & FILTER_SENTRIED) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_inclusive_filter, FILTER_SENTRIED);
         update_delayed_goto_inclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_INCLUSIVE_VETERAN:
      if(!!(delayed_goto_inclusive_filter & FILTER_VETERAN) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_inclusive_filter, FILTER_VETERAN);
         update_delayed_goto_inclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_INCLUSIVE_AUTO:
      if(!!(delayed_goto_inclusive_filter & FILTER_AUTO) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_inclusive_filter, FILTER_AUTO);
         update_delayed_goto_inclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_INCLUSIVE_IDLE:
      if(!!(delayed_goto_inclusive_filter & FILTER_IDLE) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_inclusive_filter, FILTER_IDLE);
         update_delayed_goto_inclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_INCLUSIVE_ABLE_TO_MOVE:
      if(!!(delayed_goto_inclusive_filter & FILTER_ABLE_TO_MOVE) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_inclusive_filter, FILTER_ABLE_TO_MOVE);
         update_delayed_goto_inclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_INCLUSIVE_MILITARY:
      if(!!(delayed_goto_inclusive_filter & FILTER_MILITARY) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_inclusive_filter, FILTER_MILITARY);
         update_delayed_goto_inclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_INCLUSIVE_OFF:
      if(!!(delayed_goto_inclusive_filter & FILTER_OFF) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_inclusive_filter, FILTER_OFF);
         update_delayed_goto_inclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_EXCLUSIVE_ALL:
      if(!!(delayed_goto_exclusive_filter & FILTER_ALL) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_exclusive_filter, FILTER_ALL);
         update_delayed_goto_exclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_EXCLUSIVE_NEW:
      if(!!(delayed_goto_exclusive_filter & FILTER_NEW) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_exclusive_filter, FILTER_NEW);
         update_delayed_goto_exclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_EXCLUSIVE_FORTIFIED:
      if(!!(delayed_goto_exclusive_filter & FILTER_FORTIFIED) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_exclusive_filter, FILTER_FORTIFIED);
         update_delayed_goto_exclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_EXCLUSIVE_SENTRIED:
      if(!!(delayed_goto_exclusive_filter & FILTER_SENTRIED) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_exclusive_filter, FILTER_SENTRIED);
         update_delayed_goto_exclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_EXCLUSIVE_VETERAN:
      if(!!(delayed_goto_exclusive_filter & FILTER_VETERAN) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_exclusive_filter, FILTER_VETERAN);
         update_delayed_goto_exclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_EXCLUSIVE_AUTO:
      if(!!(delayed_goto_exclusive_filter & FILTER_AUTO) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_exclusive_filter, FILTER_AUTO);
         update_delayed_goto_exclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_EXCLUSIVE_IDLE:
      if(!!(delayed_goto_exclusive_filter & FILTER_IDLE) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_exclusive_filter, FILTER_IDLE);
         update_delayed_goto_exclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_EXCLUSIVE_ABLE_TO_MOVE:
      if(!!(delayed_goto_exclusive_filter & FILTER_ABLE_TO_MOVE) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_exclusive_filter, FILTER_ABLE_TO_MOVE);
         update_delayed_goto_exclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_EXCLUSIVE_MILITARY:
      if(!!(delayed_goto_exclusive_filter & FILTER_MILITARY) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_exclusive_filter, FILTER_MILITARY);
         update_delayed_goto_exclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_EXCLUSIVE_OFF:
      if(!!(delayed_goto_exclusive_filter & FILTER_OFF) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
         filter_change(&delayed_goto_exclusive_filter, FILTER_OFF);
         update_delayed_goto_exclusive_filter_menu();
      }
      break;
   case MENU_DELAYED_GOTO_DG1_SELECT:
      delayed_goto_copy(0, 1);
      update_delayed_goto_menu(1);
      break;
   case MENU_DELAYED_GOTO_DG1_ADD:
      delayed_goto_cat(0, 1);
      update_delayed_goto_menu(1);
      break;
   case MENU_DELAYED_GOTO_DG1_RECORD:
      delayed_goto_move(1 ,0);
      update_delayed_goto_menu(1);
      break;
   case MENU_DELAYED_GOTO_DG1_CLEAR:
      delayed_goto_clear(1);
      update_delayed_goto_menu(1);
      break;
   case MENU_DELAYED_GOTO_DG1_EXECUTE:
      request_unit_execute_delayed_goto(1);
      break;
   case MENU_DELAYED_GOTO_DG2_SELECT:
      delayed_goto_copy(0, 2);
      update_delayed_goto_menu(2);
      break;
   case MENU_DELAYED_GOTO_DG2_ADD:
      delayed_goto_cat(0, 2);
      update_delayed_goto_menu(2);
      break;
   case MENU_DELAYED_GOTO_DG2_RECORD:
      delayed_goto_move(2 ,0);
      update_delayed_goto_menu(2);
      break;
   case MENU_DELAYED_GOTO_DG2_CLEAR:
      delayed_goto_clear(2);
      update_delayed_goto_menu(2);
      break;
   case MENU_DELAYED_GOTO_DG2_EXECUTE:
      request_unit_execute_delayed_goto(2);
      break;
   case MENU_DELAYED_GOTO_DG3_SELECT:
      delayed_goto_move(0, 3);
      update_delayed_goto_menu(3);
      break;
   case MENU_DELAYED_GOTO_DG3_ADD:
      delayed_goto_cat(0, 3);
      update_delayed_goto_menu(3);
      break;
   case MENU_DELAYED_GOTO_DG3_RECORD:
      delayed_goto_move(3 ,0);
      update_delayed_goto_menu(3);
      break;
   case MENU_DELAYED_GOTO_DG3_CLEAR:
      delayed_goto_clear(3);
      update_delayed_goto_menu(3);
      break;
   case MENU_DELAYED_GOTO_DG3_EXECUTE:
      request_unit_execute_delayed_goto(3);
      break;
  }
}

/****************************************************************
...
*****************************************************************/


static void airlift_menu_callback(gpointer callback_data,
				 guint callback_action, GtkWidget *widget)
{
  switch(callback_action) {
   case MENU_AIRLIFT_SET_AIRLIFT_DEST:
        request_auto_airlift_destination_selection();
        break;
   case MENU_AIRLIFT_SET_AIRLIFT_SRC:
        request_auto_airlift_source_selection();
        break;
   case MENU_AIRLIFT_AIRLIFT_SELECT_AIRPORT_CITIES:
        request_auto_airlift_source_selection_with_airport();
        break;
   case MENU_AIRLIFT_CLEAR_AIRLIFT_QUEUE:
   {
        struct unit *punit = get_unit_in_focus();

        if (punit && get_transporter_occupancy(punit) > 0) {
            key_unit_unload_all();
        } else {
            airlift_queue_clear(0);
        }
        break;
   }
   case MENU_AIRLIFT_SHOW_CITIES_IN_AIRLIFT_QUEUE:
        airlift_queue_show(0);
        break;
   case MENU_AIRLIFT_AQ1_SELECT:
      airlift_queue_copy(0, 1);
      update_airlift_menu(1);
      break;
   case MENU_AIRLIFT_AQ1_ADD:
      airlift_queue_cat(0, 1);
      update_airlift_menu(1);
      break;
   case MENU_AIRLIFT_AQ1_RECORD:
      airlift_queue_move(1 ,0);
      update_airlift_menu(1);
      break;
   case MENU_AIRLIFT_AQ1_CLEAR:
      airlift_queue_clear(1);
      update_airlift_menu(1);
      break;
   case MENU_AIRLIFT_AQ1_EXECUTE:
      do_airlift_for(1,NULL);
      break;
   case MENU_AIRLIFT_AQ1_SHOW:
      airlift_queue_show(1);
      break;
   case MENU_AIRLIFT_AQ2_SELECT:
      airlift_queue_copy(0, 2);
      update_airlift_menu(2);
      break;
   case MENU_AIRLIFT_AQ2_ADD:
      airlift_queue_cat(0, 2);
      update_airlift_menu(2);
      break;
   case MENU_AIRLIFT_AQ2_RECORD:
      airlift_queue_move(2 ,0);
      update_airlift_menu(2);
      break;
   case MENU_AIRLIFT_AQ2_CLEAR:
      airlift_queue_clear(2);
      update_airlift_menu(2);
      break;
   case MENU_AIRLIFT_AQ2_EXECUTE:
      do_airlift_for(2,NULL);
      break;
   case MENU_AIRLIFT_AQ2_SHOW:
      airlift_queue_show(2);
      break;
  case MENU_AIRLIFT_AQ3_SELECT:
      airlift_queue_copy(0, 3);
      update_airlift_menu(3);
      break;
   case MENU_AIRLIFT_AQ3_ADD:
      airlift_queue_cat(0, 3);
      update_airlift_menu(3);
      break;
   case MENU_AIRLIFT_AQ3_RECORD:
      airlift_queue_move(3 ,0);
      update_airlift_menu(3);
      break;
   case MENU_AIRLIFT_AQ3_CLEAR:
      airlift_queue_clear(3);
      update_airlift_menu(3);
      break;
   case MENU_AIRLIFT_AQ3_EXECUTE:
      do_airlift_for(3,NULL);
      break;
   case MENU_AIRLIFT_AQ3_SHOW:
      airlift_queue_show(3);
      break;
   case MENU_AIRLIFT_AQ4_SELECT:
      airlift_queue_copy(0, 4);
      update_airlift_menu(4);
      break;
   case MENU_AIRLIFT_AQ4_ADD:
      airlift_queue_cat(0, 4);
      update_airlift_menu(4);
      break;
   case MENU_AIRLIFT_AQ4_RECORD:
      airlift_queue_move(4 ,0);
      update_airlift_menu(4);
      break;
   case MENU_AIRLIFT_AQ4_CLEAR:
      airlift_queue_clear(4);
      update_airlift_menu(4);
      break;
   case MENU_AIRLIFT_AQ4_EXECUTE:
      do_airlift_for(4,NULL);
      break;
   case MENU_AIRLIFT_AQ4_SHOW:
      airlift_queue_show(4);
      break;
   case MENU_AIRLIFT_AQ5_SELECT:
      airlift_queue_copy(0, 5);
      update_airlift_menu(5);
      break;
   case MENU_AIRLIFT_AQ5_ADD:
      airlift_queue_cat(0, 5);
      update_airlift_menu(5);
      break;
   case MENU_AIRLIFT_AQ5_RECORD:
      airlift_queue_move(5 ,0);
      update_airlift_menu(5);
      break;
   case MENU_AIRLIFT_AQ5_CLEAR:
      airlift_queue_clear(5);
      update_airlift_menu(5);
      break;
   case MENU_AIRLIFT_AQ5_EXECUTE:
      do_airlift_for(5,NULL);
      break;
   case MENU_AIRLIFT_AQ5_SHOW:
      airlift_queue_show(5);
      break;
   case MENU_AIRLIFT_AQ6_SELECT:
      airlift_queue_copy(0, 6);
      update_airlift_menu(6);
      break;
   case MENU_AIRLIFT_AQ6_ADD:
      airlift_queue_cat(0, 6);
      update_airlift_menu(6);
      break;
   case MENU_AIRLIFT_AQ6_RECORD:
      airlift_queue_move(6 ,0);
      update_airlift_menu(6);
      break;
   case MENU_AIRLIFT_AQ6_CLEAR:
      airlift_queue_clear(6);
      update_airlift_menu(6);
      break;
   case MENU_AIRLIFT_AQ6_EXECUTE:
      do_airlift_for(6,NULL);
      break;
   case MENU_AIRLIFT_AQ6_SHOW:
      airlift_queue_show(6);
      break;
  }
}

/****************************************************************
...
*****************************************************************/

static void auto_caravan_menu_callback(gpointer callback_data,
				  guint callback_action, GtkWidget *widget)
{
  switch(callback_action) {
    case MENU_AUTO_CARAVAN_MY_AI_TRADE_CITY:
      key_my_ai_trade_city();
      break;
    case MENU_AUTO_CARAVAN_MY_AI_CLEAR_TRADE_CITY:
      clear_my_ai_trade_cities();
      break;
    case MENU_AUTO_CARAVAN_MY_AI_SHOW_TRADE_CITIES:
      show_cities_in_trade_plan();
      break;
    case MENU_AUTO_CARAVAN_MY_AI_SHOW_FREE_SLOTS:
      show_free_slots_in_trade_plan();
      break;
    case MENU_AUTO_CARAVAN_MY_AI_ESTIMATE_TRADE:
      calculate_trade_estimation();
      break;
    case MENU_AUTO_CARAVAN_MY_AI_TRADE_RECALCULATE:
      recalculate_trade_plan();
      break;
    case MENU_AUTO_CARAVAN_MY_AI_TRADE_WITH:
      key_my_ai_trade();
      break;
    case MENU_AUTO_CARAVAN_MY_AI_CARAVAN:
      multi_select_iterate(TRUE,punit)
      {
        my_ai_caravan(punit);
      } multi_select_iterate_end;
      break;
    case MENU_AUTO_CARAVAN_MY_AI_TRADE_EXECUTE:
      my_ai_trade_route_execute_all();
      break;
  }
}

/****************************************************************
...
*****************************************************************/
static void multi_selection_menu_callback(gpointer callback_data,
				 guint callback_action, GtkWidget *widget)
{
  switch(callback_action) {
    case MENU_MULTI_SELECTION_MS_SELECT:
      multi_select_select();
      break;
    case MENU_MULTI_SELECTION_MS_ACTIVE_ALL:
      multi_select_active_all(0);
      break;
    case MENU_MULTI_SELECTION_MS_CLEAR:
      multi_select_clear(0);
      break;
    case MENU_MULTI_SELECTION_MODE_SINGLE_UNIT:
      multi_select_place = PLACE_SINGLE_UNIT;
      break;
    case MENU_MULTI_SELECTION_MODE_IN_TRANSPORTER:
      multi_select_place = PLACE_IN_TRANSPORTER;
      break;
    case MENU_MULTI_SELECTION_MODE_ON_TILE:
      multi_select_place = PLACE_ON_TILE;
      break;
    case MENU_MULTI_SELECTION_MODE_ON_CONTINENT:
      multi_select_place = PLACE_ON_CONTINENT;
      break;
    case MENU_MULTI_SELECTION_MODE_EVERY_WHERE:
      multi_select_place = PLACE_EVERY_WHERE;
      break;
    case MENU_MULTI_SELECTION_MODE_SAME_TYPE:
      multi_select_utype = UTYPE_SAME_TYPE;
      break;
    case MENU_MULTI_SELECTION_MODE_SAME_MOVE_TYPE:
      multi_select_utype = UTYPE_SAME_MOVE_TYPE;
      break;
    case MENU_MULTI_SELECTION_MODE_ALL:
      multi_select_utype = UTYPE_ALL;
      break;
    case MENU_MULTI_SELECTION_INCLUSIVE_ALL:
      if(!!(multi_select_inclusive_filter & FILTER_ALL) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_inclusive_filter, FILTER_ALL);
        update_multi_select_inclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_INCLUSIVE_NEW:
      if(!!(multi_select_inclusive_filter & FILTER_NEW) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_inclusive_filter, FILTER_NEW);
        update_multi_select_inclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_INCLUSIVE_FORTIFIED:
      if(!!(multi_select_inclusive_filter & FILTER_FORTIFIED) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_inclusive_filter, FILTER_FORTIFIED);
        update_multi_select_inclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_INCLUSIVE_SENTRIED:
      if(!!(multi_select_inclusive_filter & FILTER_SENTRIED) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_inclusive_filter, FILTER_SENTRIED);
        update_multi_select_inclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_INCLUSIVE_VETERAN:
      if(!!(multi_select_inclusive_filter & FILTER_VETERAN) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_inclusive_filter, FILTER_VETERAN);
        update_multi_select_inclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_INCLUSIVE_AUTO:
      if(!!(multi_select_inclusive_filter & FILTER_AUTO) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_inclusive_filter, FILTER_AUTO);
        update_multi_select_inclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_INCLUSIVE_IDLE:
      if(!!(multi_select_inclusive_filter & FILTER_IDLE) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_inclusive_filter, FILTER_IDLE);
        update_multi_select_inclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_INCLUSIVE_ABLE_TO_MOVE:
      if(!!(multi_select_inclusive_filter & FILTER_ABLE_TO_MOVE) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_inclusive_filter, FILTER_ABLE_TO_MOVE);
        update_multi_select_inclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_INCLUSIVE_MILITARY:
      if(!!(multi_select_inclusive_filter & FILTER_MILITARY) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_inclusive_filter, FILTER_MILITARY);
        update_multi_select_inclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_INCLUSIVE_OFF:
      if(!!(multi_select_inclusive_filter & FILTER_OFF) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_inclusive_filter, FILTER_OFF);
        update_multi_select_inclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_EXCLUSIVE_ALL:
      if(!!(multi_select_exclusive_filter & FILTER_ALL) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_exclusive_filter, FILTER_ALL);
        update_multi_select_exclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_EXCLUSIVE_NEW:
      if(!!(multi_select_exclusive_filter & FILTER_NEW) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_exclusive_filter, FILTER_NEW);
        update_multi_select_exclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_EXCLUSIVE_FORTIFIED:
      if(!!(multi_select_exclusive_filter & FILTER_FORTIFIED) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_exclusive_filter, FILTER_FORTIFIED);
        update_multi_select_exclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_EXCLUSIVE_SENTRIED:
      if(!!(multi_select_exclusive_filter & FILTER_SENTRIED) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_exclusive_filter, FILTER_SENTRIED);
        update_multi_select_exclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_EXCLUSIVE_VETERAN:
      if(!!(multi_select_exclusive_filter & FILTER_VETERAN) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_exclusive_filter, FILTER_VETERAN);
        update_multi_select_exclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_EXCLUSIVE_AUTO:
      if(!!(multi_select_exclusive_filter & FILTER_AUTO) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_exclusive_filter, FILTER_AUTO);
        update_multi_select_exclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_EXCLUSIVE_IDLE:
      if(!!(multi_select_exclusive_filter & FILTER_IDLE) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_exclusive_filter, FILTER_IDLE);
        update_multi_select_exclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_EXCLUSIVE_ABLE_TO_MOVE:
      if(!!(multi_select_exclusive_filter & FILTER_ABLE_TO_MOVE) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_exclusive_filter, FILTER_ABLE_TO_MOVE);
        update_multi_select_exclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_EXCLUSIVE_MILITARY:
      if(!!(multi_select_exclusive_filter & FILTER_MILITARY) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_exclusive_filter, FILTER_MILITARY);
        update_multi_select_exclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_EXCLUSIVE_OFF:
      if(!!(multi_select_exclusive_filter & FILTER_OFF) ^ GTK_CHECK_MENU_ITEM(widget)->active)
      {
        filter_change(&multi_select_exclusive_filter, FILTER_OFF);
        update_multi_select_exclusive_filter_menu();
        update_unit_info_label(get_unit_in_focus());
      }
      break;
    case MENU_MULTI_SELECTION_MS1_SELECT:
      multi_select_copy(0, 1);
      update_multi_selection_menu(1);
      break;
    case MENU_MULTI_SELECTION_MS1_ADD:
      multi_select_cat(0, 1);
      update_multi_selection_menu(1);
      break;
    case MENU_MULTI_SELECTION_MS1_RECORD:
      multi_select_copy(1, 0);
      update_multi_selection_menu(1);
      break;
    case MENU_MULTI_SELECTION_MS1_CLEAR:
      multi_select_clear(1);
      update_multi_selection_menu(1);
      break;
    case MENU_MULTI_SELECTION_MS2_SELECT:
      multi_select_copy(0, 2);
      update_multi_selection_menu(2);
      break;
    case MENU_MULTI_SELECTION_MS2_ADD:
      multi_select_cat(0, 2);
      update_multi_selection_menu(2);
      break;
    case MENU_MULTI_SELECTION_MS2_RECORD:
      multi_select_copy(2, 0);
      update_multi_selection_menu(2);
      break;
    case MENU_MULTI_SELECTION_MS2_CLEAR:
      multi_select_clear(2);
      update_multi_selection_menu(2);
      break;
    case MENU_MULTI_SELECTION_MS3_SELECT:
      multi_select_copy(0, 3);
      update_multi_selection_menu(3);
      break;
    case MENU_MULTI_SELECTION_MS3_ADD:
      multi_select_cat(0, 3);
      update_multi_selection_menu(3);
      break;
    case MENU_MULTI_SELECTION_MS3_RECORD:
      multi_select_copy(3, 0);
      update_multi_selection_menu(3);
      break;
    case MENU_MULTI_SELECTION_MS3_CLEAR:
      multi_select_clear(3);
      update_multi_selection_menu(3);
      break;
    case MENU_MULTI_SELECTION_MS4_SELECT:
      multi_select_copy(0, 4);
      update_multi_selection_menu(4);
      break;
    case MENU_MULTI_SELECTION_MS4_ADD:
      multi_select_cat(0, 4);
      update_multi_selection_menu(4);
      break;
    case MENU_MULTI_SELECTION_MS4_RECORD:
      multi_select_copy(4, 0);
      update_multi_selection_menu(4);
      break;
    case MENU_MULTI_SELECTION_MS4_CLEAR:
      multi_select_clear(4);
      update_multi_selection_menu(4);
      break;
    case MENU_MULTI_SELECTION_MS5_SELECT:
      multi_select_copy(0, 5);
      update_multi_selection_menu(5);
      break;
    case MENU_MULTI_SELECTION_MS5_ADD:
      multi_select_cat(0, 5);
      update_multi_selection_menu(5);
      break;
    case MENU_MULTI_SELECTION_MS5_RECORD:
      multi_select_copy(5, 0);
      update_multi_selection_menu(5);
      break;
    case MENU_MULTI_SELECTION_MS5_CLEAR:
      multi_select_clear(5);
      update_multi_selection_menu(5);
      break;
    case MENU_MULTI_SELECTION_MS6_SELECT:
      multi_select_copy(0, 6);
      update_multi_selection_menu(6);
      break;
    case MENU_MULTI_SELECTION_MS6_ADD:
      multi_select_cat(0, 6);
      update_multi_selection_menu(6);
      break;
    case MENU_MULTI_SELECTION_MS6_RECORD:
      multi_select_copy(6, 0);
      update_multi_selection_menu(6);
      break;
    case MENU_MULTI_SELECTION_MS6_CLEAR:
      multi_select_clear(6);
      update_multi_selection_menu(6);
      break;
    case MENU_MULTI_SELECTION_MS7_SELECT:
      multi_select_copy(0, 7);
      update_multi_selection_menu(7);
      break;
    case MENU_MULTI_SELECTION_MS7_ADD:
      multi_select_cat(0, 7);
      update_multi_selection_menu(7);
      break;
    case MENU_MULTI_SELECTION_MS7_RECORD:
      multi_select_copy(7, 0);
      update_multi_selection_menu(7);
      break;
    case MENU_MULTI_SELECTION_MS7_CLEAR:
      multi_select_clear(7);
      update_multi_selection_menu(7);
      break;
    case MENU_MULTI_SELECTION_MS8_SELECT:
      multi_select_copy(0, 8);
      update_multi_selection_menu(8);
      break;
    case MENU_MULTI_SELECTION_MS8_ADD:
      multi_select_cat(0, 8);
      update_multi_selection_menu(8);
      break;
    case MENU_MULTI_SELECTION_MS8_RECORD:
      multi_select_copy(8, 0);
      update_multi_selection_menu(8);
      break;
    case MENU_MULTI_SELECTION_MS8_CLEAR:
      multi_select_clear(8);
      update_multi_selection_menu(8);
      break;
    case MENU_MULTI_SELECTION_MS9_SELECT:
      multi_select_copy(0, 9);
      update_multi_selection_menu(9);
      break;
    case MENU_MULTI_SELECTION_MS9_ADD:
      multi_select_cat(0, 9);
      update_multi_selection_menu(9);
      break;
    case MENU_MULTI_SELECTION_MS9_RECORD:
      multi_select_copy(9, 0);
      update_multi_selection_menu(9);
      break;
    case MENU_MULTI_SELECTION_MS9_CLEAR:
      multi_select_clear(9);
      update_multi_selection_menu(9);
      break;
    case MENU_MULTI_SELECTION_MY_AI_SPREAD:
      if (tiles_hilited_cities) {
        key_select_rally_point();
      } else {
        my_ai_spread_execute();
      }
      break;
    case MENU_MULTI_SELECTION_MY_AI_SPREAD_AIRPORT:
      if(spread_airport_cities^GTK_CHECK_MENU_ITEM(widget)->active)
          key_toggle_spread_airport();
        break;
    case MENU_MULTI_SELECTION_MY_AI_SPREAD_ALLY:
      if(spread_allied_cities^GTK_CHECK_MENU_ITEM(widget)->active)
          key_toggle_spread_ally();
        break;
  }
}

/****************************************************************
...
*****************************************************************/

static void miscellaneous_menu_callback(gpointer callback_data,
				 guint callback_action, GtkWidget *widget)
{
  switch(callback_action) {
   case MENU_MISCELLANEOUS_AIR_PATROL:
        if (get_unit_in_focus()) {
            if (can_unit_do_activity(get_unit_in_focus(), ACTIVITY_AIRBASE))
                key_unit_airbase();
            else {
                 key_unit_air_patrol();
            }
        }
        break;
   case MENU_MISCELLANEOUS_AIR_PATROL_DEST:
        key_airplane_patrol();
        break;
   case MENU_MISCELLANEOUS_PATROL_EXECUTE:
        my_ai_patrol_execute_all();
        break; 
   case MENU_MISCELLANEOUS_CARAVAN_POPUP:
        default_caravan_action = 0;
        break;
   case MENU_MISCELLANEOUS_CARAVAN_TRADE:
        default_caravan_action = 1;
        break;
   case MENU_MISCELLANEOUS_CARAVAN_BUILD:
        default_caravan_action = 2;
        break;
   case MENU_MISCELLANEOUS_CARAVAN_NOTHING:
        default_caravan_action = 3;
        break;
   case MENU_MISCELLANEOUS_DIPLOMAT_POPUP:
        default_diplomat_action = 0;
        break;
   case MENU_MISCELLANEOUS_DIPLOMAT_BRIBE:
        default_diplomat_action = 1;
        break;
   case MENU_MISCELLANEOUS_DIPLOMAT_SABOTAGE_UNIT:
        default_diplomat_action = 2;
        break;
   case MENU_MISCELLANEOUS_DIPLOMAT_EMBASSY:
        default_diplomat_action = 3;
        break;
   case MENU_MISCELLANEOUS_DIPLOMAT_INVESTIGATE:
        default_diplomat_action = 4;
        break;
   case MENU_MISCELLANEOUS_DIPLOMAT_SABOTAGE_CITY:
        default_diplomat_action = 5;
        break;
   case MENU_MISCELLANEOUS_DIPLOMAT_STEAL:
        default_diplomat_action = 6;
        break;
   case MENU_MISCELLANEOUS_DIPLOMAT_INCITE:
        default_diplomat_action = 7;
        break;
   case MENU_MISCELLANEOUS_DIPLOMAT_POISON:
        default_diplomat_action = 8;
        break;
   case MENU_MISCELLANEOUS_DIPLOMAT_NOTHING:
        default_diplomat_action = 9;
        break;
   case MENU_MISCELLANEOUS_DIPLOMAT_IGNORE_ALLIES:
        default_diplomat_ignore_allies = GTK_CHECK_MENU_ITEM(widget)->active;
        break;
   case MENU_MISCELLANEOUS_UNIT_IDLE:
        default_action_type = ACTION_IDLE;
        break;
   case MENU_MISCELLANEOUS_UNIT_SENTRY:
        default_action_type = ACTION_SENTRY;
        break;
   case MENU_MISCELLANEOUS_UNIT_FORTIFY:
        default_action_type = ACTION_FORTIFY;
        break;
   case MENU_MISCELLANEOUS_UNIT_SLEEP:
        default_action_type = ACTION_SLEEP;
        break;
   case MENU_MISCELLANEOUS_UNIT_FORTIFY_OR_SLEEP:
        default_action_type = ACTION_FORTIFY_OR_SLEEP;
        break;
   case MENU_MISCELLANEOUS_UNIT_MILITARY:
        default_action_military_only = GTK_CHECK_MENU_ITEM(widget)->active;
        break;
   case MENU_MISCELLANEOUS_UNIT_LOCK:
        default_action_locked = GTK_CHECK_MENU_ITEM(widget)->active;
        break;
   case MENU_MISCELLANEOUS_TOGGLE_WAKEUP:
    if (autowakeup_state ^ GTK_CHECK_MENU_ITEM(widget)->active)
		key_toggle_autowakeup();
	break;
   case MENU_MISCELLANEOUS_TOGGLE_MOVEANDATTACK:
    if (moveandattack_state ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_toggle_moveandattack();
  	break;
   case MENU_MISCELLANEOUS_SET_RALLIES:
        if (tiles_hilited_cities) {
          key_select_rally_point();
        } else {
          my_ai_spread_execute();
        }
        break;
   case MENU_MISCELLANEOUS_CLEAR_RALLIES:
        key_clear_rally_point_for_selected_cities();
        break;
    case MENU_MISCELLANEOUS_MY_AI_EXECUTE:
      multi_select_iterate(TRUE,punit)
      {
        my_ai_unit_execute(punit);
      } multi_select_iterate_end;
      break;
    case MENU_MISCELLANEOUS_MY_AI_FREE:
      unit_list_iterate(multi_select_get_units_focus(), punit) {
        my_ai_orders_free(punit);
      } unit_list_iterate_end;
      break;
    case MENU_MISCELLANEOUS_MY_AI_EXECUTE_ALL:
      my_ai_execute();
      break;
  }
}
/****************************************************************
...
*****************************************************************/
static void reports_menu_callback(gpointer callback_data,
				  guint callback_action, GtkWidget *widget)
{
  switch(callback_action) {
   case MENU_REPORT_CITIES:
    raise_city_report_dialog();
    break;
   case MENU_REPORT_UNITS:
    raise_activeunits_report_dialog();
    break;
  case MENU_REPORT_PLAYERS:
    raise_players_dialog();
    break;
   case MENU_REPORT_ECONOMY:
    raise_economy_report_dialog();
    break;
   case MENU_REPORT_SCIENCE:
    raise_science_dialog();
    break;
   case MENU_REPORT_WOW:
    send_report_request(REPORT_WONDERS_OF_THE_WORLD);
    break;
   case MENU_REPORT_TOP_CITIES:
    send_report_request(REPORT_TOP_5_CITIES);
    break;
  case MENU_REPORT_MESSAGES:
    raise_meswin_dialog();
    break;
   case MENU_REPORT_DEMOGRAPHIC:
    send_report_request(REPORT_DEMOGRAPHIC);
    break;
   case MENU_REPORT_SPACESHIP:
    popup_spaceship_dialog(get_player_ptr());
    break;
  }
}


/****************************************************************
...
*****************************************************************/
static void help_menu_callback(gpointer callback_data,
			       guint callback_action, GtkWidget *widget)
{
  switch(callback_action) {
  case MENU_HELP_LANGUAGES:
    popup_help_dialog_string(HELP_LANGUAGES_ITEM);
    break;
  case MENU_HELP_CONNECTING:
    popup_help_dialog_string(HELP_CONNECTING_ITEM);
    break;
  case MENU_HELP_CONTROLS:
    popup_help_dialog_string(HELP_CONTROLS_ITEM);
    break;
  case MENU_HELP_CHATLINE:
    popup_help_dialog_string(HELP_CHATLINE_ITEM);
    break;
  case MENU_HELP_WORKLIST_EDITOR:
    popup_help_dialog_string(HELP_WORKLIST_EDITOR_ITEM);
    break;
  case MENU_HELP_CMA:
    popup_help_dialog_string(HELP_CMA_ITEM);
    break;
  case MENU_HELP_PLAYING:
    popup_help_dialog_string(HELP_PLAYING_ITEM);
    break;
  case MENU_HELP_WARCLIENT:
    popup_help_dialog_string(HELP_WARCLIENT_ITEM);
    break;
  case MENU_HELP_IMPROVEMENTS:
    popup_help_dialog_string(HELP_IMPROVEMENTS_ITEM);
    break;
  case MENU_HELP_UNITS:
    popup_help_dialog_string(HELP_UNITS_ITEM);
    break;
  case MENU_HELP_COMBAT:
    popup_help_dialog_string(HELP_COMBAT_ITEM);
    break;
  case MENU_HELP_ZOC:
    popup_help_dialog_string(HELP_ZOC_ITEM);
    break;
  case MENU_HELP_TECH:
    popup_help_dialog_string(HELP_TECHS_ITEM);
    break;
  case MENU_HELP_TERRAIN:
    popup_help_dialog_string(HELP_TERRAIN_ITEM);
    break;
  case MENU_HELP_WONDERS:
    popup_help_dialog_string(HELP_WONDERS_ITEM);
    break;
  case MENU_HELP_GOVERNMENT:
    popup_help_dialog_string(HELP_GOVERNMENT_ITEM);
    break;
  case MENU_HELP_HAPPINESS:
    popup_help_dialog_string(HELP_HAPPINESS_ITEM);
    break;
  case MENU_HELP_SPACE_RACE:
    popup_help_dialog_string(HELP_SPACE_RACE_ITEM);
    break;
  case MENU_HELP_COPYING:
    popup_help_dialog_string(HELP_COPYING_ITEM);
    break;
  case MENU_HELP_ABOUT:
    popup_help_dialog_string(HELP_ABOUT_ITEM);
    break;
  }
}

/* This is the GtkItemFactoryEntry structure used to generate new menus.
          Item 1: The menu path. The letter after the underscore indicates an
                  accelerator key once the menu is open.
          Item 2: The accelerator key for the entry
          Item 3: The callback function.
          Item 4: The callback action.  This changes the parameters with
                  which the function is called.  The default is 0.
          Item 5: The item type, used to define what kind of an item it is.
                  Here are the possible values:

                  NULL               -> "<Item>"
                  ""                 -> "<Item>"
                  "<Title>"          -> create a title item
                  "<Item>"           -> create a simple item
                  "<CheckItem>"      -> create a check item
                  "<ToggleItem>"     -> create a toggle item
                  "<RadioItem>"      -> create a radio item
                  <path>             -> path of a radio item to link against
                  "<Separator>"      -> create a separator
                  "<Branch>"         -> create an item to hold sub items
                  "<LastBranch>"     -> create a right justified branch 

Important: The underscore is NOT just for show (see Item 1 above)!
           At the top level, use with "Alt" key to open the menu.
	   Some less often used commands in the Order menu are not underscored
	   due to possible conflicts.
*/
static GtkItemFactoryEntry menu_items[]	=
{
  /* Game menu ... */
  { "/" N_("_Game"),					NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Game") "/tearoff1",				NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("Game") "/" N_("Local _Options"),		NULL,
	game_menu_callback,	MENU_GAME_OPTIONS					},
  { "/" N_("Game") "/" N_("_Message Options"),		NULL,
	game_menu_callback,	MENU_GAME_MSG_OPTIONS					},
  { "/" N_("Game") "/" N_("_Chat Colors"),			NULL,
	game_menu_callback,		MENU_GAME_CHATLINE_COLORS			},
  { "/" N_("Game") "/" N_("_Font Size"),		NULL,
        game_menu_callback,		MENU_GAME_STYLE			},
  { "/" N_("Game") "/" N_("Sa_ve Settings"),		NULL,
	game_menu_callback,	MENU_GAME_SAVE_SETTINGS					},
  { "/" N_("Game") "/sep2",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Game") "/" N_("PepClient _Options"),   	    	NULL,
	game_menu_callback,		MENU_GAME_PEPSETTINGS				},
  { "/" N_("Game") "/" N_("Lo_ad PepClient Settings"),		NULL,
	game_menu_callback,		MENU_GAME_PEPSETTINGS_LOAD			},
  { "/" N_("Game") "/" N_("Sa_ve PepClient Settings"),		NULL,
	game_menu_callback,		MENU_GAME_PEPSETTINGS_SAVE			},
  { "/" N_("Game") "/sep3",					NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Game") "/" N_("_Initial Server Options"),NULL,
	game_menu_callback,	MENU_GAME_SERVER_OPTIONS1				},
  { "/" N_("Game") "/" N_("Server Optio_ns"),			NULL,
	game_menu_callback,	MENU_GAME_SERVER_OPTIONS2				},
  { "/" N_("Game") "/sep4",					NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Game") "/" N_("_Save Game"),		NULL,
	game_menu_callback,	MENU_GAME_SAVE_QUICK, 			"<StockItem>",
	GTK_STOCK_SAVE									},
  { "/" N_("Game") "/" N_("Save Game _As..."),		NULL,
	game_menu_callback,	MENU_GAME_SAVE_GAME,			"<StockItem>",
	GTK_STOCK_SAVE_AS								},
  { "/" N_("Game") "/sep5",					NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Game") "/" N_("E_xport Log"),		NULL,
	game_menu_callback,	MENU_GAME_OUTPUT_LOG					},
  { "/" N_("Game") "/" N_("Clear _Log"),		NULL,
	game_menu_callback,	MENU_GAME_CLEAR_OUTPUT					},
  { "/" N_("Game") "/sep6",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Game") "/" N_("L_eave"),		NULL,
	game_menu_callback,	MENU_GAME_LEAVE					},
  { "/" N_("Game") "/" N_("_Quit"),			NULL,
	game_menu_callback,	MENU_GAME_QUIT,				"<StockItem>",
	GTK_STOCK_QUIT		
							},
  /* Government menu ... */
  { "/" N_("Gov_ernment"),					NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Government") "/tearoff1",			NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("Government") "/" N_("_Tax Rates"),		"<shift>t",
	government_menu_callback,	MENU_GOVERNMENT_TAX_RATE			},
  { "/" N_("Government") "/sep1",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Government") "/" N_("_Find City"),		"<shift>f",
	government_menu_callback,	MENU_GOVERNMENT_FIND_CITY			},
  { "/" N_("Government") "/" N_("_Worklists"),		"<control>l",
	government_menu_callback,	MENU_GOVERNMENT_WORKLISTS			},
  { "/" N_("Government") "/" N_("_Clear Selected Worklists"),	"<control>w",
	government_menu_callback,	MENU_GOVERNMENT_CLEAR_SELECTED_WORKLISTS	},
  { "/" N_("Government") "/sep2",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Government") "/" N_("_Change Government"),           NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Government") "/" N_("Change Government") "/" N_("_Revolution..."),
								NULL,
	government_menu_callback,	MENU_GOVERNMENT_REVOLUTION			},
  { "/" N_("Government") "/" N_("Change Government") "/sep1",	NULL,
	NULL,			0,					"<Separator>"	},

  /* View menu ... */
  { "/" N_("_View"),					NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("View") "/tearoff1",				NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("View") "/" N_("Map _Grid"),			"<control>g",
	view_menu_callback,	MENU_VIEW_SHOW_MAP_GRID,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("National _Borders"),		NULL,
	view_menu_callback,	MENU_VIEW_SHOW_NATIONAL_BORDERS,	"<CheckItem>"	},
  { "/" N_("View") "/" N_("City _Names"),		"<control>n",
	view_menu_callback,	MENU_VIEW_SHOW_CITY_NAMES,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("City G_rowth"),			NULL,
	view_menu_callback,	MENU_VIEW_SHOW_CITY_GROWTH_TURNS,
	"<CheckItem>"	},
  { "/" N_("View") "/" N_("City _Productions"),		"<control>p",
	view_menu_callback,	MENU_VIEW_SHOW_CITY_PRODUCTIONS,	"<CheckItem>"	},
  { "/" N_("View") "/" N_("City Production Buy _Cost"),		NULL,
	view_menu_callback,		MENU_VIEW_SHOW_CITY_PRODUCTION_BUY_COST,
									"<CheckItem>"	},
  { "/" N_("View") "/" N_("City _Trade Routes"),		"<control>t",
	view_menu_callback,		MENU_VIEW_SHOW_CITY_TRADEROUTES,"<CheckItem>"	},
  { "/" N_("View") "/sep1",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("View") "/" N_("Te_rrain"),				NULL,
        view_menu_callback,     MENU_VIEW_SHOW_TERRAIN,	                "<CheckItem>"   },
  { "/" N_("View") "/" N_("C_oastline"),				NULL,
        view_menu_callback,     MENU_VIEW_SHOW_COASTLINE,       	"<CheckItem>"   },
  { "/" N_("View") "/" N_("_Improvements"),			NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("View") "/" N_("Improvements") "/tearoff1",	NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("View") "/" N_("Improvements") "/" N_("_Roads & Rails"),
								NULL,
	view_menu_callback,	MENU_VIEW_SHOW_ROADS_RAILS,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("Improvements") "/" N_("_Irrigation"), NULL,
	view_menu_callback,	MENU_VIEW_SHOW_IRRIGATION,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("Improvements") "/" N_("_Mines"),	NULL,
	view_menu_callback,	MENU_VIEW_SHOW_MINES,			"<CheckItem>"	},
  { "/" N_("View") "/" N_("Improvements") "/" N_("_Fortress & Airbase"),
								NULL,
	view_menu_callback,	MENU_VIEW_SHOW_FORTRESS_AIRBASE,	"<CheckItem>"	},
  { "/" N_("View") "/" N_("_Specials"),				NULL,
	view_menu_callback,	MENU_VIEW_SHOW_SPECIALS,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("Poll_ution & Fallout"),		NULL,
	view_menu_callback,	MENU_VIEW_SHOW_POLLUTION,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("C_ities"),				NULL,
	view_menu_callback,	MENU_VIEW_SHOW_CITIES,			"<CheckItem>"	},
  { "/" N_("View") "/" N_("_Units"),				NULL,
	view_menu_callback,	MENU_VIEW_SHOW_UNITS,			"<CheckItem>"	},
  { "/" N_("View") "/" N_("_Focus Unit"),			NULL,
	view_menu_callback,	MENU_VIEW_SHOW_FOCUS_UNIT,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("Fog of _War"),			NULL,
	view_menu_callback,	MENU_VIEW_SHOW_FOG_OF_WAR,		"<CheckItem>"	},
  { "/" N_("View") "/sep2",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("View") "/" N_("Center _View"),			"c",
	view_menu_callback,	MENU_VIEW_CENTER_VIEW					},

  /* Orders menu ... */
  { "/" N_("_Orders"),					NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Orders") "/tearoff1",			NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("Orders") "/" N_("_Build City"),		"b",
	orders_menu_callback,	MENU_ORDER_BUILD_CITY					},
  { "/" N_("Orders") "/" N_("Build _Road"),		"r",
	orders_menu_callback,	MENU_ORDER_ROAD						},
  { "/" N_("Orders") "/" N_("Build _Irrigation"),	"i",
	orders_menu_callback,	MENU_ORDER_IRRIGATE					},
  { "/" N_("Orders") "/" N_("Build _Mine"),		"m",
	orders_menu_callback,	MENU_ORDER_MINE						},
  { "/" N_("Orders") "/" N_("Transf_orm Terrain"),	"o",
	orders_menu_callback,	MENU_ORDER_TRANSFORM					},
  { "/" N_("Orders") "/" N_("Build _Fortress"),		"f",
	orders_menu_callback,	MENU_ORDER_FORTRESS					},
  { "/" N_("Orders") "/" N_("Build Airbas_e"),		"e",
	orders_menu_callback,	MENU_ORDER_AIRBASE					},
  { "/" N_("Orders") "/" N_("Clean _Pollution"),	"p",
	orders_menu_callback,	MENU_ORDER_POLLUTION					},
  { "/" N_("Orders") "/" N_("Clean _Nuclear Fallout"),	"n",
	orders_menu_callback,	MENU_ORDER_FALLOUT					},
  { "/" N_("Orders") "/sep1",			NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Orders") "/" N_("_Sentry"),			"s",
	orders_menu_callback,	MENU_ORDER_SENTRY					},
  { "/" N_("Orders") "/" N_("Sleep"),				"k",
	orders_menu_callback,		MENU_ORDER_SLEEP				},
  { "/" N_("Orders") "/" N_("Pi_llage"),				"<shift>p",
	orders_menu_callback,	MENU_ORDER_PILLAGE					},
  { "/" N_("Orders") "/sep2",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Orders") "/" N_("Make _Homecity"),		"h",
	orders_menu_callback,	MENU_ORDER_HOMECITY					},
  { "/" N_("Orders") "/" N_("_Load"),			"l",
    orders_menu_callback, MENU_ORDER_LOAD},
  { "/" N_("Orders") "/" N_("_Unload Transporter"),	"<shift>u",
	orders_menu_callback,	MENU_ORDER_UNLOAD_TRANSPORTER					},
  { "/" N_("Orders") "/" N_("_Unload"),			"u",
    orders_menu_callback, MENU_ORDER_UNLOAD},
  { "/" N_("Orders") "/" N_("Wake up o_thers"),		"<shift>w",
	orders_menu_callback,	MENU_ORDER_WAKEUP_OTHERS				},
  { "/" N_("Orders") "/sep3",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Orders") "/" N_("_Auto Settler"),		"a",
	orders_menu_callback,	MENU_ORDER_AUTO_SETTLER					},
  { "/" N_("Orders") "/" N_("Auto E_xplore"),		"x",
	orders_menu_callback,	MENU_ORDER_AUTO_EXPLORE					},
  { "/" N_("Orders") "/" N_("_Connect"),		NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Orders") "/" N_("Connect") "/" N_("Road"), 		"<ctrl>r",
   orders_menu_callback, MENU_ORDER_CONNECT_ROAD},
  { "/" N_("Orders") "/" N_("Connect") "/" N_("Rail"),	 	"<ctrl>l",
   orders_menu_callback, MENU_ORDER_CONNECT_RAIL},
  { "/" N_("Orders") "/" N_("Connect") "/" N_("Irrigate"), 	"<ctrl>i",
   orders_menu_callback, MENU_ORDER_CONNECT_IRRIGATE},
  { "/" N_("Orders") "/" N_("Patrol (_Q)"),		"q",
	orders_menu_callback,	MENU_ORDER_PATROL					},
  { "/" N_("Orders") "/" N_("_Go to"),			"g",
	orders_menu_callback,	MENU_ORDER_GOTO						},
  { "/" N_("Orders") "/" N_("Go\\/_Airlift to City"),		"<shift>l",
	orders_menu_callback,	MENU_ORDER_GOTO_CITY					},
  { "/" N_("Orders") "/" N_("Ret_urn to nearest city"),		"<shift>g",
	orders_menu_callback,	MENU_ORDER_RETURN },
  { "/" N_("Orders") "/sep4",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Orders") "/" N_("Disb_and Unit"),			"<shift>d",
	orders_menu_callback,	MENU_ORDER_DISBAND					},
  { "/" N_("Orders") "/" N_("D_iplomat\\/Spy Actions"),		"d",
	orders_menu_callback,	MENU_ORDER_DIPLOMAT_DLG					},
  { "/" N_("Orders") "/" N_("Explode _Nuclear"),		"<shift>n",
	orders_menu_callback,	MENU_ORDER_NUKE						},
  { "/" N_("Orders") "/sep5",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Orders") "/" N_("_Wait"),			"w",
	orders_menu_callback,	MENU_ORDER_WAIT						},
  { "/" N_("Orders") "/" N_("Don_e"),				"space",
	orders_menu_callback,	MENU_ORDER_DONE						},

 /* Delayed Goto menu ...*/
  { "/" N_("_Delayed Goto"),					NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Delayed Goto") "/tearoff1",				NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed go_to"),		"z",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DELAYED_GOTO			},
  { "/" N_("Delayed Goto") "/" N_("Delayed pa_radrop or nuke"),	"<ctrl>z",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DELAYED_PARADROP_OR_NUKE	},
  { "/" N_("Delayed Goto") "/" N_("Delayed _airlift"),		"<ctrl>y",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DELAYED_AIRLIFT		},
  { "/" N_("Delayed Goto") "/" N_("_Add Pause"),		       	"<ctrl>b",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_ADD_PAUSE		},
  { "/" N_("Delayed Goto") "/" N_("_Execute delayed goto"),	"y",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_EXECUTE_DELAYED_GOTO		},
  { "/" N_("Delayed Goto") "/" N_("_Clear delayed orders"),	"u",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_CLEAR_DELAYED_ORDERS		},
  { "/" N_("Delayed Goto") "/sep1",				NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto _mode"),		NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/" N_("_Single unit"),
								"<alt><shift>F1",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_GOTO_SINGLE_UNIT,
									"<RadioItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/" N_("All _units in the same transporter"),
								"<alt><shift>F2",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_GOTO_IN_TRANSPORTER,
				"<main>/Delayed Goto/Delayed goto mode/Single unit"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/" N_("All units on the _tile"),
								"<alt><shift>F3",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_GOTO_ON_TILE,
				"<main>/Delayed Goto/Delayed goto mode/Single unit"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/" N_("All units on the _continent"),
								"<alt><shift>F4",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_GOTO_ON_CONTINENT,
				"<main>/Delayed Goto/Delayed goto mode/Single unit"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/" N_("_All units"),
								"<alt><shift>F5",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_GOTO_EVERY_WHERE,
				"<main>/Delayed Goto/Delayed goto mode/Single unit"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/sep1",	NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/" N_("Only units with the _same type"),
								"<alt><shift>F6",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_GOTO_SAME_TYPE,
									"<RadioItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/" N_("Only units with the same _move type"),
								"<alt><shift>F7",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_GOTO_SAME_MOVE_TYPE,
		"<main>/Delayed Goto/Delayed goto mode/Only units with the same type"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/" N_("_All unit types"),
								"<alt><shift>F8",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_GOTO_ALL,
		"<main>/Delayed Goto/Delayed goto mode/Only units with the same type"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/sep2",	NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/" N_("Single unit (old)"),
								"F9",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_OLD_SINGLE			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/" N_("All units of the same type on the tile (old)"),
								"F10",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_OLD_TILE			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/" N_("All units on the tile (old)"),
								"F11",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_OLD_TILE_ALL_TYPES		},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto mode") "/" N_("All units of the same type on the continent (old)"),
								"F12",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_OLD_CONTINENT			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto _auto"),	NULL,
	NULL,				0,				"<Branch>"      },
  { "/" N_("Delayed Goto") "/" N_("Delayed goto auto") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},

  { "/" N_("Delayed Goto") "/" N_("_Inclusive filter"),		NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Delayed Goto")  "/" N_("Inclusive filter") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Delayed Goto") "/" N_("Inclusive filter") "/" N_("_All units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_INCLUSIVE_ALL,"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Inclusive filter") "/" N_("_New units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_INCLUSIVE_NEW,"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Inclusive filter") "/" N_("_Fortified units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_INCLUSIVE_FORTIFIED,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Inclusive filter") "/" N_("_Sentried units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_INCLUSIVE_SENTRIED,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Inclusive filter") "/" N_("_Veteran units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_INCLUSIVE_VETERAN,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Inclusive filter") "/" N_("_Auto units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_INCLUSIVE_AUTO,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Inclusive filter") "/" N_("_Idle units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_INCLUSIVE_IDLE,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Inclusive filter") "/" N_("Units abl_e to move"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_INCLUSIVE_ABLE_TO_MOVE,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Inclusive filter") "/" N_("_Military units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_INCLUSIVE_MILITARY,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Inclusive filter") "/" N_("_Off"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_INCLUSIVE_OFF,"<CheckItem>"	},
	
  { "/" N_("Delayed Goto") "/" N_("_Exclusive filter"),		NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Delayed Goto")  "/" N_("Exclusive filter") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Delayed Goto") "/" N_("Exclusive filter") "/" N_("_All units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_EXCLUSIVE_ALL,"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Exclusive filter") "/" N_("_New units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_EXCLUSIVE_NEW,"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Exclusive filter") "/" N_("_Fortified units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_EXCLUSIVE_FORTIFIED,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Exclusive filter") "/" N_("_Sentried units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_EXCLUSIVE_SENTRIED,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Exclusive filter") "/" N_("_Veteran units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_EXCLUSIVE_VETERAN,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Exclusive filter") "/" N_("_Auto units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_EXCLUSIVE_AUTO,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Exclusive filter") "/" N_("_Idle units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_EXCLUSIVE_IDLE,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Exclusive filter") "/" N_("Units abl_e to move"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_EXCLUSIVE_ABLE_TO_MOVE,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Exclusive filter") "/" N_("_Military units"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_EXCLUSIVE_MILITARY,
									"<CheckItem>"	},
  { "/" N_("Delayed Goto") "/" N_("Exclusive filter") "/" N_("_Off"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_EXCLUSIVE_OFF,
									"<CheckItem>"	},

  { "/" N_("Delayed Goto") "/sep2",				NULL,
	NULL,				0,				"<Separator>"	},

  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " _1",
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 1/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 1/" N_("_Select"),
								"<alt>1",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG1_SELECT			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 1/" N_("_Add to current selection"),
								"<shift><alt>1",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG1_ADD			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 1/" N_("_Record"),
								"<ctrl><alt>1",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG1_RECORD			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 1/" N_("_Clear"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG1_CLEAR			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 1/" N_("_Execute"),
								"<ctrl><shift><alt>1",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG1_EXECUTE			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 1/" N_("_Automatic execution"),
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 1/" N_("Automatic execution") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},

  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " _2",
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 2/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 2/" N_("_Select"),
								"<alt>2",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG2_SELECT			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 2/" N_("_Add to current selection"),
								"<shift><alt>2",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG2_ADD			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 2/" N_("_Record"),
								"<ctrl><alt>2",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG2_RECORD			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 2/" N_("_Clear"),
			       					NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG2_CLEAR			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 2/" N_("_Execute"),
								"<ctrl><shift><alt>2",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG2_EXECUTE			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 2/" N_("_Automatic execution"),
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 2/" N_("Automatic execution") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},

  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " _3",
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 3/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 3/" N_("_Select"),
								"<alt>3",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG3_SELECT			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 3/" N_("_Add to current selection"),
								"<shift><alt>3",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG3_ADD			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 3/" N_("_Record"),
								"<ctrl><alt>3",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG3_RECORD			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 3/" N_("_Clear"),
								NULL,
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG3_CLEAR			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 3/" N_("_Execute"),
								"<ctrl><shift><alt>3",
	delayed_goto_menu_callback,	MENU_DELAYED_GOTO_DG3_EXECUTE			},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 3/" N_("_Automatic execution"),
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Delayed Goto") "/" N_("Delayed goto selection") " 3/" N_("Automatic execution") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},

 /* Airlift menu ...*/
  { "/" N_("A_irlift"),						NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Airlift") "/tearoff1",				NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Airlift") "/" N_("Airlift unit _type"),		NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift unit type") "/tearoff1",	NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Airlift") "/sep1",					NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Airlift") "/" N_("Set airlift _destination"),	"<shift>y",
	airlift_menu_callback,		MENU_AIRLIFT_SET_AIRLIFT_DEST			},
  { "/" N_("Airlift") "/" N_("Set airlift _source"),		"<shift>z",
	airlift_menu_callback,		MENU_AIRLIFT_SET_AIRLIFT_SRC			},
  { "/" N_("Airlift") "/" N_("_Add all cities with airport in airlift queue"),
								"<ctrl><shift>z",
	airlift_menu_callback,		MENU_AIRLIFT_AIRLIFT_SELECT_AIRPORT_CITIES	},
  { "/" N_("Airlift") "/" N_("_Clear airlift queue"),		"<shift>u",
	airlift_menu_callback,		MENU_AIRLIFT_CLEAR_AIRLIFT_QUEUE		},
  { "/" N_("Airlift") "/" N_("Show cities in airlift _queue"),	NULL,
	airlift_menu_callback,		MENU_AIRLIFT_SHOW_CITIES_IN_AIRLIFT_QUEUE	},

  { "/" N_("Airlift") "/sep2",					NULL,
	NULL,				0,				"<Separator>"	},

  { "/" N_("Airlift") "/" N_("Airlift queue") " _4",		NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 4/tearoff1",	NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 4/" N_("_Select"),
								"<alt>4",
	airlift_menu_callback,		MENU_AIRLIFT_AQ1_SELECT				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 4/" N_("_Add to current selection"),
								"<shift><alt>4",
	airlift_menu_callback,		MENU_AIRLIFT_AQ1_ADD				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 4/" N_("_Record"),
								"<ctrl><alt>4",
	airlift_menu_callback,		MENU_AIRLIFT_AQ1_RECORD				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 4/" N_("_Clear"),NULL,
	airlift_menu_callback,		MENU_AIRLIFT_AQ1_CLEAR				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 4/" N_("_Execute"),
								"<ctrl><shift><alt>4",
	airlift_menu_callback,		MENU_AIRLIFT_AQ1_EXECUTE			},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 4/" N_("_Show"),	NULL,
	airlift_menu_callback,		MENU_AIRLIFT_AQ1_SHOW				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 4/" N_("Airlift unit _type"),
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 4/" N_("Airlift unit type") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},

  { "/" N_("Airlift") "/" N_("Airlift queue") " _5",		NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 5/tearoff1",			NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 5/" N_("_Select"),
								"<alt>5",
	airlift_menu_callback,		MENU_AIRLIFT_AQ2_SELECT				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 5/" N_("_Add to current selection"),
								"<shift><alt>5",
	airlift_menu_callback,		MENU_AIRLIFT_AQ2_ADD				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 5/" N_("_Record"),
								"<ctrl><alt>5",
	airlift_menu_callback,		MENU_AIRLIFT_AQ2_RECORD				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 5/" N_("_Clear"),NULL,
	airlift_menu_callback,		MENU_AIRLIFT_AQ2_CLEAR				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 5/" N_("_Execute"),
								"<ctrl><shift><alt>5",
	airlift_menu_callback,		MENU_AIRLIFT_AQ2_EXECUTE			},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 5/" N_("_Show"),	NULL,
	airlift_menu_callback,		MENU_AIRLIFT_AQ2_SHOW				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 5/" N_("Airlift unit _type"),
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 5/" N_("Airlift unit type") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},

  { "/" N_("Airlift") "/" N_("Airlift queue") " _6",		NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 6/tearoff1",	NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 6/" N_("_Select"),
								"<alt>6",
	airlift_menu_callback,		MENU_AIRLIFT_AQ3_SELECT				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 6/" N_("_Add to current selection"),
								"<shift><alt>6",
	airlift_menu_callback,		MENU_AIRLIFT_AQ3_ADD				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 6/" N_("_Record"),
								"<ctrl><alt>6",
	airlift_menu_callback,		MENU_AIRLIFT_AQ3_RECORD				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 6/" N_("_Clear"),NULL,
	airlift_menu_callback,		MENU_AIRLIFT_AQ3_CLEAR				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 6/" N_("_Execute"),
								"<ctrl><shift><alt>6",
	airlift_menu_callback,		MENU_AIRLIFT_AQ3_EXECUTE			},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 6/" N_("_Show"),	NULL,
	airlift_menu_callback,		MENU_AIRLIFT_AQ3_SHOW				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 6/" N_("Airlift unit _type"),
								NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 6/" N_("Airlift unit type") "/tearoff1",
								NULL,
	NULL,			0,					"<Tearoff>"	},

  { "/" N_("Airlift") "/" N_("Airlift queue") " _7",		NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 7/tearoff1",	NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 7/" N_("_Select"),
								"<alt>7",
	airlift_menu_callback,		MENU_AIRLIFT_AQ4_SELECT				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 7/" N_("_Add to current selection"),
								"<shift><alt>7",
	airlift_menu_callback,		MENU_AIRLIFT_AQ4_ADD				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 7/" N_("_Record"),
								"<ctrl><alt>7",
	airlift_menu_callback,		MENU_AIRLIFT_AQ4_RECORD				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 7/" N_("_Clear"),NULL,
	airlift_menu_callback,		MENU_AIRLIFT_AQ4_CLEAR				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 7/" N_("_Execute"),
								"<ctrl><shift><alt>7",
	airlift_menu_callback,		MENU_AIRLIFT_AQ4_EXECUTE			},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 7/" N_("_Show"),	NULL,
	airlift_menu_callback,		MENU_AIRLIFT_AQ4_SHOW				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 7/" N_("Airlift unit _type"),
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 7/" N_("Airlift unit type") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},

  { "/" N_("Airlift") "/" N_("Airlift queue") " _8",		NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 8/tearoff1",	NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 8/" N_("_Select"),
								"<alt>8",
	airlift_menu_callback,		MENU_AIRLIFT_AQ5_SELECT			       	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 8/" N_("_Add to current selection"),
								"<shift><alt>8",
	airlift_menu_callback,		MENU_AIRLIFT_AQ5_ADD				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 8/" N_("_Record"),
			       					"<ctrl><alt>8",
	airlift_menu_callback,		MENU_AIRLIFT_AQ5_RECORD				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 8/" N_("_Clear"),NULL,
	airlift_menu_callback,		MENU_AIRLIFT_AQ5_CLEAR				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 8/" N_("_Execute"),
								"<ctrl><shift><alt>8",
	airlift_menu_callback,		MENU_AIRLIFT_AQ5_EXECUTE			},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 8/" N_("_Show"),	NULL,
	airlift_menu_callback,		MENU_AIRLIFT_AQ5_SHOW				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 8/" N_("Airlift unit _type"),
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 8/" N_("Airlift unit type") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},

  { "/" N_("Airlift") "/" N_("Airlift queue") " _9",		NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 9/tearoff1",	NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 9/" N_("_Select"),
								"<alt>9",
	airlift_menu_callback,		MENU_AIRLIFT_AQ6_SELECT				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 9/" N_("_Add to current selection"),
								"<shift><alt>9",
	airlift_menu_callback,		MENU_AIRLIFT_AQ6_ADD				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 9/" N_("_Record"),
								"<ctrl><alt>9",
	airlift_menu_callback,		MENU_AIRLIFT_AQ6_RECORD				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 9/" N_("_Clear"),NULL,
	airlift_menu_callback,		MENU_AIRLIFT_AQ6_CLEAR				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 9/" N_("_Execute"),
								"<ctrl><shift><alt>9",
	airlift_menu_callback,		MENU_AIRLIFT_AQ6_EXECUTE			},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 9/" N_("_Show"),	NULL,
	airlift_menu_callback,		MENU_AIRLIFT_AQ6_SHOW				},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 9/" N_("Airlift unit _type"),
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Airlift") "/" N_("Airlift queue") " 9/" N_("Airlift unit type") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},

 /* Auto Caravan menu ...*/
  { "/" N_("Auto _Caravan"),					NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Auto Caravan") "/tearoff1",				NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Auto Caravan") "/" N_("_Add a city in trade plan"),	"<shift>a",
	auto_caravan_menu_callback,	MENU_AUTO_CARAVAN_MY_AI_TRADE_CITY		},
  { "/" N_("Auto Caravan") "/" N_("_Clear city list for trade plan"),
								NULL,
	auto_caravan_menu_callback,	MENU_AUTO_CARAVAN_MY_AI_CLEAR_TRADE_CITY	},
  { "/" N_("Auto Caravan") "/" N_("_Recalculate trade plan"),	NULL,
	auto_caravan_menu_callback,	MENU_AUTO_CARAVAN_MY_AI_TRADE_RECALCULATE	},
  { "/" N_("Auto Caravan") "/sep1",				NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Auto Caravan") "/" N_("Show trade _estimation"),	NULL,
	auto_caravan_menu_callback,	MENU_AUTO_CARAVAN_MY_AI_ESTIMATE_TRADE		},
  { "/" N_("Auto Caravan") "/" N_("Show the trade route _free slots"),
								NULL,
	auto_caravan_menu_callback,	MENU_AUTO_CARAVAN_MY_AI_SHOW_FREE_SLOTS		},
  { "/" N_("Auto Caravan") "/" N_("Show cities in _trade plan"),	NULL,
	auto_caravan_menu_callback,	MENU_AUTO_CARAVAN_MY_AI_SHOW_TRADE_CITIES	},
  { "/" N_("Auto Caravan") "/sep2",				NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Auto Caravan") "/" N_("_Set caravan destination"),	"j",
	auto_caravan_menu_callback,	MENU_AUTO_CARAVAN_MY_AI_TRADE_WITH		},
  { "/" N_("Auto Caravan") "/" N_("Auto_matic caravan orders"),	"<ctrl>j",
	auto_caravan_menu_callback,	MENU_AUTO_CARAVAN_MY_AI_CARAVAN	},
  { "/" N_("Auto Caravan") "/" N_("_Execute all trade route orders"), "<shift>j",
	auto_caravan_menu_callback,	MENU_AUTO_CARAVAN_MY_AI_TRADE_EXECUTE	},

 /* Multi-Selection menu ...*/
  { "/" N_("_Multi-Selection"),					NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/tearoff1",			NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection _select"),	"<shift>c",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS_SELECT	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection _active all units"),
								"<shift>b",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS_ACTIVE_ALL},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection _clear"),	"<shift>v",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS_CLEAR	},
  { "/" N_("Multi-Selection") "/sep1",				NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection _mode"),	NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection mode") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection mode") "/" N_("_Single unit"),
								"<shift>F1",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MODE_SINGLE_UNIT,
									"<RadioItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection mode") "/" N_("All _units in the same transporter"),
								"<shift>F2",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MODE_IN_TRANSPORTER,
			"<main>/Multi-Selection/Multi-selection mode/Single unit"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection mode") "/" N_("All units on the _tile"),
								"<shift>F3",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MODE_ON_TILE,
			"<main>/Multi-Selection/Multi-selection mode/Single unit"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection mode") "/" N_("All units on the _continent"),
								"<shift>F4",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MODE_ON_CONTINENT,
			"<main>/Multi-Selection/Multi-selection mode/Single unit"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection mode") "/" N_("_All units"),
								"<shift>F5",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MODE_EVERY_WHERE,
			"<main>/Multi-Selection/Multi-selection mode/Single unit"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection mode") "/sep1",
								NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection mode") "/" N_("Only units with the same _type"),
								"<shift>F6",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MODE_SAME_TYPE,
									"<RadioItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection mode") "/" N_("Only units with the same _move type"),
								"<shift>F7",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MODE_SAME_MOVE_TYPE,
	"<main>/Multi-Selection/Multi-selection mode/Only units with the same type"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection mode") "/" N_("_All unit types"),
								"<shift>F8",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MODE_ALL,
	"<main>/Multi-Selection/Multi-selection mode/Only units with the same type"	},

  { "/" N_("Multi-Selection") "/" N_("_Inclusive filter"),	NULL,
		NULL,			0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/" N_("Inclusive filter") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Inclusive filter") "/" N_("_All units"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_INCLUSIVE_ALL,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Inclusive filter") "/" N_("_New units"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_INCLUSIVE_NEW,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Inclusive filter") "/" N_("_Fortified units"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_INCLUSIVE_FORTIFIED,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Inclusive filter") "/" N_("_Sentried units"),
			       					NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_INCLUSIVE_SENTRIED,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Inclusive filter") "/" N_("_Veteran units"),
			       					NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_INCLUSIVE_VETERAN,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Inclusive filter") "/" N_("Aut_o units"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_INCLUSIVE_AUTO,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Inclusive filter") "/" N_("_Idle units"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_INCLUSIVE_IDLE,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Inclusive filter") "/" N_("Units abl_e to move"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_INCLUSIVE_ABLE_TO_MOVE,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Inclusive filter") "/" N_("_Military units"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_INCLUSIVE_MILITARY,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Inclusive filter") "/" N_("_Off"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_INCLUSIVE_OFF,
									"<CheckItem>"	},
	
  { "/" N_("Multi-Selection") "/" N_("_Exclusive filter"),	NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/" N_("Exclusive filter") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Exclusive filter") "/" N_("_All units"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_EXCLUSIVE_ALL,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Exclusive filter") "/" N_("_New units"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_EXCLUSIVE_NEW,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Exclusive filter") "/" N_("_Fortified units"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_EXCLUSIVE_FORTIFIED,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Exclusive filter") "/" N_("_Sentried units"),
		      						NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_EXCLUSIVE_SENTRIED,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Exclusive filter") "/" N_("_Veteran units"),
		       						NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_EXCLUSIVE_VETERAN,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Exclusive filter") "/" N_("Aut_o units"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_EXCLUSIVE_AUTO,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Exclusive filter") "/" N_("_Idle units"),
		       						NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_EXCLUSIVE_IDLE,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Exclusive filter") "/" N_("Units able to mov_e"),
		       						NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_EXCLUSIVE_ABLE_TO_MOVE,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Exclusive filter") "/" N_("_Military units"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_EXCLUSIVE_MILITARY,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Exclusive filter") "/" N_("_Off"),
			       					NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_EXCLUSIVE_OFF,
									"<CheckItem>"	},

  { "/" N_("Multi-Selection") "/sep2",				NULL,
	NULL,				0,				"<Separator>"	},

  { "/" N_("Multi-Selection") "/" N_("Multi-selection sprea_d"),"<shift>s",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MY_AI_SPREAD},
  { "/" N_("Multi-Selection") "/" N_("Spread only in cities _with airport"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MY_AI_SPREAD_AIRPORT,
									"<CheckItem>"	},
  { "/" N_("Multi-Selection") "/" N_("Allow spreadi_ng into allied cities"),
								NULL,
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MY_AI_SPREAD_ALLY,
									"<CheckItem>"	},

  { "/" N_("Multi-Selection") "/sep3",				NULL,
	NULL,				0,				"<Separator>"	},

  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " _1",	NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 1/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 1/" N_("_Select"),
								"1",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS1_SELECT			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 1/" N_("_Add to current selection"),			
								"<shift>1",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS1_ADD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 1/" N_("_Record"),
								"<ctrl>1",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS1_RECORD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 1/" N_("_Clear"),
								"<ctrl><shift>1",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS1_CLEAR			},

  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " _2",	NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 2/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 2/" N_("_Select"),
								"2",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS2_SELECT			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 2/" N_("_Add to current selection"),
								"<shift>2",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS2_ADD	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 2/" N_("_Record"),
								"<ctrl>2",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS2_RECORD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 2/" N_("_Clear"),
								"<ctrl><shift>2",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS2_CLEAR			},

  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " _3",	NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 3/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 3/" N_("_Select"),
							"3",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS3_SELECT			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 3/" N_("_Add to current selection"),
								"<shift>3",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS3_ADD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 3/" N_("_Record"),
								"<ctrl>3",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS3_RECORD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 3/" N_("_Clear"),
								"<ctrl><shift>3",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS3_CLEAR			},

  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " _4",	NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 4/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 4/" N_("_Select"),
								"4",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS4_SELECT			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 4/" N_("_Add to current selection"),
								"<shift>4",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS4_ADD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 4/" N_("_Record"),
								"<ctrl>4",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS4_RECORD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 4/" N_("_Clear"),
								"<ctrl><shift>4",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS4_CLEAR			},

  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " _5",	NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 5/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 5/" N_("_Select"),
								"5",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS5_SELECT			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 5/" N_("_Add to current selection"),
								"<shift>5",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS5_ADD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 5/" N_("_Record"),
								"<ctrl>5",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS5_RECORD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 5/" N_("_Clear"),
								"<ctrl><shift>5",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS5_CLEAR			},

  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " _6",	NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 6/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 6/" N_("_Select"),
								"6",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS6_SELECT			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 6/" N_("_Add to current selection"),
								"<shift>6",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS6_ADD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 6/" N_("_Record"),
								"<ctrl>6",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS6_RECORD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 6/" N_("_Clear"),
								"<ctrl><shift>6",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS6_CLEAR			},

  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " _7",	NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 7/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 7/" N_("_Select"),
								"7",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS7_SELECT			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 7/" N_("_Add to current selection"),
								"<shift>7",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS7_ADD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 7/" N_("_Record"),
								"<ctrl>7",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS7_RECORD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 7/" N_("_Clear"),
								"<ctrl><shift>7",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS7_CLEAR			},

  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " _8",	NULL,
		NULL,			0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 8/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 8/" N_("_Select"),
								"8",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS8_SELECT			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 8/" N_("_Add to current selection"),
								"<shift>8",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS8_ADD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 8/" N_("_Record"),
								"<ctrl>8",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS8_RECORD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 8/" N_("_Clear"),
								"<ctrl><shift>8",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS8_CLEAR			},

  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " _9",	NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 9/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 9/" N_("_Select"),
								"9",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS9_SELECT			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 9/" N_("_Add to current selection"),
								"<shift>9",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS9_ADD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 9/" N_("_Record"),
								"<ctrl>9",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS9_RECORD			},
  { "/" N_("Multi-Selection") "/" N_("Multi-selection") " 9/" N_("_Clear"),
								"<ctrl><shift>9",
	multi_selection_menu_callback,	MENU_MULTI_SELECTION_MS9_CLEAR			},

 /* Miscellaneous menu ...*/
  { "/" N_("Misce_llaneous"),					NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Miscellaneous") "/tearoff1",			NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Miscellaneous") "/" N_("Airplane _patrol"),		"e",
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_AIR_PATROL			},
  { "/" N_("Miscellaneous") "/" N_("Airplane patrol _destination"),
								"<ctrl>e",
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_AIR_PATROL_DEST		},
  { "/" N_("Miscellaneous") "/" N_("_Execute all patrol orders"),
								"<shift>e",
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_PATROL_EXECUTE		},
  { "/" N_("Miscellaneous") "/sep1",				NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Miscellaneous") "/" N_("_Caravan action upon arrival"),
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Miscellaneous") "/" N_("Caravan action upon arrival") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Miscellaneous") "/" N_("Caravan action upon arrival") "/" N_("_Popup dialog"),
								 NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_CARAVAN_POPUP,
									"<RadioItem>"	},
  { "/" N_("Miscellaneous") "/" N_("Caravan action upon arrival") "/" N_("_Establish trade route"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_CARAVAN_TRADE,
			"<main>/Miscellaneous/Caravan action upon arrival/Popup dialog"	},
  { "/" N_("Miscellaneous") "/" N_("Caravan action upon arrival") "/" N_("_Help building wonder"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_CARAVAN_BUILD,
			"<main>/Miscellaneous/Caravan action upon arrival/Popup dialog"	},
  { "/" N_("Miscellaneous") "/" N_("Caravan action upon arrival") "/" N_("_Keep going"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_CARAVAN_NOTHING,
			"<main>/Miscellaneous/Caravan action upon arrival/Popup dialog"	},

  { "/" N_("Miscellaneous") "/" N_("_Diplomat action upon arrival"),
								NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/" N_("_Popup dialog"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_DIPLOMAT_POPUP,
									"<RadioItem>"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/" N_("_Bribe unit"), 
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_DIPLOMAT_BRIBE,
		"<main>/Miscellaneous/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/" N_("_Sabotage unit (spy)"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_DIPLOMAT_SABOTAGE_UNIT,
		"<main>/Miscellaneous/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/" N_("_Establish embassy"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_DIPLOMAT_EMBASSY,
		"<main>/Miscellaneous/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/" N_("_Investigate city"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_DIPLOMAT_INVESTIGATE,
		"<main>/Miscellaneous/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/" N_("_Sabotage city"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_DIPLOMAT_SABOTAGE_CITY,
		"<main>/Miscellaneous/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/" N_("_Steal technology"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_DIPLOMAT_STEAL,
		"<main>/Miscellaneous/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/" N_("_Incite revolt"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_DIPLOMAT_INCITE,
		"<main>/Miscellaneous/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/" N_("_Poison city (spy)"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_DIPLOMAT_POISON,
		"<main>/Miscellaneous/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/" N_("_Keep going"),NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_DIPLOMAT_NOTHING,
		"<main>/Miscellaneous/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/sep",				NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Miscellaneous") "/" N_("Diplomat action upon arrival") "/"
        N_("Ignore allies"), NULL, miscellaneous_menu_callback,
        MENU_MISCELLANEOUS_DIPLOMAT_IGNORE_ALLIES,
        "<CheckItem>" },

  { "/" N_("Miscellaneous") "/sep2",				NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Miscellaneous") "/" N_("New unit default action"),	NULL,
	NULL,				0,				"<Branch>"	},
  { "/" N_("Miscellaneous") "/" N_("New unit default action") "/tearoff1",
								NULL,
	NULL,				0,				"<Tearoff>"	},
  { "/" N_("Miscellaneous") "/" N_("New unit default action") "/" N_("Idle"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_UNIT_IDLE,	"<RadioItem>"	},
  { "/" N_("Miscellaneous") "/" N_("New unit default action") "/" N_("Sentry"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_UNIT_SENTRY,
		"<main>/Miscellaneous/New unit default action/Idle"	},
  { "/" N_("Miscellaneous") "/" N_("New unit default action") "/" N_("Fortify"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_UNIT_FORTIFY,
		"<main>/Miscellaneous/New unit default action/Idle"	},
  { "/" N_("Miscellaneous") "/" N_("New unit default action") "/" N_("Sleep"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_UNIT_SLEEP,
		"<main>/Miscellaneous/New unit default action/Idle"	},
  { "/" N_("Miscellaneous") "/" N_("New unit default action") "/" N_("Fortify or Sleep"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_UNIT_FORTIFY_OR_SLEEP,
		"<main>/Miscellaneous/New unit default action/Idle"	},
  { "/" N_("Miscellaneous") "/" N_("Lock new unit default action"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_UNIT_LOCK,	"<CheckItem>"	},
  { "/" N_("Miscellaneous") "/" N_("Military units only"),
								NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_UNIT_MILITARY,
								"<CheckItem>"	},
  { "/" N_("Miscellaneous") "/sep3",				NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Miscellaneous") "/" N_("Autowakeup _sentried units"),NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_TOGGLE_WAKEUP,
									"<CheckItem>"	},
  { "/" N_("Miscellaneous") "/" N_("Move and _attack mode"),	NULL,
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_TOGGLE_MOVEANDATTACK,
									"<CheckItem>"	},
  { "/" N_("Miscellaneous") "/sep4",				NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Miscellaneous") "/" N_("_Set rallies for selected cities"),
								"<shift>s",
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_SET_RALLIES			},
  { "/" N_("Miscellaneous") "/" N_("_Clear rallies in selected cities"),
								"<shift>r",
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_CLEAR_RALLIES		},
  { "/" N_("Miscellaneous") "/sep5",				NULL,
	NULL,				0,				"<Separator>"	},
  { "/" N_("Miscellaneous") "/" N_("E_xecute automatic orders"),	"<ctrl>m",
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_MY_AI_EXECUTE		},
  { "/" N_("Miscellaneous") "/" N_("_Free automatic orders"),	"<ctrl>f",
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_MY_AI_FREE			},
  { "/" N_("Miscellaneous") "/" N_("Execute all automatic _orders"),
								"<shift>m",
	miscellaneous_menu_callback,	MENU_MISCELLANEOUS_MY_AI_EXECUTE_ALL		},

  /* Reports menu ... */
  { "/" N_("_Reports"),					NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Reports") "/tearoff1",			NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("Reports") "/" N_("_Cities"),		"F1",
	reports_menu_callback,	MENU_REPORT_CITIES					},
  { "/" N_("Reports") "/" N_("_Units"),			"F2",
	reports_menu_callback,	MENU_REPORT_UNITS					},
  { "/" N_("Reports") "/" N_("_Players"),		"F3",
	reports_menu_callback,	MENU_REPORT_PLAYERS					},
  { "/" N_("Reports") "/" N_("_Economy"),		"F5",
	reports_menu_callback,	MENU_REPORT_ECONOMY					},
  { "/" N_("Reports") "/" N_("_Science"),		"F6",
	reports_menu_callback,	MENU_REPORT_SCIENCE					},
  { "/" N_("Reports") "/sep1",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Reports") "/" N_("_Wonders of the World"),	"F7",
	reports_menu_callback,	MENU_REPORT_WOW						},
  { "/" N_("Reports") "/" N_("_Top Five Cities"),	"F8",
	reports_menu_callback,	MENU_REPORT_TOP_CITIES					},
  { "/" N_("Reports") "/" N_("_Messages"),			"<ctrl>F9",
	reports_menu_callback,	MENU_REPORT_MESSAGES					},
  { "/" N_("Reports") "/" N_("_Demographics"),			"<ctrl>F11",
	reports_menu_callback,	MENU_REPORT_DEMOGRAPHIC					},
  { "/" N_("Reports") "/" N_("S_paceship"),			"<ctrl>F12",
	reports_menu_callback,	MENU_REPORT_SPACESHIP					},

  /* Help menu ... */
  { "/" N_("_Help"),					NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Help") "/tearoff1",				NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("Help") "/" N_("Language_s"),		NULL,
	help_menu_callback,	MENU_HELP_LANGUAGES					},
  { "/" N_("Help") "/" N_("Co_nnecting"),		NULL,
	help_menu_callback,	MENU_HELP_CONNECTING					},
  { "/" N_("Help") "/" N_("C_ontrols"),			NULL,
	help_menu_callback,	MENU_HELP_CONTROLS					},
  { "/" N_("Help") "/" N_("C_hatline"),			NULL,
	help_menu_callback,	MENU_HELP_CHATLINE					},
  { "/" N_("Help") "/" N_("_Worklist Editor"),			NULL,
	help_menu_callback,	MENU_HELP_WORKLIST_EDITOR				},
  { "/" N_("Help") "/" N_("Citizen _Management"),			NULL,
	help_menu_callback,	MENU_HELP_CMA						},
  { "/" N_("Help") "/" N_("_Playing"),			NULL,
	help_menu_callback,	MENU_HELP_PLAYING					},
  { "/" N_("Help") "/" N_("_Warclient - PepClient"),    NULL,
	help_menu_callback,		MENU_HELP_WARCLIENT				},
  { "/" N_("Help") "/sep1",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Help") "/" N_("City _Improvements"),        NULL,
	help_menu_callback,	MENU_HELP_IMPROVEMENTS					},
  { "/" N_("Help") "/" N_("_Units"),			NULL,
	help_menu_callback,	MENU_HELP_UNITS						},
  { "/" N_("Help") "/" N_("Com_bat"),			NULL,
	help_menu_callback,	MENU_HELP_COMBAT					},
  { "/" N_("Help") "/" N_("_ZOC"),			NULL,
	help_menu_callback,	MENU_HELP_ZOC						},
  { "/" N_("Help") "/" N_("Techno_logy"),		NULL,
	help_menu_callback,	MENU_HELP_TECH						},
  { "/" N_("Help") "/" N_("_Terrain"),			NULL,
	help_menu_callback,	MENU_HELP_TERRAIN					},
  { "/" N_("Help") "/" N_("Won_ders"),			NULL,
	help_menu_callback,	MENU_HELP_WONDERS					},
  { "/" N_("Help") "/" N_("_Government"),		NULL,
	help_menu_callback,	MENU_HELP_GOVERNMENT					},
  { "/" N_("Help") "/" N_("Happin_ess"),		NULL,
	help_menu_callback,	MENU_HELP_HAPPINESS					},
  { "/" N_("Help") "/" N_("Space _Race"),		NULL,
	help_menu_callback,	MENU_HELP_SPACE_RACE					},
  { "/" N_("Help") "/sep2",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Help") "/" N_("_Copying"),			NULL,
	help_menu_callback,	MENU_HELP_COPYING					},
  { "/" N_("Help") "/" N_("_About"),			NULL,
	help_menu_callback,	MENU_HELP_ABOUT						}
};

#ifdef ENABLE_NLS
/****************************************************************
  gettext-translates each "/" delimited component of menu path,
  puts them back together, and returns as a static string.
  Any component which is of form "<foo>" is _not_ translated.

  Path should include underscores like in the menu itself.
*****************************************************************/
static char *menu_path_tok(char *path)
{
  bool escaped = FALSE;

  while (*path) {
    if (escaped) {
      escaped = FALSE;
    } else {
      if (*path == '\\') {
        escaped = TRUE;
      } else if (*path == '/') {
        *path = '\0';
        return path;
      }
    }

    path++;
  }

  return NULL;
}
#endif

/****************************************************************
...
*****************************************************************/
static gchar *translate_func(const gchar *path, gpointer data)
{
#ifndef ENABLE_NLS
    static gchar res[100];
    
    g_strlcpy(res, path, sizeof(res));
#else
    static struct astring in, out, tmp;   /* these are never free'd */
    char *tok, *next, *t;
    const char *trn;
    int len;
    char *res;

    /* copy to in so can modify with menu_path_tok: */
    astr_minsize(&in, strlen(path)+1);
    strcpy(in.str, path);
    astr_minsize(&out, 1);
    out.str[0] = '\0';
    freelog(LOG_DEBUG, "trans: %s", in.str);

    tok = in.str;
    do {
      next = menu_path_tok(tok);

      len = strlen(tok);
      freelog(LOG_DEBUG, "tok \"%s\", len %d", tok, len);
      if (len == 0 || (tok[0] == '<' && tok[len-1] == '>')) {
	t = tok;
      } else {
	trn = _(tok);
	len = strlen(trn) + 1;	/* string plus leading '/' */
	astr_minsize(&tmp, len+1);
	sprintf(tmp.str, "/%s", trn);
	t = tmp.str;
	len = strlen(t);
      }
      astr_minsize(&out, out.n + len);
      strcat(out.str, t);
      freelog(LOG_DEBUG, "t \"%s\", len %d, out \"%s\"", t, len, out.str);
      tok = next+1;
    } while (next);
    res = out.str;
#endif
  
  return res;
}

/****************************************************************
...
*****************************************************************/
static const char *menu_path_remove_uline(const char *path)
{
  static char res[100];
  const char *from;
  char *to;
  
  from = path;
  to = res;

  do {
    if (*from != '_') {
      *(to++) = *from;
    }
  } while (*(from++));

  return res;
}

/****************************************************************
  ...
 *****************************************************************/
void get_accel_label(GtkWidget *widget,const char *uname)
{
	if(!strcmp(uname,"Workers"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'w',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Engineers"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'e',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Alpine Troops"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'l',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Riflemen"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'r',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Marines"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'m',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Paratroopers"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'p',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Mech. Inf."))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'i',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Cavalry"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'c',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Armor"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'t',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Artillery"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'a',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Howitzer"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'h',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Diplomat"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'d',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Spy"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'s',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	else if(!strcmp(uname,"Fanatics"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'f',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
}

/****************************************************************
  ...
*****************************************************************/
static bool can_player_unit_type(Unit_Type_id utype)
{
  if(can_player_build_unit(get_player_ptr(),utype))
    return TRUE;

  unit_list_iterate(get_player_ptr()->units,punit) {
    if(punit->type==utype)
      return TRUE;
  } unit_list_iterate_end;
  return FALSE;
}

/****************************************************************
  ...
*****************************************************************/
static void pep_airlift_menu_set_active(void)
{
  int i;

  for(i = 0; i < AIRLIFT_QUEUE_NUM; i++)
  {
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(airlift_queue_get_menu_item(i, airlift_queue_get_unit_type(i))), TRUE);
  }
}

/****************************************************************
  ...
*****************************************************************/
static void pep_airlift_menu_callback(GtkWidget *widget,gpointer data)
{
  int aq = GPOINTER_TO_INT(data) / (U_LAST + 1);
  Unit_Type_id utype = GPOINTER_TO_INT(data) % (U_LAST + 1);

  airlift_queue_set_unit_type(aq, utype);
}

/****************************************************************
  Create the menu <main>/Airlift/Airlift unit type
  Sort the unit type in 4 classes:
  * civil units
  * attack units
  * defense units
  * fast units
*****************************************************************/
#define CLASS_NUM 4
static void create_pep_airlift_menu(int aq, GtkWidget *menu)
{
  GSList **group = fc_malloc(sizeof(GList *));
  GtkWidget *item;
  Unit_Type_id classes[CLASS_NUM][U_LAST];
  int index[CLASS_NUM] = {0, 0, 0, 0}, i, j, k = 0;

  unit_type_iterate(ut) {
    struct unit_type *utype = get_unit_type(ut);

    if (utype->move_type != LAND_MOVING || unit_type_flag(ut, F_NOBUILD)) {
      /* Land unit and buildable units only */
      continue;
    }

    if (utype->attack_strength == 0) {
      i = 0;
    } else if (unit_has_role(ut, L_DEFEND_GOOD)) {
      i = 2;
    } else if (unit_has_role(ut, L_ATTACK_FAST)) {
      i = 3;
    } else if (unit_has_role(ut, L_ATTACK_STRONG)) {
      i = 1;
    } else if (utype->attack_strength > utype->defense_strength) {
      if (utype->move_rate > SINGLE_MOVE
          && utype->attack_strength < 4 * utype->defense_strength) {
        i = 3;
      } else {
        i = 1;
      }
    } else {
      i = 2;
    }

    classes[i][index[i]++] = ut;
    k++;
  } unit_type_iterate_end;

  /* None unit type */
  *group = NULL;
  item = gtk_radio_menu_item_new_with_label(*group, _("None"));
  g_signal_connect(item, "activate",
                   G_CALLBACK(pep_airlift_menu_callback),
                   GINT_TO_POINTER(aq * (U_LAST + 1) + U_LAST));
  *group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  airlift_queue_set_menu_item(aq, U_LAST, item);

  for (k = i = 0; i < CLASS_NUM; i++) {
    if (index[i] == 0) {
      continue; /* No unit type in this class */
    }
    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    for (j = 0; j < index[i]; j++, k++) {
      item = gtk_radio_menu_item_new_with_label(*group,
                                                unit_name(classes[i][j]));
      g_signal_connect(item, "activate",
                       G_CALLBACK(pep_airlift_menu_callback),
                       GINT_TO_POINTER(aq * (U_LAST + 1) + classes[i][j]));
      if (aq == 0) {
        get_accel_label(item, unit_name_orig(classes[i][j]));
      }
      *group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      airlift_queue_set_menu_item(aq, classes[i][j], item);
    }
  }
}

/****************************************************************
  ...
*****************************************************************/
static void ap_menu_callback(GtkWidget *widget,gpointer data)
{
	automatic_processus *pap=(automatic_processus *)data;
	int i;

	for(i=0;i<AUTO_VALUES_NUM;i++)
		if(is_auto_value_allowed(pap,i)&&widget==pap->widget[i])
		{
			if(GTK_CHECK_MENU_ITEM(widget)->active^(bool)(pap->auto_filter&AV_TO_FV(i)))
			{
				auto_filter_change(pap,i);
				update_automatic_processus_filter_menu(pap);
			}
			return;
		}
}

/****************************************************************
  ...
*****************************************************************/
void create_automatic_processus_menus(void)
{
	GtkWidget *item,*widget;
	int i;
	automatic_processus_iterate(pap)
	{
		if(pap->menu[0]=='\0')
			continue;
		widget=gtk_item_factory_get_widget(item_factory,pap->menu);
		for(i=0;i<AUTO_VALUES_NUM;i++)
		{
			if(!is_auto_value_allowed(pap,i))
				continue;
			item=gtk_check_menu_item_new_with_label(ap_event_name(i));
			g_signal_connect(item,"activate",G_CALLBACK(ap_menu_callback),pap);
			gtk_menu_shell_append(GTK_MENU_SHELL(widget),item);
			pap->widget[i]=item;
		}
	} automatic_processus_iterate_end;
}

/****************************************************************
  ...
 *****************************************************************/
void setup_menus(GtkWidget *window, GtkWidget **menubar)
{
  const int nmenu_items = ARRAY_SIZE(menu_items);

  toplevel_accel = gtk_accel_group_new();
  item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>",
      toplevel_accel);
  gtk_item_factory_set_translate_func(item_factory, translate_func,
      NULL, NULL);

  gtk_accel_group_lock(toplevel_accel);
  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);
  gtk_window_add_accel_group(GTK_WINDOW(window), toplevel_accel);

  main_menubar = gtk_item_factory_get_widget(item_factory, "<main>");
  g_signal_connect(main_menubar, "destroy",
      G_CALLBACK(gtk_widget_destroyed), &main_menubar);

  int i;
  char buf[256];

  create_pep_airlift_menu(0, gtk_item_factory_get_widget(item_factory, "<main>/Airlift/Airlift unit type"));
  for(i = 1; i < AIRLIFT_QUEUE_NUM; i++) {
    my_snprintf(buf, sizeof(buf), "<main>/Airlift/Airlift queue %d/Airlift unit type", i + DELAYED_GOTO_NUM - 1);
    create_pep_airlift_menu(i, gtk_item_factory_get_widget(item_factory, buf));
  }
  create_automatic_processus_menus();

  if (menubar) {
    *menubar = main_menubar;
  }
}

/****************************************************************
...
*****************************************************************/
static void menus_set_sensitive(const char *path, int sensitive)
{
  GtkWidget *item;

  path = menu_path_remove_uline(path);

  if(!(item = gtk_item_factory_get_item(item_factory, path))) {
    freelog(LOG_ERROR,
	    "Can't set sensitivity for non-existent menu %s.", path);
    return;
  }

  gtk_widget_set_sensitive(item, sensitive);
}

/****************************************************************
...
*****************************************************************/
static void menus_set_active(const char *path, int active)
{
  GtkWidget *item;

  path = menu_path_remove_uline(path);

  if (!(item = gtk_item_factory_get_item(item_factory, path))) {
    freelog(LOG_ERROR,
	    "Can't set active for non-existent menu %s.", path);
    return;
  }

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), active);
}

#ifdef UNUSED 
/****************************************************************
...
*****************************************************************/
static void menus_set_shown(const char *path, int shown)
{
  GtkWidget *item;
  
  path = menu_path_remove_uline(path);
  
  if(!(item = gtk_item_factory_get_item(item_factory, path))) {
    freelog(LOG_ERROR, "Can't show non-existent menu %s.", path);
    return;
  }

  if (shown) {
    gtk_widget_show(item);
  } else {
    gtk_widget_hide(item);
  }
}
#endif /* UNUSED */

/****************************************************************
...
*****************************************************************/
static void menus_rename(const char *path, const char *s)
{
  GtkWidget *item;

  path = menu_path_remove_uline(path);

  if(!(item = gtk_item_factory_get_item(item_factory, path))) {
    freelog(LOG_ERROR, "Can't rename non-existent menu %s.", path);
    return;
  }

  gtk_label_set_text_with_mnemonic(GTK_LABEL(GTK_BIN(item)->child), s);
}

/****************************************************************
  The player has chosen a government from the menu.
*****************************************************************/
static void government_callback(GtkMenuItem *item, gpointer data)
{
  popup_revolution_dialog(GPOINTER_TO_INT(data));
}

/****************************************************************************
  Return the text for the tile, changed by the activity.

  Should only be called for irrigation, mining, or transformation, and
  only when the activity changes the base terrain type.
****************************************************************************/
static const char *get_tile_change_menu_text(struct tile *ptile,
					     enum unit_activity activity)
{
  Terrain_type_id old_terrain = ptile->terrain;
  enum tile_special_type old_special = ptile->special;
  struct tile_type *ptype = get_tile_type(ptile->terrain);
  const char *text;

  /* Change the terrain manually to avoid any side effects. */
  switch (activity) {
  case ACTIVITY_IRRIGATE:
    if (ptype->irrigation_result == ptile->terrain
	|| ptype->irrigation_result == T_NONE) {
      assert(0);
    }
    map_irrigate_tile(ptile);
    break;

  case ACTIVITY_MINE:
    if (ptype->mining_result == ptile->terrain
	|| ptype->mining_result == T_NONE) {
      assert(0);
    }
    map_mine_tile(ptile);
    break;
    
  case ACTIVITY_TRANSFORM:
    if (ptype->transform_result == ptile->terrain
	|| ptype->transform_result == T_NONE) {
      assert(0);
    }
    map_transform_tile(ptile);
    break;

  default:
    assert(0);
    return "-";
  }

  text = map_get_tile_info_text(ptile);

  /* Restore the original state of the tile. */
  ptile->terrain = old_terrain;
  ptile->special = old_special;
  reset_move_costs(ptile);

  return text;
}

/****************************************************************
  ...
*****************************************************************/
static void update_delayed_goto_inclusive_filter_menu(void)
{
  menus_set_active("<main>/Delayed Goto/Inclusive filter/All units",
		   delayed_goto_inclusive_filter & FILTER_ALL);
  menus_set_active("<main>/Delayed Goto/Inclusive filter/New units",
		   delayed_goto_inclusive_filter & FILTER_NEW);
  menus_set_active("<main>/Delayed Goto/Inclusive filter/Fortified units",
		   delayed_goto_inclusive_filter & FILTER_FORTIFIED);
  menus_set_active("<main>/Delayed Goto/Inclusive filter/Sentried units",
		   delayed_goto_inclusive_filter & FILTER_SENTRIED);
  menus_set_active("<main>/Delayed Goto/Inclusive filter/Veteran units",
		   delayed_goto_inclusive_filter & FILTER_VETERAN);
  menus_set_active("<main>/Delayed Goto/Inclusive filter/Auto units",
		   delayed_goto_inclusive_filter & FILTER_AUTO);
  menus_set_active("<main>/Delayed Goto/Inclusive filter/Idle units",
		   delayed_goto_inclusive_filter & FILTER_IDLE);
  menus_set_active("<main>/Delayed Goto/Inclusive filter/Units able to move",
		   delayed_goto_inclusive_filter & FILTER_ABLE_TO_MOVE);
  menus_set_active("<main>/Delayed Goto/Inclusive filter/Military units",
		   delayed_goto_inclusive_filter & FILTER_MILITARY);
  menus_set_active("<main>/Delayed Goto/Inclusive filter/Off",
		   delayed_goto_inclusive_filter & FILTER_OFF);
}

/****************************************************************
  ...
*****************************************************************/
static void update_delayed_goto_exclusive_filter_menu(void)
{
  menus_set_active("<main>/Delayed Goto/Exclusive filter/All units",
		   delayed_goto_exclusive_filter & FILTER_ALL);
  menus_set_active("<main>/Delayed Goto/Exclusive filter/New units",
		   delayed_goto_exclusive_filter & FILTER_NEW);
  menus_set_active("<main>/Delayed Goto/Exclusive filter/Fortified units",
		   delayed_goto_exclusive_filter & FILTER_FORTIFIED);
  menus_set_active("<main>/Delayed Goto/Exclusive filter/Sentried units",
		   delayed_goto_exclusive_filter & FILTER_SENTRIED);
  menus_set_active("<main>/Delayed Goto/Exclusive filter/Veteran units",
		   delayed_goto_exclusive_filter & FILTER_VETERAN);
  menus_set_active("<main>/Delayed Goto/Exclusive filter/Auto units",
		   delayed_goto_exclusive_filter & FILTER_AUTO);
  menus_set_active("<main>/Delayed Goto/Exclusive filter/Idle units",
		   delayed_goto_exclusive_filter & FILTER_IDLE);
  menus_set_active("<main>/Delayed Goto/Exclusive filter/Units able to move",
		   delayed_goto_exclusive_filter & FILTER_ABLE_TO_MOVE);
  menus_set_active("<main>/Delayed Goto/Exclusive filter/Military units",
		   delayed_goto_exclusive_filter & FILTER_MILITARY);
  menus_set_active("<main>/Delayed Goto/Exclusive filter/Off",
		   delayed_goto_exclusive_filter & FILTER_OFF);
}

/****************************************************************
  ...
*****************************************************************/
static void update_multi_select_inclusive_filter_menu(void)
{
  menus_set_active("<main>/Multi-Selection/Inclusive filter/All units",
		   multi_select_inclusive_filter & FILTER_ALL);
  menus_set_active("<main>/Multi-Selection/Inclusive filter/New units",
		   multi_select_inclusive_filter & FILTER_NEW);
  menus_set_active("<main>/Multi-Selection/Inclusive filter/Fortified units",
		   multi_select_inclusive_filter & FILTER_FORTIFIED);
  menus_set_active("<main>/Multi-Selection/Inclusive filter/Sentried units",
		   multi_select_inclusive_filter & FILTER_SENTRIED);
  menus_set_active("<main>/Multi-Selection/Inclusive filter/Veteran units",
		   multi_select_inclusive_filter & FILTER_VETERAN);
  menus_set_active("<main>/Multi-Selection/Inclusive filter/Auto units",
		   multi_select_inclusive_filter & FILTER_AUTO);
  menus_set_active("<main>/Multi-Selection/Inclusive filter/Idle units",
		   multi_select_inclusive_filter & FILTER_IDLE);
  menus_set_active("<main>/Multi-Selection/Inclusive filter/Units able to move",
		   multi_select_inclusive_filter & FILTER_ABLE_TO_MOVE);
  menus_set_active("<main>/Multi-Selection/Inclusive filter/Military units",
		   multi_select_inclusive_filter & FILTER_MILITARY);
  menus_set_active("<main>/Multi-Selection/Inclusive filter/Off",
		   multi_select_inclusive_filter & FILTER_OFF);
}

/****************************************************************
  ...
*****************************************************************/
static void update_multi_select_exclusive_filter_menu(void)
{
  menus_set_active("<main>/Multi-Selection/Exclusive filter/All units",
		   multi_select_exclusive_filter & FILTER_ALL);
  menus_set_active("<main>/Multi-Selection/Exclusive filter/New units",
		   multi_select_exclusive_filter & FILTER_NEW);
  menus_set_active("<main>/Multi-Selection/Exclusive filter/Fortified units",
		   multi_select_exclusive_filter & FILTER_FORTIFIED);
  menus_set_active("<main>/Multi-Selection/Exclusive filter/Sentried units",
		   multi_select_exclusive_filter & FILTER_SENTRIED);
  menus_set_active("<main>/Multi-Selection/Exclusive filter/Veteran units",
		   multi_select_exclusive_filter & FILTER_VETERAN);
  menus_set_active("<main>/Multi-Selection/Exclusive filter/Auto units",
		   multi_select_exclusive_filter & FILTER_AUTO);
  menus_set_active("<main>/Multi-Selection/Exclusive filter/Idle units",
		   multi_select_exclusive_filter & FILTER_IDLE);
  menus_set_active("<main>/Multi-Selection/Exclusive filter/Units able to move",
		   multi_select_exclusive_filter & FILTER_ABLE_TO_MOVE);
  menus_set_active("<main>/Multi-Selection/Exclusive filter/Military units",
		   multi_select_exclusive_filter & FILTER_MILITARY);
  menus_set_active("<main>/Multi-Selection/Exclusive filter/Off",
		   multi_select_exclusive_filter & FILTER_OFF);
}

/****************************************************************
  ... for automatic processus
*****************************************************************/
void update_automatic_processus_filter_menu(automatic_processus *pap)
{
  int i;

  if (!pap || pap->menu[0] == '\0') {
    return;
  }

  for (i = 0; i < AUTO_VALUES_NUM; i++) {
    if (is_auto_value_allowed(pap, i)) {
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pap->widget[i]),
				     pap->auto_filter & AV_TO_FV(i));
    }
  }
}

/****************************************************************
  Update the menus about the delayed goto queues.
*****************************************************************/
void update_delayed_goto_menu(int i)
{
  if (!can_client_issue_orders() || !main_menubar) {
    return;
  }

  assert(i >= 0 && i < DELAYED_GOTO_NUM);

  char buf[256], *m;
  bool cond = delayed_goto_size(0) > 0;

  if (i == 0) {
    menus_set_sensitive("<main>/Delayed Goto/Execute delayed goto", cond);
    menus_set_sensitive("<main>/Delayed Goto/Clear delayed orders", cond);
    for (i = 1; i < DELAYED_GOTO_NUM; i++) {
      update_delayed_goto_menu(i);
    }
    return;
  } else {
    m = buf + my_snprintf(buf, sizeof(buf),
                          "<main>/Delayed Goto/Delayed goto selection %d", i);
    if (delayed_goto_size(i) > 0) {
      menus_set_sensitive(buf, TRUE);
      strcpy(m, "/Select");
      menus_set_sensitive(buf, TRUE);
      strcpy(m, "/Add to current selection");
      menus_set_sensitive(buf, TRUE);
      strcpy(m, "/Record");
      menus_set_sensitive(buf, cond);
      strcpy(m, "/Clear");
      menus_set_sensitive(buf, TRUE);
      strcpy(m, "/Execute");
      menus_set_sensitive(buf, TRUE);
    } else if (cond) {
      menus_set_sensitive(buf, TRUE);
      strcpy(m, "/Select");
      menus_set_sensitive(buf, FALSE);
      strcpy(m, "/Add to current selection");
      menus_set_sensitive(buf, FALSE);
      strcpy(m, "/Record");
      menus_set_sensitive(buf, TRUE);
      strcpy(m, "/Clear");
      menus_set_sensitive(buf, FALSE);
      strcpy(m, "/Execute");
      menus_set_sensitive(buf, FALSE);
    } else {
      menus_set_sensitive(buf, FALSE);
    }
  }
}

/****************************************************************
  Update the menus about the airlift queue i.
*****************************************************************/
void update_airlift_menu(int i)
{
  if (!can_client_issue_orders() || !main_menubar) {
    return;
  }

  assert(i >= 0 && i < AIRLIFT_QUEUE_NUM);

  char buf[256], *m;
  bool cond = airlift_queue_size(0) > 0;

  if (i == 0) {
    menus_set_sensitive("<main>/Airlift/Clear airlift queue", cond);
    menus_set_sensitive("<main>/Airlift/Show cities in airlift queue", cond);
    cond = cond && airlift_queue_get_unit_type(0) != U_LAST;
    menus_set_sensitive("<main>/Airlift/Set airlift destination", cond);
    menus_set_sensitive("<main>/Delayed Goto/Delayed airlift", cond);
    for (i = 1; i < AIRLIFT_QUEUE_NUM; i++) {
      update_airlift_menu(i);
    }
    return;
  } else {
    m = buf + my_snprintf(buf, sizeof(buf), "<main>/Airlift/Airlift queue %d",
                          i + DELAYED_GOTO_NUM - 1);
    if (airlift_queue_size(i) > 0) {
      menus_set_sensitive(buf, TRUE);
      strcpy(m, "/Select");
      menus_set_sensitive(buf, TRUE);
      strcpy(m, "/Add to current selection");
      menus_set_sensitive(buf, TRUE);
      strcpy(m, "/Record");
      menus_set_sensitive(buf, cond);
      strcpy(m, "/Clear");
      menus_set_sensitive(buf, TRUE);
      strcpy(m, "/Execute");
      menus_set_sensitive(buf, airlift_queue_get_unit_type(i) != U_LAST);
      strcpy(m, "/Show");
      menus_set_sensitive(buf, TRUE);
      menus_set_sensitive("<main>/Airlift/Set airlift destination", TRUE);
      menus_set_sensitive("<main>/Delayed Goto/Delayed airlift", TRUE);
    } else if (cond) {
      menus_set_sensitive(buf, TRUE);
      strcpy(m, "/Select");
      menus_set_sensitive(buf, FALSE);
      strcpy(m, "/Add to current selection");
      menus_set_sensitive(buf, FALSE);
      strcpy(m, "/Record");
      menus_set_sensitive(buf, TRUE);
      strcpy(m, "/Clear");
      menus_set_sensitive(buf, FALSE);
      strcpy(m, "/Execute");
      menus_set_sensitive(buf, FALSE);
      strcpy(m, "/Show");
      menus_set_sensitive(buf, FALSE);
    } else {
      menus_set_sensitive(buf, FALSE);
    }
  }
}

/****************************************************************
  Update the menus about the trade planning.
*****************************************************************/
void update_auto_caravan_menu(void)
{
  if (!can_client_issue_orders() || !main_menubar) {
    return;
  }

  bool cond = city_list_size(my_ai_get_trade_cities()) > 0;

  menus_set_sensitive("<main>/Auto Caravan/Clear city list for trade plan",
		      cond);
  menus_set_sensitive("<main>/Auto Caravan/Show cities in trade plan", cond);
  menus_set_sensitive("<main>/Auto Caravan/Show the trade route free slots",
		      cond);
  menus_set_sensitive("<main>/Auto Caravan/Show trade estimation",
		      (my_ai_trade_level
		       && my_ai_count_activity(MY_AI_TRADE_ROUTE) > 0)
		      || (my_ai_trade_manual_trade_route_enable
			  && trade_route_list_size(
			         estimate_non_ai_trade_route()) > 0));
  menus_set_sensitive("<main>/Auto Caravan/Recalculate trade plan", cond);
  menus_set_sensitive("<main>/Auto Caravan/Execute all trade route orders",
		      my_ai_trade_level
		      && my_ai_count_activity(MY_AI_TRADE_ROUTE) > 0);
}

/****************************************************************
  Update the menus about the multi-selection i.
*****************************************************************/
void update_multi_selection_menu(int i)
{
  if (!can_client_issue_orders() || !main_menubar) {
    return;
  }

  assert(i > 0 && i < MULTI_SELECT_NUM);

  char buf[256], *m;
  bool cond = multi_select_size(0) > 0;

  m = buf + my_snprintf(buf, sizeof(buf),
                        "<main>/Multi-Selection/Multi-selection %d", i);
  if (multi_select_size(i) > 0) {
    menus_set_sensitive(buf, TRUE);
    strcpy(m, "/Select");
    menus_set_sensitive(buf, TRUE);
    strcpy(m, "/Add to current selection");
    menus_set_sensitive(buf, TRUE);
    strcpy(m, "/Record");
    menus_set_sensitive(buf, cond);
    strcpy(m, "/Clear");
    menus_set_sensitive(buf, TRUE);
  } else if (cond) {
    menus_set_sensitive(buf, TRUE);
    strcpy(m, "/Select");
    menus_set_sensitive(buf, FALSE);
    strcpy(m, "/Add to current selection");
    menus_set_sensitive(buf, FALSE);
    strcpy(m, "/Record");
    menus_set_sensitive(buf, TRUE);
    strcpy(m, "/Clear");
    menus_set_sensitive(buf, FALSE);
  } else {
    menus_set_sensitive(buf, FALSE);
  }
}

/****************************************************************
  Update the menus about the hilited cities.
*****************************************************************/
void update_miscellaneous_menu(void)
{
  if (!can_client_issue_orders() || !main_menubar) {
    return;
  }

  bool cond = (tiles_hilited_cities);

  menus_set_sensitive("<main>/Miscellaneous/Set rallies for selected cities", cond);
  menus_set_sensitive("<main>/Miscellaneous/Clear rallies in selected cities", cond);
  menus_set_sensitive("<main>/Miscellaneous/Execute all automatic orders", my_ai_enable && my_ai_count_activity(MY_AI_LAST));
  menus_set_sensitive("<main>/Miscellaneous/Execute all patrol orders", my_ai_enable && my_ai_count_activity(MY_AI_PATROL));
}

/****************************************************************
  ...
*****************************************************************/
void start_turn_menus_udpate(void)
{
  int i, j;

  /* Airlift */
  for(i = 0; i < U_LAST; i++) {
    if(!airlift_queue_get_menu_item(0, i))
      continue;
    bool sensitive = can_player_unit_type(i);
    for(j = 0; j < AIRLIFT_QUEUE_NUM; j++)
      gtk_widget_set_sensitive(airlift_queue_get_menu_item(j, i), sensitive);
  }

  /* Miscellaneous */
  switch(default_action_type)
  {
    case ACTION_IDLE: menus_set_active("<main>/Miscellaneous/New unit default action/Idle", TRUE); break;
    case ACTION_SENTRY: menus_set_active("<main>/Miscellaneous/New unit default action/Sentry", TRUE); break;
    case ACTION_FORTIFY: menus_set_active("<main>/Miscellaneous/New unit default action/Fortify", TRUE); break;
    case ACTION_SLEEP: menus_set_active("<main>/Miscellaneous/New unit default action/Sleep", TRUE); break;
    case ACTION_FORTIFY_OR_SLEEP: menus_set_active("<main>/Miscellaneous/New unit default action/Fortify or Sleep", TRUE); break;
    default: break;
  }
}

/****************************************************************
 ...
*****************************************************************/
void init_menus(void)
{
  if(!main_menubar)
    return;
  bool cond;
  int i;

  cond=(can_client_access_hack() && get_client_state() >= CLIENT_GAME_RUNNING_STATE);
  menus_set_sensitive("<main>/_Game/Save Game _As...", cond);
  menus_set_sensitive("<main>/_Game/_Save Game", cond);
  menus_set_sensitive("<main>/_Game/Server O_ptions", aconnection.established);
  menus_set_sensitive("<main>/_Game/_Initial Server Options", get_client_state() >= CLIENT_GAME_RUNNING_STATE);
  menus_set_sensitive("<main>/_Game/L_eave", aconnection.established);

  if(!can_client_change_view())
  {
    menus_set_sensitive("<main>/_Reports", FALSE);
    menus_set_sensitive("<main>/_Government", FALSE);
    menus_set_sensitive("<main>/_View", FALSE);
    menus_set_sensitive("<main>/_Orders", FALSE);
    menus_set_sensitive("<main>/Multi-Selection", FALSE);
    menus_set_sensitive("<main>/Delayed Goto", FALSE);
    menus_set_sensitive("<main>/Airlift", FALSE);
    menus_set_sensitive("<main>/Auto Caravan", FALSE);
    menus_set_sensitive("<main>/Miscellaneous", FALSE);
    return;
  }

  cond=(can_client_issue_orders());
  menus_set_sensitive("<main>/_Reports", TRUE);
  menus_set_sensitive("<main>/_Government", TRUE);
  menus_set_sensitive("<main>/_View", TRUE);
  menus_set_sensitive("<main>/_Orders", cond);
  menus_set_sensitive("<main>/Multi-Selection", cond);
  menus_set_sensitive("<main>/Delayed Goto", cond);
  menus_set_sensitive("<main>/Airlift", cond);
  menus_set_sensitive("<main>/Auto Caravan", cond );
  menus_set_sensitive("<main>/Miscellaneous", cond );

  menus_set_sensitive("<main>/_Government/_Tax Rates", game.ruleset_game.changable_tax && cond);
  menus_set_sensitive("<main>/_Government/_Worklists", cond);
  menus_set_sensitive("<main>/_Government/_Change Government", cond);

  menus_set_active("<main>/_View/Map _Grid", draw_map_grid);
  menus_set_sensitive("<main>/_View/National _Borders", game.ruleset_control.borders > 0);
  menus_set_active("<main>/_View/National _Borders", draw_borders);
  menus_set_active("<main>/_View/City _Names", draw_city_names);
  menus_set_active("<main>/_View/City G_rowth", draw_city_growth);
  menus_set_active("<main>/_View/City _Productions", draw_city_productions);
  menus_set_active("<main>/_View/City _Trade Routes", draw_city_traderoutes);
  menus_set_active("<main>/_View/Terrain", draw_terrain);
  menus_set_active("<main>/_View/Coastline", draw_coastline);
  menus_set_active("<main>/_View/Improvements/Roads & Rails", draw_roads_rails);
  menus_set_active("<main>/_View/Improvements/Irrigation", draw_irrigation);
  menus_set_active("<main>/_View/Improvements/Mines", draw_mines);
  menus_set_active("<main>/_View/Improvements/Fortress & Airbase", draw_fortress_airbase);
  menus_set_active("<main>/_View/Specials", draw_specials);
  menus_set_active("<main>/_View/Pollution & Fallout", draw_pollution);
  menus_set_active("<main>/_View/Cities", draw_cities);
  menus_set_active("<main>/_View/Units", draw_units);
  menus_set_active("<main>/_View/Focus Unit", draw_focus_unit);
  menus_set_active("<main>/_View/Fog of War", draw_fog_of_war);

  /* Delayed goto menu */
  switch (delayed_goto_place) {
    case PLACE_SINGLE_UNIT:
      menus_set_active("<main>/Delayed Goto/Delayed goto mode/Single unit",
		       TRUE);
      break;
    case PLACE_IN_TRANSPORTER:
      menus_set_active("<main>/Delayed Goto/Delayed goto mode/"
		       "All units in the same transporter", TRUE);
      break;
    case PLACE_ON_TILE:
      menus_set_active("<main>/Delayed Goto/Delayed goto mode/"
		       "All units on the tile", TRUE);
      break;
    case PLACE_ON_CONTINENT:
      menus_set_active("<main>/Delayed Goto/Delayed goto mode/"
		       "All units on the continent", TRUE);
      break;
    case PLACE_EVERY_WHERE:
      menus_set_active("<main>/Delayed Goto/Delayed goto mode/All units", TRUE);
      break;
    default:
      break;
  }
  switch (delayed_goto_utype) {
    case UTYPE_SAME_TYPE:
      menus_set_active("<main>/Delayed Goto/Delayed goto mode/"
		       "Only units with the same type", TRUE);
      break;
    case UTYPE_SAME_MOVE_TYPE:
      menus_set_active("<main>/Delayed Goto/Delayed goto mode/"
		       "Only units with the same move type", TRUE);
      break;
    case UTYPE_ALL:
      menus_set_active("<main>/Delayed Goto/Delayed goto mode/All unit types",
		       TRUE);
      break;
    default:
      break;
  }
  update_delayed_goto_inclusive_filter_menu();
  update_delayed_goto_exclusive_filter_menu();
  for (i = 0; i < DELAYED_GOTO_NUM; i++) {
    update_delayed_goto_menu(i);
  }

  /* Airlift menu */
  for (i = 0; i < AIRLIFT_QUEUE_NUM; i++) {
    update_airlift_menu(i);
  }
  pep_airlift_menu_set_active();

  /* Auto Caravan menu */
  update_auto_caravan_menu();

  /* Multi-Selection menu */
  switch (multi_select_place) {
    case PLACE_SINGLE_UNIT:
      menus_set_active("<main>/Multi-Selection/Multi-selection mode/"
		       "Single unit", TRUE);
      break;
    case PLACE_IN_TRANSPORTER:
      menus_set_active("<main>/Multi-Selection/Multi-selection mode/"
		       "All units in the same transporter", TRUE);
      break;
    case PLACE_ON_TILE:
      menus_set_active("<main>/Multi-Selection/Multi-selection mode/"
		       "All units on the tile", TRUE);
      break;
    case PLACE_ON_CONTINENT:
      menus_set_active("<main>/Multi-Selection/Multi-selection mode/"
		       "All units on the continent", TRUE);
      break;
    case PLACE_EVERY_WHERE:
      menus_set_active("<main>/Multi-Selection/Multi-selection mode/"
		       "All units", TRUE);
      break;
    default:
      break;
  }
  switch (multi_select_utype) {
    case UTYPE_SAME_TYPE:
      menus_set_active("<main>/Multi-Selection/Multi-selection mode/"
		       "Only units with the same type", TRUE);
      break;
    case UTYPE_SAME_MOVE_TYPE:
      menus_set_active("<main>/Multi-Selection/Multi-selection mode/"
		       "Only units with the same move type", TRUE);
      break;
    case UTYPE_ALL:
      menus_set_active("<main>/Multi-Selection/Multi-selection mode/"
		       "All unit types", TRUE);
      break;
    default:
      break;
  }
  menus_set_active("<main>/Multi-Selection/Spread only in cities with airport",
		   spread_airport_cities);
  menus_set_active("<main>/Multi-Selection/Allow spreading into allied cities",
		   spread_allied_cities);
  update_multi_select_inclusive_filter_menu();
  update_multi_select_exclusive_filter_menu();

  /* Miscellaneous menu */
  switch (default_caravan_action) {
    case 0:
      menus_set_active("<main>/Miscellaneous/Caravan action upon arrival/"
		       "Popup dialog", TRUE);
      break;
    case 1:
      menus_set_active("<main>/Miscellaneous/Caravan action upon arrival/"
		       "Establish trade route", TRUE);
      break;
    case 2:
      menus_set_active("<main>/Miscellaneous/Caravan action upon arrival/"
		       "Help building wonder", TRUE);
      break;
    case 3:
      menus_set_active("<main>/Miscellaneous/Caravan action upon arrival/"
		       "Keep going", TRUE);
      break;
    default:
      break;
  }
  switch (default_diplomat_action) {
    case 0:
      menus_set_active("<main>/Miscellaneous/Diplomat action upon arrival/"
		       "Popup dialog", TRUE);
      break;
    case 1:
      menus_set_active("<main>/Miscellaneous/Diplomat action upon arrival/"
		       "Bribe unit", TRUE);
      break;
    case 2:
      menus_set_active("<main>/Miscellaneous/Diplomat action upon arrival/"
		       "Sabotage unit (spy)", TRUE);
     break;
    case 3:
      menus_set_active("<main>/Miscellaneous/Diplomat action upon arrival/"
		       "Establish embassy", TRUE);
      break;
    case 4:
      menus_set_active("<main>/Miscellaneous/Diplomat action upon arrival/"
		       "Investigate city", TRUE);
      break;
    case 5:
      menus_set_active("<main>/Miscellaneous/Diplomat action upon arrival/"
		       "Sabotage city", TRUE);
      break;
    case 6:
      menus_set_active("<main>/Miscellaneous/Diplomat action upon arrival/"
		       "Steal technology", TRUE);
      break;
    case 7:
      menus_set_active("<main>/Miscellaneous/Diplomat action upon arrival/"
		       "Incite revolt", TRUE);
      break;
    case 8:
      menus_set_active("<main>/Miscellaneous/Diplomat action upon arrival/"
		       "Poison city (spy)", TRUE);
      break;
    case 9:
      menus_set_active("<main>/Miscellaneous/Diplomat action upon arrival/"
		       "Keep going", TRUE);
      break;
    default:
      break;
  }
  menus_set_active("<main>/Miscellaneous/Diplomat action upon arrival"
                   "/Ignore allies", default_diplomat_ignore_allies);
  menus_set_active("<main>/Miscellaneous/Lock new unit default action",
		   default_action_locked);
  menus_set_active("<main>/Miscellaneous/Military units only",
		   default_action_military_only);
  menus_set_active("<main>/Miscellaneous/Autowakeup sentried units",
		   autowakeup_state);
  menus_set_active("<main>/Miscellaneous/Move and attack mode",
		   moveandattack_state);
  update_miscellaneous_menu();

  automatic_processus_iterate(pap) {
    update_automatic_processus_filter_menu(pap);
  } automatic_processus_iterate_end;

  start_turn_menus_udpate();
  update_menus();
}

/****************************************************************
Note: the menu strings should contain underscores as in the
menu_items struct. The underscores will be removed elsewhere if
the string is used for a lookup via gtk_item_factory_get_widget()
*****************************************************************/
void update_menus(void)
{
  if (!main_menubar||!can_client_change_view()) {
    return;
  }

  if(can_client_change_view()) {
    bool cond;
    int i;
    struct unit *punit;
    const char *path =
      menu_path_remove_uline("<main>/_Government/_Change Government");
    GtkWidget *parent = gtk_item_factory_get_widget(item_factory, path);

    if (parent) {
      GList *list, *iter;

      /* remove previous government entries. */
      list = gtk_container_get_children(GTK_CONTAINER(parent));
      for (iter = g_list_nth(list, 2); iter; iter = g_list_next(iter)) {
	gtk_widget_destroy(GTK_WIDGET(iter->data));
      }
      g_list_free(list);

      /* add new government entries. */
      government_iterate(g) {
        if (g->index != game.ruleset_control.government_when_anarchy) {
          GtkWidget *item, *image;
          struct Sprite *gsprite;
	  char buf[256];

	  my_snprintf(buf, sizeof(buf), _("%s..."), g->name);
          item = gtk_image_menu_item_new_with_label(buf);

	  if ((gsprite = g->sprite)) {
	    image = gtk_image_new_from_pixmap(gsprite->pixmap, gsprite->mask);
	    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
	    gtk_widget_show(image);
	  }

          g_signal_connect(item, "activate",
            G_CALLBACK(government_callback), GINT_TO_POINTER(g->index));

          if (!can_change_to_government(get_player_ptr(), g->index)) {
            gtk_widget_set_sensitive(item, FALSE);
	  }

          gtk_menu_shell_append(GTK_MENU_SHELL(parent), item);
          gtk_widget_show(item);
        }
      } government_iterate_end;
    }

    menus_set_sensitive("<main>/_Reports/S_paceship",
			(get_player_ptr()->spaceship.state!=SSHIP_NONE));

    menus_set_sensitive("<main>/_View/City G_rowth", draw_city_names);
    menus_set_sensitive("<main>/_View/Coastline", !draw_terrain);
    menus_set_sensitive("<main>/_View/Focus Unit", !draw_units);
    for (i = 1; i < MULTI_SELECT_NUM; i++) {
      update_multi_selection_menu(i);
    }

/*  This should be done in a future version for all warclient menu items
    menus_set_sensitive("<main>/Miscellaneous/Set rallies for selected cities", tiles_hilited_cities); */
    /* Remaining part of this function: Update Orders menu */

    if (!can_client_issue_orders()) {
      return;
    }

    if((punit=get_unit_in_focus())) {
      const char *irrfmt = _("Change to %s (_I)");
      const char *minfmt = _("Change to %s (_M)");
      const char *transfmt = _("Transf_orm to %s");
      char irrtext[128], mintext[128], transtext[128];
      const char *roadtext;
      Terrain_type_id  ttype;
      struct tile_type *      tinfo;

      sz_strlcpy(irrtext, _("Build _Irrigation"));
      sz_strlcpy(mintext, _("Build _Mine"));
      sz_strlcpy(transtext, _("Transf_orm Terrain"));
      
      /* Enable the button for adding to a city in all cases, so we
	 get an eventual error message from the server if we try. */
      menus_set_sensitive("<main>/_Orders/_Build City",
			  can_unit_add_or_build_city(punit) ||
			  unit_can_help_build_wonder_here(punit));
      menus_set_sensitive("<main>/_Orders/Build _Road",
                          (can_unit_do_activity(punit, ACTIVITY_ROAD) ||
                           can_unit_do_activity(punit, ACTIVITY_RAILROAD) ||
                           unit_can_est_traderoute_here(punit)));
      menus_set_sensitive("<main>/_Orders/Build _Irrigation",
                          can_unit_do_activity(punit, ACTIVITY_IRRIGATE));
      menus_set_sensitive("<main>/_Orders/Build _Mine",
                          can_unit_do_activity(punit, ACTIVITY_MINE));
      menus_set_sensitive("<main>/_Orders/Transf_orm Terrain",
			  can_unit_do_activity(punit, ACTIVITY_TRANSFORM));
      menus_set_sensitive("<main>/_Orders/Build _Fortress",
                          (can_unit_do_activity(punit, ACTIVITY_FORTRESS) ||
                           can_unit_do_activity(punit, ACTIVITY_FORTIFYING)));
      menus_set_sensitive("<main>/_Orders/Build Airbas_e",
			  can_unit_do_activity(punit, ACTIVITY_AIRBASE));
      menus_set_sensitive("<main>/_Orders/Clean _Pollution",
                          (can_unit_do_activity(punit, ACTIVITY_POLLUTION) ||
                           can_unit_paradrop(punit)));
      menus_set_sensitive("<main>/_Orders/Clean _Nuclear Fallout",
			  can_unit_do_activity(punit, ACTIVITY_FALLOUT));
      menus_set_sensitive("<main>/_Orders/_Sentry",
			  can_unit_do_activity(punit, ACTIVITY_SENTRY));
      menus_set_sensitive("<main>/_Orders/Pillage",
			  can_unit_do_activity(punit, ACTIVITY_PILLAGE));
      menus_set_sensitive("<main>/_Orders/_Disband Unit",
			  !unit_flag(punit, F_UNDISBANDABLE));
      menus_set_sensitive("<main>/_Orders/Make _Homecity",
			  can_unit_change_homecity(punit));
      menus_set_sensitive("<main>/_Orders/_Unload Transporter",
			  get_transporter_occupancy(punit) > 0);
      menus_set_sensitive("<main>/_Orders/_Load",
	can_unit_load(punit, find_transporter_for_unit(punit,
						       punit->tile)));
      menus_set_sensitive("<main>/_Orders/_Unload",
	(can_unit_unload(punit, find_unit_by_id(punit->transported_by))
	 && can_unit_exist_at_tile(punit, punit->tile)));
      menus_set_sensitive("<main>/_Orders/Wake up o_thers", 
			  is_unit_activity_on_tile(ACTIVITY_SENTRY,
                                                   punit->tile));
      menus_set_sensitive("<main>/_Orders/_Auto Settler",
                          can_unit_do_auto(punit));
      menus_set_sensitive("<main>/_Orders/Auto E_xplore",
                          can_unit_do_activity(punit, ACTIVITY_EXPLORE));
      menus_set_sensitive("<main>/_Orders/_Connect/_Road",
                          can_unit_do_connect(punit, ACTIVITY_ROAD));
      menus_set_sensitive("<main>/_Orders/_Connect/_Rail",
                          can_unit_do_connect(punit, ACTIVITY_RAILROAD));
      menus_set_sensitive("<main>/_Orders/_Connect/_Irrigate",
                          can_unit_do_connect(punit, ACTIVITY_IRRIGATE));
      menus_set_sensitive("<main>/_Orders/Return to nearest city",
			  !(is_air_unit(punit) || is_heli_unit(punit)));
      menus_set_sensitive("<main>/_Orders/Diplomat\\/Spy Actions",
                          (is_diplomat_unit(punit)
                           && diplomat_can_do_action(punit, DIPLOMAT_ANY_ACTION,
						     punit->tile)));
      menus_set_sensitive("<main>/_Orders/Explode Nuclear",
			  unit_flag(punit, F_NUCLEAR));
      if (unit_flag(punit, F_HELP_WONDER))
	menus_rename("<main>/_Orders/_Build City", _("Help _Build Wonder"));
      else if (unit_flag(punit, F_CITIES)) {
	if (map_get_city(punit->tile))
	  menus_rename("<main>/_Orders/_Build City", _("Add to City (_B)"));
	else
	  menus_rename("<main>/_Orders/_Build City", _("_Build City"));
      }
      else 
	menus_rename("<main>/_Orders/_Build City", _("_Build City"));
 
      if (unit_flag(punit, F_TRADE_ROUTE))
	menus_rename("<main>/_Orders/Build _Road", _("Make Trade _Route"));
      else if (unit_flag(punit, F_SETTLERS)) {
	if (map_has_special(punit->tile, S_ROAD)) {
	  roadtext = _("Build _Railroad");
	  road_activity=ACTIVITY_RAILROAD;  
	} 
	else {
	  roadtext = _("Build _Road");
	  road_activity=ACTIVITY_ROAD;  
	}
	menus_rename("<main>/_Orders/Build _Road", roadtext);
      }
      else
	menus_rename("<main>/_Orders/Build _Road", _("Build _Road"));

      ttype = punit->tile->terrain;
      tinfo = get_tile_type(ttype);
      if (tinfo->irrigation_result != T_NONE
	  && tinfo->irrigation_result != ttype) {
	my_snprintf(irrtext, sizeof(irrtext), irrfmt,
		    get_tile_change_menu_text(punit->tile,
					      ACTIVITY_IRRIGATE));
      } else if (map_has_special(punit->tile, S_IRRIGATION)
		 && player_knows_techs_with_flag(get_player_ptr(),
						 TF_FARMLAND)) {
	sz_strlcpy(irrtext, _("Bu_ild Farmland"));
      }
      if (tinfo->mining_result != T_NONE
	  && tinfo->mining_result != ttype) {
	my_snprintf(mintext, sizeof(mintext), minfmt,
		    get_tile_change_menu_text(punit->tile, ACTIVITY_MINE));
      }
      if (tinfo->transform_result != T_NONE
	  && tinfo->transform_result != ttype) {
	my_snprintf(transtext, sizeof(transtext), transfmt,
		    get_tile_change_menu_text(punit->tile,
					      ACTIVITY_TRANSFORM));
      }

      menus_rename("<main>/_Orders/Build _Irrigation", irrtext);
      menus_rename("<main>/_Orders/Build _Mine", mintext);
      menus_rename("<main>/_Orders/Transf_orm Terrain", transtext);

      if (can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
	menus_rename("<main>/_Orders/Build _Fortress", _("_Fortify"));
        menus_set_sensitive("<main>/_Orders/Sleep", FALSE);
      } else {
        menus_set_sensitive("<main>/_Orders/Sleep", TRUE);
        if (can_unit_do_activity(punit, ACTIVITY_FORTRESS)) {
	  menus_rename("<main>/_Orders/Build _Fortress", _("Build _Fortress"));
        }
      }

      if (unit_flag(punit, F_PARATROOPERS)) {
	menus_rename("<main>/_Orders/Clean _Pollution", _("_Paradrop"));
      } else {
	menus_rename("<main>/_Orders/Clean _Pollution", _("Clean _Pollution"));
      }

      if (!unit_flag(punit, F_SETTLERS)) {
	menus_rename("<main>/_Orders/_Auto Settler", _("_Auto Attack"));
      } else {
	menus_rename("<main>/_Orders/_Auto Settler", _("_Auto Settler"));
      }

      menus_set_sensitive("<main>/Delayed Goto/Delayed goto", TRUE);
      menus_set_sensitive("<main>/Delayed Goto/Delayed paradrop or nuke", TRUE);
      cond = (my_ai_enable && unit_type(punit)->fuel > 0);
      menus_set_sensitive("<main>/Miscellaneous/Airplane patrol",
			  cond
			  && !can_unit_do_activity(punit, ACTIVITY_AIRBASE));
      menus_set_sensitive("<main>/Miscellaneous/Airplane patrol destination",
			  cond);
      menus_set_sensitive("<main>/Multi-Selection/Multi-selection select",
			  TRUE);
      menus_set_sensitive("<main>/Multi-Selection/Multi-selection "
			  "active all units", TRUE);
      menus_set_sensitive("<main>/Multi-Selection/Multi-selection clear",
			  TRUE);
      menus_set_sensitive("<main>/Multi-Selection/Multi-selection spread",
			  TRUE);
      cond = (my_ai_trade_level && unit_flag(punit, F_TRADE_ROUTE));
      menus_set_sensitive("<main>/Auto Caravan/Set caravan destination",
			  my_ai_enable && cond);
      menus_set_sensitive("<main>/Auto Caravan/Automatic caravan orders",
			  my_ai_enable && cond);
      menus_set_sensitive("<main>/Miscellaneous/Free automatic orders",
			  punit->my_ai.control);
      menus_set_sensitive("<main>/Miscellaneous/Execute automatic orders",
			  my_ai_enable && punit->my_ai.control);

      menus_set_sensitive("<main>/_Orders", TRUE);
    } else {
      menus_set_sensitive("<main>/Delayed Goto/Delayed goto", FALSE);
      menus_set_sensitive("<main>/Delayed Goto/Delayed paradrop or nuke",
			  FALSE);
      menus_set_sensitive("<main>/Miscellaneous/Airplane patrol", FALSE);
      menus_set_sensitive("<main>/Miscellaneous/Airplane patrol destination",
			  FALSE);
      menus_set_sensitive("<main>/Multi-Selection/Multi-selection select",
			  FALSE);
      menus_set_sensitive("<main>/Multi-Selection/Multi-selection "
			  "active all units", FALSE);
      menus_set_sensitive("<main>/Multi-Selection/Multi-selection clear",
			  FALSE);
      menus_set_sensitive("<main>/Multi-Selection/Multi-selection spread",
			  FALSE);
      menus_set_sensitive("<main>/Auto Caravan/Automatic caravan orders",
			  FALSE);
      menus_set_sensitive("<main>/Auto Caravan/Set caravan destination", FALSE);
      menus_set_sensitive("<main>/Miscellaneous/Free automatic orders", FALSE);
      menus_set_sensitive("<main>/Miscellaneous/Execute automatic orders",
			  FALSE);

      menus_set_sensitive("<main>/_Orders", FALSE);
    }
  }
}
