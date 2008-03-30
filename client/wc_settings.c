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

#include <string.h>

#include "city.h"
#include "cm.h"
#include "fcintl.h"
#include "game.h"
#include "hash.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "player.h"
#include "registry.h"
#include "support.h"
#include "unit.h"

#include "chatline_g.h"
#include "civclient.h"
#include "clinet.h"
#include "cma_core.h"
#include "control.h"
#include "mapview_g.h"
#include "multiselect.h"
#include "myai.h"
#include "wc_settings.h"

/********************************************************************** 
  Warclient settings...
***********************************************************************/
const char *pepsettings_file_name(void);

static const char *peppagenames[PAGE_NUM] = {
  N_("Main"),
  N_("Multi-Selection"),
  N_("Delayed Goto"),
  N_("Trade"),
};

static int turns = 0;
static int load_pepsettings_mode;
static int save_turns;
static int errors_max;

bool save_pepsettings_on_exit;
bool my_ai_enable;

bool multi_select_count_all;
bool multi_select_blink_all;
bool multi_select_blink;
bool multi_select_map_selection;
bool spread_airport_cities;
bool spread_allied_cities;

enum my_ai_level my_ai_trade_level;
int my_ai_establish_trade_route_level;
bool my_ai_trade_manual_trade_route_enable;
bool my_ai_trade_external;
int my_ai_trade_plan_level;
int my_ai_trade_plan_time_max;
bool my_ai_trade_plan_enable_free_slots;
bool my_ai_trade_plan_recalculate_auto;
bool my_ai_trade_plan_change_homecity;

/********************************************************************** 
  Settings definitions.
***********************************************************************/
static struct pepsetting static_pepsettings[] = {
  PSGEN_INT(PAGE_PMAIN, load_pepsettings_mode,
            N_("Warclient setting load mode"),
            N_("0: Off\n"
               "1: Only static settings\n"
               "2: Only automatic execution values\n"
               "3: Static settings and automatic execution values\n"
               "4: Only dynamic settings\n"
               "5: Dynamic and static settings\n"
               "6: Dynamic settings and automatic execution values\n"
               "7: All"), LOAD_PEPSETTINGS_MODE),
  PSGEN_INT(PAGE_PMAIN, errors_max,
            N_("Maximum warning before aborting load"),
            N_("When the dynamic settings are loaded, it will stop "
               "to load them if more than this value errors are found.\n\n"
               "0: Unlimited"), ERRORS_MAX),
  PSGEN_INT(PAGE_PMAIN, save_turns,
            N_("Save settings all the x turns"),
            N_("0: Disable"), SAVE_TURNS),
  PSGEN_BOOL(PAGE_PMAIN, save_pepsettings_on_exit,
             N_("Save settings on server exit"),
             N_("If this option is enabled, this settings will be saved "
                "when you exit a server."), SAVE_ON_EXIT),
  PSGEN_BOOL(PAGE_PMAIN, my_ai_enable,
             N_("Enable automatics orders"),
             N_("If this option is disabled, you won't be able to use "
                "any automatic orders for manange units."), MY_AI_ENABLE),
  PSGEN_BOOL(PAGE_PMAIN, autowakeup_state,
             N_("Autowakeup sentried units"),
             N_("If disable, sentried units cannot wake up. "
                "All sentried units are considered as sleeping."), AUTOWAKEUP),
  PSGEN_BOOL(PAGE_PMAIN, moveandattack_state,
             N_("Move and attack mode"),
             N_("When this option is enabled, the units will try to "
                "autoattack after every move. Note that will cancel "
                "the path for the next turn."), MOVEATTACK),
  PSGEN_INT(PAGE_PMAIN, default_caravan_action,
            N_("Caravan action upon arrival"),
            N_("The caravans will execute this order when they "
               "will arrive to a destination city.\n\n"
               "0: Popup dialog\n"
               "1: Establish trade route\n"
               "2: Help building wonder\n"
               "3: Keep going"), CARAVAN_ACTION),
  PSGEN_INT(PAGE_PMAIN, default_diplomat_unit_action,
            N_("Diplomat action upon unit arrival"),
            N_("Diplomats execute this order when they "
               "arrive at another player's unit.\n\n"
               "0: Popup dialog\n"
               "1: Bribe unit\n"
               "2: Sabotage unit (spy)\n"
               "3: Keep going"), DIPLOMAT_UNIT_ACTION),
  PSGEN_INT(PAGE_PMAIN, default_diplomat_city_action,
            N_("Diplomat action upon city arrival"),
            N_("Diplomat execute this order when they "
               "arrive at another player's city.\n\n"
               "0: Popup dialog\n"
               "1: Establish embassy\n"
               "2: Investigate city\n"
               "3: Sabotage city\n"
               "4: Steal technology\n"
               "5: Incite revolt\n"
               "6: Poison city (spy)\n"
               "7: Keep going"), DIPLOMAT_CITY_ACTION),
  PSGEN_BOOL(PAGE_PMAIN, default_diplomat_ignore_allies,
             N_("Diplomat ignores allies"),
             N_("If this option is set, diplomats and spies "
                "will ignore all allied units and cities, i.e. "
                "they will pass over them instead of performing "
                "an action."), DIPLOMAT_IGNORE_ALLIES),
  PSGEN_INT(PAGE_PMAIN, default_action_type,
            N_("New unit default action"),
            N_("The new created units will do this activity automaticely.\n\n"
               "0: Idle\n"
               "1: Sentry\n"
               "2: Fortify\n"
               "3: Sleep\n"
               "4: Fortify or Sleep (depend of unit)\n"), UNIT_ACTION),
  PSGEN_BOOL(PAGE_PMAIN, default_action_locked,
             N_("Lock the new unit default action"),
             N_("If this setting is disabled, the new unit default action "
                "will be canceled every turn"), ACTION_LOCKED),
  PSGEN_BOOL(PAGE_PMAIN, default_action_military_only,
             N_("New unit default action for military units only"),
             N_("If enabled, units not able to attack won't do the "
                "default action."), ACTION_MILITARY),

