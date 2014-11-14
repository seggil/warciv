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
#ifndef WC_COMMON_EFFECTS_H
#define WC_COMMON_EFFECTS_H

#include "shared.hh"            /* bool */

#include "connection.hh"
#include "wc_types.hh"
#include "tech.hh"
#include "terrain.hh"
#include "city.hh"

/* Range of effects (used in equiv_range and effect.range fields)
 * These must correspond to effect_range_names[] in improvement.c. */
enum effect_range {
  EFFECT_RANGE_LOCAL,
  EFFECT_RANGE_CITY,
  EFFECT_RANGE_CONTINENT,
  EFFECT_RANGE_PLAYER,
  EFFECT_RANGE_WORLD,
  EFFECT_RANGE_LAST   /* keep this last */
};

/* Type of effects. (Used in effect.type field)
 * These must correspond to effect_type_names[] in effects.c. */
enum effect_type {
  EFFECT_TYPE_TECH_PARASITE,
  EFFECT_TYPE_AIRLIFT,
  EFFECT_TYPE_ANY_GOVERNMENT,
  EFFECT_TYPE_CAPITAL_CITY,
  EFFECT_TYPE_CORRUPT_PCT,
  EFFECT_TYPE_WASTE_PCT,
  EFFECT_TYPE_ENABLE_NUKE,
  EFFECT_TYPE_ENABLE_SPACE,
  EFFECT_TYPE_FOOD_ADD_TILE,
  /* TODO: EFT_FOOD_BONUS, */
  /* TODO: EFT_FOOD_PCT, */
  EFFECT_TYPE_FOOD_INC_TILE,
  EFFECT_TYPE_FOOD_PER_TILE,
  EFFECT_TYPE_FORCE_CONTENT,
  /* TODO: EFT_FORCE_CONTENT_PCT, */
  EFFECT_TYPE_GIVE_IMM_TECH,
  EFFECT_TYPE_GROWTH_FOOD,
  EFFECT_TYPE_HAVE_EMBASSIES,
  EFFECT_TYPE_LUXURY_BONUS,
  /* TODO: EFT_LUXURY_PCT, */
  EFFECT_TYPE_MAKE_CONTENT,
  EFFECT_TYPE_MAKE_CONTENT_MIL,
  EFFECT_TYPE_MAKE_CONTENT_MIL_PER,
  /* TODO: EFT_MAKE_CONTENT_PCT, */
  EFFECT_TYPE_MAKE_HAPPY,
  EFFECT_TYPE_NO_ANARCHY,
  EFFECT_TYPE_NO_SINK_DEEP,
  EFFECT_TYPE_NUKE_PROOF,
  /* TODO: EFT_POLLU_ADJ, */
  /* TODO: EFT_POLLU_PCT, */
  /* TODO: EFT_POLLU_POP_ADJ, */
  EFFECT_TYPE_POLLU_POP_PCT,
  /* TODO: EFT_POLLU_PROD_ADJ, */
  EFFECT_TYPE_POLLU_PROD_PCT,
  EFFECT_TYPE_PROD_ADD_TILE,
  EFFECT_TYPE_PROD_BONUS,
  /* TODO: EFT_PROD_PCT, */
  EFFECT_TYPE_PROD_INC_TILE,
  EFFECT_TYPE_PROD_PER_TILE,
  EFFECT_TYPE_PROD_TO_GOLD,
  EFFECT_TYPE_REVEAL_CITIES,
  EFFECT_TYPE_REVEAL_MAP,
  /* TODO: EFT_INCITE_DIST_ADJ, */
  EFFECT_TYPE_INCITE_DIST_PCT,
  EFFECT_TYPE_SCIENCE_BONUS,
  /* TODO: EFT_SCIENCE_PCT, */
  EFFECT_TYPE_SIZE_ADJ,
  EFFECT_TYPE_SIZE_UNLIMIT,
  EFFECT_TYPE_SS_STRUCTURAL,
  EFFECT_TYPE_SS_COMPONENT,
  EFFECT_TYPE_SS_MODULE,
  EFFECT_TYPE_SPY_RESISTANT,
  EFFECT_TYPE_TAX_BONUS,
  /* TODO: EFT_TAX_PCT, */
  EFFECT_TYPE_TRADE_ADD_TILE,
  /* TODO: EFT_TRADE_BONUS, */
  /* TODO: EFT_TRADE_PCT, */
  EFFECT_TYPE_TRADE_INC_TILE,
  EFFECT_TYPE_TRADE_PER_TILE,
  EFFECT_TYPE_SEA_MOVE,
  EFFECT_TYPE_UNIT_NO_LOSE_POP,
  EFFECT_TYPE_UNIT_RECOVER,
  EFFECT_TYPE_UPGRADE_UNIT,
  EFFECT_TYPE_UPKEEP_FREE,
  EFFECT_TYPE_NO_UNHAPPY,
  EFFECT_TYPE_LAND_VETERAN,
  EFFECT_TYPE_SEA_VETERAN,
  EFFECT_TYPE_AIR_VETERAN,
  EFFECT_TYPE_LAND_VET_COMBAT,
  /* TODO: EFT_SEA_VET_COMBAT, */
  /* TODO: EFT_AIR_VET_COMBAT, */
  EFFECT_TYPE_LAND_REGEN,
  EFFECT_TYPE_SEA_REGEN,
  EFFECT_TYPE_AIR_REGEN,
  EFFECT_TYPE_LAND_DEFEND,
  EFFECT_TYPE_SEA_DEFEND,
  EFFECT_TYPE_AIR_DEFEND,
  EFFECT_TYPE_MISSILE_DEFEND,
  EFFECT_TYPE_NO_INCITE,
  EFFECT_TYPE_REGEN_REPUTATION,
  EFFECT_TYPE_GAIN_AI_LOVE,
  EFFECT_TYPE_LAST      /* keep this last */
};

