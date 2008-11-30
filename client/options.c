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
#include <stdlib.h>
#include <string.h>

#include "events.h"
#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "mem.h"
#include "registry.h"
#include "shared.h"
#include "support.h"
#include "version.h"

#include "audio.h"
#include "chatline_g.h"
#include "cityrepdata.h"
#include "civclient.h"
#include "clinet.h"
#include "cma_fec.h"
#include "control.h"
#include "mapview_common.h"
#include "multiselect.h"
#include "plrdlg_common.h"
#include "tilespec.h"

#include "options.h"

/* The following function are define in gui/gamedlgs. */
void fullscreen_mode_callback(struct client_option *poption);
void map_scrollbars_callback(struct client_option *poption);
void mapview_redraw_callback(struct client_option *poption);
void split_message_window_callback(struct client_option *poption);

static const char *category_name[COC_NUM] = {
  N_("Graphics"),		/* COC_GRAPHICS */
  N_("Interface"),		/* COC_INTERFACE */
  N_("Sound"),			/* COC_SOUND */
  N_("Chat"),			/* COC_CHAT */
  N_("Messages"),		/* COC_MESSAGE */
  N_("Network"),		/* COC_NETWORK */
  N_("Gameplay"),		/* COC_GAMEPLAY */
  N_("Multi-selection"),	/* COC_MULTI_SELECTION */
  N_("Delayed goto"),		/* COC_DELAYED_GOTO */
  N_("Trade routes")		/* COC_TRADE */
};

/** Defaults for options normally on command line **/

char default_user_name[512];
char default_password[512];
char default_user_nation[512];
char default_user_tech_goal[512];
char default_server_host[512];
int  default_server_port;
char default_metaserver[512];
char default_tileset_name[512];
char default_sound_set_name[512];
char default_sound_plugin_name[512];

struct worklist global_worklists[MAX_NUM_WORKLISTS];

/** Local Options: **/
bool random_leader;
bool solid_color_behind_units;
bool sound_bell_at_new_turn;
#ifdef AUDIO_VOLUME
int sound_volume;
#endif /* AUDIO_VOLUME */
int  smooth_move_unit_msec;
int smooth_center_slide_msec;
bool do_combat_animation;
bool ai_popup_windows;
bool ai_manual_turn_done;
bool auto_center_on_unit;
bool auto_center_on_combat;
bool auto_center_each_turn;
bool wakeup_focus;
bool goto_into_unknown;
bool center_when_popup_city;
bool concise_city_production;
bool auto_turn_done;
bool meta_accelerators;
bool map_scrollbars;
bool dialogs_on_top;
bool ask_city_name;
bool popup_new_cities;
bool keyboardless_goto;
bool show_task_icons;
char chat_time_format[128];
char city_name_formats[128];
bool show_split_message_window;
bool do_not_recenter_overview;
bool use_digits_short_cuts;
bool use_voteinfo_bar;
bool show_new_vote_in_front;
bool disable_chatline_scroll_on_window_resize;
bool always_show_votebar;
bool do_not_show_votebar_if_not_player;
bool warn_before_add_to_city;
bool prevent_duplicate_notify_tabs;
bool enable_chat_logging;
char chat_log_directory[MAX_LEN_PATH];
enum player_colors_modes player_colors_mode;
#ifndef ASYNC_TRADE_PLANNING
int trade_time_limit;
#endif	/* ASYNC_TRADE_PLANNING */
bool trade_mode_best_values;
bool trade_mode_allow_free_other;
bool trade_mode_internal_first;
bool trade_mode_homecity_first;
bool fullscreen_mode;
bool enable_tabs;
bool solid_unit_icon_bg;
bool better_fog;
bool save_options_on_exit;

/* This option is currently set by the client - not by the user. */
bool update_city_text_in_refresh_tile = TRUE;

/********************************************************************** 
  Caravan action names accessor.
***********************************************************************/
static const char *get_caravan_action_name(
		       enum default_caravan_unit_actions action)
{
  switch (action) {
  case DCA_POPUP_DIALOG:
    return N_("Popup dialog");
  case DCA_ESTABLISH_TRADEROUTE:
    return N_("Establish trade route");
  case DCA_HELP_BUILD_WONDER:
    return N_("Help building wonder");
  case DCA_KEEP_MOVING:
    return N_("Keep going");
  }
  /* Don't set as default case to be warned if we forgot to add a value */
  return NULL;
}

/********************************************************************** 
  Diplomat action upon unit names accessor.
***********************************************************************/
static const char *get_diplomat_action_upon_unit_name(
		       enum default_diplomat_unit_actions action)
{
  switch (action) {
  case DDUA_POPUP_DIALOG:
    return N_("Popup dialog");
  case DDUA_BRIBE:
    return N_("Bribe unit");
  case DDUA_SABOTAGE:
    return N_("Sabotage unit (spy)");
  case DDUA_KEEP_MOVING:
    return N_("Keep going");
  }
  /* Don't set as default case to be warned if we forgot to add a value */
  return NULL;
}

/********************************************************************** 
  Diplomat action upon city names accessor.
***********************************************************************/
static const char *get_diplomat_action_upon_city_name(
		       enum default_diplomat_city_actions action)
{
  switch (action) {
  case DDCA_POPUP_DIALOG:
    return N_("Popup dialog");
  case DDCA_EMBASSY:
    return N_("Establish embassy");
  case DDCA_INVESTIGATE:
    return N_("Investigate city");
  case DDCA_SABOTAGE:
    return N_("Sabotage city");
  case DDCA_STEAL_TECH:
    return N_("Steal technology");
  case DDCA_INCITE_REVOLT:
    return N_("Incite revolt");
  case DDCA_POISON:
    return N_("Poison city (spy)");
  case DDCA_KEEP_MOVING:
    return N_("Keep going");
  }
  /* Don't set as default case to be warned if we forgot to add a value */
  return NULL;
}

/********************************************************************** 
  New unit action names accessor.
***********************************************************************/
static const char *get_new_unit_action_name(enum new_unit_action action)
{
  switch (action) {
  case ACTION_IDLE:
    return N_("Idle");
  case ACTION_SENTRY:
    return N_("Sentry");
  case ACTION_FORTIFY:
    return N_("Fortify");
  case ACTION_SLEEP:
    return N_("Sleep");
  case ACTION_FORTIFY_OR_SLEEP:
    return N_("Fortify or Sleep");
  }
  /* Don't set as default case to be warned if we forgot to add a value */
  return NULL;
}

/********************************************************************** 
  Place mode name accessor.
***********************************************************************/
static const char *get_place_mode_name(enum place_value place)
{
  switch (place) {
  case PLACE_SINGLE_UNIT:
    return N_("Single unit");
  case PLACE_IN_TRANSPORTER:
    return N_("All units on the same transporter");
  case PLACE_ON_TILE:
    return N_("All units on the tile");
  case PLACE_ON_CONTINENT:
    return N_("All units on the continent");
  case PLACE_EVERY_WHERE:
    return N_("All units");
  }
  /* Don't set as default case to be warned if we forgot to add a value */
  return NULL;
}

/********************************************************************** 
  Unit type mode name accessor.
***********************************************************************/
static const char *get_utype_mode_name(enum utype_value type)
{
  switch (type) {
  case UTYPE_SAME_TYPE:
    return N_("Only units with the same type");
  case UTYPE_SAME_MOVE_TYPE:
    return N_("Only units with the same move type");
  case UTYPE_ALL:
    return N_("All unit types");
  }
  /* Don't set as default case to be warned if we forgot to add a value */
  return NULL;
}

/********************************************************************** 
  Unit type mode name accessor.
***********************************************************************/
static const char *get_filter_value_name(enum filter_value value)
{
  switch (value) {
  case FILTER_ALL:
    return N_("All units");
  case FILTER_NEW:
    return N_("New units");
  case FILTER_FORTIFIED:
    return N_("Fortified units");
  case FILTER_SENTRIED:
    return N_("Sentried units");
  case FILTER_VETERAN:
    return N_("Veteran units");
  case FILTER_AUTO:
    return N_("Auto units");
  case FILTER_IDLE:
    return N_("Idle units");
  case FILTER_ABLE_TO_MOVE:
    return N_("Units able to move");
  case FILTER_FULL_MOVES:
    return N_("Units with full moves");
  case FILTER_FULL_HP:
    return N_("Units with full hp");
  case FILTER_MILITARY:
    return N_("Military units");
  case FILTER_OFF:
    return N_("Off");
  }
  /* Don't set as default case to be warned if we forgot to add a value */
  return NULL;
}

