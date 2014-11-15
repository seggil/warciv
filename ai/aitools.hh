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
#ifndef WC_AI_AITOOLS_H
#define WC_AI_AITOOLS_H

#include "shared.hh"            /* bool type */

#include "wc_types.hh"
#include "unit.hh"              /* enum ai_unit_task */
#include "unittype.hh"          /* Unit_Type_id */

struct ai_choice;
struct pf_path;

#ifdef DEBUG
#define CHECK_UNIT(punit)                                        \
 (assert(punit != NULL),                                         \
  assert(punit->type < U_LAST),                                  \
  assert(punit->owner < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS),   \
  assert(find_unit_by_id(punit->id) != NULL))
#else
#define CHECK_UNIT(punit) assert(TRUE)
#endif

enum bodyguard_enum {
  BODYGUARD_WANTED=-1,
  BODYGUARD_NONE
};

int military_amortize(player_t *pplayer, city_t *pcity,
                      int value, int delay, int build_cost);
int stack_cost(unit_t *pdef);

bool ai_unit_execute_path(unit_t *punit, struct pf_path *path);
bool ai_gothere(player_t *pplayer, unit_t *punit,
                tile_t *dst_tile);
bool ai_unit_goto(unit_t *punit, tile_t *ptile);

void ai_unit_new_role(unit_t *punit, enum ai_unit_task task,
                      tile_t *ptile);
bool ai_unit_make_homecity(unit_t *punit, city_t *pcity);
bool ai_unit_attack(unit_t *punit, tile_t *ptile);
bool ai_unit_move(unit_t *punit, tile_t *ptile);

city_t *dist_nearest_city(player_t *pplayer, tile_t *ptile,
                          bool everywhere, bool enemy);

void ai_government_change(player_t *pplayer, int gov);
int ai_gold_reserve(player_t *pplayer);

void init_choice(struct ai_choice *choice);
void adjust_choice(int value, struct ai_choice *choice);
void copy_if_better_choice(struct ai_choice *cur, struct ai_choice *best);
void ai_advisor_choose_building(city_t *pcity, struct ai_choice *choice);
bool ai_assess_military_unhappiness(city_t *pcity, struct government *g);

bool ai_wants_no_science(player_t *pplayer);

bool is_player_dangerous(player_t *pplayer, player_t *aplayer);

#endif  /* WC_AI_AITOOLS_H */