/* lookups */
enum effect_range effect_range_from_str(const char *str);
const char *effect_range_name(enum effect_range effect_range);
enum effect_type effect_type_from_str(const char *str);
const char *effect_type_name(enum effect_type effect_type);

/* A building_vector is an array of building types. */
#define SPECVEC_TAG building
#define SPECVEC_TYPE Impr_Type_id
#include "specvec.hh"
#define building_vector_iterate(vector, pbldg) \
  TYPED_VECTOR_ITERATE(Impr_Type_id, vector, pbldg)
#define building_vector_iterate_end  VECTOR_ITERATE_END

/* An effect_type_vector is an array of effect types. */
#define SPECVEC_TAG effect_type
#define SPECVEC_TYPE enum effect_type
#include "specvec.hh"
#define effect_type_vector_iterate(vector, ptype) \
  TYPED_VECTOR_ITERATE(enum effect_type, vector, ptype)
#define effect_type_vector_iterate_end VECTOR_ITERATE_END

/* Effect requirement type. */
enum effect_req_type {
  REQ_NONE,
  REQ_TECH,
  REQ_GOV,
  REQ_BUILDING,
  REQ_SPECIAL,
  REQ_TERRAIN,
  REQ_LAST
};

struct effect_group;

/* An effect is provided by a source.  If the source is present, and the
 * other conditions (described below) are met, the effect will be active.
 * Note the difference between effect and effect_type.  The effect doesn't
 * give an effect_type because all effects in an effect_vector will be of
 * the same type. */
struct effect {
  /* The range the effect applies to, relative to the source.  For instance
   * if the source is a building an effect with range "city" will apply to
   * everything in that city. */
  enum effect_range range;

  /* The "value" of the effect.  The meaning of this varies between
   * effects.  When get_xxx_bonus() is called the value of all applicable
   * effects will be summed up. */
  int value;

  /* The group this effect is in.  If more than one source in the group
   * provides the effect, only one will be active. */
  struct effect_group *group;

  /* If true, this effect will survive even if its source is destroyed. */
  bool survives;

  /* An effect can have a single requirement.  The effect will only be
   * active if this requirement is met.  The req is one of several types. */
  struct effect_req {
    enum effect_req_type type;                  /* requirement type */

