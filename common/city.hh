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
#ifndef WC_COMMON_CITY_H
#define WC_COMMON_CITY_H

#include "wc_types.hh"
#include "improvement.hh"
//#include "packets.h"
#include "unit.hh"               /* struct unit_list */
#include "worklist.hh"

typedef enum city_unit_option_e {
  /* The first 4 are whether to auto-attack versus each unit move_type
   * from with auto-attack units within this city.  Note that these
   * should stay the first four, and must stay in the same order as
   * enum unit_move_type. */
  CITY_UNIT_AUTO_ATTACK_LAND=0,
  CITY_UNIT_AUTO_ATTACK_SEA,
  CITY_UNIT_AUTO_ATTACK_HELI,
  CITY_UNIT_AUTO_ATTACK_AIR,

  /* The next is whether building a settler at size 1 disbands a city. */
  CITY_NEW_SETTLER_DISBAND,

  /* The following 2 are what to do of new citizens when the city grows:
   * make them workers, scientists, or taxmen. Should have only one set,
   * or if neither is set, that means make workers. */
  CITY_NEW_CITIZEN_EINSTEIN,
  CITY_NEW_CITIZEN_TAXMAN

  /* Any more than 8 options requires a protocol extension, since
   * we only send 8 bits.
   */
} city_unit_option;

/* first four bits are for auto-attack: */
#define CITYOPT_AUTOATTACK_BITS 0xF

/* for new city: default auto-attack options all on, others off: */
#define CITYOPT_DEFAULT (CITYOPT_AUTOATTACK_BITS)


/* Number of tiles a city can use */
#define CITY_TILES city_tiles

#define INCITE_IMPOSSIBLE_COST (1000 * 1000 * 1000)

/*
 * Size of the biggest possible city.
 *
 * The constant may be changed since it isn't externally visible.
 */
#define MAX_CITY_SIZE                                   100

/*
 * Iterate a city map.  This iterates over all city positions in the
 * city map (i.e., positions that are workable by the city) in unspecified
 * order.
 */
#define city_map_iterate(x, y)                                              \
{                                                                           \
  int _itr;                                                                 \
                                                                            \
  for (_itr = 0; _itr < CITY_MAP_SIZE * CITY_MAP_SIZE; _itr++) {            \
    const int x = _itr % CITY_MAP_SIZE, y = _itr / CITY_MAP_SIZE;           \
                                                                            \
    if (is_valid_city_coords(x, y)) {

#define city_map_iterate_end                                                \
    }                                                                       \
  }                                                                         \
}

/* Iterate a city map, from the center (the city) outwards */
extern struct iter_index {
  int dx, dy, dist;
} *city_map_iterate_outwards_indices;
extern int city_tiles;

/* Iterate a city map, from the center (the city) outwards.
 * (city_x, city_y) will be the city coordinates. */
#define city_map_iterate_outwards(city_x, city_y)                           \
{                                                                           \
  int city_x, city_y, _index;                                               \
                                                                            \
  for (_index = 0; _index < CITY_TILES; _index++) {                         \
    city_x = city_map_iterate_outwards_indices[_index].dx + CITY_MAP_RADIUS;\
    city_y = city_map_iterate_outwards_indices[_index].dy + CITY_MAP_RADIUS;

#define city_map_iterate_outwards_end                                       \
  }                                                                         \
}

/*
 * Iterate a city map in checked real map coordinates. The center of
 * the city is given as a map position (x0,y0). cx and cy will be
 * elements of [0,CITY_MAP_SIZE). mx and my will form the map position
 * (mx,my).
 */
#define city_map_checked_iterate(city_tile, cx, cy, itr_tile) {     \
  city_map_iterate_outwards(cx, cy) {                          \
    tile_t *itr_tile;                                     \
    if ((itr_tile = base_city_map_to_map(city_tile, cx, cy))) {

#define city_map_checked_iterate_end \
    }                                \
  } city_map_iterate_outwards_end    \
}

/* Does the same thing as city_map_checked_iterate, but keeps the city
 * coordinates hidden. */
