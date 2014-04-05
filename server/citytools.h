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
#ifndef WC_SERVER_CITYTOOLS_H
#define WC_SERVER_CITYTOOLS_H

#include "city.h"
#include "events.h"
#include "nation.h"             /* for struct city_name */
#include "packets.h"

#define FOOD_WEIGHTING 10
#define SHIELD_WEIGHTING 10
#define TRADE_WEIGHTING 10
#define LUXURY_WEIGHTING 10
#define SCIENCE_WEIGHTING 10
#define GOLD_WEIGHTING 10
#define HAPPY_WEIGHTING 10
#define UNHAPPY_WEIGHTING 10
#define POLLUTION_WEIGHTING 1 /* tentative */
#define MAX_AI_CITIES 15

/* The Trade Weighting has to about as large as the Shield Weighting,
   otherwise the AI will build Barracks to create veterans in cities
   with only 1 shields production.
    8 is too low
   18 is too high
 */
#define WARMING_FACTOR 50

bool can_sell_building(city_t *pcity, Impr_Type_id id);
bool city_rapture_grow(const city_t *pcity);
city_t *find_city_wonder(Impr_Type_id id);
int build_points_left(city_t *pcity);
int do_make_unit_veteran(city_t *pcity, Unit_Type_id id);
int city_shield_bonus(city_t *pcity);
int city_luxury_bonus(city_t *pcity);
int city_science_bonus(city_t *pcity);
int city_tax_bonus(city_t *pcity);

void transfer_city_units(player_t *pplayer, player_t *pvictim,
                         struct unit_list *units, city_t *pcity,
                         city_t *exclude_city,
                         int kill_outside, bool verbose);
void transfer_city(player_t *ptaker, city_t *pcity,
                   int kill_outside, bool transfer_unit_verbose,
                   bool resolve_stack, bool raze);
city_t *find_closest_owned_city(player_t *pplayer,
                                tile_t *ptile,
                                bool sea_required,
                                city_t *pexclcity);
void handle_unit_enter_city(struct unit *punit, city_t *pcity);

void send_city_info(player_t *dest, city_t *pcity);
void send_city_info_at_tile(player_t *pviewer, struct conn_list *dest,
                            city_t *pcity, tile_t *ptile);
void send_all_known_cities(struct conn_list *dest);
void send_all_known_trade_routes(struct conn_list *dest);
void send_player_cities(player_t *pplayer);
void package_city(city_t *pcity, struct packet_city_info *packet,
                  bool dipl_invest);

void reality_check_city(player_t *pplayer, tile_t *ptile);
bool update_dumb_city(player_t *pplayer, city_t *pcity);
void refresh_dumb_city(city_t *pcity);

void create_city(player_t *pplayer, tile_t *ptile,
                 const char *name);
void remove_city(city_t *pcity);

void establish_trade_route(city_t *pc1, city_t *pc2);

void do_sell_building(player_t *pplayer, city_t *pcity,
                      Impr_Type_id id);
void building_lost(city_t *pcity, Impr_Type_id id);
void change_build_target(player_t *pplayer, city_t *pcity,
                         int target, bool is_unit, enum event_type event);

bool is_allowed_city_name(player_t *pplayer, const char *city_name,
                          char *error_buf, size_t bufsz);
char *city_name_suggestion(player_t *pplayer, tile_t *ptile);


bool city_can_work_tile(city_t *pcity, int city_x, int city_y);
void server_remove_worker_city(city_t *pcity, int city_x, int city_y);
void server_set_worker_city(city_t *pcity, int city_x, int city_y);
bool update_city_tile_status_map(city_t *pcity, tile_t *ptile);
void sync_cities(void);
bool can_place_worker_here(city_t *pcity, int city_x, int city_y);
void check_city_workers(player_t *pplayer);
void city_landlocked_sell_coastal_improvements(tile_t *ptile);

void send_city_manager_param(struct conn_list *clist,
                             struct packet_city_manager_param *packet,
                             player_t *pplayer,
                             bool include_player);
void send_city_manager_info(struct conn_list *clist, city_t *pcity,
                            bool include_player);
void send_all_known_city_manager_infos(struct conn_list *clist);
void clear_city_manager_param(city_t *pcity);
void reset_city_manager_params(player_t *pplayer);

void clear_rally_point(city_t *pcity, bool send_info);
void reset_rally_points(player_t *pplayer);

#endif  /* WC_SERVER_CITYTOOLS_H */