  PSGEN_INT(PAGE_MS, multi_select_place,
            N_("Multi-selection place mode"),
            N_("This option affects where the units will be selected.\n\n"
               "0: Single unit\n"
               "1: All units on the tile\n"
               "2: All units on the continent\n"
               "3: All units"), MS_PLACE),
  PSGEN_INT(PAGE_MS, multi_select_utype,
            N_("Multi-selection unit type mode"),
            N_("This option affects what kinds of units will be selected.\n\n"
               "0: Only units with the same type\n"
               "1: Only units with the same move type\n"
               "2: All unit types"), MS_UTYPE),
  PSGEN_BOOL(PAGE_MS, multi_select_count_all,
             N_("Count all units in the selection"),
             N_("If this option is enabled, the unit count (in the unit label) "
                "will include excluded units (by filters)."),MS_COUNT_ALL),
  PSGEN_BOOL(PAGE_MS, multi_select_blink_all,
             N_("Blink all units in the selection"),
             N_("If this option is enabled, all selected units will blink, "
                "including excluded units (by filters)."), MS_BLINK_ALL),
  PSGEN_BOOL(PAGE_MS, multi_select_blink,
             N_("Blink units in the selection"),
             N_("If this option is disabled, only the first unit wil blink."),
             MS_BLINK),
  PSGEN_BOOL(PAGE_MS, multi_select_map_selection,
             N_("Allow multi-selection at map"),
             N_("If this option is enabled, you will be able to select units "
                "with the yellow rectangle (Dragging right click)."), MS_MAP),
  PSGEN_BOOL(PAGE_MS, spread_airport_cities,
             N_("Spread only in cities with airport"),
             N_("If this option is enabled, spreading units command "
                "will spread selected units only in the cities which "
                "have an aitport."), SPREAD_AIRPORT),
  PSGEN_BOOL(PAGE_MS, spread_allied_cities,
             N_("Allow spreading into allied cities"),
             N_("If this option is enabled, spreading units command "
                "will spread selected units in allies cities or in "
                "your own ciries indifferently"), SPREAD_ALLY),
  PSGEN_FILTER(PAGE_MS, multi_select_inclusive_filter,
               N_("Multi-selection inclusive filter"), MS_INCLUSIVE),
  PSGEN_FILTER(PAGE_MS, multi_select_exclusive_filter,
               N_("Multi-selection exclusive filter"), MS_EXCLUSIVE),

  PSGEN_INT(PAGE_DG, unit_limit,
            N_("Delayed goto unit limit"),
            N_("0: Unlimited units"), DG_LIMIT),
  PSGEN_INT(PAGE_DG, delayed_goto_place,
            N_("Delayed goto place mode"),
            N_("This option affects where the units will be selected.\n\n"
               "0: Single unit\n"
               "1: All units on the tile\n"
               "2: All units on the continent\n"
               "3: All units"), DG_PLACE),
  PSGEN_INT(PAGE_DG, delayed_goto_utype,
            N_("Delayed goto unit type mode"),
            N_("This option affects what kinds of units will be selected.\n\n"
               "0: Only units with the same type\n"
               "1: Only units with the same move type\n"
               "2: All unit types"), DG_UTYPE),
  PSGEN_FILTER(PAGE_DG, delayed_goto_inclusive_filter,
               N_("Delayed goto inclusive filter"), DG_INCLUSIVE),
  PSGEN_FILTER(PAGE_DG, delayed_goto_exclusive_filter,
               N_("Delayed goto exclusive filter"), DG_EXCLUSIVE),

  PSGEN_INT(PAGE_TRADE, my_ai_trade_level,
            N_("Automatic trade order level"),
            N_("This option will determinate how the trade routes "
               "will be chosen.\n\n"
               "0: Off\n"
               "1: Basic check\n"
               "2: Good check, including freeing running caravans\n"
               "3: Best check, can be slower, but should be faster in turns"),
            MY_AI_TRADE_LEVEL),
  PSGEN_INT(PAGE_TRADE, my_ai_establish_trade_route_level,
            N_("Improved trade route establishing level"),
            N_("This option affect how good the tiles will be switched "
               "for cities before a trade route will be established.\n\n"
               "0: Off\n"
               "1: Only in the caravan homecity and the destination city"
               "2 and more: also in cities this previous cities had trade "
               "routes with\n\n"
               "Note that more the value is huge, more it will be slow"),
            MY_AI_ESTABLISH_LEVEL),
  PSGEN_BOOL(PAGE_TRADE, my_ai_trade_manual_trade_route_enable,
             N_("Allow manual trade orders"),
             N_("If this option is enabled, the client will take care about "
               "non-automatic caravans."), MY_AI_TRADE_MANUAL),
  PSGEN_BOOL(PAGE_TRADE, my_ai_trade_external,
             N_("Allow external trade"),
             N_("If this option is enabled, the client will take care about "
                "about non-owned cities to trade with."), MY_AI_TRADE_EXTERNAL),
  PSGEN_INT(PAGE_TRADE, my_ai_trade_plan_time_max,
            N_("Maximum time allowed to trade planning calculation"),
            N_("This value is in seconds.\n\n"
               "0: No limit"), MY_AI_TRADE_TIME),
  PSGEN_BOOL(PAGE_TRADE, my_ai_trade_plan_recalculate_auto,
             N_("Automatic trade planning calcul"),
             N_("If this option is enabled, the trade planning will be "
                "calculated every time a city is added or removed."),
             MY_AI_TRADE_PLAN_AUTO),
  PSGEN_BOOL(PAGE_TRADE, my_ai_trade_plan_change_homecity,
             N_("Change homecity if a trade route is faster"),
             N_("If this option is disabled, the caravan will check the "
                "planned trade route for the caravan's homecity before "
                "checking for other cities trade routes."),
             MY_AI_TRADE_PLAN_HOMECITY),

