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
#ifndef WC_SERVER_UNITTOOLS_H
#define WC_SERVER_UNITTOOLS_H

#include "wc_types.h"
#include "packets.h"            /* enum unit_info_use */
#include "unit.h"

#include "gotohand.h"

/* battle related */
int find_a_unit_type(int role, int role_tech);
bool maybe_make_veteran(unit_t *punit);
void unit_versus_unit(unit_t *attacker, unit_t *defender,
                      bool bombard);
bool kills_citizen_after_attack(unit_t *punit);

/* move check related */
bool is_airunit_refuel_point(tile_t *ptile, player_t *pplayer,
                             Unit_Type_id type, bool unit_is_on_tile);

/* turn update related */
void player_restore_units(player_t *pplayer);
void update_unit_activities(player_t *pplayer);

/* various */
char *get_location_str_in(player_t *pplayer, tile_t *ptile);
char *get_location_str_at(player_t *pplayer, tile_t *ptile);
enum goto_move_restriction get_activity_move_restriction(enum unit_activity activity);
void make_partisans(city_t *pcity);
bool enemies_at(unit_t *punit, tile_t *ptile);
bool teleport_unit_to_city(unit_t *punit, city_t *pcity, int move_cost,
                          bool verbose);
void resolve_unit_stacks(player_t *pplayer, player_t *aplayer,
                         bool verbose);
void remove_allied_visibility(player_t* pplayer, player_t* aplayer);
int get_watchtower_vision(unit_t *punit);
bool unit_profits_of_watchtower(unit_t *punit);
void bounce_unit(unit_t *punit, bool verbose);

/* creation/deletion/upgrading */
void upgrade_unit(unit_t *punit, Unit_Type_id to_unit, bool has_to_pay);
unit_t *create_unit(player_t *pplayer, tile_t *ptile, Unit_Type_id type,
                    int veteran_level, int homecity_id, int moves_left);
unit_t *create_unit_full(player_t *pplayer, tile_t *ptile,
                         Unit_Type_id type, int veteran_level,
                         int homecity_id, int moves_left, int hp_left,
                         unit_t *ptrans);
void wipe_unit(unit_t *punit);
void wipe_unit_spec_safe(unit_t *punit,
                         bool wipe_cargo, bool ignore_gameloss);
void kill_unit(unit_t *pkiller, unit_t *punit);

/* sending to client */
void package_unit(unit_t *punit, struct packet_unit_info *packet);
void package_short_unit(unit_t *punit,
                        struct packet_unit_short_info *packet,
                        enum unit_info_use packet_use, int info_city_id,
                        bool new_serial_num);
void send_unit_info(player_t *dest, unit_t *punit);
void send_unit_info_to_onlookers(struct connection_list *dest, unit_t *punit,
                                 tile_t *ptile, bool remove_unseen);
void send_all_known_units(struct connection_list *dest);
void unit_goes_out_of_sight(player_t *pplayer, unit_t *punit);

/* doing a unit activity */
void do_nuclear_explosion(player_t *pplayer, tile_t *ptile);
bool try_move_unit(unit_t *punit, tile_t *ptile);
bool do_airlift(unit_t *punit, city_t *city2);
bool do_paradrop(unit_t *punit, tile_t *ptile);
void load_unit_onto_transporter(unit_t *punit, unit_t *ptrans);
void unload_unit_from_transporter(unit_t *punit);
bool move_unit(unit_t *punit, tile_t *ptile, int move_cost);
bool execute_orders(unit_t *punit);

void reset_air_patrol(player_t *pplayer);

#endif  /* WC_SERVER_UNITTOOLS_H */
