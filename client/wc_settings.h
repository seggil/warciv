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
#ifndef FC__PEPTOOL_H
#define FC__PEPTOOL_H

#include "city.h"
#include "map.h"
#include "unit.h"

/********************************************************************** 
  Warclient settings...
***********************************************************************/
#define PEPSETTINGS_FILE_NAME ".pepsettings"

enum my_ai_level {
  LEVEL_OFF = 0,
  LEVEL_ON,
  LEVEL_GOOD,
  LEVEL_BEST /* may be slower */
};

enum peppage {
  PAGE_PMAIN = 0,
  PAGE_MS,
  PAGE_DG,
  PAGE_TRADE,
  PAGE_NUM
};

enum peptype {
  TYPE_BOOL,
  TYPE_INT,
  TYPE_FILTER,
  TYPE_END
};

struct pepsetting {
  enum peppage page;
  const char *name;
  const char *description;
  const char *help_text;
  enum peptype type;
  void *data;
  long def,min,max;
};

extern struct pepsetting *pepsettings;

extern bool auto_load;
extern bool save_pepsettings_on_exit;
extern bool my_ai_enable;

extern bool multi_select_count_all;
extern bool multi_select_blink_all;
extern bool multi_select_blink;
extern bool multi_select_map_selection;
extern bool spread_airport_cities;
extern bool spread_allied_cities;

extern enum my_ai_level my_ai_trade_level;
extern int my_ai_establish_trade_route_level;
extern bool my_ai_trade_external;
extern bool my_ai_trade_manual_trade_route_enable;
extern int my_ai_trade_plan_time_max;
extern bool my_ai_trade_plan_recalculate_auto;
extern bool my_ai_trade_plan_change_homecity;

#define DEFAULT_LOAD_PEPSETTINGS_MODE 7
#define MIN_LOAD_PEPSETTINGS_MODE 0
#define MAX_LOAD_PEPSETTINGS_MODE 7

#define DEFAULT_SAVE_TURNS 3
#define MIN_SAVE_TURNS 0
#define MAX_SAVE_TURNS 10

#define DEFAULT_SAVE_ON_EXIT TRUE

#define DEFAULT_ERRORS_MAX 20
#define MIN_ERRORS_MAX 0
#define MAX_ERRORS_MAX 65535

#define DEFAULT_MY_AI_ENABLE TRUE

#define DEFAULT_AUTOWAKEUP TRUE

#define DEFAULT_MOVEATTACK FALSE

#define DEFAULT_CARAVAN_ACTION 0
#define MIN_CARAVAN_ACTION 0
#define MAX_CARAVAN_ACTION 3

#define DEFAULT_DIPLOMAT_UNIT_ACTION DDUA_POPUP_DIALOG
#define MIN_DIPLOMAT_UNIT_ACTION DDUA_POPUP_DIALOG
#define MAX_DIPLOMAT_UNIT_ACTION DDUA_KEEP_MOVING

#define DEFAULT_DIPLOMAT_CITY_ACTION DDCA_POPUP_DIALOG
#define MIN_DIPLOMAT_CITY_ACTION DDCA_POPUP_DIALOG
#define MAX_DIPLOMAT_CITY_ACTION DDCA_KEEP_MOVING

#define DEFAULT_DIPLOMAT_IGNORE_ALLIES TRUE

#define DEFAULT_UNIT_ACTION ACTION_IDLE
#define MIN_UNIT_ACTION ACTION_IDLE
#define MAX_UNIT_ACTION ACTION_FORTIFY_OR_SLEEP

#define DEFAULT_ACTION_LOCKED FALSE

#define DEFAULT_ACTION_MILITARY TRUE

#define DEFAULT_MS_PLACE PLACE_ON_CONTINENT
#define MIN_MS_PLACE PLACE_SINGLE_UNIT
#define MAX_MS_PLACE PLACE_EVERY_WHERE