#define map_city_radius_iterate(city_tile, itr_tile)     \
{                                                                 \
  city_map_checked_iterate(city_tile, _cx, _cy, itr_tile) {

#define map_city_radius_iterate_end                               \
  } city_map_checked_iterate_end;                                 \
}


enum choice_type { CT_NONE = 0, CT_BUILDING = 0, CT_NONMIL, CT_ATTACKER,
                   CT_DEFENDER, CT_LAST };

/* FIXME:

   This should detect also cases where type is just initialized with
   CT_NONE (probably in order to silence compiler warnings), but no real value
   is given. You have to change value of CT_BUILDING into 1 before you
   can add this check. It's left this way for now, is case hardcoded
   value 0 is still used somewhere instead of CT_BUILDING.

   -- Caz
*/
#define ASSERT_REAL_CHOICE_TYPE(type)                                    \
        assert(type >= 0 && type < CT_LAST /* && type != CT_NONE */ );

struct ai_choice {
  int choice;            /* what the advisor wants */
  int want;              /* how bad it wants it (0-100) */
  int type;              /* unit/building or other depending on question */
  bool need_boat;        /* unit being built wants a boat */
};

struct ai_city {
  /* building desirabilities - easiest to handle them here -- Syela */
  int building_want[B_LAST];    /* not sure these will always be < 256 */

  unsigned int danger;          /* danger to be compared to assess_defense */
  bool diplomat_threat;         /* enemy diplomat or spy is near the city */
  bool has_diplomat;            /* this city has diplomat or spy defender */
  unsigned int urgency;         /* how close the danger is; if zero,
                                   bodyguards can leave */
  unsigned int grave_danger;    /* danger, should show positive feedback */
  int wallvalue;                /* how much it helps for defenders to be
                                   ground units */
  int trade_want;               /* saves a zillion calculations */
  struct ai_choice choice;      /* to spend gold in the right place only */
  int downtown;                 /* distance from neighbours, for locating
                                   wonders wisely */
  int distance_to_wonder_city;  /* wondercity will set this for us,
                                   avoiding paradox */
  bool celebrate;               /* try to celebrate in this city */

  /* Used for caching when settlers evalueate which tile to improve,
     and when we place workers. */
  signed short int detox[CITY_MAP_SIZE][CITY_MAP_SIZE];
  signed short int derad[CITY_MAP_SIZE][CITY_MAP_SIZE];
  signed short int mine[CITY_MAP_SIZE][CITY_MAP_SIZE];
  signed short int irrigate[CITY_MAP_SIZE][CITY_MAP_SIZE];
  signed short int road[CITY_MAP_SIZE][CITY_MAP_SIZE];
  signed short int railroad[CITY_MAP_SIZE][CITY_MAP_SIZE];
  signed short int transform[CITY_MAP_SIZE][CITY_MAP_SIZE];
  signed short int tile_value[CITY_MAP_SIZE][CITY_MAP_SIZE];

  /* so we can contemplate with warmap fresh and decide later */
  int settler_want, founder_want; /* for builder (F_SETTLERS) and founder (F_CITIES) */
  int next_founder_want_recalc; /* do not recalc founder_want every turn */
  bool founder_boat; /* if the city founder will need a boat */
  int invasion; /* who's coming to kill us, for attack co-ordination */
  int attack, bcost; /* This is also for invasion - total power and value of
                      * all units coming to kill us. */

  int worth; /* Cache city worth here, sum of all weighted incomes */
  int next_recalc; /* Only recalc every Nth turn */
};

struct city_common {
  int id;
  int owner;
  tile_t *tile;
  char name[MAX_LEN_NAME];

  int pop_size;  /* the people */

  /* How the citizens feel:
     people_*[0] is distribution before any of the modifiers below.
     people_*[1] is distribution after luxury.
     people_*[2] is distribution after after building effects.
     people_*[3] is distribution after units enfored martial order.
     people_*[4] is distribution after wonders. (final result.) */
  int people_happy[5];
  int people_content[5];
  int people_unhappy[5];
  int people_angry[5];

  /* Specialists */
  int specialists[SP_COUNT];

  /* trade routes */
  struct trade_route_list *trade_routes;

