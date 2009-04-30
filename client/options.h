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
#ifndef FC__OPTIONS_H
#define FC__OPTIONS_H

#include "events.h"
#include "registry.h"
#include "shared.h"		/* bool type */
#include "traderoute.h"		/* ASYNC_TRADE_PLANNING */
#include "worklist.h"		/* MAX_NUM_WORKLISTS */

#include "multiselect.h"	/* filter type */

enum client_option_category {
  COC_GRAPHICS,
  COC_INTERFACE,
  COC_SOUND,
  COC_CHAT,
  COC_MESSAGE,
  COC_NETWORK,
  COC_GAMEPLAY,
  COC_MULTI_SELECTION,
  COC_DELAYED_GOTO,
  COC_TRADE,

  COC_NUM
};

enum client_option_type {
  COT_BOOLEAN,
  COT_INTEGER,
  COT_STRING,
  COT_PASSWORD,
  COT_STRING_VEC,
  COT_ENUM_LIST,
  COT_FILTER,
  COT_VOLUME
};

enum player_colors_modes {
  PCM_CLASSIC = 0,
  PCM_TEAM,

  NUM_PLAYER_COLORS_MODES
};

struct client_option {
  const char const *name;
  const char const *description;
  const char const *help_text;
  const enum client_option_category category;
  const enum client_option_type type;

  union {
    struct {
      bool *const pvalue;
      const bool def;
    } boolean;
    struct {
      int *const pvalue;
      const int def, min, max;
    } integer;
    struct {
      char *const pvalue;
      const size_t size;
      const char *def;
      /* 
       * A function to return a static NULL-terminated list of possible
       * string values, or NULL for none. 
       */
      const char **(*const val_accessor)(void);
    } string;
    struct {
      struct string_vector **pvector;
      const char *const *const def;
    } string_vec;
    struct {
      int *const pvalue;
      const int def;
      /* 
       * A function to return static string values, or NULL for none. 
       */
      const char *(*const str_accessor)(int);
    } enum_list;
    struct {
      filter *const pvalue;
      const filter def;
      /* 
       * A function which toggle a bit of the filter. It must return TRUE
       * if other bits are also changed.
       */
      bool (*const change)(filter *, filter);
      /* 
       * A function to return static string values, or NULL for none. 
       */
      const char *(*const str_accessor)(filter);
      filter temp;
    } filter;
  };
  void (*const change_callback)(struct client_option *option);

  /* volatile */
  void *gui_data;
};
extern struct client_option *const options;
extern const int num_options;

#define client_options_iterate(o)                                           \
{                                                                           \
  int _i;                                                                   \
  for (_i = 0; _i < num_options; _i++) {                                    \
    struct client_option *o = options + _i;                                 \
    {

#define client_options_iterate_end                                          \
    }                                                                       \
  }                                                                         \
}

extern char default_user_name[512];
extern char default_password[512];
extern char default_server_host[512];
extern int default_server_port; 
extern char default_metaserver[512];
extern char default_tileset_name[512];
extern char default_sound_set_name[512];
extern char default_sound_plugin_name[512];

extern struct worklist global_worklists[MAX_NUM_WORKLISTS];

