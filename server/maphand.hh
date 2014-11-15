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
#ifndef WC_SERVER_MAPHAND_H
#define WC_SERVER_MAPHAND_H

#include "wc_types.hh"
#include "map.hh"
#include "packets.hh"
#include "terrain.hh"

#include "hand_gen.hh"

enum ocean_land_change { OLC_NONE, OLC_OCEAN_TO_LAND, OLC_LAND_TO_OCEAN };

struct section_file;

struct dumb_city{
  int id;
  bool has_walls;
  bool occupied;
  bool happy, unhappy;
  char name[MAX_LEN_NAME];
  unsigned short size;
  unsigned char owner;
};

struct player_tile {
  Terrain_type_id terrain;
  enum tile_special_type special;
  unsigned short seen;
  unsigned short own_seen;
  /* If you build a city with an unknown square within city radius
     the square stays unknown. However, we still have to keep count
     of the seen points, so they are kept in here. When the tile
     then becomes known they are moved to seen. */
  unsigned short pending_seen;
  struct dumb_city* city;
  short last_updated;
};

/* The maximum number of continents and oceans. */
#define MAP_NCONT 1024

void assign_continent_numbers(bool skip_unsafe);

void global_warming(int effect);
void nuclear_winter(int effect);
void give_map_from_player_to_player(player_t *pfrom, player_t *pdest);
void give_seamap_from_player_to_player(player_t *pfrom, player_t *pdest);
void give_citymap_from_player_to_player(city_t *pcity,
                                        player_t *pfrom, player_t *pdest);
void send_all_known_tiles(struct connection_list *dest);
void send_tile_info(struct connection_list *dest, tile_t *ptile);
void reveal_hidden_units(player_t *pplayer, tile_t *ptile);
void conceal_hidden_units(player_t *pplayer, tile_t *ptile);
void unfog_area(player_t *pplayer, tile_t *ptile, int len);
void fog_area(player_t *pplayer, tile_t *ptile, int len);
void upgrade_city_rails(player_t *pplayer, bool discovery);
void send_map_info(struct connection_list *dest);
void map_fog_city_area(city_t *pcity);
void map_unfog_city_area(city_t *pcity);
void remove_unit_sight_points(unit_t *punit);
void show_area(player_t *pplayer,tile_t *ptile, int len);
void map_unfog_pseudo_city_area(player_t *pplayer, tile_t *ptile);
void map_fog_pseudo_city_area(player_t *pplayer, tile_t *ptile);

bool map_is_known_and_seen(const tile_t *ptile, player_t *pplayer);
void map_change_seen(tile_t *ptile, player_t *pplayer, int change);
bool map_is_known(const tile_t *ptile, player_t *pplayer);
void map_set_known(tile_t *ptile, player_t *pplayer);
void map_clear_known(tile_t *ptile, player_t *pplayer);
void map_know_all(player_t *pplayer);
void map_know_and_see_all(player_t *pplayer);
void show_map_to_all(void);

void player_map_allocate(player_t *pplayer);
void player_map_free(player_t *pplayer);
struct player_tile *map_get_player_tile(const tile_t *ptile,
                                        player_t *pplayer);
bool update_player_tile_knowledge(player_t *pplayer, tile_t *ptile);
void update_tile_knowledge(tile_t *ptile);
void update_player_tile_last_seen(player_t *pplayer, tile_t *ptile);

void give_shared_vision(player_t *pfrom, player_t *pto);
void remove_shared_vision(player_t *pfrom, player_t *pto);

void enable_fog_of_war(void);
void disable_fog_of_war(void);

void map_update_borders_city_destroyed(tile_t *ptile);
void map_update_borders_city_change(city_t *pcity);
void map_update_borders_landmass_change(tile_t *ptile);
void map_calculate_borders(void);

enum ocean_land_change check_terrain_ocean_land_change(tile_t *ptile,
                                              Terrain_type_id oldter);
int get_continent_size(Continent_id id);
int get_ocean_size(Continent_id id);

void assign_continent_flood(tile_t *ptile, bool is_land,
                            int nr, bool skip_unsafe);

#endif  /* WC_SERVER_MAPHAND_H */