  PSGEN_END
};

struct pepsetting *pepsettings = static_pepsettings;

/********************************************************************** 
  Accessor and translator function.
***********************************************************************/
const char *get_page_name(enum peppage page)
{
  assert(page >= 0 && page < PAGE_NUM);

  return _(peppagenames[page]);
}

/********************************************************************** 
  Init all settings, set them to default.
***********************************************************************/
void init_all_settings(void)
{
  pepsettings_iterate(pset) {
    switch (pset->type) {
      case TYPE_BOOL:
	*((bool *)pset->data) = pset->def;
	break;
      case TYPE_INT:
	*((int *)pset->data) = pset->def;
	break;
      case TYPE_FILTER:
	*((filter *)pset->data) = pset->def;
	break;
      default:
	break;
    }
  } pepsettings_iterate_end;

  automatic_processus_iterate(pap) {
    pap->auto_filter = pap->default_auto_filter;
  } automatic_processus_iterate_end;
}

/********************************************************************** 
  Build the file name.
***********************************************************************/
const char *pepsettings_file_name(void)
{
  static char buf[256];
  char *name;

  name = user_home_dir();
  if (name) {
    my_snprintf(buf, sizeof(buf), "%s/%s", name, PEPSETTINGS_FILE_NAME);
  } else {
    sz_strlcpy(buf, PEPSETTINGS_FILE_NAME);
  }
  return buf;
}

/********************************************************************** 
  Load settings system.

  Note: the following return TRUE when a fatal error is detected.
***********************************************************************/
bool load_city(struct section_file *psf, struct city **ppcity,
	       const char *format, ...)
               fc__attribute((__format__ (__printf__, 3, 4)));
bool load_player(struct section_file *psf, struct player **ppplayer,
		 const char *format, ...)
               fc__attribute((__format__ (__printf__, 3, 4)));
bool load_tile(struct section_file *psf, struct tile **pptile,
	       const char *format, ...)
               fc__attribute((__format__ (__printf__, 3, 4)));
bool load_unit(struct section_file *psf, struct unit **ppunit,
	       const char *format, ...)
               fc__attribute((__format__ (__printf__, 3, 4)));

/********************************************************************** 
  Load a city from the file.
***********************************************************************/
bool load_city(struct section_file *psf, struct city **ppcity,
	       const char *format, ...)
{
  char buf[256];
  int id, x, y;
  va_list args;

  va_start(args, format);
  my_vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  id = secfile_lookup_int_default(psf, -1, "%s.%s", buf, "id");
  x = secfile_lookup_int_default(psf, -1, "%s.%s", buf, "x");
  y = secfile_lookup_int_default(psf, -1, "%s.%s", buf, "y");

  *ppcity = find_city_by_id(id);
  if (*ppcity && ((*ppcity)->tile->x != x || (*ppcity)->tile->y != y)) {
    /* The city is not at the right place */
    return TRUE;
  }
  return FALSE;
}

/********************************************************************** 
  Load a player from the file.
***********************************************************************/
bool load_player(struct section_file *psf, struct player **ppplayer,
		 const char *format, ...)
{
  char buf[256], name[MAX_LEN_NAME];
  int id;
  va_list args;

  va_start(args, format);
  my_vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  id = secfile_lookup_int_default(psf, -1, "%s.%s", buf, "id");
  sz_strlcpy(name,
	     secfile_lookup_str_default(psf, "\0" , "%s.%s", buf , "name"));

  if (id < 0 && strcmp(name, "NULL") == 0) {
    /* This is the NULL player */
    *ppplayer = NULL;
    return FALSE;
  }

  *ppplayer = get_player(id);
  if ((*ppplayer) == NULL || strcmp((*ppplayer)->name, name) != 0) {
    /* There is no player or it's not the right name */
    return TRUE;
  }
  return FALSE;
}

/********************************************************************** 
  Load a tile from the file.
***********************************************************************/
bool load_tile(struct section_file *psf, struct tile **pptile,
	       const char *format, ...)
{
  char buf[256];
  int x, y;
  va_list args;

  va_start(args, format);
  my_vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  x = secfile_lookup_int_default(psf, -1, "%s.%s", buf, "x");
  y = secfile_lookup_int_default(psf, -1, "%s.%s", buf, "y");

  if (!is_normal_map_pos(x, y)) {
    /* Not a right tile in this game */
    return TRUE;
  }

  *pptile = map_pos_to_tile(x, y);
  if ((*pptile) == NULL) {
    /* Shouldn't occur here */
    return TRUE;
  }
  return FALSE;
}

/********************************************************************** 
  Load an unit from the file.
***********************************************************************/
bool load_unit(struct section_file *psf, struct unit **ppunit,
	       const char *format, ...)
{
  char buf[256];
  int id, type;
  va_list args;

  va_start(args, format);
  my_vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  id = secfile_lookup_int_default(psf, -1, "%s.%s", buf, "id");
  type = secfile_lookup_int_default(psf, -1, "%s.%s", buf, "type");

  *ppunit = find_unit_by_id(id);
  if (*ppunit && (*ppunit)->type != type) {
    /* Not the right type */
    return TRUE;
  }
  return FALSE;
}

