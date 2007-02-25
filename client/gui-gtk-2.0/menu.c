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
#include "messagedlg.h"
#include "messagewin.h"
#include "multiselect.h"//*pepeto*
#include "myai.h"//*pepeto*
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

#include "menu.h"

/****************************************************************
... *pepeto*
*****************************************************************/
//names
#define FN_ALL "All units"
#define FN_NEW "New units"
#define FN_FORTIFIED "Fortified units"
#define FN_SENTRIED "Sentried units"
#define FN_VETERAN "Veteran units"
#define FN_AUTO "Auto units"
#define FN_IDLE "Idle units"
#define FN_ABLE_TO_MOVE "Units able to move"
#define FN_MILITARY "Military unit"
#define FN_OFF "Off"

#define PN_SINGLE_UNIT "Single unit"
#define PN_ON_TILE "All units on the tile"
#define PN_ON_CONTINENT "All units on the continent"
#define PN_EVERY_WHERE "All units"

#define TN_SAME_TYPE "Only units with the same type"
#define TN_SAME_MOVE_TYPE "Only units with the same move type"
#define TN_ALL "All unit types"

#define SN_SELECT "Select"
#define SN_ADD "Add to current selection"
#define SN_RECORD "Record"
#define SN_CLEAR "Clear"

#define LN_OFF "Off"
#define LN_ON "On"
#define LN_GOOD "Good"
#define LN_BEST "Best level"

//filters
#define FILTER_GEN_MID(menu)	\
	menu##_ALL,	\
	menu##_NEW,	\
	menu##_FORTIFIED,	\
	menu##_SENTRIED,	\
	menu##_VETERAN,	\
	menu##_AUTO,	\
	menu##_IDLE,	\
	menu##_ABLE_TO_MOVE,	\
	menu##_MILITARY,	\
	menu##_OFF

#define PLACE_GEN_MID(menu)	\
	menu##_SINGLE_UNIT,	\
	menu##_ON_TILE,	\
	menu##_ON_CONTINENT,	\
	menu##_EVERY_WHERE

#define UTYPE_GEN_MID(menu)	\
	menu##_SAME_TYPE,	\
	menu##_SAME_MOVE_TYPE,	\
	menu##_ALL