/** Local Options: **/
extern bool random_leader;
extern bool solid_color_behind_units;
extern bool sound_bell_at_new_turn;
#ifdef AUDIO_VOLUME
extern int sound_volume;
#endif /* AUDIO_VOLUME */
extern int smooth_move_unit_msec;
extern int smooth_center_slide_msec;
extern bool do_combat_animation;
extern bool ai_popup_windows;
extern bool ai_manual_turn_done;
extern bool auto_center_on_unit;
extern bool auto_center_on_combat;
extern bool auto_center_each_turn;
extern bool wakeup_focus;
extern bool goto_into_unknown;
extern bool center_when_popup_city;
extern bool concise_city_production;
extern bool auto_turn_done;
extern bool meta_accelerators;
extern bool map_scrollbars;
extern bool dialogs_on_top;
extern bool ask_city_name;
extern bool popup_new_cities;
extern bool update_city_text_in_refresh_tile;
extern bool keyboardless_goto;
extern bool show_task_icons;
extern char chat_time_format[128];
extern struct string_vector *city_name_formats;
extern struct string_vector *default_user_nation;
extern struct string_vector *default_user_tech_goal;
extern bool show_split_message_window;
extern bool do_not_recenter_overview;
extern bool use_digits_short_cuts;
extern bool use_voteinfo_bar;
extern bool show_new_vote_in_front;
extern bool disable_chatline_scroll_on_window_resize;
extern bool always_show_votebar;
extern bool do_not_show_votebar_if_not_player;
extern bool warn_before_add_to_city;
extern bool prevent_duplicate_notify_tabs;
extern bool enable_chat_logging;
extern char chat_log_directory[MAX_LEN_PATH];
enum player_colors_modes player_colors_mode;
#ifndef ASYNC_TRADE_PLANNING
extern int trade_time_limit;
#endif	/* ASYNC_TRADE_PLANNING */
extern bool trade_mode_best_values;
extern bool trade_mode_allow_free_other;
extern bool trade_mode_internal_first;
extern bool trade_mode_homecity_first;
extern bool fullscreen_mode;
extern bool enable_tabs;
extern bool solid_unit_icon_bg;
extern bool better_fog;
extern bool save_options_on_exit;
extern bool chatline_autocomplementation;
extern bool disable_custom_dns;

/** View Options: **/

extern bool draw_map_grid;
extern bool draw_city_names;
extern bool draw_city_growth;
extern bool draw_city_productions;
extern bool draw_city_traderoutes;
extern bool draw_city_production_buy_cost;
extern bool draw_terrain;
extern bool draw_coastline;
extern bool draw_roads_rails;
extern bool draw_irrigation;
extern bool draw_mines;
extern bool draw_fortress_airbase;
extern bool draw_specials;
extern bool draw_pollution;
extern bool draw_cities;
extern bool draw_units;
extern bool draw_focus_unit;
extern bool draw_fog_of_war;
extern bool draw_borders;

typedef struct {
  const char *name;
  bool *p_value;
} view_option;
extern view_option view_options[];

/** Message Options: **/

/* for specifying which event messages go where: */
#define NUM_MW 3
#define MW_OUTPUT    1		/* add to the output window */
#define MW_MESSAGES  2		/* add to the messages window */
#define MW_POPUP     4		/* popup an individual window */

extern unsigned int messages_where[];	/* OR-ed MW_ values [E_LAST] */
extern int sorted_events[];	        /* [E_LAST], sorted by the
					   translated message text */
const char *get_message_text(enum event_type event);
unsigned int get_default_messages_where(enum event_type type);
void init_messages_where(void);

void client_options_init(void);
void client_options_free(void);
const char *option_file_name(void);
void check_ruleset_specific_options(void);
void save_options(void);

void load_general_options(void);
bool load_option_bool(struct section_file *file,
		      struct client_option *o, bool def);
int load_option_int(struct section_file *file,
		    struct client_option *o, int def);
const char *load_option_string(struct section_file *file,
			       struct client_option *o, const char *def);
void load_option_string_vec(struct section_file *file,
			    struct client_option *o,
			    const char *const *def,
			    struct string_vector *vector);
int load_option_enum_list(struct section_file *file,
			  struct client_option *o, int def);
filter load_option_filter(struct section_file *file,
			  struct client_option *o, filter def);

const char *get_sound_tag_for_event(enum event_type event);
bool is_city_event(enum event_type event);
const char *get_option_category_name(enum client_option_category category);

void save_global_worklist(struct section_file *file, const char *path, 
			  int wlinx, struct worklist *pwl);
void load_global_worklist(struct section_file *file, const char *path,
			  int wlinx, struct worklist *pwl);
bool check_global_worklist(struct worklist *pwl);

int revert_str_accessor(const char *(*str_accessor)(int), const char *str);
filter filter_revert_str_accessor(const char *(*str_accessor)(filter),
				  const char *str);
#endif  /* FC__OPTIONS_H */