#define DEFAULT_MS_UTYPE UTYPE_SAME_TYPE
#define MIN_MS_UTYPE UTYPE_SAME_TYPE
#define MAX_MS_UTYPE UTYPE_ALL

#define DEFAULT_MS_COUNT_ALL FALSE

#define DEFAULT_MS_BLINK_ALL FALSE

#define DEFAULT_MS_BLINK TRUE

#define DEFAULT_MS_MAP FALSE

#define DEFAULT_SPREAD_AIRPORT FALSE

#define DEFAULT_SPREAD_ALLY FALSE

#define DEFAULT_MS_INCLUSIVE FILTER_ABLE_TO_MOVE | FILTER_IDLE
#define DEFAULT_MS_EXCLUSIVE FILTER_FORTIFIED | FILTER_SENTRIED | FILTER_AUTO

#define DEFAULT_DG_LIMIT 0
#define MIN_DG_LIMIT 0
#define MAX_DG_LIMIT 255

#define DEFAULT_DG_PLACE PLACE_ON_TILE
#define MIN_DG_PLACE PLACE_SINGLE_UNIT
#define MAX_DG_PLACE PLACE_EVERY_WHERE

#define DEFAULT_DG_UTYPE UTYPE_SAME_TYPE
#define MIN_DG_UTYPE UTYPE_SAME_TYPE
#define MAX_DG_UTYPE UTYPE_ALL

#define DEFAULT_DG_INCLUSIVE FILTER_ALL
#define DEFAULT_DG_EXCLUSIVE FILTER_OFF

#define DEFAULT_MY_AI_TRADE_LEVEL LEVEL_GOOD
#define MIN_MY_AI_TRADE_LEVEL LEVEL_OFF
#define MAX_MY_AI_TRADE_LEVEL LEVEL_BEST

#define DEFAULT_MY_AI_ESTABLISH_LEVEL 1
#define MIN_MY_AI_ESTABLISH_LEVEL 0
#define MAX_MY_AI_ESTABLISH_LEVEL 99

#define DEFAULT_MY_AI_TRADE_MODE 2
#define MIN_MY_AI_TRADE_MODE 0
#define MAX_MY_AI_TRADE_MODE 3

#define DEFAULT_MY_AI_TRADE_MANUAL TRUE

#define DEFAULT_MY_AI_TRADE_EXTERNAL FALSE

#define DEFAULT_MY_AI_TRADE_TIME 10
#define MIN_MY_AI_TRADE_TIME 0
#define MAX_MY_AI_TRADE_TIME 60

#define DEFAULT_MY_AI_TRADE_PLAN_AUTO TRUE

#define DEFAULT_MY_AI_TRADE_PLAN_HOMECITY FALSE

#define PSGEN_BOOL(page, name, desc, help, value) \
  {page, #name, desc, help, TYPE_BOOL, &name, DEFAULT_##value, FALSE, TRUE}
#define PSGEN_INT(page, name, desc, help, value)  \
  {page, #name, desc, help, TYPE_INT, &name,      \
   DEFAULT_##value, MIN_##value, MAX_##value}
#define PSGEN_FILTER(page, name, desc, value)     \
  {page, #name, desc, NULL, TYPE_FILTER, &name,   \
   DEFAULT_##value, 1, FILTER_MAX_VALUE}
#define PSGEN_END {PAGE_NUM, NULL, NULL, NULL, TYPE_END, NULL, 0, 0, 0}

const char *get_page_name(enum peppage page);

#define pepsettings_iterate(pset) { \
  int _i = 0;                       \
  struct pepsetting *pset;          \
  while (TRUE) {                    \
    pset = &pepsettings[_i];        \
    if (pset->type == TYPE_END) {   \
      break;                        \
    }
#define pepsettings_iterate_end	    \
    _i++;                           \
  }	                            \
}

/* load/save settings */
void init_all_settings(void);
void load_static_settings(void);
void load_dynamic_settings(void);
void load_all_settings(void);
void save_all_settings(void);
void autosave_settings(void);

#endif /* FC__PEPTOOL_H */