/********************************************************************** 
  Load the client options for Warclient.
***********************************************************************/
static void base_load_static_settings(struct section_file *psf)
{
  char buf[256];
  int i, num;

  /* Static settings */
  if (load_pepsettings_mode & 1) {
    freelog(LOG_DEBUG, "Loading static settings");

    pepsettings_iterate(pset) {
      switch (pset->type) {
	case TYPE_BOOL:
	  *((bool *)pset->data) =
	    secfile_lookup_bool_default(psf, *((bool *)pset->data),
					"static.%s", pset->name);
	  break;
	case TYPE_INT:
	  *((int *)pset->data) =
	    secfile_lookup_int_default(psf, *((int *)pset->data),
				       "static.%s", pset->name);
	  break;
	case TYPE_FILTER:
	  *((filter *)pset->data) =
	    secfile_lookup_int_default(psf, *((filter *)pset->data),
				       "static.%s", pset->name);
	  break;
	default:
	  break;
      }
      if (!(load_pepsettings_mode & 1)) {
	/* Maybe we shouldn't loaf this! */
        break;
      }
    } pepsettings_iterate_end;
  }

  /* Automatic processus (see multiselect.h) */
  if (load_pepsettings_mode & 2) {
    freelog(LOG_DEBUG, "Loading automatic processus");
    automatic_processus *pap;
    num = secfile_lookup_int_default(psf, -1, "automatic_processus.num");
    for (i = 0; i < num; i++) {
      sz_strlcpy(buf, secfile_lookup_str_default(psf, "\0",
			"automatic_processus.ap%d.name", i));
      if ((pap = find_automatic_processus_by_name(buf))) {
	pap->auto_filter =
	  secfile_lookup_int_default(psf, pap->auto_filter,
				     "automatic_processus.ap%d.filter", i);
	auto_filter_normalize(pap);
      } else {
	freelog(LOG_ERROR, "No automatic_processus named '%s'", buf);
      }
    }
  }
}