#define FILTERx_GEN_CALLBACK(menu,type,data,value)	\
	case menu##_##value:	\
      if(!!(type##_##data&FILTER_##value)^GTK_CHECK_MENU_ITEM(widget)->active)	\
	  {	\
		filter_change(&type##_##data,FILTER_##value);	\
		update_##type##_##data##_menu();	\
		type##_update(0);	\
	  }	\
	break;

#define FILTER_GEN_CALLBACK(menu,type,data)	\
	  FILTERx_GEN_CALLBACK(menu,type,data,ALL)	\
	  FILTERx_GEN_CALLBACK(menu,type,data,NEW)	\
	  FILTERx_GEN_CALLBACK(menu,type,data,FORTIFIED)	\
	  FILTERx_GEN_CALLBACK(menu,type,data,SENTRIED)	\
	  FILTERx_GEN_CALLBACK(menu,type,data,VETERAN)	\
	  FILTERx_GEN_CALLBACK(menu,type,data,AUTO)	\
	  FILTERx_GEN_CALLBACK(menu,type,data,IDLE)	\
	  FILTERx_GEN_CALLBACK(menu,type,data,ABLE_TO_MOVE)	\
	  FILTERx_GEN_CALLBACK(menu,type,data,MILITARY)	\
	  FILTERx_GEN_CALLBACK(menu,type,data,OFF)

#define PLUTx_GEN_CALLBACK(menu,data,prefix,value)	\
	case menu##_##value:	\
		data=prefix##_##value;	\
	break;

#define PLACE_GEN_CALLBACK(menu,data)	\
	PLUTx_GEN_CALLBACK(menu,data,PLACE,SINGLE_UNIT)	\
	PLUTx_GEN_CALLBACK(menu,data,PLACE,ON_TILE)	\
	PLUTx_GEN_CALLBACK(menu,data,PLACE,ON_CONTINENT)	\
	PLUTx_GEN_CALLBACK(menu,data,PLACE,EVERY_WHERE)
	
#define UTYPE_GEN_CALLBACK(menu,data)	\
	PLUTx_GEN_CALLBACK(menu,data,UTYPE,SAME_TYPE)	\
	PLUTx_GEN_CALLBACK(menu,data,UTYPE,SAME_MOVE_TYPE)	\
	PLUTx_GEN_CALLBACK(menu,data,UTYPE,ALL)

#define FILTERx_GEN_ITEM(access,callback,menuid)	\
	{access,NULL,callback,menuid,"<CheckItem>"}

#define FILTER_GEN_ITEMS(access,callback,menu)	\
	{access,NULL,NULL,0,"<Branch>"},	\
	{access "/tearoff1",NULL,NULL,0,"<Tearoff>"},	\
	FILTERx_GEN_ITEM(access "/" N_(FN_ALL),callback,menu##_ALL),	\
	FILTERx_GEN_ITEM(access "/" N_(FN_NEW),callback,menu##_NEW),	\
	FILTERx_GEN_ITEM(access "/" N_(FN_FORTIFIED),callback,menu##_FORTIFIED),	\
	FILTERx_GEN_ITEM(access "/" N_(FN_SENTRIED),callback,menu##_SENTRIED),	\
	FILTERx_GEN_ITEM(access "/" N_(FN_VETERAN),callback,menu##_VETERAN),	\
	FILTERx_GEN_ITEM(access "/" N_(FN_AUTO),callback,menu##_AUTO),	\
	FILTERx_GEN_ITEM(access "/" N_(FN_IDLE),callback,menu##_IDLE),	\
	FILTERx_GEN_ITEM(access "/" N_(FN_ABLE_TO_MOVE),callback,menu##_ABLE_TO_MOVE),	\
	FILTERx_GEN_ITEM(access "/" N_(FN_MILITARY),callback,menu##_MILITARY),	\
	FILTERx_GEN_ITEM(access "/" N_(FN_OFF),callback,menu##_OFF)

#define PLUTx_GEN_ITEM(access,key,callback,menuid,radioitem)	\
	{access,key,callback,menuid,radioitem}

#define PLACE_GEN_ITEMS(access,key,callback,menu)	\
	PLUTx_GEN_ITEM(access "/" N_(PN_SINGLE_UNIT),key "<shift>F1",callback,menu##_SINGLE_UNIT,"<RadioItem>"),	\
	PLUTx_GEN_ITEM(access "/" N_(PN_ON_TILE),key "<shift>F2",callback,menu##_ON_TILE,"<main>" access "/" PN_SINGLE_UNIT),	\
	PLUTx_GEN_ITEM(access "/" N_(PN_ON_CONTINENT),key "<shift>F3",callback,menu##_ON_CONTINENT,"<main>" access "/" PN_SINGLE_UNIT),	\
	PLUTx_GEN_ITEM(access "/" N_(PN_EVERY_WHERE),key "<shift>F4",callback,menu##_EVERY_WHERE,"<main>" access "/" PN_SINGLE_UNIT)

#define UTYPE_GEN_ITEMS(access,key,callback,menu)	\
	PLUTx_GEN_ITEM(access "/" N_(TN_SAME_TYPE),key "<shift>F5",callback,menu##_SAME_TYPE,"<RadioItem>"),	\
	PLUTx_GEN_ITEM(access "/" N_(TN_SAME_MOVE_TYPE),key "<shift>F6",callback,menu##_SAME_MOVE_TYPE,"<main>" access "/" TN_SAME_TYPE),	\
	PLUTx_GEN_ITEM(access "/" N_(TN_ALL),key "<shift>F7",callback,menu##_ALL,"<main>" access "/" TN_SAME_TYPE)

#define UPDATE_FILTER_NAME(filter)	\
	update_##filter##_menu

#define UPDATE_FILTER_PROTOTYPE(filter)	\
	static void UPDATE_FILTER_NAME(filter)(void)

#define UPDATE_FILTER(filter,access)	\
	UPDATE_FILTER_PROTOTYPE(filter)	\
	{	\
		menus_set_active(access "/" FN_ALL,filter&FILTER_ALL);	\
		menus_set_active(access "/" FN_NEW,filter&FILTER_NEW);	\
		menus_set_active(access "/" FN_FORTIFIED,filter&FILTER_FORTIFIED);	\
		menus_set_active(access "/" FN_SENTRIED,filter&FILTER_SENTRIED);	\
		menus_set_active(access "/" FN_VETERAN,filter&FILTER_VETERAN);	\
		menus_set_active(access "/" FN_AUTO,filter&FILTER_AUTO);	\
		menus_set_active(access "/" FN_IDLE,filter&FILTER_IDLE);	\
		menus_set_active(access "/" FN_ABLE_TO_MOVE,filter&FILTER_ABLE_TO_MOVE);	\
		menus_set_active(access "/" FN_MILITARY,filter&FILTER_MILITARY);	\
		menus_set_active(access "/" FN_OFF,filter&FILTER_OFF);	\
	}

//selections
#define SELECT_GEN_MID(menu,selection)	\
	menu##selection##_SELECT,	\
	menu##selection##_ADD,	\
	menu##selection##_RECORD,	\
	menu##selection##_CLEAR
	
#define DGS_GEN_MID(menu,selection)	\
	SELECT_GEN_MID(menu,selection),	\
	menu##selection##_EXECUTE

#define SELECT_GEN_CALLBACK(menu,selection,data)	\
	case menu##selection##_SELECT:	\
		data##_copy(0,selection);	\
		update_menus();	\
	break;	\
	case menu##selection##_ADD:	\
		data##_cat(0,selection);	\
		update_menus();	\
	break;	\
	case menu##selection##_RECORD:	\
		data##_copy(selection,0);	\
		update_menus();	\
	break;	\
	case menu##selection##_CLEAR:	\
		data##_clear(selection);	\
		update_menus();	\
	break;

#define DGS_GEN_CALLBACK(menu,selection,data)	\
	SELECT_GEN_CALLBACK(menu,selection,data)	\
	case menu##selection##_EXECUTE:	\
		request_unit_execute_delayed_goto(NULL,selection);	\
	break;

#define SELECT_GEN_ITEMS(access,key,callback,menu,selection)	\
	{access " " #selection,NULL,NULL,0,"<Branch>"},	\
	{access " " #selection "/tearoff1",NULL,NULL,0,"<Tearoff>"},	\
	{access " " #selection "/" N_(SN_SELECT),key #selection,	\
		callback,menu##selection##_SELECT,NULL},	\
	{access " " #selection "/" N_(SN_ADD),key "<shift>" #selection,	\
		callback,menu##selection##_ADD,NULL},	\
	{access " " #selection "/" N_(SN_RECORD),key "<ctrl>" #selection,	\
		callback,menu##selection##_RECORD,NULL},	\
	{access " " #selection "/" N_(SN_CLEAR),key "<ctrl><shift>" #selection,	\
		callback,menu##selection##_CLEAR,NULL}

#define DGS_GEN_ITEMS(access,key,callback,menu,selection)	\
	{access " " #selection,NULL,NULL,0,"<Branch>"},	\
	{access " " #selection "/tearoff1",NULL,NULL,0,"<Tearoff>"},	\
	{access " " #selection "/" N_(SN_SELECT),key #selection,	\
		callback,menu##selection##_SELECT,NULL},	\
	{access " " #selection "/" N_(SN_ADD),key "<shift>" #selection,	\
		callback,menu##selection##_ADD,NULL},	\
	{access " " #selection "/" N_(SN_RECORD),key "<ctrl>" #selection,	\
		callback,menu##selection##_RECORD,NULL},	\
	{access " " #selection "/" N_(SN_CLEAR),NULL,	\
		callback,menu##selection##_CLEAR,NULL},	\
	{access " " #selection "/" N_("Execute"),key "<ctrl><shift>" #selection,	\
		callback,menu##selection##_EXECUTE,NULL},	\
	{access " " #selection "/" N_("Automatic execution"),NULL,NULL,0,"<Branch>"},	\
	{access " " #selection "/" N_("Automatic execution") "/tearoff1",NULL,NULL,0,"<Tearoff>"}

#define UPDATE_SELECTION(access,type)	\
	{	\
		type##_update(0); \
		char _buf[256],*_s,*_m;	\
		int _cond=type##_size(0),_i;	\
		strcpy(_buf,access);	\
		_s=_buf+strlen(_buf);	\
		_m=_s+2;	\
		for(_i=1;_i<=9;_i++)	\
		{	\
			type##_update(_i); \
			my_snprintf(_s,5," %d",_i);	\
			if(type##_size(_i))	\
			{	\
				menus_set_sensitive(_buf,TRUE);	\
				strcpy(_m,"/" SN_SELECT);	\
				menus_set_sensitive(_buf,TRUE);	\
				strcpy(_m,"/" SN_ADD);	\
				menus_set_sensitive(_buf,TRUE);	\
				strcpy(_m,"/" SN_RECORD);	\
				menus_set_sensitive(_buf,_cond);	\
				strcpy(_m,"/" SN_CLEAR);	\
				menus_set_sensitive(_buf,TRUE);	\
			}	\
			else if(_cond)	\
			{	\
				menus_set_sensitive(_buf,TRUE);	\
				strcpy(_m,"/" SN_SELECT);	\
				menus_set_sensitive(_buf,FALSE);	\
				strcpy(_m,"/" SN_ADD);	\
				menus_set_sensitive(_buf,FALSE);	\
				strcpy(_m,"/" SN_RECORD);	\
				menus_set_sensitive(_buf,TRUE);	\
				strcpy(_m,"/" SN_CLEAR);	\
				menus_set_sensitive(_buf,FALSE);	\
			}	\
			else	\
				menus_set_sensitive(_buf,FALSE);	\
		}	\
	}

#define UPDATE_DGS(access,type)	\
	{	\
		type##_update(0); \
		char _buf[256],*_s,*_m;	\
		int _cond=type##_size(0),_i;	\
		strcpy(_buf,access);	\
		_s=_buf+strlen(_buf);	\
		_m=_s+2;	\
		for(_i=1;_i<=9;_i++)	\
		{	\
			type##_update(_i); \
			my_snprintf(_s,5," %d",_i);	\
			if(type##_size(_i))	\
			{	\
				menus_set_sensitive(_buf,TRUE);	\
				strcpy(_m,"/" SN_SELECT);	\
				menus_set_sensitive(_buf,TRUE);	\
				strcpy(_m,"/" SN_ADD);	\
				menus_set_sensitive(_buf,TRUE);	\
				strcpy(_m,"/" SN_RECORD);	\
				menus_set_sensitive(_buf,_cond);	\
				strcpy(_m,"/" SN_CLEAR);	\
				menus_set_sensitive(_buf,TRUE);	\
				strcpy(_m,"/Execute");	\
				menus_set_sensitive(_buf,TRUE);	\
			}	\
			else if(_cond)	\
			{	\
				menus_set_sensitive(_buf,TRUE);	\
				strcpy(_m,"/" SN_SELECT);	\
				menus_set_sensitive(_buf,FALSE);	\
				strcpy(_m,"/" SN_ADD);	\
				menus_set_sensitive(_buf,FALSE);	\
				strcpy(_m,"/" SN_RECORD);	\
				menus_set_sensitive(_buf,TRUE);	\
				strcpy(_m,"/" SN_CLEAR);	\
				menus_set_sensitive(_buf,FALSE);	\
				strcpy(_m,"/Execute");	\
				menus_set_sensitive(_buf,FALSE);	\
			}	\
			else	\
				menus_set_sensitive(_buf,FALSE);	\
		}	\
	}

//levels for my ai
#define LEVEL_GEN_MID(menu)	\
	menu##_OFF,	\
	menu##_ON,	\
	menu##_GOOD,	\
	menu##_BEST

#define LEVELx_GEN_CALLBACK(menu,data,prefix,value)	\
	case menu##_##value:	\
		if(GTK_CHECK_MENU_ITEM(widget)->active)	\
		{	\
			data=prefix##_##value;	\
			update_menus();	\
		}	\
	break;

#define LEVEL_GEN_CALLBACK(menu,data)	\
	LEVELx_GEN_CALLBACK(menu,data,LEVEL,OFF)	\
	LEVELx_GEN_CALLBACK(menu,data,LEVEL,ON)	\
	LEVELx_GEN_CALLBACK(menu,data,LEVEL,GOOD)	\
	LEVELx_GEN_CALLBACK(menu,data,LEVEL,BEST)
	
#define LEVEL_GEN_ITEMS(access,callback,menu)	\
	{access,NULL,NULL,0,"<Branch>"},	\
	{access "/tearoff1",NULL,NULL,0,"<Tearoff>"},	\
	PLUTx_GEN_ITEM(access "/" N_(LN_OFF),"",callback,menu##_OFF,"<RadioItem>"),	\
	PLUTx_GEN_ITEM(access "/" N_(LN_ON),"",callback,menu##_ON,"<main>" access "/" LN_OFF),	\
	PLUTx_GEN_ITEM(access "/" N_(LN_GOOD),"",callback,menu##_GOOD,"<main>" access "/" LN_OFF),	\
	PLUTx_GEN_ITEM(access "/" N_(LN_BEST),"",callback,menu##_BEST,"<main>" access "/" LN_OFF)

//set active
#define PLACE_SET_ACTIVE(access,data)	\
	switch(data)	\
	{	\
		case PLACE_SINGLE_UNIT:menus_set_active(access "/" PN_SINGLE_UNIT,TRUE);break;	\
		case PLACE_ON_TILE:menus_set_active(access "/" PN_ON_TILE,data==TRUE);break;	\
		case PLACE_ON_CONTINENT:menus_set_active(access "/" PN_ON_CONTINENT,data==TRUE);break;	\
		case PLACE_EVERY_WHERE:menus_set_active(access "/" PN_EVERY_WHERE,data==TRUE);break;	\
		default:break;	\
	}

#define UTYPE_SET_ACTIVE(access,data)	\
	switch(data)	\
	{	\
		case UTYPE_SAME_TYPE:menus_set_active(access "/" TN_SAME_TYPE,TRUE);break;	\
		case UTYPE_SAME_MOVE_TYPE:menus_set_active(access "/" TN_SAME_MOVE_TYPE,data==TRUE);break;	\
		case UTYPE_ALL:menus_set_active(access "/" TN_ALL,data==TRUE);break;	\
		default:break;	\
	}

//airlift menu
struct airlift_menu_data {
	Unit_Type_id utype;
	GtkWidget *widget;
};

void create_automatic_processus_menus(void);
void update_automatic_processus_filter_menu(automatic_processus *pap);

static struct airlift_menu_data airlift_menu_datas[U_LAST];
static int airlift_menu_num=0;
static GSList *group;

static void get_accel_label(GtkWidget *widget,const char *uname);
static bool can_player_unit_type(Unit_Type_id utype);
static void airlift_menu_set_sensitive(void);
static void airlift_menu_set_active(void);
static void airlift_menu_callback(GtkWidget *widget,gpointer data);
static void create_airlift_menu(GtkWidget *widget);

static GtkItemFactory *item_factory = NULL;
static GtkWidget *main_menubar = NULL;
GtkAccelGroup *toplevel_accel = NULL;
static enum unit_activity road_activity;

UPDATE_FILTER_PROTOTYPE(delayed_goto_inclusive_filter);
UPDATE_FILTER_PROTOTYPE(delayed_goto_exclusive_filter);
UPDATE_FILTER_PROTOTYPE(multi_select_inclusive_filter);
UPDATE_FILTER_PROTOTYPE(multi_select_exclusive_filter);

static void menus_rename(const char *path, const char *s);

/****************************************************************
...
*****************************************************************/
enum MenuID {
  MENU_END_OF_LIST=0,

  MENU_GAME_OPTIONS,
  MENU_GAME_MSG_OPTIONS,
  MENU_GAME_CHATLINE_COLORS,
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

  MENU_WARCLIENT_DELAYED_GOTO,
  MENU_WARCLIENT_DELAYED_PARADROP_OR_NUKE,
  MENU_WARCLIENT_DELAYED_AIRLIFT,
  MENU_WARCLIENT_EXECUTE_DELAYED_GOTO,
  MENU_WARCLIENT_CLEAR_DELAYED_ORDERS,
  MENU_WARCLIENT_TOGGLE_WAKEUP,
  MENU_WARCLIENT_TOGGLE_MOVEANDATTACK,
  PLACE_GEN_MID(MENU_WARCLIENT_GOTO),
  UTYPE_GEN_MID(MENU_WARCLIENT_GOTO),
  DGS_GEN_MID(MENU_WARCLIENT_DG,1),
  DGS_GEN_MID(MENU_WARCLIENT_DG,2),
  DGS_GEN_MID(MENU_WARCLIENT_DG,3),
  DGS_GEN_MID(MENU_WARCLIENT_DG,4),
  DGS_GEN_MID(MENU_WARCLIENT_DG,5),
  DGS_GEN_MID(MENU_WARCLIENT_DG,6),
  DGS_GEN_MID(MENU_WARCLIENT_DG,7),
  DGS_GEN_MID(MENU_WARCLIENT_DG,8),
  DGS_GEN_MID(MENU_WARCLIENT_DG,9),
  MENU_WARCLIENT_SET_AIRLIFT_DEST,
  MENU_WARCLIENT_SET_AIRLIFT_SRC,
  MENU_WARCLIENT_CLEAR_AIRLIFT_QUEUE,
  MENU_WARCLIENT_SHOW_CITIES_IN_AIRLIFT_QUEUE,
  MENU_WARCLIENT_PATROL_DEST,
  MENU_WARCLIENT_PATROL_FORCE,
  MENU_WARCLIENT_PATROL_EXECUTE,
  MENU_WARCLIENT_SET_RALLIES,
  MENU_WARCLIENT_CLEAR_RALLIES,
  MENU_WARCLIENT_CARAVAN_POPUP,
  MENU_WARCLIENT_CARAVAN_TRADE,
  MENU_WARCLIENT_CARAVAN_BUILD,
  MENU_WARCLIENT_CARAVAN_NOTHING,
  MENU_WARCLIENT_DIPLOMAT_POPUP,
  MENU_WARCLIENT_DIPLOMAT_BRIBE,
  MENU_WARCLIENT_DIPLOMAT_SABOTAGE_UNIT,
  MENU_WARCLIENT_DIPLOMAT_EMBASSY,
  MENU_WARCLIENT_DIPLOMAT_INVESTIGATE,
  MENU_WARCLIENT_DIPLOMAT_SABOTAGE_CITY,
  MENU_WARCLIENT_DIPLOMAT_STEAL,
  MENU_WARCLIENT_DIPLOMAT_INCITE,
  MENU_WARCLIENT_DIPLOMAT_POISON,
  MENU_WARCLIENT_DIPLOMAT_NOTHING,
  FILTER_GEN_MID(MENU_WARCLIENT_INCLUSIVE),
  FILTER_GEN_MID(MENU_WARCLIENT_EXCLUSIVE),
  
  MENU_PEPCLIENT_MS_SELECT,
  MENU_PEPCLIENT_MS_ACTIVE_ALL,
  MENU_PEPCLIENT_MS_CLEAR,
  FILTER_GEN_MID(MENU_PEPCLIENT_INCLUSIVE),
  FILTER_GEN_MID(MENU_PEPCLIENT_EXCLUSIVE),
  PLACE_GEN_MID(MENU_PEPCLIENT_MODE),
  UTYPE_GEN_MID(MENU_PEPCLIENT_MODE),
  MENU_PEPCLIENT_MY_AI_SPREAD,
  MENU_PEPCLIENT_MY_AI_SPREAD_AIRPORT,
  SELECT_GEN_MID(MENU_PEPCLIENT_MS,1),
  SELECT_GEN_MID(MENU_PEPCLIENT_MS,2),
  SELECT_GEN_MID(MENU_PEPCLIENT_MS,3),
  SELECT_GEN_MID(MENU_PEPCLIENT_MS,4),
  SELECT_GEN_MID(MENU_PEPCLIENT_MS,5),
  SELECT_GEN_MID(MENU_PEPCLIENT_MS,6),
  SELECT_GEN_MID(MENU_PEPCLIENT_MS,7),
  SELECT_GEN_MID(MENU_PEPCLIENT_MS,8),
  SELECT_GEN_MID(MENU_PEPCLIENT_MS,9),
  MENU_PEPCLIENT_MY_AI_EXECUTE,
  MENU_PEPCLIENT_MY_AI_FREE,
  MENU_PEPCLIENT_MY_AI_EXECUTE_ALL,
  MENU_PEPCLIENT_MY_AI_TRADE_CITY,
  MENU_PEPCLIENT_MY_AI_CLEAR_TRADE_CITY,
  MENU_PEPCLIENT_MY_AI_SHOW_TRADE_CITIES,
  MENU_PEPCLIENT_MY_AI_SHOW_FREE_SLOTS,
  MENU_PEPCLIENT_MY_AI_TRADE_RECALCULATE,
  MENU_PEPCLIENT_MY_AI_TRADE_WITH,
  MENU_PEPCLIENT_MY_AI_CARAVAN,
  MENU_PEPCLIENT_MY_AI_TRADE_EXECUTE,
  MENU_PEPCLIENT_MY_AI_WONDER_EXECUTE,

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
    popup_worklists_report();
    break;
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
   case MENU_ORDER_AIRBASE:
    if (get_unit_in_focus()) {
      if (can_unit_do_activity(get_unit_in_focus(), ACTIVITY_AIRBASE))
        key_unit_airbase();
      else if(hover_state==HOVER_MYPATROL)
      {
      	multi_select_iterate(TRUE,punit)
      	{
      		my_ai_patrol_alloc(punit,punit->tile,TRUE);
      	} multi_select_iterate_end;
      	hover_state=HOVER_NONE;
      	update_hover_cursor();
      }
      else
        key_airplane_patrol(TRUE);
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
		if(unit_flag(get_unit_in_focus(),F_TRADE_ROUTE)&&hover_state!=HOVER_MY_AI_TRADE)
			key_my_ai_trade();
		else
		{
			hover_state=HOVER_NONE;
			update_hover_cursor();
			key_unit_homecity();
		}
		break;
    break;
   case MENU_ORDER_UNLOAD_TRANSPORTER:
    key_unit_unload_all();
    break;
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
    key_unit_connect(ACTIVITY_RAILROAD);
    break;
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
static void warclient_menu_callback(gpointer callback_data,
				 guint callback_action, GtkWidget *widget)
{
  switch(callback_action) {
   case MENU_WARCLIENT_DELAYED_GOTO:
	key_unit_delayed_goto(0);
    break;
   case MENU_WARCLIENT_DELAYED_PARADROP_OR_NUKE:
	key_unit_delayed_goto(1);
    break;
   case MENU_WARCLIENT_DELAYED_AIRLIFT:
    key_unit_delayed_airlift();
    break;
   case MENU_WARCLIENT_EXECUTE_DELAYED_GOTO:
	key_unit_execute_delayed_goto();
    break;
   case MENU_WARCLIENT_CLEAR_DELAYED_ORDERS:
	key_unit_clear_delayed_orders();
    break;
   case MENU_WARCLIENT_TOGGLE_WAKEUP:
    if (autowakeup_state ^ GTK_CHECK_MENU_ITEM(widget)->active)
		key_toggle_autowakeup();
	break;
   case MENU_WARCLIENT_TOGGLE_MOVEANDATTACK:
    if (moveandattack_state ^ GTK_CHECK_MENU_ITEM(widget)->active)
      key_toggle_moveandattack();
  	break;
   PLACE_GEN_CALLBACK(MENU_WARCLIENT_GOTO,delayed_goto_place)
   UTYPE_GEN_CALLBACK(MENU_WARCLIENT_GOTO,delayed_goto_utype)
   case MENU_WARCLIENT_SET_AIRLIFT_DEST:
        request_auto_airlift_destination_selection();
        break;
   case MENU_WARCLIENT_SET_AIRLIFT_SRC:
        request_auto_airlift_source_selection();
        break;
   case MENU_WARCLIENT_CLEAR_AIRLIFT_QUEUE:
        request_clear_auto_airlift_queue();
        break;
   case MENU_WARCLIENT_SHOW_CITIES_IN_AIRLIFT_QUEUE:
        show_cities_in_airlift_queue();
        break;
   case   MENU_WARCLIENT_PATROL_DEST:
      if(hover_state==HOVER_MYPATROL)
      {
      	multi_select_iterate(TRUE,punit)
      	{
      		my_ai_patrol_alloc(punit,punit->tile,FALSE);
      	} multi_select_iterate_end;
      	hover_state=HOVER_NONE;
      	update_hover_cursor();
      }
      else
        key_airplane_patrol(FALSE);
    break;
   case   MENU_WARCLIENT_PATROL_FORCE:
    if (get_unit_in_focus()) {
      if (can_unit_do_activity(get_unit_in_focus(), ACTIVITY_AIRBASE))
        key_unit_airbase();
      else if(hover_state==HOVER_MYPATROL)
      {
      	multi_select_iterate(TRUE,punit)
      	{
      		my_ai_patrol_alloc(punit,punit->tile,TRUE);
      	} multi_select_iterate_end;
      	hover_state=HOVER_NONE;
      	update_hover_cursor();
      }
      else
        key_airplane_patrol(TRUE);
    }
    break;
   case MENU_WARCLIENT_PATROL_EXECUTE:
		my_ai_patrol_execute_all();
    break; 
   case MENU_WARCLIENT_SET_RALLIES:
        key_select_rally_point();
        break;
   case MENU_WARCLIENT_CLEAR_RALLIES:
        key_clear_rally_point_for_selected_cities();
        break;
   case MENU_WARCLIENT_CARAVAN_POPUP:
        default_caravan_action = 0;
        break;
   case MENU_WARCLIENT_CARAVAN_TRADE:
        default_caravan_action = 1;
        break;
   case MENU_WARCLIENT_CARAVAN_BUILD:
        default_caravan_action = 2;
        break;
   case MENU_WARCLIENT_CARAVAN_NOTHING:
        default_caravan_action = 3;
        break;
   case MENU_WARCLIENT_DIPLOMAT_POPUP:
        default_diplomat_action = 0;
        break;
   case MENU_WARCLIENT_DIPLOMAT_BRIBE:
        default_diplomat_action = 1;
        break;
   case MENU_WARCLIENT_DIPLOMAT_SABOTAGE_UNIT:
        default_diplomat_action = 2;
        break;
   case MENU_WARCLIENT_DIPLOMAT_EMBASSY:
        default_diplomat_action = 3;
        break;
   case MENU_WARCLIENT_DIPLOMAT_INVESTIGATE:
        default_diplomat_action = 4;
        break;
   case MENU_WARCLIENT_DIPLOMAT_SABOTAGE_CITY:
        default_diplomat_action = 5;
        break;
   case MENU_WARCLIENT_DIPLOMAT_STEAL:
        default_diplomat_action = 6;
        break;
   case MENU_WARCLIENT_DIPLOMAT_INCITE:
        default_diplomat_action = 7;
        break;
   case MENU_WARCLIENT_DIPLOMAT_POISON:
        default_diplomat_action = 8;
        break;
   case MENU_WARCLIENT_DIPLOMAT_NOTHING:
        default_diplomat_action = 9;
        break;
   FILTER_GEN_CALLBACK(MENU_WARCLIENT_INCLUSIVE,delayed_goto,inclusive_filter)
   FILTER_GEN_CALLBACK(MENU_WARCLIENT_EXCLUSIVE,delayed_goto,exclusive_filter)
   DGS_GEN_CALLBACK(MENU_WARCLIENT_DG,1,delayed_goto)
   DGS_GEN_CALLBACK(MENU_WARCLIENT_DG,2,delayed_goto)
   DGS_GEN_CALLBACK(MENU_WARCLIENT_DG,3,delayed_goto)
   DGS_GEN_CALLBACK(MENU_WARCLIENT_DG,4,delayed_goto)
   DGS_GEN_CALLBACK(MENU_WARCLIENT_DG,5,delayed_goto)
   DGS_GEN_CALLBACK(MENU_WARCLIENT_DG,6,delayed_goto)
   DGS_GEN_CALLBACK(MENU_WARCLIENT_DG,7,delayed_goto)
   DGS_GEN_CALLBACK(MENU_WARCLIENT_DG,8,delayed_goto)
   DGS_GEN_CALLBACK(MENU_WARCLIENT_DG,9,delayed_goto)
  }
}

/****************************************************************
...
*****************************************************************/
static void pepclient_menu_callback(gpointer callback_data,
				  guint callback_action, GtkWidget *widget)
{
  switch(callback_action) {
	case MENU_PEPCLIENT_MS_SELECT:
		multi_select_select();
		break;
	case MENU_PEPCLIENT_MS_ACTIVE_ALL:
		multi_select_active_all(0);
		break;
	case MENU_PEPCLIENT_MS_CLEAR:
		multi_select_clear(0);
		break;
	PLACE_GEN_CALLBACK(MENU_PEPCLIENT_MODE,multi_select_place)
	UTYPE_GEN_CALLBACK(MENU_PEPCLIENT_MODE,multi_select_utype)
	FILTER_GEN_CALLBACK(MENU_PEPCLIENT_INCLUSIVE,multi_select,inclusive_filter)
	FILTER_GEN_CALLBACK(MENU_PEPCLIENT_EXCLUSIVE,multi_select,exclusive_filter)
	SELECT_GEN_CALLBACK(MENU_PEPCLIENT_MS,1,multi_select)
	SELECT_GEN_CALLBACK(MENU_PEPCLIENT_MS,2,multi_select)
	SELECT_GEN_CALLBACK(MENU_PEPCLIENT_MS,3,multi_select)
	SELECT_GEN_CALLBACK(MENU_PEPCLIENT_MS,4,multi_select)
	SELECT_GEN_CALLBACK(MENU_PEPCLIENT_MS,5,multi_select)
	SELECT_GEN_CALLBACK(MENU_PEPCLIENT_MS,6,multi_select)
	SELECT_GEN_CALLBACK(MENU_PEPCLIENT_MS,7,multi_select)
	SELECT_GEN_CALLBACK(MENU_PEPCLIENT_MS,8,multi_select)
	SELECT_GEN_CALLBACK(MENU_PEPCLIENT_MS,9,multi_select)
	case MENU_PEPCLIENT_MY_AI_SPREAD:
		my_ai_spread_execute();
		break;
	case MENU_PEPCLIENT_MY_AI_SPREAD_AIRPORT:
		if(spread_airport_cities^GTK_CHECK_MENU_ITEM(widget)->active)
      	key_toggle_spread_airport();
      break;
	case MENU_PEPCLIENT_MY_AI_EXECUTE:
		multi_select_iterate(TRUE,punit)
		{
			my_ai_unit_execute(punit);
		} multi_select_iterate_end;
		break;
	case MENU_PEPCLIENT_MY_AI_FREE:
		unit_list_iterate(*multi_select_get_units_focus(),punit)
		{
			my_ai_orders_free(punit);
		} unit_list_iterate_end;
		break;
	case MENU_PEPCLIENT_MY_AI_EXECUTE_ALL:
		my_ai_execute();
		break;
	case MENU_PEPCLIENT_MY_AI_TRADE_CITY:
		key_my_ai_trade_city();
		break;
	case MENU_PEPCLIENT_MY_AI_CLEAR_TRADE_CITY:
		clear_my_ai_trade_cities();
		break;
	case MENU_PEPCLIENT_MY_AI_SHOW_TRADE_CITIES:
		show_cities_in_trade_plan();
		break;
	case MENU_PEPCLIENT_MY_AI_SHOW_FREE_SLOTS:
		show_free_slots_in_trade_plan();
		break;
	case MENU_PEPCLIENT_MY_AI_TRADE_RECALCULATE:
		recalculate_trade_plan();
		break;
	case MENU_PEPCLIENT_MY_AI_TRADE_WITH:
		if(unit_flag(get_unit_in_focus(),F_TRADE_ROUTE)&&hover_state!=HOVER_MY_AI_TRADE)
			key_my_ai_trade();
		else
		{
			hover_state=HOVER_NONE;
			update_hover_cursor();
			key_unit_homecity();
		}
		break;
	case MENU_PEPCLIENT_MY_AI_CARAVAN:
		multi_select_iterate(TRUE,punit)
		{
			my_ai_caravan(punit);
		} multi_select_iterate_end;
		break;
	case MENU_PEPCLIENT_MY_AI_TRADE_EXECUTE:
		my_ai_trade_route_execute_all();
		break;
	case MENU_PEPCLIENT_MY_AI_WONDER_EXECUTE:
		my_ai_help_wonder_execute_all();
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
    popup_spaceship_dialog(game.player_ptr);
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
  { "/" N_("Game") "/" N_("_Chat Colors"),		NULL,
	game_menu_callback,	MENU_GAME_CHATLINE_COLORS					},
  { "/" N_("Game") "/" N_("Sa_ve Settings"),		NULL,
	game_menu_callback,	MENU_GAME_SAVE_SETTINGS					},
  { "/" N_("Game") "/sep2",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Game") "/" N_("PepClient Settings"),		NULL,
	game_menu_callback,	MENU_GAME_PEPSETTINGS					},
  { "/" N_("Game") "/" N_("_Load PepClient Settings"),		NULL,
	game_menu_callback,	MENU_GAME_PEPSETTINGS_LOAD					},
  { "/" N_("Game") "/" N_("Save PepClient Settings"),		NULL,
	game_menu_callback,	MENU_GAME_PEPSETTINGS_SAVE					},
  { "/" N_("Game") "/sep3",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Game") "/" N_("_Initial Server Options"),NULL,
	game_menu_callback,	MENU_GAME_SERVER_OPTIONS1				},
  { "/" N_("Game") "/" N_("Server O_ptions"),	NULL,
	game_menu_callback,	MENU_GAME_SERVER_OPTIONS2				},
  { "/" N_("Game") "/sep4",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Game") "/" N_("_Save Game"),		NULL,
	game_menu_callback,	MENU_GAME_SAVE_QUICK, 			"<StockItem>",
	GTK_STOCK_SAVE									},
  { "/" N_("Game") "/" N_("Save Game _As..."),		NULL,
	game_menu_callback,	MENU_GAME_SAVE_GAME,			"<StockItem>",
	GTK_STOCK_SAVE_AS								},
  { "/" N_("Game") "/sep5",				NULL,
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
	GTK_STOCK_QUIT									},
  /* Government menu ... */
  { "/" N_("Gover_nment"),					NULL,
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
  { "/" N_("Government") "/" N_("_Clear Selected Worklists"),		"<control>w",
	government_menu_callback,	MENU_GOVERNMENT_CLEAR_SELECTED_WORKLISTS			},
  { "/" N_("Government") "/sep2",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Government") "/" N_("_Change Government"),           NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Government") "/" N_("_Change Government") "/" N_("_Revolution..."),
                                                        NULL,
	government_menu_callback,	MENU_GOVERNMENT_REVOLUTION			},
  { "/" N_("_Government") "/" N_("_Change Government") "/sep1", NULL,
	NULL,			0,					"<Separator>"	},
  /* View menu ... */
  { "/" N_("_View"),					NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("View") "/tearoff1",				NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("View") "/" N_("Map _Grid"),			"<control>g",
	view_menu_callback,	MENU_VIEW_SHOW_MAP_GRID,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("National _Borders"),		"<control>b",
	view_menu_callback,	MENU_VIEW_SHOW_NATIONAL_BORDERS,	"<CheckItem>"	},
  { "/" N_("View") "/" N_("City _Names"),		"<control>n",
	view_menu_callback,	MENU_VIEW_SHOW_CITY_NAMES,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("City G_rowth"),		"",
	view_menu_callback,	MENU_VIEW_SHOW_CITY_GROWTH_TURNS,
	"<CheckItem>"	},
  { "/" N_("View") "/" N_("City _Productions"),		"<control>p",
	view_menu_callback,	MENU_VIEW_SHOW_CITY_PRODUCTIONS,	"<CheckItem>"	},
  { "/" N_("View") "/" N_("City Production Buy _Cost"),		"",
	view_menu_callback,	MENU_VIEW_SHOW_CITY_PRODUCTION_BUY_COST,	"<CheckItem>"	},
  { "/" N_("View") "/" N_("City _Trade Routes"),		"<control>t",
	view_menu_callback,	MENU_VIEW_SHOW_CITY_TRADEROUTES,	"<CheckItem>"	},
  { "/" N_("View") "/sep1",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("View") "/" N_("Terrain"),                   NULL,
        view_menu_callback,     MENU_VIEW_SHOW_TERRAIN,	                "<CheckItem>"   },
  { "/" N_("View") "/" N_("Coastline"),	                NULL,
        view_menu_callback,     MENU_VIEW_SHOW_COASTLINE,       	"<CheckItem>"   },
  { "/" N_("View") "/" N_("Improvements"),		NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("View") "/" N_("Improvements") "/tearoff1",	NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("View") "/" N_("Improvements") "/" N_("Roads & Rails"), NULL,
	view_menu_callback,	MENU_VIEW_SHOW_ROADS_RAILS,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("Improvements") "/" N_("Irrigation"), NULL,
	view_menu_callback,	MENU_VIEW_SHOW_IRRIGATION,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("Improvements") "/" N_("Mines"),	NULL,
	view_menu_callback,	MENU_VIEW_SHOW_MINES,			"<CheckItem>"	},
  { "/" N_("View") "/" N_("Improvements") "/" N_("Fortress & Airbase"), NULL,
	view_menu_callback,	MENU_VIEW_SHOW_FORTRESS_AIRBASE,	"<CheckItem>"	},
  { "/" N_("View") "/" N_("Specials"),			NULL,
	view_menu_callback,	MENU_VIEW_SHOW_SPECIALS,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("Pollution & Fallout"),	NULL,
	view_menu_callback,	MENU_VIEW_SHOW_POLLUTION,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("Cities"),			NULL,
	view_menu_callback,	MENU_VIEW_SHOW_CITIES,			"<CheckItem>"	},
  { "/" N_("View") "/" N_("Units"),			NULL,
	view_menu_callback,	MENU_VIEW_SHOW_UNITS,			"<CheckItem>"	},
  { "/" N_("View") "/" N_("Focus Unit"),		NULL,
	view_menu_callback,	MENU_VIEW_SHOW_FOCUS_UNIT,		"<CheckItem>"	},
  { "/" N_("View") "/" N_("Fog of War"),		NULL,
	view_menu_callback,	MENU_VIEW_SHOW_FOG_OF_WAR,		"<CheckItem>"	},
  { "/" N_("View") "/sep2",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("View") "/" N_("_Center View"),		"c",
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
  { "/" N_("Orders") "/" N_("Pillage"),		        "<shift>p",
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
  {"/" N_("Orders") "/" N_("_Connect") "/" N_("_Road"), "<ctrl>r",
   orders_menu_callback, MENU_ORDER_CONNECT_ROAD},
  {"/" N_("Orders") "/" N_("_Connect") "/" N_("Rai_l"), "<ctrl>c",
   orders_menu_callback, MENU_ORDER_CONNECT_RAIL},
  {"/" N_("Orders") "/" N_("_Connect") "/" N_("_Irrigate"), "<ctrl>i",
   orders_menu_callback, MENU_ORDER_CONNECT_IRRIGATE},
  { "/" N_("Orders") "/" N_("Patrol (_Q)"),		"q",
	orders_menu_callback,	MENU_ORDER_PATROL					},
  { "/" N_("Orders") "/" N_("_Go to"),			"g",
	orders_menu_callback,	MENU_ORDER_GOTO						},
  { "/" N_("Orders") "/" N_("Go\\/Airlift to City"),	"<shift>l",
	orders_menu_callback,	MENU_ORDER_GOTO_CITY					},
  { "/" N_("Orders") "/" N_("Return to nearest city"),	"<shift>g",
	orders_menu_callback,	MENU_ORDER_RETURN },
  { "/" N_("Orders") "/sep4",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Orders") "/" N_("Disband Unit"),		"<shift>d",
	orders_menu_callback,	MENU_ORDER_DISBAND					},
  { "/" N_("Orders") "/" N_("Diplomat\\/Spy Actions"),	"d",
	orders_menu_callback,	MENU_ORDER_DIPLOMAT_DLG					},
  { "/" N_("Orders") "/" N_("Explode Nuclear"),        "<shift>n",
	orders_menu_callback,	MENU_ORDER_NUKE						},
  { "/" N_("Orders") "/sep5",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Orders") "/" N_("_Wait"),			"w",
	orders_menu_callback,	MENU_ORDER_WAIT						},
  { "/" N_("Orders") "/" N_("Done"),			"space",
	orders_menu_callback,	MENU_ORDER_DONE						},
 /* Warclient menu ...*/
  { "/" N_("Warclient"),					NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Warclient") "/tearoff1",			NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("Warclient") "/" N_("Delayed goto"),			"z",
	warclient_menu_callback,	MENU_WARCLIENT_DELAYED_GOTO						},
  { "/" N_("Warclient") "/" N_("Delayed paradrop or nuke"),			"<ctrl>z",
	warclient_menu_callback,	MENU_WARCLIENT_DELAYED_PARADROP_OR_NUKE						},
  { "/" N_("Warclient") "/" N_("Delayed airlift"),			"<ctrl>y",
	warclient_menu_callback,	MENU_WARCLIENT_DELAYED_AIRLIFT						},
  { "/" N_("Warclient") "/" N_("Execute delayed goto"),			"y",
	warclient_menu_callback,	MENU_WARCLIENT_EXECUTE_DELAYED_GOTO						},
  { "/" N_("Warclient") "/" N_("Clear delayed orders"),			"u",
	warclient_menu_callback,	MENU_WARCLIENT_CLEAR_DELAYED_ORDERS						},
  { "/" N_("Warclient") "/" N_("Delayed goto auto"),					NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Warclient")  "/" N_("Delayed goto auto") "/tearoff1",			NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("Warclient") "/" N_("Delayed goto mode"),		NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Warclient") "/" N_("Delayed goto mode") "/tearoff1",	NULL,
	NULL,			0,					"<Tearoff>"	},
  PLACE_GEN_ITEMS("/" N_("Warclient") "/" N_("Delayed goto mode"),"<alt>",warclient_menu_callback,MENU_WARCLIENT_GOTO),
	{ "/" N_("Warclient") "/" N_("Delayed goto mode") "/sep1",NULL,NULL,0,"<Separator>"},
  UTYPE_GEN_ITEMS("/" N_("Warclient") "/" N_("Delayed goto mode"),"<alt>",warclient_menu_callback,MENU_WARCLIENT_GOTO),

  FILTER_GEN_ITEMS("/" N_("Warclient") "/" N_("Inclusive filter"),warclient_menu_callback,MENU_WARCLIENT_INCLUSIVE),
  FILTER_GEN_ITEMS("/" N_("Warclient") "/" N_("Exclusive filter"),warclient_menu_callback,MENU_WARCLIENT_EXCLUSIVE),
  DGS_GEN_ITEMS("/" N_("Warclient") "/" N_("Delayed goto selection"),"<alt>",warclient_menu_callback,MENU_WARCLIENT_DG,1),
  DGS_GEN_ITEMS("/" N_("Warclient") "/" N_("Delayed goto selection"),"<alt>",warclient_menu_callback,MENU_WARCLIENT_DG,2),
  DGS_GEN_ITEMS("/" N_("Warclient") "/" N_("Delayed goto selection"),"<alt>",warclient_menu_callback,MENU_WARCLIENT_DG,3),
  DGS_GEN_ITEMS("/" N_("Warclient") "/" N_("Delayed goto selection"),"<alt>",warclient_menu_callback,MENU_WARCLIENT_DG,4),
  DGS_GEN_ITEMS("/" N_("Warclient") "/" N_("Delayed goto selection"),"<alt>",warclient_menu_callback,MENU_WARCLIENT_DG,5),
  DGS_GEN_ITEMS("/" N_("Warclient") "/" N_("Delayed goto selection"),"<alt>",warclient_menu_callback,MENU_WARCLIENT_DG,6),
  DGS_GEN_ITEMS("/" N_("Warclient") "/" N_("Delayed goto selection"),"<alt>",warclient_menu_callback,MENU_WARCLIENT_DG,7),
  DGS_GEN_ITEMS("/" N_("Warclient") "/" N_("Delayed goto selection"),"<alt>",warclient_menu_callback,MENU_WARCLIENT_DG,8),
  DGS_GEN_ITEMS("/" N_("Warclient") "/" N_("Delayed goto selection"),"<alt>",warclient_menu_callback,MENU_WARCLIENT_DG,9),

  { "/" N_("Warclient") "/sep1",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Warclient") "/" N_("Autowakeup sentried units"),		"v",
	warclient_menu_callback,	MENU_WARCLIENT_TOGGLE_WAKEUP,	"<CheckItem>"	},
  { "/" N_("Warclient") "/" N_("Move and attack mode"),		"<ctrl>a",
	warclient_menu_callback,	MENU_WARCLIENT_TOGGLE_MOVEANDATTACK,	"<CheckItem>"	},
  { "/" N_("Warclient") "/sep2",				NULL,
	NULL,			0,					"<Separator>"	},

  { "/" N_("Warclient") "/" N_("Caravan action upon arrival"),		NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Warclient") "/" N_("Caravan action upon arrival") "/tearoff1",	NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("Warclient") "/" N_("Caravan action upon arrival") "/" N_("Popup dialog"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_CARAVAN_POPUP,		"<RadioItem>"	},
  { "/" N_("Warclient") "/" N_("Caravan action upon arrival") "/" N_("Establish trade route"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_CARAVAN_TRADE,		"<main>/Warclient/Caravan action upon arrival/Popup dialog"	},
  { "/" N_("Warclient") "/" N_("Caravan action upon arrival") "/" N_("Help building wonder"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_CARAVAN_BUILD,		"<main>/Warclient/Caravan action upon arrival/Popup dialog"	},
  { "/" N_("Warclient") "/" N_("Caravan action upon arrival") "/" N_("Keep going"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_CARAVAN_NOTHING,		"<main>/Warclient/Caravan action upon arrival/Popup dialog"	},

  { "/" N_("Warclient") "/" N_("Diplomat action upon arrival"),		NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Warclient") "/" N_("Diplomat action upon arrival") "/tearoff1",	NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("Warclient") "/" N_("Diplomat action upon arrival") "/" N_("Popup dialog"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_DIPLOMAT_POPUP,		"<RadioItem>"	},
  { "/" N_("Warclient") "/" N_("Diplomat action upon arrival") "/" N_("Bribe unit"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_DIPLOMAT_BRIBE,		"<main>/Warclient/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Warclient") "/" N_("Diplomat action upon arrival") "/" N_("Sabotage unit (spy)"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_DIPLOMAT_SABOTAGE_UNIT,		"<main>/Warclient/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Warclient") "/" N_("Diplomat action upon arrival") "/" N_("Establish embassy"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_DIPLOMAT_EMBASSY,		"<main>/Warclient/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Warclient") "/" N_("Diplomat action upon arrival") "/" N_("Investigate city"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_DIPLOMAT_INVESTIGATE,		"<main>/Warclient/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Warclient") "/" N_("Diplomat action upon arrival") "/" N_("Sabotage city"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_DIPLOMAT_SABOTAGE_CITY,		"<main>/Warclient/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Warclient") "/" N_("Diplomat action upon arrival") "/" N_("Steal technology"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_DIPLOMAT_STEAL,		"<main>/Warclient/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Warclient") "/" N_("Diplomat action upon arrival") "/" N_("Incite revolt"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_DIPLOMAT_INCITE,		"<main>/Warclient/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Warclient") "/" N_("Diplomat action upon arrival") "/" N_("Poison city (spy)"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_DIPLOMAT_POISON,		"<main>/Warclient/Diplomat action upon arrival/Popup dialog"	},
  { "/" N_("Warclient") "/" N_("Diplomat action upon arrival") "/" N_("Keep going"), NULL,
	warclient_menu_callback,	MENU_WARCLIENT_DIPLOMAT_NOTHING,		"<main>/Warclient/Diplomat action upon arrival/Popup dialog"	},


  { "/" N_("Warclient") "/sep3",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Warclient") "/" N_("Set airlift destination"),			"<shift>y",
	warclient_menu_callback,	MENU_WARCLIENT_SET_AIRLIFT_DEST						},
  { "/" N_("Warclient") "/" N_("Set airlift source"),			"<shift>z",
	warclient_menu_callback,	MENU_WARCLIENT_SET_AIRLIFT_SRC						},
  { "/" N_("Warclient") "/" N_("Clear airlift queue"),			NULL,
	warclient_menu_callback,	MENU_WARCLIENT_CLEAR_AIRLIFT_QUEUE						},
  { "/" N_("Warclient") "/" N_("Show cities in airlift queue"),			"",
	warclient_menu_callback,	MENU_WARCLIENT_SHOW_CITIES_IN_AIRLIFT_QUEUE						},
  { "/" N_("Warclient") "/" N_("Airlift unit type"),		NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("Warclient") "/" N_("Airlift unit type") "/tearoff1",	NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("Warclient") "/sep4",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Warclient") "/" N_("Set airplane patrol destination"),			"<ctrl>e",
	warclient_menu_callback,	MENU_WARCLIENT_PATROL_DEST						},
  { "/" N_("Warclient") "/" N_("Force airplane patrol destination"),			"e",
	warclient_menu_callback,	MENU_WARCLIENT_PATROL_FORCE						},
  { "/" N_("Warclient") "/" N_("Execute all patrol orders"),			"<shift>e",
	warclient_menu_callback,	MENU_WARCLIENT_PATROL_EXECUTE						},
  { "/" N_("Warclient") "/sep4",				NULL,
	NULL,			0,					"<Separator>"	},
  { "/" N_("Warclient") "/" N_("Set rallies for selected cities"),			"<shift>s",
	warclient_menu_callback,	MENU_WARCLIENT_SET_RALLIES						},
  { "/" N_("Warclient") "/" N_("Clear rallies in selected cities"),			"<shift>r",
	warclient_menu_callback,	MENU_WARCLIENT_CLEAR_RALLIES						},

  /* PepClient menu ... */
  { "/" N_("PepClient"),					NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("PepClient") "/tearoff1",			NULL,
	NULL,			0,					"<Tearoff>"	},
  { "/" N_("PepClient") "/" N_("Multi-selection select"),			"<shift>c",
	pepclient_menu_callback,			MENU_PEPCLIENT_MS_SELECT	},
  { "/" N_("PepClient") "/" N_("Multi-selection active all units"),			"<shift>b",
	pepclient_menu_callback,			MENU_PEPCLIENT_MS_ACTIVE_ALL	},
  { "/" N_("PepClient") "/" N_("Multi-selection clear"),			"<shift>v",
	pepclient_menu_callback,			MENU_PEPCLIENT_MS_CLEAR	},
  { "/" N_("PepClient") "/" N_("Multi-selection mode"),					NULL,
	NULL,			0,					"<Branch>"	},
  { "/" N_("PepClient") "/" N_("Multi-selection mode") "/tearoff1",			NULL,
	NULL,			0,					"<Tearoff>"	},
  PLACE_GEN_ITEMS("/" N_("PepClient") "/" N_("Multi-selection mode"),"",pepclient_menu_callback,MENU_PEPCLIENT_MODE),
	{ "/" N_("PepClient") "/" N_("Multi-selection mode") "/sep1",NULL,NULL,0,"<Separator>"},
  UTYPE_GEN_ITEMS("/" N_("PepClient") "/" N_("Multi-selection mode"),"",pepclient_menu_callback,MENU_PEPCLIENT_MODE),
  FILTER_GEN_ITEMS("/" N_("PepClient") "/" N_("Inclusive filter"),pepclient_menu_callback,MENU_PEPCLIENT_INCLUSIVE),
  FILTER_GEN_ITEMS("/" N_("PepClient") "/" N_("Exclusive filter"),pepclient_menu_callback,MENU_PEPCLIENT_EXCLUSIVE),
  { "/" N_("PepClient") "/" N_("Multi-selection spread"),			"<shift>s",
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_SPREAD	},
  { "/" N_("PepClient") "/" N_("Spread only in cities with airport"),			NULL,
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_SPREAD_AIRPORT	,		"<CheckItem>"		},
  SELECT_GEN_ITEMS("/" N_("PepClient") "/" N_("Multi-selection"),"",pepclient_menu_callback,MENU_PEPCLIENT_MS,1),
  SELECT_GEN_ITEMS("/" N_("PepClient") "/" N_("Multi-selection"),"",pepclient_menu_callback,MENU_PEPCLIENT_MS,2),
  SELECT_GEN_ITEMS("/" N_("PepClient") "/" N_("Multi-selection"),"",pepclient_menu_callback,MENU_PEPCLIENT_MS,3),
  SELECT_GEN_ITEMS("/" N_("PepClient") "/" N_("Multi-selection"),"",pepclient_menu_callback,MENU_PEPCLIENT_MS,4),
  SELECT_GEN_ITEMS("/" N_("PepClient") "/" N_("Multi-selection"),"",pepclient_menu_callback,MENU_PEPCLIENT_MS,5),
  SELECT_GEN_ITEMS("/" N_("PepClient") "/" N_("Multi-selection"),"",pepclient_menu_callback,MENU_PEPCLIENT_MS,6),
  SELECT_GEN_ITEMS("/" N_("PepClient") "/" N_("Multi-selection"),"",pepclient_menu_callback,MENU_PEPCLIENT_MS,7),
  SELECT_GEN_ITEMS("/" N_("PepClient") "/" N_("Multi-selection"),"",pepclient_menu_callback,MENU_PEPCLIENT_MS,8),
  SELECT_GEN_ITEMS("/" N_("PepClient") "/" N_("Multi-selection"),"",pepclient_menu_callback,MENU_PEPCLIENT_MS,9),

  { "/" N_("PepClient") "/sep1",				NULL,
	NULL,			0,					"<Separator>"	},

  { "/" N_("PepClient") "/" N_("Execute automatic orders"),			"<ctrl>m",
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_EXECUTE	},
  { "/" N_("PepClient") "/" N_("Free automatic orders"),			"<ctrl>f",
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_FREE	},
  { "/" N_("PepClient") "/" N_("Execute all automatic orders"),			"<shift>m",
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_EXECUTE_ALL	},

  { "/" N_("PepClient") "/sep2",				NULL,
	NULL,			0,					"<Separator>"	},

  { "/" N_("PepClient") "/" N_("Add a city in trade plan"),			"<shift>a",
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_TRADE_CITY	},
  { "/" N_("PepClient") "/" N_("Clear city list for trade plan"),			NULL,
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_CLEAR_TRADE_CITY	},
  { "/" N_("PepClient") "/" N_("Show cities in trade plan"),			NULL,
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_SHOW_TRADE_CITIES	},
  { "/" N_("PepClient") "/" N_("Show the trade route free slots"),			NULL,
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_SHOW_FREE_SLOTS	},
  { "/" N_("PepClient") "/" N_("Recalculate trade plan"),			NULL,
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_TRADE_RECALCULATE	},
  { "/" N_("PepClient") "/" N_("Set caravan destination"),			"h",
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_TRADE_WITH	},
  { "/" N_("PepClient") "/" N_("Automatic caravan orders"),			"<ctrl>h",
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_CARAVAN	},
  { "/" N_("PepClient") "/" N_("Execute all trade route orders"),			"<shift>h",
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_TRADE_EXECUTE	},
  { "/" N_("PepClient") "/" N_("Execute all help wonder orders"),			"<shift>i",
	pepclient_menu_callback,			MENU_PEPCLIENT_MY_AI_WONDER_EXECUTE	},

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
  { "/" N_("Reports") "/" N_("_Messages"),		"<ctrl>F9",
	reports_menu_callback,	MENU_REPORT_MESSAGES					},
  { "/" N_("Reports") "/" N_("_Demographics"),		"<ctrl>F11",
	reports_menu_callback,	MENU_REPORT_DEMOGRAPHIC					},
  { "/" N_("Reports") "/" N_("S_paceship"),		"<ctrl>F12",
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
  { "/" N_("Help") "/" N_("Warclient"),                 NULL,
	help_menu_callback,	MENU_HELP_WARCLIENT					},
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
  ... *pepeto*
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
	else if(!strcmp(uname,"Freight"))
		gtk_widget_add_accelerator(widget,"activate",toplevel_accel,'f',GDK_SHIFT_MASK|GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
}

/****************************************************************
 ... *pepeto*
 *****************************************************************/
static bool can_player_unit_type(Unit_Type_id utype)
{
	if(can_player_build_unit(game.player_ptr,utype))
		return TRUE;

	unit_list_iterate(game.player_ptr->units,punit)
	{
		if(punit->type==utype)
			return TRUE;
	} unit_list_iterate_end;
	return FALSE;
}

/****************************************************************
 ... *pepeto*
 *****************************************************************/
static void airlift_menu_set_sensitive(void)
{
	int i;

	for(i=0;i<airlift_menu_num;i++)
		gtk_widget_set_sensitive(airlift_menu_datas[i].widget,can_player_unit_type(airlift_menu_datas[i].utype));
}

/****************************************************************
 ... *pepeto*
 *****************************************************************/
static void airlift_menu_set_active(void)
{
	int i;

	for(i=0;i<airlift_menu_num;i++)
		if(airliftunittype==airlift_menu_datas[i].utype)
		{
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(airlift_menu_datas[i].widget),TRUE);
			break;
		}
}

/****************************************************************
  ... *pepeto*
 *****************************************************************/
static void airlift_menu_callback(GtkWidget *widget,gpointer data)
{
	airliftunittype=GPOINTER_TO_INT(data);
}

/****************************************************************
  ... *pepeto*
 *****************************************************************/
static void create_airlift_menu(GtkWidget *widget)
{
  GtkWidget *item;
  int i;
  Tech_Type_id tid = find_tech_by_name_orig("Never");

  group=NULL;
  for (i=airlift_menu_num=0;i<game.num_unit_types;i++)
  {
	if(unit_types[i].move_type!=LAND_MOVING||unit_types[i].tech_requirement==tid||unit_type_flag(i,F_NOBUILD))
		continue;
	item=gtk_radio_menu_item_new_with_label(group,unit_types[i].name);
	g_signal_connect(item,"activate",G_CALLBACK(airlift_menu_callback),GINT_TO_POINTER(i));
	group=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
	get_accel_label(item,unit_name_orig(i));
	gtk_menu_shell_append(GTK_MENU_SHELL(widget),item);
	airlift_menu_datas[airlift_menu_num].utype=i;
	airlift_menu_datas[airlift_menu_num].widget=item;
	airlift_menu_num++;
  }
}

/****************************************************************
  ... *pepeto*
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
  ... *pepeto*
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

  create_airlift_menu(gtk_item_factory_get_widget(item_factory, "<main>/Warclient/Airlift unit type"));//*pepeto*
  create_automatic_processus_menus();//*pepeto*

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
    assert(ptype->irrigation_result != ptile->terrain
	   && ptype->irrigation_result != T_NONE);
    map_irrigate_tile(ptile);
    break;

  case ACTIVITY_MINE:
    assert(ptype->mining_result != ptile->terrain
	   && ptype->mining_result != T_NONE);
    map_mine_tile(ptile);
    break;

  case ACTIVITY_TRANSFORM:
    assert(ptype->transform_result != ptile->terrain
	   && ptype->transform_result != T_NONE);
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
 ... *pepeto* for compatibility with Warclient for F9-F12
*****************************************************************/
void set_delayed_goto_mode(const char *path)
{
    if(!main_menubar)
		return;
	char buf[256]="<main>/Warclient/Delayed goto mode/",*b;

	b=buf+strlen(buf);
	strcpy(b,path);
	menus_set_active(buf,TRUE);
}

UPDATE_FILTER(delayed_goto_inclusive_filter,"<main>/Warclient/Inclusive filter")
UPDATE_FILTER(delayed_goto_exclusive_filter,"<main>/Warclient/Exclusive filter")
UPDATE_FILTER(multi_select_inclusive_filter,"<main>/PepClient/Inclusive filter")
UPDATE_FILTER(multi_select_exclusive_filter,"<main>/PepClient/Exclusive filter")

/****************************************************************
 ... *pepeto* for automatic processus
*****************************************************************/
void update_automatic_processus_filter_menu(automatic_processus *pap)
{
	if(!pap||pap->menu[0]=='\0')
		return;
	int i;

	for(i=0;i<AUTO_VALUES_NUM;i++)
		if(is_auto_value_allowed(pap,i))
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pap->widget[i]),pap->auto_filter&AV_TO_FV(i));
}

/****************************************************************
 ... *pepeto*
*****************************************************************/
void init_menus(void)
{
	if(!main_menubar)
		return;
	bool cond;

	cond=(can_client_access_hack()&&get_client_state()>=CLIENT_GAME_RUNNING_STATE);
	menus_set_sensitive("<main>/_Game/Save Game _As...",cond);
	menus_set_sensitive("<main>/_Game/_Save Game",cond);
	menus_set_sensitive("<main>/_Game/Server O_ptions",aconnection.established);
	menus_set_sensitive("<main>/_Game/_Initial Server Options",get_client_state()>=CLIENT_GAME_RUNNING_STATE);
	menus_set_sensitive("<main>/_Game/L_eave",aconnection.established);

	if(!can_client_change_view())
	{
		menus_set_sensitive("<main>/_Reports",FALSE);
		menus_set_sensitive("<main>/_Government",FALSE);
		menus_set_sensitive("<main>/_View",FALSE);
		menus_set_sensitive("<main>/_Orders",FALSE);
		menus_set_sensitive("<main>/Warclient",FALSE);
		menus_set_sensitive("<main>/PepClient",FALSE);
		return;
	}

	cond=(can_client_issue_orders());
	menus_set_sensitive("<main>/_Reports",TRUE);
	menus_set_sensitive("<main>/_Government",TRUE);
	menus_set_sensitive("<main>/_View",TRUE);
	menus_set_sensitive("<main>/_Orders",cond);
	menus_set_sensitive("<main>/Warclient",cond);
	menus_set_sensitive("<main>/PepClient",cond);

	menus_set_sensitive("<main>/_Government/_Tax Rates",game.rgame.changable_tax&&cond);
	menus_set_sensitive("<main>/_Government/_Worklists",cond);
	menus_set_sensitive("<main>/_Government/_Change Government",cond);

	menus_set_active("<main>/_View/Map _Grid",draw_map_grid);
	menus_set_sensitive("<main>/_View/National _Borders",game.borders > 0);
	menus_set_active("<main>/_View/National _Borders",draw_borders);
	menus_set_active("<main>/_View/City _Names",draw_city_names);
	menus_set_active("<main>/_View/City G_rowth",draw_city_growth);
	menus_set_active("<main>/_View/City _Productions",draw_city_productions);
	menus_set_active("<main>/_View/City _Trade Routes",draw_city_traderoutes);
	menus_set_active("<main>/_View/Terrain",draw_terrain);
	menus_set_active("<main>/_View/Coastline",draw_coastline);
	menus_set_active("<main>/_View/Improvements/Roads & Rails",draw_roads_rails);
	menus_set_active("<main>/_View/Improvements/Irrigation",draw_irrigation);
	menus_set_active("<main>/_View/Improvements/Mines",draw_mines);
	menus_set_active("<main>/_View/Improvements/Fortress & Airbase",draw_fortress_airbase);
	menus_set_active("<main>/_View/Specials",draw_specials);
	menus_set_active("<main>/_View/Pollution & Fallout",draw_pollution);
	menus_set_active("<main>/_View/Cities",draw_cities);
	menus_set_active("<main>/_View/Units",draw_units);
	menus_set_active("<main>/_View/Focus Unit",draw_focus_unit);
	menus_set_active("<main>/_View/Fog of War",draw_fog_of_war);

	menus_set_active("<main>/Warclient/Autowakeup sentried units", autowakeup_state);
   menus_set_active("<main>/Warclient/Move and attack mode", moveandattack_state);
  	PLACE_SET_ACTIVE("<main>/Warclient/Delayed goto mode",delayed_goto_place);
	UTYPE_SET_ACTIVE("<main>/Warclient/Delayed goto mode",delayed_goto_utype);
	switch(default_caravan_action)
	{
		case 0:menus_set_active("<main>/Warclient/Caravan action upon arrival/Popup dialog",TRUE);break;
		case 1:menus_set_active("<main>/Warclient/Caravan action upon arrival/Establish trade route",TRUE);break;
		case 2:menus_set_active("<main>/Warclient/Caravan action upon arrival/Help building wonder",TRUE);break;
		case 3:menus_set_active("<main>/Warclient/Caravan action upon arrival/Keep going",TRUE);break;
		default:break;
	}
	switch(default_diplomat_action)
	{
		case 0:menus_set_active("<main>/Warclient/Diplomat action upon arrival/Popup dialog",TRUE);break;
		case 1:menus_set_active("<main>/Warclient/Diplomat action upon arrival/Bribe unit",TRUE);break;
		case 2:menus_set_active("<main>/Warclient/Diplomat action upon arrival/Sabotage unit (spy)",TRUE);break;
		case 3:menus_set_active("<main>/Warclient/Diplomat action upon arrival/Establish embassy",TRUE);break;
		case 4:menus_set_active("<main>/Warclient/Diplomat action upon arrival/Investigate city",TRUE);break;
		case 5:menus_set_active("<main>/Warclient/Diplomat action upon arrival/Sabotage city",TRUE);break;
		case 6:menus_set_active("<main>/Warclient/Diplomat action upon arrival/Steal technology",TRUE);break;
		case 7:menus_set_active("<main>/Warclient/Diplomat action upon arrival/Incite revolt",TRUE);break;
		case 8:menus_set_active("<main>/Warclient/Diplomat action upon arrival/Poison city (spy)",TRUE);break;
		case 9:menus_set_active("<main>/Warclient/Diplomat action upon arrival/Keep going",TRUE);break;
		default:break;
	}
	airlift_menu_set_active();

	PLACE_SET_ACTIVE("<main>/PepClient/Multi-selection mode",multi_select_place);
	UTYPE_SET_ACTIVE("<main>/PepClient/Multi-selection mode",multi_select_utype);
   menus_set_active("<main>/PepClient/Spread only in cities with airport",spread_airport_cities);

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
        if (g->index != game.government_when_anarchy) {
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

          if (!can_change_to_government(game.player_ptr, g->index)) {
            gtk_widget_set_sensitive(item, FALSE);
	  }

          gtk_menu_shell_append(GTK_MENU_SHELL(parent), item);
          gtk_widget_show(item);
        }
      } government_iterate_end;
    }

    menus_set_sensitive("<main>/_Reports/S_paceship",
			(game.player_ptr->spaceship.state!=SSHIP_NONE));

    menus_set_sensitive("<main>/_View/City G_rowth", draw_city_names);
    menus_set_sensitive("<main>/_View/Coastline", !draw_terrain);
    menus_set_sensitive("<main>/_View/Focus Unit", !draw_units);
    //*pepeto*
    cond=(my_ai_trade_city_list_size(my_ai_get_trade_cities()));
    menus_set_sensitive("<main>/PepClient/Clear city list for trade plan",cond);
    menus_set_sensitive("<main>/PepClient/Show cities in trade plan",cond);
    menus_set_sensitive("<main>/PepClient/Show the trade route free slots",cond);
    menus_set_sensitive("<main>/PepClient/Recalculate trade plan",cond);
    menus_set_sensitive("<main>/PepClient/Execute all automatic orders",my_ai_enable&&my_ai_count_activity(MY_AI_LAST));
    menus_set_sensitive("<main>/PepClient/Execute all trade route orders",my_ai_trade_level&&my_ai_count_activity(MY_AI_TRADE_ROUTE));
    menus_set_sensitive("<main>/PepClient/Execute all help wonder orders",my_ai_wonder_level&&my_ai_count_activity(MY_AI_HELP_WONDER));
    menus_set_sensitive("<main>/Warclient/Execute all patrol orders",my_ai_enable&&my_ai_count_activity(MY_AI_PATROL));
    UPDATE_FILTER_NAME(delayed_goto_inclusive_filter)();
    UPDATE_FILTER_NAME(delayed_goto_exclusive_filter)();
    UPDATE_FILTER_NAME(multi_select_inclusive_filter)();
    UPDATE_FILTER_NAME(multi_select_exclusive_filter)();
    UPDATE_DGS("<main>/Warclient/Delayed goto selection",delayed_goto);
    UPDATE_SELECTION("<main>/PepClient/Multi-selection",multi_select);
    airlift_menu_set_sensitive();
    automatic_processus_iterate(pap)
    {
      update_automatic_processus_filter_menu(pap);
    } automatic_processus_iterate_end;
    cond=(delayed_goto_size(0));
    menus_set_sensitive("<main>/Warclient/Execute delayed goto",cond);
    menus_set_sensitive("<main>/Warclient/Clear delayed orders",cond);
    cond=(is_tile_in_airlift_queue());
    menus_set_sensitive("<main>/Warclient/Set airlift destination",cond);
    menus_set_sensitive("<main>/Warclient/Clear airlift queue",cond);
    menus_set_sensitive("<main>/Warclient/Show cities in airlift queue",cond);
    cond=(tiles_hilited_cities);
    menus_set_sensitive("<main>/Warclient/Set rallies for selected cities",cond);
    menus_set_sensitive("<main>/Warclient/Clear rallies in selected cities",cond);
/*  This should be done in a future version for all warclient menu items
    menus_set_sensitive("<main>/Warclient/Set rallies for selected cities", tiles_hilited_cities); */
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
		 && player_knows_techs_with_flag(game.player_ptr,
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
      } else {
	menus_rename("<main>/_Orders/Build _Fortress", _("Build _Fortress"));
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

	//*pepeto*
	menus_set_sensitive("<main>/Warclient/Delayed goto",TRUE);
	menus_set_sensitive("<main>/Warclient/Delayed paradrop or nuke",TRUE);
	menus_set_sensitive("<main>/Warclient/Set airplane patrol destination",my_ai_enable);
	menus_set_sensitive("<main>/Warclient/Force airplane patrol destination",my_ai_enable&&!can_unit_do_activity(punit,ACTIVITY_AIRBASE));
	menus_set_sensitive("<main>/PepClient/Multi-selection select",TRUE);
	menus_set_sensitive("<main>/PepClient/Multi-selection active all units",TRUE);
	menus_set_sensitive("<main>/PepClient/Multi-selection clear",TRUE);
	menus_set_sensitive("<main>/PepClient/Multi-selection spread",TRUE);
	cond=(my_ai_trade_level&&unit_flag(punit,F_TRADE_ROUTE));
	menus_set_sensitive("<main>/PepClient/Set caravan destination",my_ai_enable&&cond);
	menus_set_sensitive("<main>/PepClient/Automatic caravan orders",my_ai_enable&&(cond||(my_ai_wonder_level
		&&unit_flag(punit,F_HELP_WONDER))));
	menus_set_sensitive("<main>/PepClient/Free automatic orders",punit->my_ai.control);
	menus_set_sensitive("<main>/PepClient/Execute automatic orders",my_ai_enable&&punit->my_ai.control);

      menus_set_sensitive("<main>/_Orders", TRUE);
    } else {
	menus_set_sensitive("<main>/Warclient/Delayed goto",FALSE);
	menus_set_sensitive("<main>/Warclient/Delayed paradrop or nuke",FALSE);
	menus_set_sensitive("<main>/Warclient/Set airplane patrol destination",FALSE);
	menus_set_sensitive("<main>/Warclient/Force airplane patrol destination",FALSE);
	menus_set_sensitive("<main>/PepClient/Multi-selection select",FALSE);
	menus_set_sensitive("<main>/PepClient/Multi-selection active all units",FALSE);
	menus_set_sensitive("<main>/PepClient/Multi-selection clear",FALSE);
	menus_set_sensitive("<main>/PepClient/Multi-selection spread",FALSE);
 	menus_set_sensitive("<main>/PepClient/Automatic caravan orders",FALSE);
	menus_set_sensitive("<main>/PepClient/Free automatic orders",FALSE);
	menus_set_sensitive("<main>/PepClient/Execute automatic orders",FALSE);

	menus_set_sensitive("<main>/_Orders", FALSE);
    }
  }
}