    union effect_req_value {
      Tech_Type_id tech;                        /* requirement tech */
      int gov;                                  /* requirement government */
      Impr_Type_id building;                    /* requirement building */
      enum tile_special_type special;           /* requirement special */
      Terrain_type_id terrain;                  //* requirement terrain type */
    } value;                                    /* requirement value */
  } req;
};

/* An effect_vector is an array of effects. */
#define SPECLIST_TAG effect
#define SPECLIST_TYPE struct effect
#include "speclist.hh"
#define effect_list_iterate(effect_list, peffect) \
  TYPED_LIST_ITERATE(struct effect, effect_list, peffect)
#define effect_list_iterate_end LIST_ITERATE_END

/* This struct contains a source building along with the effect value.  It is
 * primarily useful for get_city_bonus_sources(). */
struct effect_source {
  Impr_Type_id building;
  int effect_value;
};
#define SPECVEC_TAG effect_source
#define SPECVEC_TYPE struct effect_source
#include "specvec.hh"
#define effect_source_vector_iterate(vector, psource) \
  TYPED_VECTOR_ITERATE(struct effect_source, vector, psource)
#define effect_source_vector_iterate_end VECTOR_ITERATE_END

/* ruleset cache creation and communication functions */
void ruleset_cache_init(void);
void ruleset_cache_free(void);
void ruleset_cache_add(Impr_Type_id source, enum effect_type effect_type,
                       enum effect_range range, bool survives, int eff_value,
                       enum effect_req_type req_type, int req_value,
                       int group_id);
void send_ruleset_cache(struct connection_list *dest);

/* equivalent effect group */
struct effect_group *effect_group_new(const char *name);
void effect_group_add_element(struct effect_group *group,
                              Impr_Type_id source_building,
                              enum effect_range range, bool survives);
int find_effect_group_id(const char *name);

/* name string to value functions */
enum effect_req_type effect_req_type_from_str(const char *str);
int parse_effect_requirement(Impr_Type_id source,
                             enum effect_req_type req_type,
                             const char *req_value);

/* An effect is targeted at a certain target type.  For instance a
 * production bonus applies to a city (or a tile within a city: same thing).
 * In this case the target is TARGET_CITY.  Note the effect has a range as
 * well: the effect applies to all targets within that range.  So for a
 * TARGET_CITY, EFR_PLAYER effect it applies to all cities owned by the
 * player. */
enum target_type {
  TARGET_PLAYER,
  TARGET_CITY,
  TARGET_BUILDING
};
#if 0
typedef struct city_s city_t;
#endif
bool is_effect_useful(enum target_type target,
                      const player_t *target_player,
                      const city_t *target_pcity,
                      Impr_Type_id target_building,
                      const tile_t *target_tile,
                      Impr_Type_id source,
                      const struct effect *effect);

bool is_building_replaced(const city_t *pcity, Impr_Type_id building);

/* functions to know the bonuses a certain effect is granting */
int get_player_bonus(const player_t *plr, enum effect_type effect_type);
int get_city_bonus(const city_t *pcity, enum effect_type effect_type);
int get_city_tile_bonus(const city_t *pcity, const tile_t *ptile,
                        enum effect_type effect_type);
int get_building_bonus(const city_t *pcity, Impr_Type_id building,
                       enum effect_type effect_type);

/* miscellaneous auxiliary effects functions */
struct effect_list *get_building_effects(Impr_Type_id building,
                                         enum effect_type effect_type);
struct effect_type_vector *get_building_effect_types(Impr_Type_id building);

int get_player_bonus_sources(struct effect_source_vector *sources,
    const player_t *pplayer, enum effect_type effect_type);
int get_city_bonus_sources(struct effect_source_vector *sources,
    const city_t *pcity, enum effect_type effect_type);

bool building_has_effect(Impr_Type_id building,
                         enum effect_type effect_type);
int get_current_construction_bonus(const city_t *pcity,
                                   enum effect_type effect_type);

Impr_Type_id ai_find_source_building(player_t *pplayer,
                                     enum effect_type effect_type);
Impr_Type_id get_building_for_effect(enum effect_type effect_type);

#endif  /* WC_COMMON_EFFECTS_H */