/********************************************************************** 
  Accessor macros.
***********************************************************************/
#define load(type, data, ...)						       \
  freelog(LOG_DEBUG, "Loading one %s at %s, line %d",			       \
          #type, __FILE__, __LINE__);					       \
  struct type *data;							       \
  if (load_##type(psf, &data, __VA_ARGS__)) {				       \
    freelog(LOG_NORMAL, _("Cannot load dynamics settings "		       \
			  "(%s: line %d, wrong %s), aborting..."),	       \
	    __FILE__, __LINE__, #type);					       \
    goto free_datas;							       \
  }									       \
  if (!(data)) {							       \
    freelog(LOG_VERBOSE, "Cannot load the %s at %s, line %d",		       \
            #type, __FILE__, __LINE__);					       \
    if (errors_max > 0 && ++error_count >= errors_max) {		       \
      freelog(LOG_NORMAL, _("Cannot load dynamics settings, too much warning " \
			    "(%d/%d), aborting..."), error_count, errors_max); \
      goto free_datas;							       \
    }									       \
  } else

#define load_owner(type, data, ...)					    \
  load(type, data, __VA_ARGS__) if (data->owner!=get_player_idx()) {	    \
    freelog(LOG_VERBOSE, "Cannot load the %s at %s, line %d (wrong owner)", \
            #type, __FILE__,__LINE__);					    \
    data = NULL;							    \
  }	  								    \
  else

/********************************************************************** 
  Load the game queues for Warclient.
***********************************************************************/
static void base_load_dynamic_settings(struct section_file *psf)
{
  int i, j, num, error_count = 0;

  turns = 0;

  /* Dynamic settings */
  if (load_pepsettings_mode & 4) {
    /* First, let's so a compatibity test between
     * the current game and the saved game. */
    bool compatible = TRUE;

    i = secfile_lookup_int_default(psf, -1, "game_info.xsize");
    if (i != -1 && i != map.xsize) {
      compatible = FALSE;
    }
    i = secfile_lookup_int_default(psf, -1, "game_info.ysize");
    if (i != -1 && i != map.ysize) {
      compatible = FALSE;
    }
    i = secfile_lookup_int_default(psf, -1, "game_info.topology_id");
    if (i != -1 && i != map.topology_id) {
      compatible = FALSE;
    }
    num = secfile_lookup_int_default(psf, -1, "game_info.nplayers");
    if (num != -1 && num != game.info.nplayers) {
      compatible = FALSE;
    } else {
      for (i = 0; i < num; i++) {
	struct player *pplayer;

	if (load_player(psf, &pplayer, "game_info.player%d", i)) {
	  compatible = FALSE;
	  break;
	}
      }
    }

    if (!compatible) {
      freelog(LOG_NORMAL, 
	      _("Dynamic settings were saved for an other "
		"game, they cannot be loaded"));
      goto end;
    }

    /* It seems this game look like the saved one */
    freelog(LOG_DEBUG, "Loading dynamic settings");

    /* Initialize */
    struct multi_select tmultiselect[MULTI_SELECT_NUM];
    struct delayed_goto tdelayedgoto[DELAYED_GOTO_NUM];
    struct airlift_queue tairliftqueue[AIRLIFT_QUEUE_NUM];
    struct trade_route_list *ttraders, *ttradeplan;
    struct city_list *trallypoint, *ttradecities;
    struct unit_list *tpatrolers;

    trallypoint = city_list_new();
    for (i = 0; i < MULTI_SELECT_NUM; i++) {
      tmultiselect[i].ulist = unit_list_new();
      tmultiselect[i].punit_focus = NULL;
    }
    for (i = 0; i <DELAYED_GOTO_NUM; i++) {
      tdelayedgoto[i].dglist = delayed_goto_data_list_new();
      tdelayedgoto[i].pplayer = NULL;
    }
    for (i = 0; i < AIRLIFT_QUEUE_NUM; i++) {
      tairliftqueue[i].tlist = tile_list_new();
      tairliftqueue[i].utype = U_LAST;
    }
    ttraders = trade_route_list_new();
    ttradeplan = trade_route_list_new();
    ttradecities = city_list_new();
    tpatrolers = unit_list_new();

    /* Load the rally points */
    num = secfile_lookup_int_default(psf, -1, "dynamic.rally.city_num");
    for (i = 0; i < num; i++) {
      load_owner(city, pcity, "dynamic.rally.city%d", i) {
	load(tile, ptile, "dynamic.rally.city%d.tile", i) {
	  struct city *ccity = fc_malloc(sizeof(struct city));
	  *ccity = *pcity;
	  ccity->rally_point = ptile;
	  city_list_append(trallypoint, ccity);
	}
      }
    }

    /* Load the unit selections */
    for (i = 0; i < MULTI_SELECT_NUM; i++) {
      num = secfile_lookup_int_default(psf, 0,
				       "dynamic.multiselect%ds.unit_num", i);
      for (j = 0; j < num; j++) {
	load_owner(unit, punit, "dynamic.multiselect%ds.unit%d", i, j) {
	  unit_list_append(tmultiselect[i].ulist, punit);
	}
      }
      if (num > 0) {
	load_owner(unit, punit, "dynamic.multiselect%ds.unit_focus", i);
	tmultiselect[i].punit_focus = punit;
      }
    }

    /* Load the delayed goto queues */
    for (i = 0; i < DELAYED_GOTO_NUM; i++) {
      num = secfile_lookup_int_default(psf, 0,
				       "dynamic.delayedgoto%ds.data_num", i);
      for (j = 0; j < num; j++) {
	struct delayed_goto_data *pdgd =
	  fc_malloc(sizeof(struct delayed_goto_data));
	pdgd->id = secfile_lookup_int_default(psf, 0,
		     "dynamic.delayedgoto%ds.data%d.id", i, j);
	pdgd->type = secfile_lookup_int_default(psf, 0,
		       "dynamic.delayedgoto%ds.data%d.type", i, j);
	/* Here, we ignore the NULL tile */
	load_tile(psf, &pdgd->ptile,
		  "dynamic.delayedgoto%ds.data%d.tile", i, j);
	delayed_goto_data_list_append(tdelayedgoto[i].dglist, pdgd);
      }
      if (num > 0) {
	load(player, pplayer, "dynamic.delayedgoto%ds.player", i);
	tdelayedgoto[i].pplayer = pplayer;
      }
    }

    /* Load the airlift queues */
    for (i = 0; i < AIRLIFT_QUEUE_NUM; i++) {
      num = secfile_lookup_int_default(psf, 0,
				       "dynamic.airliftqueue%ds.tile_num", i);
      for (j = 0; j < num; j++) {
	load(tile, ptile, "dynamic.airliftqueue%ds.tile%d", i, j) {
	  tile_list_append(tairliftqueue[i].tlist, ptile);
	}
      }
      tairliftqueue[i].utype = secfile_lookup_int_default(psf, U_LAST,
				 "dynamic.airliftqueue%ds.unit_type", i);
    }

    /* Load the trade planning */
    num = secfile_lookup_int_default(psf, -1, "dynamic.trade_route.unit_num");
    for (i = 0; i < num; i++) {
      load_owner(unit, punit, "dynamic.trade_route.unit%d", i) {
	load_owner(city, pc1, "dynamic.trade_route.unit%d.city1", i) {
	  load(city, pc2, "dynamic.trade_route.unit%d.city2", i) {
	    /* Doesn't need to be owned... */
	    trade_route_list_append(ttraders,
	                            trade_route_new(punit, pc1, pc2,
	                            secfile_lookup_bool_default(psf, FALSE,
	                            "dynamic.trade_route.unit%d.planned", i)));
	  }
	}
      }
    }
    num = secfile_lookup_int_default(psf, -1, "dynamic.trade_cities.city_num");
    for (i = 0; i < num; i++) {
      load_owner(city, pcity, "dynamic.trade_cities.city%d", i)  {
	city_list_append(ttradecities, pcity);
      }
    }
    num = secfile_lookup_int_default(psf, -1, "dynamic.trade_plan.tr_num");
    for (i = 0; i < num; i++) {
      load_owner(city, pc1, "dynamic.trade_plan.tr%d.city1", i) {
	load_owner(city, pc2, "dynamic.trade_plan.tr%d.city2", i) {
	  trade_route_list_append(ttradeplan, trade_route_new(NULL, pc1,
							       pc2, TRUE));
	}
      }
    }

    /* Load patrolling units */
    num = secfile_lookup_int_default(psf, -1, "dynamic.patrol.unit_num");
    for (i = 0; i < num; i++) {
      load_owner(unit, punit, "dynamic.patrol.unit%d", i) {
	load(tile, ptile, "dynamic.patrol.unit%d.tile", i) {
	  struct unit *cunit = fc_malloc(sizeof(struct unit));
	  *cunit = *punit;
	  cunit->my_ai.data = (void *)ptile;
	  unit_list_append(tpatrolers, cunit);
	}
      }
    }

    /* Load CMA and apply it... */
    num = secfile_lookup_int_default(psf, -1, "dynamic.cma.city_num");
    for (i = 0; i < num; i++) {
      load_owner(city, pcity, "dynamic.cma.city%d", i) {
        struct cm_parameter parameter;

        for (j = 0; j < NUM_STATS; j++) {
	  parameter.minimal_surplus[j] =
	    secfile_lookup_int_default(psf, 0,
				       "dynamic.cma.city%d.minimal_surplus%d",
				       i, j);
	  parameter.factor[j] =
	    secfile_lookup_int_default(psf, 1, "dynamic.cma.city%d.factor%d",
				       i, j);
        }
	parameter.require_happy = secfile_lookup_bool_default(psf, FALSE,
				    "dynamic.cma.city%d.require_happy", i);
	parameter.allow_disorder = secfile_lookup_bool_default(psf, FALSE,
				     "dynamic.cma.city%d.allow_disorder", i);
	parameter.allow_specialists = secfile_lookup_bool_default(psf, TRUE,
				     "dynamic.cma.city%d.allow_specialists", i);
	parameter.happy_factor = secfile_lookup_int_default(psf, 1,
				   "dynamic.cma.city%d.happy_factor", i);
	cma_put_city_under_agent(pcity, &parameter);
      }
    }

    /* Ok, all looks fine, ready to apply... */
    freelog(LOG_DEBUG, "Apply dynamic settings");
    city_list_iterate(trallypoint, ccity) {
      city_set_rally_point(player_find_city_by_id(get_player_ptr(),ccity->id),
			   ccity->rally_point);
    } city_list_iterate_end;
    for (i = 0; i < MULTI_SELECT_NUM; i++) {
      multi_select_set(i, &tmultiselect[i]);
    }
    for (i = 0; i < DELAYED_GOTO_NUM; i++) {
      delayed_goto_set(i, &tdelayedgoto[i]);
    }
    for (i = 0; i <AIRLIFT_QUEUE_NUM; i++) {
      airlift_queue_set(i, &tairliftqueue[i]);
    }
    city_list_iterate(ttradecities, pcity) {
      my_ai_add_trade_city(pcity, TRUE);
    } city_list_iterate_end;
    /*
     * N.B.: my_ai_orders_free() could add some trade routes
     * in the trade planning if the routes were planned. That's why, the 
     * trade planning is overwritten AFTER with set_trade_planning().
     */
    trade_route_list_iterate(ttraders, ptr) {
      my_ai_orders_free(ptr->punit);
      my_ai_trade_route_alloc(ptr);
    } trade_route_list_iterate_end;
    set_trade_planning(ttradeplan);
    unit_list_iterate(tpatrolers, cunit) {
      struct unit *punit = player_find_unit_by_id(get_player_ptr(), cunit->id);
      my_ai_orders_free(punit);
      my_ai_patrol_alloc(punit, (struct tile *)cunit->my_ai.data);
    } unit_list_iterate_end;
    update_unit_info_label(get_unit_in_focus());

free_datas:
    /* Free datas */
    city_list_iterate(trallypoint, pcity) {
      free(pcity);
    } city_list_iterate_end;
    city_list_free(trallypoint);
    for (i = 0; i < MULTI_SELECT_NUM; i++) {
      unit_list_free(tmultiselect[i].ulist);
    }
    for (i = 0; i < DELAYED_GOTO_NUM; i++) {
      delayed_goto_data_list_iterate(tdelayedgoto[i].dglist, pdg) {
	free(pdg);
      } delayed_goto_data_list_iterate_end;
      delayed_goto_data_list_free(tdelayedgoto[i].dglist);
    }
    for (i = 0; i < AIRLIFT_QUEUE_NUM; i++) {
      tile_list_free(tairliftqueue[i].tlist);
    }
    trade_route_list_iterate(ttraders, ptr) {
      free(ptr);
    } trade_route_list_iterate_end;
    trade_route_list_free(ttraders);
    city_list_free(ttradecities);
    unit_list_iterate(tpatrolers, punit) {
      free(punit);
    } unit_list_iterate_end;
    unit_list_free(tpatrolers);
  }

end:
  append_output_window(_("Warclient: Settings loaded"));
}

/********************************************************************** 
  Open the file
***********************************************************************/
static struct section_file *open_pepsettings_file(void)
{
  const char *name = pepsettings_file_name();
  char buf[256];
  static struct section_file sf;

  if (!section_file_load(&sf, name)) {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Cannot load setting file %s"), name);
    append_output_window(buf);
    return NULL;
  }

  return &sf;
}

/********************************************************************** 
  ...
***********************************************************************/
void load_static_settings(void)
{
  struct section_file *psf = open_pepsettings_file();

  if (psf) {
    base_load_static_settings(psf);
    section_file_free(psf);
  }
}

/********************************************************************** 
  ...
***********************************************************************/
void load_dynamic_settings(void)
{
  struct section_file *psf = open_pepsettings_file();

  if (psf) {
    base_load_dynamic_settings(psf);
    section_file_free(psf);
  }
}

/********************************************************************** 
  ...
***********************************************************************/
void load_all_settings(void)
{
  struct section_file *psf = open_pepsettings_file();

  if (psf) {
    base_load_static_settings(psf);
    base_load_dynamic_settings(psf);
    section_file_free(psf);
  }
}

/********************************************************************** 
  Save settings system.
***********************************************************************/
void save_city(struct section_file *psf, struct city *pcity,
	       const char *format, ...)
               fc__attribute((__format__ (__printf__, 3, 4)));
void save_player(struct section_file *psf, struct player *pplayer,
	       const char *format, ...)
               fc__attribute((__format__ (__printf__, 3, 4)));
void save_tile(struct section_file *psf, struct tile *ptile,
	       const char *format, ...)
               fc__attribute((__format__ (__printf__, 3, 4)));
void save_unit(struct section_file *psf, struct unit *punit,
	       const char *format, ...)
               fc__attribute((__format__ (__printf__, 3, 4)));

/********************************************************************** 
  Save a city.
***********************************************************************/
void save_city(struct section_file *psf, struct city *pcity,
	       const char *format, ...)
{
  char buf[256];
  va_list args;

  va_start(args, format);
  my_vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  secfile_insert_int(psf, pcity ? pcity->id : -1, "%s.%s", buf, "id");
  secfile_insert_int(psf, pcity ? pcity->tile->x : -1, "%s.%s", buf, "x");
  secfile_insert_int(psf, pcity ? pcity->tile->y : -1, "%s.%s", buf, "y");
}

/********************************************************************** 
  Save a player.
***********************************************************************/
void save_player(struct section_file *psf, struct player *pplayer,
	       const char *format, ...)
{
  char buf[256];
  va_list args;

  va_start(args, format);
  my_vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  secfile_insert_int(psf, pplayer ? pplayer->player_no : -1,
		     "%s.%s", buf, "id");
  secfile_insert_str(psf, pplayer ? pplayer->name : "NULL",
		     "%s.%s" , buf, "name");
}

/********************************************************************** 
  Save a tile.
***********************************************************************/
void save_tile(struct section_file *psf, struct tile *ptile,
	       const char *format, ...)
{
  char buf[256];
  va_list args;

  va_start(args, format);
  my_vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  secfile_insert_int(psf, ptile ? ptile->x : -1, "%s.%s", buf, "x");
  secfile_insert_int(psf, ptile ? ptile->y : -1, "%s.%s", buf, "y");
}

/********************************************************************** 
  Save an unit.
***********************************************************************/
void save_unit(struct section_file *psf, struct unit *punit,
	       const char *format, ...)
{
  char buf[256];
  va_list args;

  va_start(args, format);
  my_vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  secfile_insert_int(psf, punit ? punit->id : -1, "%s.%s", buf, "id");
  secfile_insert_int(psf, punit ? punit->type : -1, "%s.%s", buf, "type");
}

/********************************************************************** 
  ...
***********************************************************************/
void save_all_settings(void)
{
  struct section_file sf;
  char buf[256];
  const char *name = pepsettings_file_name();
  int i, j;

  section_file_init(&sf);
	
  /* Static settings */
  pepsettings_iterate(pset) {
    switch (pset->type) {
      case TYPE_BOOL:
	secfile_insert_bool(&sf, *((bool*)pset->data), "static.%s", pset->name);
	break;
      case TYPE_INT:
	secfile_insert_int(&sf, *((int*)pset->data), "static.%s", pset->name);
	break;
      case TYPE_FILTER:
	secfile_insert_int(&sf, (int)*((filter*)pset->data),
			   "static.%s", pset->name);
	break;
      default:
	break;
    }
  } pepsettings_iterate_end;

  /* Automatic processus */
  i = 0;
  automatic_processus_iterate(pap) {
    const char *name = pap->description;

    if (name[0] == '\0' ) {
      continue;
    }

    secfile_insert_str(&sf, name, "automatic_processus.ap%d.name", i);
    secfile_insert_int(&sf, pap->auto_filter,
		       "automatic_processus.ap%d.filter", i);
    i++;
  } automatic_processus_iterate_end;
  secfile_insert_int_comment(&sf, i, _("don't modify this!"),
			     "automatic_processus.num");

  /* Dynamic settings */
  /* Some game infos for the main compatibity test */
  secfile_insert_int(&sf, map.xsize, "game_info.xsize");
  secfile_insert_int(&sf, map.ysize, "game_info.ysize");
  secfile_insert_int(&sf, map.topology_id, "game_info.topology_id");
  secfile_insert_int(&sf, game.info.nplayers, "game_info.nplayers");
  i = 0;
  players_iterate(pplayer) {
    save_player(&sf, pplayer, "game_info.player%d", i);
    i++;
  } players_iterate_end;

  /* Rally points */
  i = 0;
  city_list_iterate(get_player_ptr()->cities, pcity) {
    if (!pcity->rally_point) {
      continue;
    }
    save_city(&sf, pcity, "dynamic.rally.city%d", i);
    save_tile(&sf, pcity->rally_point, "dynamic.rally.city%d.tile", i);
    i++;
  } city_list_iterate_end;
  secfile_insert_int_comment(&sf, i, _("don't modify this!"),
			     "dynamic.rally.city_num");

  /* Units selections */
  const struct multi_select *pms;
  for (i = 0; i < MULTI_SELECT_NUM; i++) {
    pms = multi_select_get(i);
    secfile_insert_int_comment(&sf, unit_list_size(pms->ulist),
			       _("don't modify this!"),
			       "dynamic.multiselect%ds.unit_num", i);
    j = 0;
    unit_list_iterate(pms->ulist, punit) {
      save_unit(&sf, punit, "dynamic.multiselect%ds.unit%d", i, j);
      j++;
    } unit_list_iterate_end;
    if (j > 0) {
      save_unit(&sf, pms->punit_focus, "dynamic.multiselect%ds.unit_focus", i);
    }
  }

  /* Delayed goto queues */
  const struct delayed_goto *pdg;
  for (i = 0; i < DELAYED_GOTO_NUM; i++) {
    pdg = delayed_goto_get(i);
    secfile_insert_int_comment(&sf, delayed_goto_data_list_size(pdg->dglist),
			       _("don't modify this!"),
			       "dynamic.delayedgoto%ds.data_num", i);
    j = 0;
    delayed_goto_data_list_iterate(pdg->dglist, pdgd) {
      secfile_insert_int(&sf, pdgd->id,
			 "dynamic.delayedgoto%ds.data%d.id", i, j);
      secfile_insert_int(&sf, pdgd->type,
			 "tdynamic.delayedgoto%ds.data%d.ype", i, j);
      save_tile(&sf, pdgd->ptile, "dynamic.delayedgoto%ds.data%d.tile", i, j);
      j++;
    } delayed_goto_data_list_iterate_end;
    if (j > 0) {
      save_player(&sf, pdg->pplayer, "dynamic.delayedgoto%ds.player", i);
    }
  }

  /* Airlift queues */
  const struct airlift_queue *paq;
  for (i = 0; i < AIRLIFT_QUEUE_NUM; i++) {
    paq = airlift_queue_get(i);
    secfile_insert_int_comment(&sf, tile_list_size(paq->tlist),
			       _("don't modify this!"),
			       "dynamic.airliftqueue%ds.tile_num", i);
    j = 0;
    tile_list_iterate(paq->tlist, ptile) {
      save_tile(&sf, ptile, "dynamic.airliftqueue%ds.tile%d", i, j);
      j++;
    } tile_list_iterate_end;
    secfile_insert_int(&sf, airlift_queue_get_unit_type(i),
		       "dynamic.airliftqueue%ds.unit_type", i);
  }

  /* Trade planning */
  const struct unit_list *pul = my_ai_get_units(MY_AI_TRADE_ROUTE);
  secfile_insert_int_comment(&sf, unit_list_size(pul), _("don't modify this!"),
			     "dynamic.trade_route.unit_num");
  i = 0;
  unit_list_iterate(pul, punit) {
    struct trade_route *ptr = (struct trade_route *)punit->my_ai.data;
    save_unit(&sf, punit, "dynamic.trade_route.unit%d", i);
    save_city(&sf, ptr->pc1, "dynamic.trade_route.unit%d.city1", i);
    save_city(&sf, ptr->pc2, "dynamic.trade_route.unit%d.city2", i);
    secfile_insert_bool(&sf, ptr->planned,
			"dynamic.trade_route.unit%d.planned", i);
    i++;
  } unit_list_iterate_end;
  const struct city_list *ptcl = my_ai_get_trade_cities();
  secfile_insert_int_comment(&sf, city_list_size(ptcl), _("don't modify this!"),
			     "dynamic.trade_cities.city_num");
  i = 0;
  city_list_iterate(ptcl, pcity) {
    save_city(&sf, pcity, "dynamic.trade_cities.city%d", i);
    i++;
  } city_list_iterate_end;
  const struct trade_route_list *ptrl = my_ai_trade_plan_get();
  secfile_insert_int_comment(&sf, trade_route_list_size(ptrl),
			     _("don't modify this!"),
			     "dynamic.trade_plan.tr_num");
  i = 0;
  trade_route_list_iterate(ptrl, ptr) {
    save_city(&sf, ptr->pc1, "dynamic.trade_plan.tr%d.city1", i);
    save_city(&sf, ptr->pc2, "dynamic.trade_plan.tr%d.city2", i);
    i++;
  } trade_route_list_iterate_end;

  /* Patrolling units */
  pul = my_ai_get_units(MY_AI_PATROL);
  secfile_insert_int_comment(&sf, unit_list_size(pul), _("don't modify this!"),
			     "dynamic.patrol.unit_num");
  i = 0;
  unit_list_iterate(pul, punit) {
    save_unit(&sf, punit, "dynamic.patrol.unit%d", i);
    save_tile(&sf, (struct tile *)punit->my_ai.data,
	      "dynamic.patrol.unit%d.tile", i);
    i++;
  } unit_list_iterate_end;

  /* CMA */
  struct cm_parameter parameter;

  i = 0;
  city_list_iterate(get_player_ptr()->cities, pcity) {
    if (!cma_is_city_under_agent(pcity,&parameter)) {
      continue;
    }
    save_city(&sf, pcity, "dynamic.cma.city%d", i);
    for (j = 0; j < NUM_STATS; j++) {
      secfile_insert_int(&sf, parameter.minimal_surplus[j],
			 "dynamic.cma.city%d.minimal_surplus%d", i, j);
      secfile_insert_int(&sf, parameter.factor[j],
			 "dynamic.cma.city%d.factor%d", i, j);
    }
    secfile_insert_bool(&sf, parameter.require_happy,
			"dynamic.cma.city%d.require_happy", i);
    secfile_insert_bool(&sf, parameter.allow_disorder,
			"dynamic.cma.city%d.allow_disorder", i);
    secfile_insert_bool(&sf, parameter.allow_specialists,
			"dynamic.cma.city%d.allow_specialists", i);
    secfile_insert_int(&sf, parameter.happy_factor,
		       "dynamic.cma.city%d.happy_factor", i);
    i++;
  } city_list_iterate_end;
  secfile_insert_int_comment(&sf, i, _("don't modify this!"),
			     "dynamic.cma.city_num");

  /* Save to disk */
  if (!section_file_save(&sf, name, 0)) {
    my_snprintf(buf, sizeof(buf),
		_("Warclient: Save failed, cannot write to file %s"), name);
  } else {
    my_snprintf(buf, sizeof(buf),
		_("Warclient: Saved settings to file %s") ,name);
  }

  append_output_window(buf);
  section_file_free(&sf);
}

/********************************************************************** 
  ... Called every turn.
***********************************************************************/
void autosave_settings(void)
{
  if (save_turns == 0 || client_is_observer() || !get_player_ptr()) {
    return;
  }

  if (++turns >= save_turns) {
    turns = 0;
    save_all_settings();
  }
}