  /* the productions */
  int food_prod;
  int food_surplus;
  /* Shield production is shields produced minus shield_waste*/
  int shield_prod;
  int shield_surplus;
  int shield_waste;
  int trade_prod, corruption, tile_trade;

  /* Cached values for CPU savings. */
  int shield_bonus, luxury_bonus, tax_bonus, science_bonus;

  /* the totals */
  int luxury_total;
  int tax_total;
  int science_total;

  /* the physics */
  int food_stock;
  int shield_stock;
  int pollution;

  bool is_building_unit;    /* boolean unit/improvement */
  int currently_building;

  Impr_Status improvements[B_LAST];

  struct worklist worklist;

  enum city_tile_type city_map[CITY_MAP_SIZE][CITY_MAP_SIZE];

  struct unit_list *units_supported;

  /* turn states */
  bool did_buy;
  bool did_sell;
  int turn_last_built;        /* The last year in which something was built */
  int changed_from_id;        /* If changed this turn, what changed from (id) */
  bool changed_from_is_unit;   /* If changed this turn, what changed from (unit?) */
  int disbanded_shields;      /* If you disband unit in a city. Count them */
  int caravan_shields;        /* If caravan has helped city to build wonder. */
  int before_change_shields;  /* If changed this turn, shields before penalty */
  int last_turns_shield_surplus; /* The surplus we had last turn. */
  int anarchy;                /* anarchy rounds count */
  int rapture;                /* rapture rounds count */
  bool was_happy;
  bool airlift;
  int city_options;             /* bitfield; positions as enum city_options */

  int turn_founded;             /* In which turn was the city founded? */

  tile_t *rally_point;
};

struct city_s {
  struct city_common common;
  union city_u {
    struct client_part_s {
      /* Only used at the client (the server is omniscient). */
      bool occupied;
      bool happy;
      bool unhappy;

      /* Info for dipl/spy investigation. */
      struct unit_list *info_units_supported;
      struct unit_list *info_units_present;

      /* The color is an index into the city_colors array in mapview_common */
      bool colored;
      int color_index;

      bool traderoute_drawing_disabled;
    } client;

    struct server_part_s {
      /* Indicates if the city map is synced with the client. */
      bool synced;

      /* If > 0, workers will not be rearranged until they are unfrozen. */
      int workers_frozen;

      /* If set, workers need to be arranged when the city is unfrozen.  Only
       * set inside auto_arrange_workers. */
      bool needs_arrange;

      /* Diplomats steal once; for spies, gets harder */
      int steal;

      /* Original owner */
      int original;

      /* Used to build unit or city at the end of the turn. */
      bool delayed_build;

      /* Used to share CMA paramters */
      bool managed;
      struct cm_parameter parameter;

      struct ai_city ai;
      bool debug;
    } server;
  } u;

  /* Nothing behind the union please! */
};

/* city drawing styles */

#define MAX_CITY_TILES 8