#define GEN_INT_OPTION(oname, odesc, ohelp, ocat, odef, omin, omax, ocb) \
{									 \
  .name = #oname,							 \
  .description = odesc,							 \
  .help_text = ohelp,							 \
  .category = ocat,							 \
  .type = COT_INTEGER,							 \
  .o = {								 \
    .integer = {							 \
      .pvalue = &oname,							 \
      .def = odef,							 \
      .min = omin,							 \
      .max = omax							 \
    }									 \
  },									 \
  .change_callback = ocb,						 \
}

#define GEN_BOOL_OPTION(oname, odesc, ohelp, ocat, odef, ocb) \
{ .name = #oname,					      \
  .description = odesc,					      \
  .help_text = ohelp,					      \
  .category = ocat,					      \
  .type = COT_BOOLEAN,					      \
  .o = {						      \
    .boolean = {					      \
      .pvalue = &oname,					      \
      .def = odef,					      \
    }							      \
  },							      \
  .change_callback = ocb,				      \
}

#define GEN_STR_OPTION(oname, odesc, ohelp, ocat, odef, ocb) \
{ .name = #oname,					     \
  .description = odesc,					     \
  .help_text = ohelp,					     \
  .category = ocat,					     \
  .type = COT_STRING,					     \
  .o = {						     \
    .string = {						     \
      .pvalue = oname,					     \
      .size = sizeof(oname),				     \
      .def = odef,					     \
      .val_accessor = NULL				     \
    }							     \
  },							     \
  .change_callback = ocb,				     \
}

#define GEN_PWD_OPTION(oname, odesc, ohelp, ocat, odef, ocb) \
{ .name = #oname,					     \
  .description = odesc,					     \
  .help_text = ohelp,					     \
  .category = ocat,					     \
  .type = COT_PASSWORD,					     \
  .o = {						     \
    .string = {						     \
      .pvalue = oname,					     \
      .size = sizeof(oname),				     \
      .def = odef,					     \
      .val_accessor = NULL				     \
    }							     \
  },							     \
  .change_callback = ocb,				     \
}

#define GEN_STR_LIST_OPTION(oname, odesc, ohelp, ocat, odef, oacc, ocb)	\
{ .name = #oname,							\
  .description = odesc,							\
  .help_text = ohelp,							\
  .category = ocat,							\
  .type = COT_STRING,							\
  .o = {								\
    .string = {								\
      .pvalue = oname,							\
      .size = sizeof(oname),						\
      .def = odef,							\
      .val_accessor = oacc						\
    }									\
  },									\
  .change_callback = ocb,						\
}

#define GEN_NUM_LIST_OPTION(oname, odesc, ohelp, ocat, odef, oacc, ocb)	\
{ .name = #oname,							\
  .description = odesc,							\
  .help_text = ohelp,							\
  .category = ocat,							\
  .type = COT_NUMBER_LIST,						\
  .o = {								\
    .list = {								\
      .pvalue = (int *)(void *) &oname,					\
      .def = odef,							\
      .str_accessor = (const char *(*)(int)) oacc			\
    }									\
  },									\
  .change_callback = ocb,						\
}

#define GEN_FILTER_OPTION(oname, odesc, ohelp, ocat, odef, ochange, oacc, ocb) \
{ .name = #oname,							       \
  .description = odesc,							       \
  .help_text = ohelp,							       \
  .category = ocat,							       \
  .type = COT_FILTER,							       \
  .o = {								       \
    .filter = {								       \
      .pvalue = &oname,							       \
      .def = odef,							       \
      .change = (bool(*)(filter *, filter)) ochange,			       \
      .str_accessor = (const char *(*)(filter)) oacc			       \
    }									       \
  },									       \
  .change_callback = ocb,						       \
}

#define GEN_VOLUME_OPTION(oname, odesc, ohelp, ocat, odef, omin, omax, ocb) \
{									    \
  .name = #oname,							    \
  .description = odesc,							    \
  .help_text = ohelp,							    \
  .category = ocat,							    \
  .type = COT_VOLUME,							    \
  .o = {								    \
    .integer = {							    \
      .pvalue = &oname,							    \
      .def = odef,							    \
      .min = omin,							    \
      .max = omax							    \
    }									    \
  },									    \
  .change_callback = ocb,						    \
}

static struct client_option client_options[] = {
  GEN_STR_LIST_OPTION(default_tileset_name, N_("Tileset"),
		      N_("By changing this option you change the active "
			 "tileset. This is the same as using the -t "
			 "command-line parameter."),
		      COC_GRAPHICS, "",
		      get_tileset_list, tilespec_reread_callback),
  GEN_BOOL_OPTION(solid_color_behind_units,
		  N_("Solid unit background color"),
		  N_("Setting this option will cause units on the map "
		     "view to be drawn with a solid background color "
		     "instead of the flag backdrop."),
		  COC_GRAPHICS, FALSE, mapview_redraw_callback),
  GEN_BOOL_OPTION(solid_unit_icon_bg,
		  N_("Solid unit icon background color in city dialog"),
		  N_("If this is enabled then a better method is used "
		     "for drawing fog-of-war.  It is not any slower but "
		     "will consume about twice as much memory."),
		  COC_GRAPHICS, FALSE, mapview_redraw_callback),
  GEN_INT_OPTION(smooth_move_unit_msec,
		 N_("Unit movement animation time (milliseconds)"),
		 N_("This option controls how long unit \"animation\" takes "
		    "when a unit moves on the map view.  Set it to 0 to "
		    "disable animation entirely."),
		 COC_GRAPHICS, 30, 0, 1000, NULL),
  GEN_INT_OPTION(smooth_center_slide_msec,
		 N_("Mapview recentering time (milliseconds)"),
		 N_("When the map view is recentered, it will slide "
		    "smoothly over the map to its new position.  This "
		    "option controls how long this slide lasts.  Set it to "
		    "0 to disable mapview sliding entirely."),
		 COC_GRAPHICS, 200, 0, 1000, NULL),
  GEN_BOOL_OPTION(do_combat_animation, N_("Show combat animation"),
		  N_("Disabling this option will turn off combat animation "
		     "between units on the mapview."),
		  COC_GRAPHICS, TRUE, NULL),
  GEN_BOOL_OPTION(concise_city_production, N_("Concise City Production"),
		  N_("Set this option to make the city production (as shown "
		     "in the city dialog) to be more compact."),
		  COC_GRAPHICS, FALSE, NULL),
  GEN_BOOL_OPTION(show_task_icons, N_("Show worklist task icons"),
		  N_("Disabling this will turn off the unit and building "
		     "icons in the worklist dialog and the production "
		     "tab of the city dialog."),
		  COC_GRAPHICS, TRUE, NULL),
  GEN_BOOL_OPTION(do_not_recenter_overview,
                  N_("Do not recenter the overview for wrapped maps"),
		  N_("When enabled, the overview map is not centred on "
		     "the position you are watching. This is usefull in very "
		     "large maps to remember absolute positions."),
		  COC_GRAPHICS, FALSE, NULL),
  GEN_BOOL_OPTION(map_scrollbars, N_("Show Map Scrollbars"),
		  N_("Disable this option to hide the scrollbars on the "
		     "map view."),
		  COC_GRAPHICS, TRUE, map_scrollbars_callback),
  GEN_BOOL_OPTION(fullscreen_mode, N_("Fullscreen Mode"),
		  N_("If enabled, the main windows will take the whole "
		     "screen."),
		  COC_GRAPHICS, FALSE, fullscreen_mode_callback),
  GEN_BOOL_OPTION(better_fog,
		  N_("Better fog-of-war drawing"),
		  N_("If this is enabled then a better method is used "
		     "for drawing fog-of-war.  It is not any slower but "
		     "will consume about twice as much memory."),
		  COC_GRAPHICS, TRUE, mapview_redraw_callback),
  GEN_BOOL_OPTION(use_voteinfo_bar, N_("Enable vote bar"),
                  N_("If this option is turned on, the vote bar will be "
		     "displayed to show vote information."),
		  COC_GRAPHICS, TRUE, NULL),
  GEN_BOOL_OPTION(show_new_vote_in_front, N_("Set new votes at front"),
                  N_("If this option is enabled, then the new votes will go "
		     "to the front of the vote list"),
		  COC_GRAPHICS, FALSE, NULL),
  GEN_BOOL_OPTION(always_show_votebar, N_("Always display the vote bar"),
                  N_("If this option is turned on, the vote bar will never be "
		     "hiden, notably when there won't be any running vote."),
		  COC_GRAPHICS, FALSE, NULL),
  GEN_BOOL_OPTION(do_not_show_votebar_if_not_player,
		  N_("Do not show vote bar if not a player"),
                  N_("If this option is enabled, the client won't show the "
		     "vote bar if you are not a player."),
		  COC_GRAPHICS, FALSE, NULL),
  GEN_NUM_LIST_OPTION(player_colors_mode, N_("Player colors display mode"),
		      N_("This setting controls how the player colors are "
			 "attributed to every player. Also, it controls "
			 "how tiles, cities, and units are drawn in the map "
			 "overview window"),
		      COC_GRAPHICS, PCM_CLASSIC,
		      player_colors_mode_get_name,
		      player_colors_mode_option_callback),

