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
#ifndef WC_AI_AIUNIT_H
#define WC_AI_AIUNIT_H

#include "combat.hh"
#include "wc_types.hh"
#include "unittype.hh"

/*
 * To prevent integer overflows the product "power * hp * firepower"
 * is divided by POWER_DIVIDER.
 *
 * The constant may be changed since it isn't externally visible used.
 */
#define POWER_DIVIDER   (POWER_FACTOR * 3)

/* Simple military power macros */
#define DEFENCE_POWER(punit) \
 (unit_type(punit)->defense_strength * unit_type(punit)->hp \
  * unit_type(punit)->firepower)
#define ATTACK_POWER(punit) \
 (unit_type(punit)->attack_strength * unit_type(punit)->hp \
  * unit_type(punit)->firepower)
#define IS_ATTACKER(punit) \
  (unit_type(punit)->attack_strength \
        > unit_type(punit)->transport_capacity)
#define HOSTILE_PLAYER(pplayer, ai, aplayer) \
  (pplayers_at_war(pplayer, aplayer)         \
   || ai->diplomacy.target == aplayer)
#define UNITTYPE_COSTS(ut)                             \
  (ut->pop_cost * 3 + ut->happy_cost + ut->shield_cost \
   + ut->food_cost + ut->gold_cost)

struct ai_choice;
struct pf_path;

extern Unit_Type_id simple_ai_types[U_LAST];

void ai_manage_units(player_t *pplayer);
void ai_manage_unit(player_t *pplayer, unit_t *punit);
void ai_manage_military(player_t *pplayer,unit_t *punit);
city_t *find_nearest_safe_city(unit_t *punit);
int could_unit_move_to_tile(unit_t *punit, tile_t *dst_tile);
int look_for_charge(player_t *pplayer, unit_t *punit,
                    unit_t **aunit, city_t **acity);

int turns_to_enemy_city(Unit_Type_id our_type, city_t *acity,
                        int speed, bool go_by_boat,
                        unit_t *boat, Unit_Type_id boattype);
int turns_to_enemy_unit(Unit_Type_id our_type, int speed, tile_t *ptile,
                        Unit_Type_id enemy_type);
int find_something_to_kill(player_t *pplayer, unit_t *punit,
                           tile_t **ptile);
bool find_beachhead(unit_t *punit, tile_t *dst_tile,
                    tile_t **ptile);

int build_cost_balanced(Unit_Type_id type);
int unittype_att_rating(Unit_Type_id type, int veteran,
                        int moves_left, int hp);
int unit_att_rating(unit_t *punit);
int unit_def_rating_basic(unit_t *punit);
int unit_def_rating_basic_sq(unit_t *punit);
int unittype_def_rating_sq(Unit_Type_id att_type, Unit_Type_id def_type,
                           tile_t *ptile, bool fortified, int veteran);
int kill_desire(int benefit, int attack, int loss, int vuln, int attack_count);

bool is_on_unit_upgrade_path(Unit_Type_id test, Unit_Type_id base);

Unit_Type_id ai_wants_role_unit(player_t *pplayer, city_t *pcity,
                                int role, int want);
void ai_choose_role_unit(player_t *pplayer, city_t *pcity,
                         struct ai_choice *choice, int role, int want);
void update_simple_ai_types(void);

#define simple_ai_unit_type_iterate(m_i)     \
{                                            \
  int m_c;                                   \
  for (m_c = 0;; m_c++) {                    \
    Unit_Type_id m_i = simple_ai_types[m_c]; \
    if (m_i == U_LAST) {                     \
      break;                                 \
    }

#define simple_ai_unit_type_iterate_end      \
 }                                           \
}

#endif  /* WC_AI_AIUNIT_H */