struct citystyle {
  const char *name; /* Translated string - doesn't need freeing. */
  char name_orig[MAX_LEN_NAME];       /* untranslated */
  char graphic[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  char citizens_graphic[MAX_LEN_NAME];
  char citizens_graphic_alt[MAX_LEN_NAME];
  int techreq;                  /* tech required to use a style      */
  int replaced_by;              /* index to replacing style          */
                                /* client side-only:                 */
  int tresh[MAX_CITY_TILES];    /* treshholds - what city size to use a tile */
  int tiles_num;                /* number of "normal" city tiles,    */
};                              /* not incl. wall and occupied tiles */

extern struct citystyle *city_styles;

/* get 'struct city_list' and related functions: */
#define SPECLIST_TAG city
#undef  SPECLIST_TYPE
#define SPECLIST_TYPEDEF city_t
#include "speclist.hh"
#undef SPECLIST_TYPEDEF

#define city_list_iterate(citylist, pcity) \
    TYPED_LIST_ITERATE(city_t, citylist, pcity)
#define city_list_iterate_end  LIST_ITERATE_END

#define cities_iterate(pcity)                                               \
{                                                                           \
  players_iterate(CI_player) {                                              \
    city_list_iterate(CI_player->cities, pcity) {

#define cities_iterate_end                                                  \
    } city_list_iterate_end;                                                \
  } players_iterate_end;                                                    \
}


/* properties */

player_t *city_owner(const city_t *pcity);
int city_population(const city_t *pcity);
int city_gold_surplus(const city_t *pcity, int tax_total);
int city_buy_cost(const city_t *pcity);
bool city_happy(const city_t *pcity);  /* generally use celebrating instead */
bool city_unhappy(const city_t *pcity);                /* anarchy??? */
bool base_city_celebrating(const city_t *pcity);
bool city_celebrating(const city_t *pcity);            /* love the king ??? */
bool city_rapture_grow(const city_t *pcity);

/* city related improvement and unit functions */

bool city_has_terr_spec_gate(const city_t *pcity, Impr_Type_id id);
int improvement_upkeep(const city_t *pcity, Impr_Type_id i);
bool can_build_improvement_direct(const city_t *pcity, Impr_Type_id id);
bool can_build_improvement(const city_t *pcity, Impr_Type_id id);
bool can_eventually_build_improvement(const city_t *pcity,
                                      Impr_Type_id id);
bool can_build_unit(const city_t *pcity, Unit_Type_id id);
bool can_build_unit_direct(const city_t *pcity, Unit_Type_id id);
bool can_eventually_build_unit(const city_t *pcity, Unit_Type_id id);
bool city_can_use_specialist(const city_t *pcity,
                             Specialist_type_id type);
bool city_got_building(const city_t *pcity,  Impr_Type_id id);
bool is_capital(const city_t *pcity);
bool city_got_citywalls(const city_t *pcity);
bool building_replaced(const city_t *pcity, Impr_Type_id id);
int city_change_production_penalty(const city_t *pcity,
                                   int target, bool is_unit);
int city_turns_to_build(const city_t *pcity, int id, bool id_is_unit,
                        bool include_shield_stock);
int city_turns_to_grow(const city_t *pcity);
bool city_can_grow_to(const city_t *pcity, int pop_size);
bool city_can_change_build(const city_t *pcity);

/* textual representation of buildings */

const char *get_impr_name_ex(const city_t *pcity, Impr_Type_id id);

/* tile production functions */

int get_shields_tile(const tile_t *ptile); /* shield on spot */
int get_trade_tile(const tile_t *ptile);   /* trade on spot */
int get_food_tile(const tile_t *ptile);    /* food on spot */

/* city map functions */

bool is_valid_city_coords(const int city_x, const int city_y);
bool map_to_city_map(int *city_map_x, int *city_map_y,
                     const city_t *const pcity,
                     const tile_t *ptile);

tile_t *base_city_map_to_map(const tile_t *city_center_tile,
                                  int city_map_x, int city_map_y);
tile_t *city_map_to_map(const city_t *const pcity,
                             int city_map_x, int city_map_y);

/* Initialization functions */
int compare_iter_index(const void *a, const void *b);
void generate_city_map_indices(void);

/* shield on spot */
int city_get_shields_tile(int city_x, int city_y, const city_t *pcity);
int base_city_get_shields_tile(int city_x, int city_y,
                               const city_t *pcity, bool is_celebrating);
/* trade  on spot */
int city_get_trade_tile(int city_x, int city_y, const city_t *pcity);
int base_city_get_trade_tile(int city_x, int city_y,
                             const city_t *pcity, bool is_celebrating);
/* food   on spot */
int city_get_food_tile(int city_x, int city_y, const city_t *pcity);
int base_city_get_food_tile(int city_x, int city_y,
                            const city_t *pcity, bool is_celebrating);

void set_worker_city(city_t *pcity, int city_x, int city_y,
                     enum city_tile_type type);
enum city_tile_type get_worker_city(const city_t *pcity, int city_x,
                                    int city_y);
void get_worker_on_map_position(const tile_t *ptile,
                                enum city_tile_type *result_city_tile_type,
                                city_t **result_pcity);
bool is_worker_here(const city_t *pcity, int city_x, int city_y);

bool city_can_be_built_here(const tile_t *ptile, unit_t *punit);

/* trade functions */
bool can_cities_trade(const city_t *pc1, const city_t *pc2);
bool can_establish_trade_route(const city_t *pc1, const city_t *pc2);
bool have_cities_trade_route(const city_t *pc1, const city_t *pc2);
int trade_between_cities(const city_t *pc1, const city_t *pc2);
int city_num_trade_routes(const city_t *pcity);
int get_caravan_enter_city_trade_bonus(const city_t *pc1,
                                       const city_t *pc2);
int get_city_min_trade_route(const city_t *pcity,
                             struct trade_route **slot);

/* list functions */
city_t *city_list_find_id(struct city_list *This, int id);
city_t *city_list_find_name(struct city_list *This, const char *name);

int city_name_compare(const void *p1, const void *p2);

/* city free cost values depending on government: */
int citygov_free_shield(const city_t *pcity, struct government *gov);
int citygov_free_happy(const city_t *pcity, struct government *gov);
int citygov_free_food(const city_t *pcity, struct government *gov);

/* city style functions */
int get_city_style(const city_t *pcity);
int get_player_city_style(player_t *plr);
int get_style_by_name(const char *);
int get_style_by_name_orig(const char *);
const char *get_city_style_name(int style);
char* get_city_style_name_orig(int style);

city_t *is_enemy_city_tile(const tile_t *ptile,
                           player_t *pplayer);
city_t *is_allied_city_tile(const tile_t *ptile,
                            player_t *pplayer);
city_t *is_non_attack_city_tile(const tile_t *ptile,
                                player_t *pplayer);
city_t *is_non_allied_city_tile(const tile_t *ptile,
                                player_t *pplayer);

bool is_unit_near_a_friendly_city(unit_t *punit);
bool is_friendly_city_near(player_t *owner, const tile_t *ptile);
bool city_exists_within_city_radius(const tile_t *ptile,
                                    bool may_be_on_center);

/* granary size as a function of city size */
int city_granary_size(int city_size);

void city_add_improvement(city_t *pcity,Impr_Type_id impr);
void city_remove_improvement(city_t *pcity,Impr_Type_id impr);

/* city update functions */
void generic_city_refresh(city_t *pcity,
                          bool refresh_trade_route_cities,
                          void (*send_unit_info) (player_t * pplayer,
                                                  unit_t * punit));
void adjust_city_free_cost(int *num_free, int *this_cost);
int city_corruption(const city_t *pcity, int trade);
int city_waste(const city_t *pcity, int shields);
int city_specialists(const city_t *pcity);                 /* elv+tax+scie */
const char *specialists_string(const int *specialists);
int get_city_tax_bonus(const city_t *pcity);
int get_city_luxury_bonus(const city_t *pcity);
int get_city_shield_bonus(const city_t *pcity);
int get_city_science_bonus(const city_t *pcity);
bool city_built_last_turn(const city_t *pcity);

/* city creation / destruction */
city_t *create_city_virtual(player_t *pplayer, tile_t *ptile,
                            const char *name);
void remove_city_virtual(city_t *pcity);

/* misc */
bool is_city_option_set(const city_t *pcity, city_unit_option option);
void city_styles_alloc(int num);
void city_styles_free(void);

void get_food_trade_shields(const city_t *pcity, int *food, int *trade,
                            int *shields);
void get_tax_income(player_t *pplayer, int trade, int *sci,
                    int *lux, int *tax);
int get_city_tithes_bonus(const city_t *pcity);
int city_pollution(city_t *pcity, int shield_total);

/*
 * Iterates over all improvements which are built in the given city.
 */
#define built_impr_iterate(m_pcity, m_i)                                      \
  impr_type_iterate(m_i) {                                                    \
    if((m_pcity)->common.improvements[m_i] == I_NONE) {                       \
      continue;                                                               \
    }

#define built_impr_iterate_end                                                \
  } impr_type_iterate_end;

/**************************************************************************
  Return TRUE iff the given city coordinate pair is the center tile of
  the citymap.
**************************************************************************/
static inline bool is_city_center(int city_x, int city_y)
{
  return CITY_MAP_RADIUS == city_x && CITY_MAP_RADIUS == city_y;
}

#endif  /* WC_COMMON_CITY_H */
