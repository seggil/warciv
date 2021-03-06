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
#ifndef WC_COMMON_COMBAT_H
#define WC_COMMON_COMBAT_H

#include "wc_types.hh"
#include "unittype.hh"

/*
 * attack_strength and defense_strength are multiplied by POWER_FACTOR
 * to yield the base of attack_power and defense_power.
 *
 * The constant may be changed since it isn't externally visible used.
 */
#define POWER_FACTOR    10

bool can_player_attack_tile(player_t *pplayer,
                            const tile_t *ptile);
bool can_unit_attack_unit_at_tile(unit_t *punit, unit_t *pdefender,
                                  const tile_t *dest_tile);
bool can_unit_attack_all_at_tile(unit_t *punit,
                                 const tile_t *ptile);
bool can_unit_attack_tile(unit_t *punit, const tile_t *ptile);

double win_chance(int as, int ahp, int afp, int ds, int dhp, int dfp);

void get_modified_firepower(unit_t *attacker, unit_t *defender,
                            int *att_fp, int *def_fp);
double unit_win_chance(unit_t *attacker, unit_t *defender);

bool unit_really_ignores_citywalls(unit_t *punit);
bool unit_on_fortress(unit_t *punit);
city_t *sdi_defense_close(player_t *owner,
                          const tile_t *ptile);

int get_attack_power(unit_t *punit);
int base_get_attack_power(Unit_Type_id type, int veteran, int moves_left);
int base_get_defense_power(unit_t *punit);
int get_defense_power(unit_t *punit);
int get_total_defense_power(unit_t *attacker, unit_t *defender);
int get_virtual_defense_power(Unit_Type_id att_type, Unit_Type_id def_type,
                              const tile_t *ptile,
                              bool fortified, int veteran);
int get_total_attack_power(unit_t *attacker, unit_t *defender);

unit_t *get_defender(unit_t *attacker, const tile_t *ptile);
unit_t *get_attacker(unit_t *defender, const tile_t *ptile);

bool is_stack_vulnerable(const tile_t *ptile);

#endif /* WC_COMMON_COMBAT_H */