  GEN_BOOL_OPTION(ai_popup_windows, N_("Popup dialogs in AI Mode"),
		  N_("Disable this option if you do not want to "
		     "to get windows popups when watching an AI player."),
		  COC_INTERFACE, FALSE, NULL),
  GEN_BOOL_OPTION(ai_manual_turn_done, N_("Manual Turn Done in AI Mode"),
		  N_("Disable this option if you do not want to "
		     "press the Turn Done button manually when watching "
		     "an AI player."),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(auto_center_on_unit, N_("Auto Center on Units"),
		  N_("Set this option to have the active unit centered "
		     "automatically when the unit focus changes."),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(auto_center_on_combat, N_("Auto Center on Combat"),
		  N_("Set this option to have any combat be centered "
		     "automatically.  Disabled this will speed up the time "
		     "between turns but may cause you to miss combat "
		     "entirely."),
		  COC_INTERFACE, FALSE, NULL),
  GEN_BOOL_OPTION(auto_center_each_turn, N_("Auto Center on New Turn"),
                  N_("Set this option to have the client automatically "
                     "recenter the map on a suitable location at the "
                     "start of each turn."),
                  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(wakeup_focus, N_("Focus on Awakened Units"),
		  N_("Set this option to have newly awoken units be "
		     "focused automatically."),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(goto_into_unknown, N_("Allow goto into the unknown"),
		  N_("Setting this option will make the game consider "
		     "moving into unknown tiles.  If not, then goto routes "
		     "will detour around or be blocked by unknown tiles."),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(center_when_popup_city, N_("Center map when Popup city"),
		  N_("Setting this option makes the mapview center on a "
		     "city when its city dialog is popped up."),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(auto_turn_done, N_("End Turn when done moving"),
		  N_("Setting this option makes your turn end automatically "
		     "when all your units are done moving."),
		  COC_INTERFACE, FALSE, NULL),
  GEN_BOOL_OPTION(ask_city_name, N_("Prompt for city names"),
		  N_("Disabling this option will make the names of newly "
		     "founded cities chosen automatically by the server."),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(popup_new_cities, N_("Pop up city dialog for new cities"),
		  N_("Setting this option will pop up a newly-founded "
		     "city's city dialog automatically."),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(show_split_message_window,
		  N_("Split chat and message window"),
		  N_("When this option is turned to on, the chat and "
                     "the message window will be splited in the main "
                     "booknote mark."),
		  COC_INTERFACE, TRUE, split_message_window_callback),
  GEN_BOOL_OPTION(use_digits_short_cuts,
                  N_("Use the shorts cuts 1-9 for Warclient features"),
		  N_("If this option is turned off, the keys 1-9 will "
		     "be used to moves your units instead of Warclient "
		     "shortcuts. This option is useful for laptops."),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(warn_before_add_to_city,
                  N_("Warn before adding a settler to a city"),
		  N_("If this option is turned on, you will get a confirmation "
		     "window popup before adding settlers in a city."),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(meta_accelerators, N_("Use Alt/Meta for accelerators"),
		  N_("Use Alt/Meta for accelerators"),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(keyboardless_goto, N_("Keyboardless goto"),
		  N_("If this option is set then a goto may be initiated "
		     "by left-clicking and then holding down the mouse "
		     "button while dragging the mouse onto a different "
		     "tile."),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(dialogs_on_top, N_("Keep dialogs on top"),
		  N_("If this option is set then dialog windows will always "
		     "remain in front of the main Freeciv window. "
		     "Disabling this has no effect in fullscreen mode."),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(enable_tabs, N_("Enable status report tabs"),
		  N_("If this option is enabled then report dialogs will "
		     "be shown as separate tabs rather than in popup "
		     "dialogs."),
		  COC_INTERFACE, TRUE, NULL),
  GEN_BOOL_OPTION(prevent_duplicate_notify_tabs,
		  N_("No tabs duplications"),
                  N_("If this option is turned on, new notify tabs will "
		     "replace those of the same name."),
		  COC_INTERFACE, FALSE, NULL),

  GEN_STR_LIST_OPTION(default_sound_set_name, N_("Default name of sound set"),
		      N_("This is the soundset that will be used.  Changing "
			 "this is the same as using the -S command-line "
			 "parameter."),
		      COC_SOUND, "stdsounds",
		      get_soundset_list, audio_change_soundset),
  GEN_STR_LIST_OPTION(default_sound_plugin_name, N_("Default sound plugin"),
		      N_("If you have a problem with sound, try changing the "
			 "sound plugin.  The new plugin won't take effect "
			 "until you restart Freeciv.  Changing this is the "
			 "same as using the -P command-line option."),
		      COC_SOUND, "",
		      get_soundplugin_list, audio_change_plugin),
  GEN_BOOL_OPTION(sound_bell_at_new_turn, N_("Sound bell at new turn"),
		  N_("Set this option to have a \"bell\" event be generated "
		     "at the start of a new turn.  You can control the "
		     "behavior of the \"bell\" event by editing the message "
		     "options."),
		  COC_SOUND, FALSE, NULL),
#ifdef AUDIO_VOLUME
  GEN_VOLUME_OPTION(sound_volume, N_("Volume"),
		    N_("This parameter controls the volume of the sounds. "
		       "Note that this option is currently only supported "
		       "by the SDL plugin."),
		    COC_SOUND, AUDIO_VOLUME_MAX, AUDIO_VOLUME_MIN,
		    AUDIO_VOLUME_MAX, audio_change_volume),
#endif /* AUDIO_VOLUME */

  GEN_STR_OPTION(chat_time_format, N_("Time format for chat messages"),
		 N_("All chat window messages will be prefixed by this "
                    "string, followed by a space. The string may contain "
                    "escape sequences understood by the C library function "
                    "strftime(3) or the unix command date(1)."),
		 COC_CHAT, "[%H:%M:%S]", NULL),
  GEN_BOOL_OPTION(enable_chat_logging, N_("Log all chat output to a file"),
		  N_("If this option is enabled, a log file will be always "
		    "generated."),
		  COC_CHAT, FALSE, NULL),
  GEN_STR_OPTION(chat_log_directory,
                 N_("Directory where chat logs are to be saved"),
		 N_("This option affect where the log file will be placed."),
		 COC_CHAT, "~/.freeciv/chatlogs", NULL),
  GEN_BOOL_OPTION(disable_chatline_scroll_on_window_resize,
		  N_("Disable chatline scrolling"),
                  N_("If this option is turned to on, the chatline won't"
		     "be scrolled to the bottom when the window is resized."),
		  COC_CHAT, FALSE, NULL),

  GEN_STR_OPTION(default_user_name, N_("Default player's login name"),
		 N_("This is the default login username that will be used "
		    "in the connection dialogs or with the -a command-line "
		    "parameter."),
		 COC_NETWORK, NULL, NULL),
  GEN_PWD_OPTION(default_password, N_("Default password"),
		 N_("This password will be automaticly used for any "
		    "connections to a server."),
		 COC_NETWORK, "", NULL),
  GEN_STR_OPTION(default_server_host, N_("Default server"),
		 N_("This is the default server hostname that will be used "
		    "in the connection dialogs or with the -a command-line "
		    "parameter."),
		 COC_NETWORK, "localhost", NULL),
  GEN_INT_OPTION(default_server_port, N_("Default server's port"),
		 N_("This is the default server port that will be used "
		    "in the connection dialogs or with the -a command-line "
		    "parameter."),
		 COC_NETWORK, DEFAULT_SOCK_PORT, 0, 9999, NULL),
  GEN_STR_OPTION(default_metaserver, N_("Default metaserver"),
		 N_("The metaserver is a host that the client contacts to "
		    "find out about games on the internet.  Don't change "
		    "this from its default value unless you know what "
		    "you're doing."),
		 COC_NETWORK, DEFAULT_METALIST_ADDR, NULL),
  GEN_BOOL_OPTION(save_options_on_exit, N_("Save options on exit"),
		  N_("If this option is enabled, the client options will be "
		     "saved when the client disconnects form the server."),
		  COC_NETWORK, TRUE, NULL),

  GEN_STR_OPTION(default_user_nation, N_("Default player's nation(s)"),
		 N_("A list of nations names whose the first existant nation "
		    "will be proposed to you by default."),
		 COC_GAMEPLAY, "", NULL),
  GEN_STR_OPTION(default_user_tech_goal, N_("Default player's technology goal"),
		 N_("This technology name will be used as first technology "
		    "goal when the game will start."),
		 COC_GAMEPLAY, "", NULL),
  GEN_BOOL_OPTION(random_leader, N_("Select random leader name"),
		  N_("If this option is disabled, your login name will be used "
		     "as default to select your leader name."),
		  COC_GAMEPLAY, FALSE, NULL),
  GEN_STR_OPTION(city_name_formats, N_("City name formats"),
		 N_("Here set your different city name formats for "
                    "auto-naming, separed by a ; . The format may contain "
		    "this following escape sequences:\n"
		    " %c: the continent number.\n"
		    " %C: the continent number as string.\n"
		    " %n: the city numero on this continent.\n"
		    " %n: the city numero on this continent as string.\n"
		    " %g: a global numero.\n"
		    " %G: a global numero as string."),
		 COC_GAMEPLAY, "island%c-city%2n;city%3g", NULL),
  GEN_BOOL_OPTION(autowakeup_state, N_("Autowakeup sentried units"),
		  N_("If disable, sentried units cannot wake up. "
		     "All sentried units are considered as sleeping."),
		  COC_GAMEPLAY, TRUE, NULL),
  GEN_BOOL_OPTION(moveandattack_state, N_("Move and attack mode"),
		  N_("When this option is enabled, the units will try to "
		     "autoattack after every move. Note that will cancel "
		     "the path for the next turn."),
		  COC_GAMEPLAY, FALSE, NULL),
  GEN_NUM_LIST_OPTION(default_caravan_action, N_("Caravan action upon arrival"),
		      N_("The caravans will execute this order when they "
			 "will arrive to a destination city."),
		      COC_GAMEPLAY, DCA_POPUP_DIALOG,
		      get_caravan_action_name, NULL),
  GEN_NUM_LIST_OPTION(default_diplomat_unit_action,
		      N_("Diplomat action upon unit arrival"),
		      N_("Diplomats execute this order when they "
			 "arrive at another player's unit."),
		      COC_GAMEPLAY, DDUA_POPUP_DIALOG,
		      get_diplomat_action_upon_unit_name, NULL),
  GEN_NUM_LIST_OPTION(default_diplomat_city_action,
		     N_("Diplomat action upon city arrival"),
		     N_("Diplomat execute this order when they "
			"arrive at another player's city."),
		     COC_GAMEPLAY, DDCA_POPUP_DIALOG,
		     get_diplomat_action_upon_city_name, NULL),
  GEN_BOOL_OPTION(default_diplomat_ignore_allies,
		  N_("Diplomat ignores allies"),
		  N_("If this option is set, diplomats and spies "
		     "will ignore all allied units and cities, i.e. "
		     "they will pass over them instead of performing "
		     "an action."),
		  COC_GAMEPLAY, TRUE, NULL),
  GEN_NUM_LIST_OPTION(default_action_type, N_("New unit default action"),
		      N_("The new created units will do this activity "
			 "automaticely."),
		      COC_GAMEPLAY, ACTION_IDLE,
		      get_new_unit_action_name, NULL),
  GEN_BOOL_OPTION(default_action_locked, N_("Lock the new unit default action"),
		  N_("If this setting is disabled, the new unit default action "
		     "will be canceled every turn"),
		  COC_GAMEPLAY, FALSE, NULL),
  GEN_BOOL_OPTION(default_action_military_only,
		  N_("New unit default action for military units only"),
		  N_("If enabled, units not able to attack won't do the "
		     "default action."),
		  COC_GAMEPLAY, TRUE, NULL),

  GEN_NUM_LIST_OPTION(multi_select_place, N_("Multi-selection place mode"),
		      N_("This option affects where the units "
			 "will be selected."),
		      COC_MULTI_SELECTION, PLACE_ON_CONTINENT,
		      get_place_mode_name, NULL),
  GEN_NUM_LIST_OPTION(multi_select_utype, N_("Multi-selection unit type mode"),
		      N_("This option affects what kinds of units "
			 "will be selected."),
		      COC_MULTI_SELECTION, UTYPE_SAME_TYPE,
		      get_utype_mode_name, NULL),
  GEN_BOOL_OPTION(multi_select_count_all,
		  N_("Count all units in the selection"),
		  N_("If this option is enabled, the unit count "
		     "(in the unit label) will include excluded units "
		     "(by filters)."),
		  COC_MULTI_SELECTION, FALSE, NULL),
  GEN_BOOL_OPTION(multi_select_blink_all,
		  N_("Blink all units in the selection"),
		  N_("If this option is enabled, all selected units "
		     "will blink, including excluded units (by filters)."),
		  COC_MULTI_SELECTION, FALSE, NULL),
  GEN_BOOL_OPTION(multi_select_blink, N_("Blink units in the selection"),
		  N_("If this option is disabled, only the first unit "
		     "will blink."),
		  COC_MULTI_SELECTION, TRUE, NULL),
  GEN_BOOL_OPTION(multi_select_map_selection,
		  N_("Allow multi-selection at map"),
		  N_("If this option is enabled, you will be able "
		     "to select units with the yellow rectangle "
		     "(Dragging right click)."),
		  COC_MULTI_SELECTION, FALSE, NULL),
  GEN_BOOL_OPTION(multi_select_spread_airport_cities,
		  N_("Spread only in cities with airport"),
		  N_("If this option is enabled, spreading units command "
		     "will spread selected units only in the cities which "
		     "have an airport."),
		  COC_MULTI_SELECTION, FALSE, NULL),
  GEN_BOOL_OPTION(multi_select_spread_allied_cities,
		  N_("Allow spreading into allied cities"),
		  N_("If this option is enabled, spreading units command "
		     "will spread selected units in allies cities or in "
		     "your own ciries indifferently"),
		  COC_MULTI_SELECTION, FALSE, NULL),
  GEN_FILTER_OPTION(multi_select_inclusive_filter,
		    N_("Multi-selection inclusive filter"),
		    N_("This condition filter will be used to determinate "
		       "the unit which will be included to the selection."
		       "The units must satisfy all condition of this filter."),
		    COC_MULTI_SELECTION, FILTER_ABLE_TO_MOVE | FILTER_IDLE,
		    filter_change, get_filter_value_name, NULL),
  GEN_FILTER_OPTION(multi_select_exclusive_filter,
		    N_("Multi-selection exclusive filter"),
		    N_("This condition filter will be used to determinate "
		       "the unit which will be excluded from the selection."
		       "The units have to don't satisfy any of this condition "
		       "of this filter."),
		    COC_MULTI_SELECTION,
		    FILTER_FORTIFIED | FILTER_SENTRIED | FILTER_AUTO,
		    filter_change, get_filter_value_name, NULL),

  GEN_INT_OPTION(delayed_goto_unit_limit, N_("Delayed goto unit limit"),
		 N_("This number represents the maximal orders number that a "
		    "delayed goto queue can exexute at once. 0 means "
		    "unlimited units."),
		 COC_DELAYED_GOTO, 0, 0, 1000, NULL),
  GEN_NUM_LIST_OPTION(delayed_goto_place, N_("Delayed goto place mode"),
		      N_("This option affects where the units "
			 "will be selected."),
		      COC_DELAYED_GOTO, PLACE_ON_TILE,
		      get_place_mode_name, NULL),
  GEN_NUM_LIST_OPTION(delayed_goto_utype, N_("Delayed goto unit type mode"),
		      N_("This option affects what kinds of units will be "
			 "selected."),
		      COC_DELAYED_GOTO, UTYPE_SAME_TYPE,
		      get_utype_mode_name, NULL),
  GEN_FILTER_OPTION(delayed_goto_inclusive_filter,
		    N_("Delayed goto inclusive filter"),
		    N_("This condition filter will be used to determinate "
		       "the unit which will be included to the selection."
		       "The units must satisfy all condition of this filter."),
		    COC_DELAYED_GOTO, FILTER_ALL,
		    filter_change, get_filter_value_name, NULL),
  GEN_FILTER_OPTION(delayed_goto_exclusive_filter,
		    N_("Delayed goto exclusive filter"),
		    N_("This condition filter will be used to determinate "
		       "the unit which will be excluded from the selection."
		       "The units have to don't satisfy any of this condition "
		       "of this filter."),
		    COC_DELAYED_GOTO, FILTER_OFF,
		    filter_change, get_filter_value_name, NULL),
  GEN_FILTER_OPTION(delayed_goto_list[0].automatic_execution,
		    N_("Delayed goto automatic execution"),
		    N_("The delayed goto queue will be executed automatically "
		       "when one of the set event will occur."),
		    COC_DELAYED_GOTO, AUTO_WAR_DIPLSTATE,
		    delayed_goto_auto_filter_change,
		    delayed_goto_get_auto_name, NULL),

#ifndef ASYNC_TRADE_PLANNING
  GEN_INT_OPTION(trade_time_limit,
		 N_("Time limit for trade planning calculation (seconds)"),
		 N_("This option controls how long the trade planning "
		    "can be calculated. If you set to 0, it will be "
		    "unlimited."),
		 COC_TRADE, 10, 0, 60, NULL),
#endif	/* ASYNC_TRADE_PLANNING */
  GEN_BOOL_OPTION(trade_mode_best_values, N_("Best value trade route mode"),
		  N_("If you set this option on, auto-trade will check the "
		     "best trade route by value you can establish. If you "
                     "turn it off, it will check the faster trade route."),
		  COC_TRADE, FALSE, NULL),
  GEN_BOOL_OPTION(trade_mode_allow_free_other,
                  N_("Allow free other caravans mode"),
		  N_("If you set this option on, auto-trade can free an "
		     "other caravan, if the new caravan is faster."),
		  COC_TRADE, TRUE, NULL),
  GEN_BOOL_OPTION(trade_mode_internal_first, N_("Internal trade first mode"),
		  N_("If you set this option on, auto-trade won't check "
		     "external trade routes while there are planned "
                      "internal trade routes."),
		  COC_TRADE, TRUE, NULL),
  GEN_BOOL_OPTION(trade_mode_homecity_first,
                  N_("Homecity trade first mode"),
		  N_("If you set this option on, auto-trade will check "
		     "the trade routes you can establish from the caravan "
                     "homecity."),
		  COC_TRADE, TRUE, NULL),
};

#undef GEN_INT_OPTION
#undef GEN_BOOL_OPTION
#undef GEN_STR_OPTION

struct client_option *const options = client_options;
const int num_options = ARRAY_SIZE(client_options);

/** View Options: **/

bool draw_map_grid = FALSE;
bool draw_city_names = TRUE;
bool draw_city_growth = TRUE;
bool draw_city_productions = FALSE;
bool draw_city_traderoutes = FALSE;
bool draw_city_production_buy_cost = FALSE;
bool draw_terrain = TRUE;
bool draw_coastline = FALSE;
bool draw_roads_rails = TRUE;
bool draw_irrigation = TRUE;
bool draw_mines = TRUE;
bool draw_fortress_airbase = TRUE;
bool draw_specials = TRUE;
bool draw_pollution = TRUE;
bool draw_cities = TRUE;
bool draw_units = TRUE;
bool draw_focus_unit = FALSE;
bool draw_fog_of_war = TRUE;
bool draw_borders = TRUE;

#define VIEW_OPTION(name) { #name, &name }
#define VIEW_OPTION_TERMINATOR { NULL, NULL }

view_option view_options[] = {
  VIEW_OPTION(draw_map_grid),
  VIEW_OPTION(draw_city_names),
  VIEW_OPTION(draw_city_growth),
  VIEW_OPTION(draw_city_productions),
  VIEW_OPTION(draw_city_traderoutes),
  VIEW_OPTION(draw_city_production_buy_cost),
  VIEW_OPTION(draw_terrain),
  VIEW_OPTION(draw_coastline),
  VIEW_OPTION(draw_roads_rails),
  VIEW_OPTION(draw_irrigation),
  VIEW_OPTION(draw_mines),
  VIEW_OPTION(draw_fortress_airbase),
  VIEW_OPTION(draw_specials),
  VIEW_OPTION(draw_pollution),
  VIEW_OPTION(draw_cities),
  VIEW_OPTION(draw_units),
  VIEW_OPTION(draw_focus_unit),
  VIEW_OPTION(draw_fog_of_war),
  VIEW_OPTION(draw_borders),
  VIEW_OPTION_TERMINATOR
};

#undef VIEW_OPTION
#undef VIEW_OPTION_TERMINATOR

/** Message Options: **/

unsigned int messages_where[E_LAST];
int sorted_events[E_LAST];

#define GEN_EV(descr, event) { #event, NULL, descr, NULL, event }
#define GEN_EV_TERMINATOR { NULL, NULL, NULL, NULL, E_NOEVENT }

/*
 * Holds information about all event types. The entries don't have
 * to be sorted.
 */
static struct {
  const char *enum_name;
  char *tag_name;
  const char *descr_orig;
  const char *descr;
  enum event_type event;
} events[] = {
  GEN_EV(N_("City: Building Unavailable Item"),       E_CITY_CANTBUILD),
  GEN_EV(N_("City: Captured/Destroyed"),              E_CITY_LOST),
  GEN_EV(N_("City: Celebrating"),                     E_CITY_LOVE),
  GEN_EV(N_("City: Civil Disorder"),                  E_CITY_DISORDER),
  GEN_EV(N_("City: Famine"),                          E_CITY_FAMINE),
  GEN_EV(N_("City: Famine Feared"),       	      E_CITY_FAMINE_FEARED),
  GEN_EV(N_("City: Growth"),                          E_CITY_GROWTH),
  GEN_EV(N_("City: May Soon Grow"),                   E_CITY_MAY_SOON_GROW),
  GEN_EV(N_("City: Needs Aqueduct"),                  E_CITY_AQUEDUCT),
  GEN_EV(N_("City: Needs Aqueduct Being Built"),      E_CITY_AQ_BUILDING),
  GEN_EV(N_("City: Normal"),                          E_CITY_NORMAL),
  GEN_EV(N_("City: Nuked"),                           E_CITY_NUKED),
  GEN_EV(N_("City: Released from CMA"),               E_CITY_CMA_RELEASE),
  GEN_EV(N_("City: Suggest Growth Throttling"),       E_CITY_GRAN_THROTTLE),
  GEN_EV(N_("City: Transfer"),                        E_CITY_TRANSFER),
  GEN_EV(N_("City: Was Built"),                       E_CITY_BUILD),
  GEN_EV(N_("City: Worklist Events"),                 E_WORKLIST),
  GEN_EV(N_("Civ: Barbarian Uprising"),               E_UPRISING ),
  GEN_EV(N_("Civ: Civil War"),                        E_CIVIL_WAR),
  GEN_EV(N_("Civ: Collapse to Anarchy"),              E_ANARCHY),
  GEN_EV(N_("Civ: First Contact"),                    E_FIRST_CONTACT),
  GEN_EV(N_("Civ: Learned New Government"),	      E_NEW_GOVERNMENT),
  GEN_EV(N_("Civ: Low Funds"),                        E_LOW_ON_FUNDS),
  GEN_EV(N_("Civ: Pollution"),                        E_POLLUTION),
  GEN_EV(N_("Civ: Revolt Ended"),                     E_REVOLT_DONE),
  GEN_EV(N_("Civ: Revolt Started"),                   E_REVOLT_START),
  GEN_EV(N_("Civ: Spaceship Events"),                 E_SPACESHIP),
  GEN_EV(N_("Diplomat Action: Bribe"),              E_MY_DIPLOMAT_BRIBE),
  GEN_EV(N_("Diplomat Action: Caused Incident"),    E_DIPLOMATIC_INCIDENT),
  GEN_EV(N_("Diplomat Action: Escape"),             E_MY_DIPLOMAT_ESCAPE),
  GEN_EV(N_("Diplomat Action: Embassy"),            E_MY_DIPLOMAT_EMBASSY),
  GEN_EV(N_("Diplomat Action: Failed"),             E_MY_DIPLOMAT_FAILED),
  GEN_EV(N_("Diplomat Action: Incite"),             E_MY_DIPLOMAT_INCITE),
  GEN_EV(N_("Diplomat Action: Poison"),             E_MY_DIPLOMAT_POISON),
  GEN_EV(N_("Diplomat Action: Sabotage"),           E_MY_DIPLOMAT_SABOTAGE),
  GEN_EV(N_("Diplomat Action: Theft"),              E_MY_DIPLOMAT_THEFT),
  GEN_EV(N_("Enemy Diplomat: Bribe"),               E_ENEMY_DIPLOMAT_BRIBE),
  GEN_EV(N_("Enemy Diplomat: Embassy"),             E_ENEMY_DIPLOMAT_EMBASSY),
  GEN_EV(N_("Enemy Diplomat: Failed"),              E_ENEMY_DIPLOMAT_FAILED),
  GEN_EV(N_("Enemy Diplomat: Incite"),              E_ENEMY_DIPLOMAT_INCITE),
  GEN_EV(N_("Enemy Diplomat: Poison"),              E_ENEMY_DIPLOMAT_POISON),
  GEN_EV(N_("Enemy Diplomat: Sabotage"),            E_ENEMY_DIPLOMAT_SABOTAGE),
  GEN_EV(N_("Enemy Diplomat: Theft"),               E_ENEMY_DIPLOMAT_THEFT),
  GEN_EV(N_("Game: Broadcast Report"),                E_BROADCAST_REPORT),
  GEN_EV(N_("Game: Game Ended"),                      E_GAME_END),
  GEN_EV(N_("Game: Game Started"),                    E_GAME_START),
  GEN_EV(N_("Game: Message from Server Operator"),    E_MESSAGE_WALL),
  GEN_EV(N_("Game: Nation Selected"),                 E_NATION_SELECTED),
  GEN_EV(N_("Game: Player Destroyed"),                E_DESTROYED),
  GEN_EV(N_("Game: Report"),                          E_REPORT),
  GEN_EV(N_("Game: Turn Bell"),                       E_TURN_BELL),
  GEN_EV(N_("Game: Year Advance"),                    E_NEXT_YEAR),
  GEN_EV(N_("Global: Eco-Disaster"),                  E_GLOBAL_ECO),
  GEN_EV(N_("Global: Nuke Detonated"),                E_NUKE),
  GEN_EV(N_("Hut: Barbarians in a Hut Roused"),       E_HUT_BARB),
  GEN_EV(N_("Hut: City Founded from Hut"),            E_HUT_CITY),
  GEN_EV(N_("Hut: Gold Found in Hut"),                E_HUT_GOLD),
  GEN_EV(N_("Hut: Killed by Barbarians in a Hut"),    E_HUT_BARB_KILLED),
  GEN_EV(N_("Hut: Mercenaries Found in Hut"),         E_HUT_MERC),
  GEN_EV(N_("Hut: Settler Found in Hut"),             E_HUT_SETTLER),
  GEN_EV(N_("Hut: Tech Found in Hut"),                E_HUT_TECH),
  GEN_EV(N_("Hut: Unit Spared by Barbarians"),        E_HUT_BARB_CITY_NEAR),
  GEN_EV(N_("Improvement: Bought"),                   E_IMP_BUY),
  GEN_EV(N_("Improvement: Built"),                    E_IMP_BUILD),
  GEN_EV(N_("Improvement: Forced to Sell"),           E_IMP_AUCTIONED),
  GEN_EV(N_("Improvement: New Improvement Selected"), E_IMP_AUTO),
  GEN_EV(N_("Improvement: Sold"),                     E_IMP_SOLD),
  GEN_EV(N_("Tech: Learned From Great Library"),      E_TECH_GAIN),
  GEN_EV(N_("Tech: Learned New Tech"),                E_TECH_LEARNED),
  GEN_EV(N_("Treaty: Alliance"),                      E_TREATY_ALLIANCE),
  GEN_EV(N_("Treaty: Broken"),                        E_TREATY_BROKEN),
  GEN_EV(N_("Treaty: Ceasefire"),                     E_TREATY_CEASEFIRE),
  GEN_EV(N_("Treaty: Peace"),                         E_TREATY_PEACE),
  GEN_EV(N_("Treaty: Shared Vision"),                 E_TREATY_SHARED_VISION),
  GEN_EV(N_("Unit: Attack Failed"),                   E_UNIT_LOST_ATT),
  GEN_EV(N_("Unit: Attack Succeeded"),                E_UNIT_WIN_ATT),
  GEN_EV(N_("Unit: Bought"),                          E_UNIT_BUY),
  GEN_EV(N_("Unit: Built"),                           E_UNIT_BUILT),
  GEN_EV(N_("Unit: Defender Destroyed"),              E_UNIT_LOST),
  GEN_EV(N_("Unit: Defender Survived"),               E_UNIT_WIN),
  GEN_EV(N_("Unit: Became More Veteran"),             E_UNIT_BECAME_VET),
  GEN_EV(N_("Unit: Production Upgraded"),             E_UNIT_UPGRADED),
  GEN_EV(N_("Unit: Relocated"),                       E_UNIT_RELOCATED),
  GEN_EV(N_("Unit: Orders / goto events"),            E_UNIT_ORDERS),
  GEN_EV(N_("Wonder: Finished"),                      E_WONDER_BUILD),
  GEN_EV(N_("Wonder: Made Obsolete"),                 E_WONDER_OBSOLETE),
  GEN_EV(N_("Wonder: Started"),                       E_WONDER_STARTED),
  GEN_EV(N_("Wonder: Stopped"),                       E_WONDER_STOPPED),
  GEN_EV(N_("Wonder: Will Finish Next Turn"),         E_WONDER_WILL_BE_BUILT),
  GEN_EV(N_("Diplomatic Message"),                    E_DIPLOMACY),
  GEN_EV(N_("City: Production changed"),              E_CITY_PRODUCTION_CHANGED),
  GEN_EV(N_("Treaty: Embassy"),                       E_TREATY_EMBASSY),
  GEN_EV_TERMINATOR
};

/* 
 * Maps from enum event_type to indexes of events[]. Set by
 * init_messages_where. 
 */
static int event_to_index[E_LAST];

static void save_cma_preset(struct section_file *file, char *name,
			    const struct cm_parameter *const pparam,
			    int inx);
static void load_cma_preset(struct section_file *file, int inx);

/**************************************************************************
  Returns the translated description of the given event.
**************************************************************************/
const char *get_message_text(enum event_type event)
{
  assert(event >= 0 && event < E_LAST);

  if (events[event_to_index[event]].event == event) {
    return events[event_to_index[event]].descr;
  }
  freelog(LOG_ERROR, "get_message_text: unknown event %d", event);
  return "UNKNOWN EVENT";
}

/**************************************************************************
  Comparison function for qsort; i1 and i2 are pointers to an event
  (enum event_type).
**************************************************************************/
static int compar_message_texts(const void *i1, const void *i2)
{
  int j1 = *(const int*)i1;
  int j2 = *(const int*)i2;
  
  return mystrcasecmp(get_message_text(j1), get_message_text(j2));
}

/****************************************************************
  ...
*****************************************************************/
unsigned int get_default_messages_where(enum event_type type)
{
  switch (type) {
  case E_IMP_BUY:
  case E_IMP_SOLD:
  case E_UNIT_BUY:
  case E_UNIT_LOST_ATT:
  case E_UNIT_WIN_ATT:
  case E_GAME_START:
  case E_NATION_SELECTED:
  case E_CITY_BUILD:
  case E_NEXT_YEAR:
  case E_CITY_PRODUCTION_CHANGED:
  case E_CITY_MAY_SOON_GROW:
  case E_WORKLIST:
    return 0;
    break;
  case E_MESSAGE_WALL:
    return MW_MESSAGES | MW_POPUP;
  default:
    return MW_MESSAGES;
  }
}

/****************************************************************
  These could be a static table initialisation, except
  its easier to do it this way.
  Now also initialise sorted_events[].
*****************************************************************/
void init_messages_where(void)
{
  int i;

  for (i = 0; i < ARRAY_SIZE(event_to_index); i++) {
    event_to_index[i] = 0;
  }

  for (i = 0;; i++) {
    int j;

    if (events[i].event == E_NOEVENT) {
      break;
    }
    events[i].descr = _(events[i].descr_orig);
    event_to_index[events[i].event] = i;
    events[i].tag_name = mystrdup(events[i].enum_name);
    for (j = 0; j < strlen(events[i].tag_name); j++) {
      events[i].tag_name[j] = my_tolower(events[i].tag_name[j]);
    }
    freelog(LOG_DEBUG,
	    "event[%d]=%d: name='%s' / '%s'\n\tdescr_orig='%s'\n\tdescr='%s'",
	    i, events[i].event, events[i].enum_name, events[i].tag_name,
	    events[i].descr_orig, events[i].descr);
  }

  for(i=0;i<E_LAST;i++)  {
    sorted_events[i] = i;
  }
  qsort(sorted_events, E_LAST, sizeof(int), compar_message_texts);
}

/****************************************************************
  Set all options to their default value and translate all.
*****************************************************************/
void client_options_init(void)
{
  int i;

  client_options_iterate(o) {
    switch (o->type) {
    case COT_BOOLEAN:
      *o->o.boolean.pvalue = o->o.boolean.def;
      break;
    case COT_INTEGER:
    case COT_VOLUME:
      assert(o->o.integer.def >= o->o.integer.min);
      assert(o->o.integer.def <= o->o.integer.max);
      *o->o.integer.pvalue = o->o.integer.def;
      break;
    case COT_STRING:
    case COT_PASSWORD:
      /* HACK: fix default username */
      if (o->o.string.pvalue == default_user_name) {
	o->o.string.def = user_username();
      }
      /* HACK: fix default tileset */
      if (o->o.string.pvalue == default_tileset_name) {
	o->o.string.def = get_default_tilespec_name();
      }
      mystrlcpy(o->o.string.pvalue, o->o.string.def, o->o.string.size);
      break;
    case COT_NUMBER_LIST:
      assert(NULL != o->o.list.str_accessor);
      assert(NULL != o->o.list.str_accessor(o->o.list.def));
      *o->o.list.pvalue = o->o.list.def;
      break;
    case COT_FILTER:
      *o->o.filter.pvalue = o->o.filter.def;      
      break;
    }
  } client_options_iterate_end;

  for (i = 0; i < ARRAY_SIZE(global_worklists); i++) {
    init_worklist(&global_worklists[i]);
  }
}

/****************************************************************
  ...
*****************************************************************/
filter secfile_lookup_filter_default(struct section_file *sf, filter def,
				     struct client_option *o,
				     const char *path, ...)
{
  va_list args;
  char buf[256];
  char **vec;
  int i, size;
  bool ok = FALSE;
  filter ret = 0, f;

  assert(o && o->type == COT_FILTER);

  va_start(args, path);
  my_vsnprintf(buf, sizeof(buf), path, args);
  va_end(args);

  vec = secfile_lookup_str_vec(sf, &size, "%s", buf);
  if (!vec || !size) {
    return def;
  }

  for (i = 0; i < size; i++) {
    f = filter_revert_str_accessor(o->o.filter.str_accessor, vec[i]);
    if (f) {
      o->o.filter.change(&ret, f);
      ok = TRUE;
    } else {
      freelog(LOG_NORMAL, _("The value '%s' is not supported for '%s'"),
	      vec[i], o->name);
    }
  }

  return ok ? ret : def;
}

/****************************************************************
  Revert the str_accessor. Find an index by name.
  Returns -1 on error.
*****************************************************************/
int revert_str_accessor(const char *(*str_accessor)(int), const char *str)
{
  if (!str_accessor || !str) {
    return -1;
  }

  const char *p;
  int i;

  for (i = 0; (p = str_accessor(i)); i++) {
    if (0 == strcasecmp(p, str)) {
      return i;
    }
  }

  return -1;
}

/****************************************************************
  Revert the str_accessor. Find an index by name.
  Returns 0 on error.
*****************************************************************/
filter filter_revert_str_accessor(const char *(*str_accessor)(filter),
				  const char *str)
{
  if (!str_accessor || !str) {
    return 0;
  }

  const char *p;
  filter i;

  for (i = 1; (p = str_accessor(i)); i <<= 1) {
    if (0 == strcasecmp(p, str)) {
      return i;
    }
  }

  return 0;
}

/****************************************************************
 The "options" file handles actual "options", and also view options,
 message options, city report settings, cma settings, and 
 saved global worklists
*****************************************************************/

/****************************************************************
 Returns pointer to static memory containing name of option file.
 Ie, based on FREECIV_OPT env var, and home dir. (or a
 OPTION_FILE_NAME define defined in config.h)
 Or NULL if problem.
*****************************************************************/
const char *option_file_name(void)
{
  static char name_buffer[256];
  char *name;

  name = getenv("FREECIV_OPT");

  if (name) {
    sz_strlcpy(name_buffer, name);
  } else {
#ifndef OPTION_FILE_NAME
    name = user_home_dir();
    if (!name) {
      append_output_window(_("Cannot find your home directory"));
      return NULL;
    }
    mystrlcpy(name_buffer, name, 231);
    sz_strlcat(name_buffer, "/.civclientrc");
#else
    mystrlcpy(name_buffer, OPTION_FILE_NAME, sizeof(name_buffer));
#endif
  }
  freelog(LOG_VERBOSE, "settings file is %s", name_buffer);
  return name_buffer;
}

/****************************************************************
 this loads from the rc file any options which are not ruleset specific 
 it is called on client init.
*****************************************************************/
void load_general_options(void)
{
  struct section_file sf;
  const char * const prefix = "client";
  const char *name;
  int i, num;
  view_option *v;

  name = option_file_name();
  if (!name) {
    /* fail silently */
    return;
  }
  if (!section_file_load(&sf, name)) {
    create_default_cma_presets();
    create_default_chatline_colors();
    return;  
  }

  client_options_iterate(o) {
    switch (o->type) {
    case COT_BOOLEAN:
      *o->o.boolean.pvalue =
	  secfile_lookup_bool_default(&sf, o->o.boolean.def, "%s.%s",
				      prefix, o->name);
      break;
    case COT_INTEGER:
    case COT_VOLUME:
      *o->o.integer.pvalue =
	  secfile_lookup_int_default(&sf, o->o.integer.def, "%s.%s",
				      prefix, o->name);
      if (*o->o.integer.pvalue < o->o.integer.min
	  || *o->o.integer.pvalue > o->o.integer.max) {
	freelog(LOG_ERROR, "The option '%s' (%d) is out of scale [%d, %d], "
		"using default...", o->name, *o->o.integer.pvalue,
		o->o.integer.min, o->o.integer.max);
	*o->o.integer.pvalue = o->o.integer.def;
      }
      break;
    case COT_STRING:
    case COT_PASSWORD:
      mystrlcpy(o->o.string.pvalue,
		secfile_lookup_str_default(&sf, o->o.string.def, "%s.%s",
                     prefix, o->name), o->o.string.size);
      break;
    case COT_NUMBER_LIST:
      *o->o.list.pvalue =
	  revert_str_accessor(o->o.list.str_accessor,
			      secfile_lookup_str_default(&sf, NULL, "%s.%s",
							 prefix, o->name));
      if (*o->o.list.pvalue == -1) {
	*o->o.list.pvalue = o->o.list.def;
      }
      break;
    case COT_FILTER:
      *o->o.filter.pvalue = secfile_lookup_filter_default(&sf, o->o.filter.def,
							  o, "%s.%s",
							  prefix, o->name);
      break;
    }
  } client_options_iterate_end;
  for (v = view_options; v->name; v++) {
    *(v->p_value) =
	secfile_lookup_bool_default(&sf, *(v->p_value), "%s.%s", prefix,
				    v->name);
  }
  for (i = 0; i < E_LAST; i++) {
    messages_where[i] =
      secfile_lookup_int_default(&sf, messages_where[i],
				 "%s.message_where_%02d", prefix, i);
  }
  for (i = 1; i < num_city_report_spec(); i++) {
    bool *ip = city_report_spec_show_ptr(i);
    *ip = secfile_lookup_bool_default(&sf, *ip, "%s.city_report_%s", prefix,
				     city_report_spec_tagname(i));
  }
  
  for(i = 1; i < num_player_dlg_columns; i++) {
    bool *show = &(player_dlg_columns[i].show);
    *show = secfile_lookup_bool_default(&sf, *show, "%s.player_dlg_%s", prefix,
                                        player_dlg_columns[i].tagname);
  }

  /* Load cma presets. If cma.number_of_presets doesn't exist, don't load 
   * any, the order here should be reversed to keep the order the same */
  num = secfile_lookup_int_default(&sf, -1, "cma.number_of_presets");
  if (num == -1) {
    create_default_cma_presets();
  } else {
    for (i = num - 1; i >= 0; i--) {
      load_cma_preset(&sf, i);
    }
  }
 
  /* Load global worklists: we always do it, to don't lose datas. Not however,
   * that is_valid will be set only in check_ruleset_specific_options(). */
  for (i = 0; i < ARRAY_SIZE(global_worklists); i++) {
    load_global_worklist(&sf, "worklists.worklist%d", i, &global_worklists[i]);
  }

  secfile_load_chatline_colors (&sf);
 
  section_file_free(&sf);
}

/****************************************************************
  Set the validity of the worklists for the current ruleset.
*****************************************************************/
void check_ruleset_specific_options(void)
{
  int i;

  /* Check if the worklists match the ruleset options. */
  for (i = 0; i < ARRAY_SIZE(global_worklists); i++) {
    global_worklists[i].is_valid = check_global_worklist(&global_worklists[i]);
  }
}

/****************************************************************
... 
*****************************************************************/
void save_options(void)
{
  struct section_file sf;
  const char *name = option_file_name();
  char output_buffer[256];
  view_option *v;
  int i;

  if (!name) {
    append_output_window(_("Save failed, cannot find a filename."));
    return;
  }

  section_file_init(&sf);
  secfile_insert_str(&sf, VERSION_STRING, "client.version");

  client_options_iterate(o) {
    switch (o->type) {
    case COT_BOOLEAN:
      secfile_insert_bool(&sf, *o->o.boolean.pvalue, "client.%s", o->name);
      break;
    case COT_INTEGER:
    case COT_VOLUME:
      secfile_insert_int(&sf, *o->o.integer.pvalue, "client.%s", o->name);
      break;
    case COT_STRING:
    case COT_PASSWORD:
      secfile_insert_str(&sf, o->o.string.pvalue, "client.%s", o->name);
      break;
    case COT_NUMBER_LIST:
      secfile_insert_str(&sf, o->o.list.str_accessor(*o->o.list.pvalue),
			 "client.%s", o->name);
      break;
    case COT_FILTER:
      {
#define MAX_VALUES 64
	const char *values[MAX_VALUES]; 
	size_t size = 0, vec;
	filter f = *o->o.filter.pvalue;

	for (f = *o->o.filter.pvalue, vec = 0; f != 0 && size < MAX_VALUES;
	     f >>= 1, vec++) {
	  if (f & 1) {
	    values[size++] = o->o.filter.str_accessor(1 << vec);
	  }
	}
	if (f != 0) {
	  freelog(LOG_ERROR, "The filter '%s' required more values number.",
		  o->name);
	} else {
	  secfile_insert_str_vec(&sf, values, size, "client.%s", o->name);
	}
      }
      break;
    }
  } client_options_iterate_end;

  for (v = view_options; v->name; v++) {
    secfile_insert_bool(&sf, *(v->p_value), "client.%s", v->name);
  }

  for (i = 0; i < E_LAST; i++) {
    secfile_insert_int_comment(&sf, messages_where[i],
			       get_message_text(i),
			       "client.message_where_%02d", i);
  }

  for (i = 1; i < num_city_report_spec(); i++) {
    secfile_insert_bool(&sf, *(city_report_spec_show_ptr(i)),
		       "client.city_report_%s",
		       city_report_spec_tagname(i));
  }
  
  for (i = 1; i < num_player_dlg_columns; i++) {
    secfile_insert_bool(&sf, player_dlg_columns[i].show,
                        "client.player_dlg_%s",
                        player_dlg_columns[i].tagname);
  }

  /* insert global worklists */
  for(i = 0; i < ARRAY_SIZE(global_worklists); i++) {
    save_global_worklist(&sf, "worklists.worklist%d", i,  &global_worklists[i]);
  }

  /* insert cma presets */
  secfile_insert_int_comment(&sf, cmafec_preset_num(),
			     _("If you add a preset by "
			       "hand, also update \"number_of_presets\""),
			     "cma.number_of_presets");
  for (i = 0; i < cmafec_preset_num(); i++) {
    save_cma_preset(&sf, cmafec_preset_get_descr(i),
		    cmafec_preset_get_parameter(i), i);
  }

  secfile_save_chatline_colors (&sf);

  /* save to disk */
  if (!section_file_save(&sf, name, 0)) {
    my_snprintf(output_buffer, sizeof(output_buffer),
		_("Save failed, cannot write to file %s"), name);
  } else {
    my_snprintf(output_buffer, sizeof(output_buffer),
		_("Saved settings to file %s"), name);
  }

  append_output_window(output_buffer);
  section_file_free(&sf);
}

/****************************************************************
 Does heavy lifting for looking up a preset.
*****************************************************************/
static void load_cma_preset(struct section_file *file, int inx)
{
  struct cm_parameter parameter;
  const char *name;
  int i;

  name = secfile_lookup_str_default(file, "preset", 
				    "cma.preset%d.name", inx);
  for (i = 0; i < CM_NUM_STATS; i++) {
    parameter.minimal_surplus[i] =
	secfile_lookup_int_default(file, 0, "cma.preset%d.minsurp%d", inx, i);
    parameter.factor[i] =
	secfile_lookup_int_default(file, 0, "cma.preset%d.factor%d", inx, i);
  }
  parameter.require_happy =
      secfile_lookup_bool_default(file, FALSE, "cma.preset%d.reqhappy", inx);
  parameter.happy_factor =
      secfile_lookup_int_default(file, 0, "cma.preset%d.happyfactor", inx);
  parameter.allow_disorder = FALSE;
  parameter.allow_specialists = TRUE;

  cmafec_preset_add(name, &parameter);
}

/****************************************************************
 Does heavy lifting for inserting a preset.
*****************************************************************/
static void save_cma_preset(struct section_file *file, char *name,
			    const struct cm_parameter *const pparam,
			    int inx)
{
  int i;

  secfile_insert_str(file, name, "cma.preset%d.name", inx);
  for (i = 0; i < CM_NUM_STATS; i++) {
    secfile_insert_int(file, pparam->minimal_surplus[i],
		       "cma.preset%d.minsurp%d", inx, i);
    secfile_insert_int(file, pparam->factor[i],
		       "cma.preset%d.factor%d", inx, i);
  }
  secfile_insert_bool(file, pparam->require_happy,
		      "cma.preset%d.reqhappy", inx);
  secfile_insert_int(file, pparam->happy_factor,
		     "cma.preset%d.happyfactor", inx);
}

/****************************************************************
... 
*****************************************************************/
const char *get_sound_tag_for_event(enum event_type event)
{
  if (event == E_NOEVENT) {
    return NULL;
  }

  assert(event >= 0 && event < E_LAST);

  if (events[event_to_index[event]].event == event) {
    return events[event_to_index[event]].tag_name;
  }
  freelog(LOG_ERROR, "get_sound_tag_for_event: unknown event %d", event);
  return NULL;
}

/****************************************************************
  Loads global worklist from rc file.
*****************************************************************/
void load_global_worklist(struct section_file *file, const char *path,
			  int wlinx, struct worklist *pwl)
{
  char prefix[64];
  int i;

  my_snprintf(prefix, sizeof(prefix), path, wlinx);

  /* We don't care about is_valid, because it will be set later. */
  pwl->is_valid = FALSE;
  strcpy(pwl->name, secfile_lookup_str_default(file, "", "%s.name", prefix));
  for (i = 0; i < MAX_LEN_WORKLIST; i++) {
    pwl->wlefs[i] =
      secfile_lookup_int_default(file, WEF_END, "%s.wlef%d", prefix, i);
    pwl->wlids[i] =
      secfile_lookup_int_default(file, 0, "%s.wlid%d", prefix, i);
  }
}

/****************************************************************
  Saves global worklist to rc file.
*****************************************************************/
void save_global_worklist(struct section_file *file, const char *path,
			  int wlinx, struct worklist *pwl)
{
  char prefix[64];
  int i;

  my_snprintf(prefix, sizeof(prefix), path, wlinx);

  /* To don't lose data in ruleset changes, let's say that only empty global
   * lists are valids. */
  secfile_insert_bool(file, !worklist_is_empty(pwl), "%s.is_valid", prefix);
  secfile_insert_str(file, pwl->name, "%s.name", prefix);
  for (i = 0; i < MAX_LEN_WORKLIST; i++) {
    secfile_insert_int(file, pwl->wlefs[i], "%s.wlef%d", prefix, i);
    secfile_insert_int(file, pwl->wlids[i], "%s.wlid%d", prefix, i);
  }
}

/****************************************************************
  Returns TRUE iff this worlist is valid.
*****************************************************************/
bool check_global_worklist(struct worklist *pwl)
{
  int i;

  for (i = 0; i < MAX_LEN_WORKLIST && pwl->wlefs[i] != WEF_END; i++) {
    if (pwl->wlefs[i] >= WEF_LAST
	|| (pwl->wlefs[i] == WEF_UNIT && !unit_type_exists(pwl->wlids[i]))
	|| (pwl->wlefs[i] == WEF_IMPR && !improvement_exists(pwl->wlids[i]))) {
      return FALSE;
    }
  }
  return TRUE;
}

/****************************************************************
 If is_city_event is FALSE this event doesn't effect a city even if
 there is a city at the event location.
*****************************************************************/
bool is_city_event(enum event_type event)
{
  switch (event) {
  case E_GLOBAL_ECO:
  case E_CITY_LOST:
  case E_UNIT_LOST:
  case E_UNIT_WIN:
  case E_ENEMY_DIPLOMAT_FAILED:
  case E_ENEMY_DIPLOMAT_EMBASSY:
  case E_ENEMY_DIPLOMAT_POISON:
  case E_ENEMY_DIPLOMAT_BRIBE:
  case E_ENEMY_DIPLOMAT_INCITE:
  case E_ENEMY_DIPLOMAT_SABOTAGE:
  case E_ENEMY_DIPLOMAT_THEFT:
  case E_MY_DIPLOMAT_FAILED:
  case E_MY_DIPLOMAT_EMBASSY:
  case E_MY_DIPLOMAT_POISON:
  case E_MY_DIPLOMAT_BRIBE:
  case E_MY_DIPLOMAT_INCITE:
  case E_MY_DIPLOMAT_SABOTAGE:
  case E_MY_DIPLOMAT_THEFT:
  case E_MY_DIPLOMAT_ESCAPE:
  case E_UNIT_LOST_ATT:
  case E_UNIT_WIN_ATT:
  case E_UPRISING:
  case E_UNIT_RELOCATED:
    return FALSE;

  default:
    return TRUE;
  }
}

/****************************************************************
  ...
*****************************************************************/
const char *get_option_category_name(enum client_option_category category)
{
  assert(category >= 0 && category < COC_NUM);
  return _(category_name[category]);
}
