/**********************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef WC_COMMON_FC_TYPES_H
#define WC_COMMON_FC_TYPES_H

#include "shared.h"     /* bool type */

/* This file serves to reduce the cross-inclusion of header files which
 * occurs when a type which is defined in one file is needed for a fuction
 * definition in another file */

typedef signed short Continent_id;
typedef int Terrain_type_id;
typedef enum specialist_type Specialist_type_id;
typedef int Impr_Type_id;

struct city;
struct government;
struct player;
struct tile;
struct trade_route;
struct unit;

/* Changing these will probably break network compatability. */
#define MAX_LEN_DEMOGRAPHY 16
#define MAX_LEN_ALLOW_TAKE 16
#define MAX_ID_LEN 33
#define MAX_GRANARY_INIS 24
#define MAX_LEN_STARTUNIT (20 + 1)

/*
 * Number of traderoutes a city can have in standard Freeciv.
 */
#define OLD_NUM_TRADEROUTES     4

/* Server setting types.  Changing these will break network compatability. */
enum sset_type {
  SSET_BOOL, SSET_INT, SSET_STRING
};

enum server_states { 
  PRE_GAME_STATE, 
  SELECT_RACES_STATE, 
  RUN_GAME_STATE,
  GAME_OVER_STATE
};

enum client_states { 
  CLIENT_BOOT_STATE,
  CLIENT_PRE_GAME_STATE,
  CLIENT_SELECT_RACE_STATE,
  CLIENT_WAITING_FOR_GAME_START_STATE,
  CLIENT_GAME_RUNNING_STATE,
  CLIENT_GAME_OVER_STATE
};

/* The direction8 gives the 8 possible directions.  These may be used in
 * a number of ways, for instance as an index into the DIR_DX/DIR_DY
 * arrays.  Not all directions may be valid; see is_valid_dir and
 * is_cardinal_dir. */
enum direction8 {
  /* The DIR8/direction8 naming system is used to avoid conflict with
   * DIR4/direction4 in client/tilespec.h
   *
   * Changing the order of the directions will break network compatability.
   *
   * Some code assumes that the first 4 directions are the reverses of the
   * last 4 (in no particular order).  See client/goto.c. */
  DIR8_NORTHWEST = 0,
  DIR8_NORTH = 1,
  DIR8_NORTHEAST = 2,
  DIR8_WEST = 3,
  DIR8_EAST = 4,
  DIR8_SOUTHWEST = 5,
  DIR8_SOUTH = 6,
  DIR8_SOUTHEAST = 7
};
#define DIR8_LAST 8
#define DIR8_COUNT DIR8_LAST

enum production_class_type {
  TYPE_UNIT,
  TYPE_NORMAL_IMPROVEMENT,
  TYPE_WONDER
};

enum city_tile_type {
  C_TILE_EMPTY,
  C_TILE_WORKER,
  C_TILE_UNAVAILABLE
};

enum specialist_type {
  SP_ELVIS,
  SP_SCIENTIST,
  SP_TAXMAN,
  SP_COUNT
};

/* Changing this requires updating CITY_TILES and network capabilities. */
#define CITY_MAP_RADIUS 2

/* Diameter of the workable city area.  Some places harcdode this number. */
#define CITY_MAP_SIZE (CITY_MAP_RADIUS * 2 + 1) 

enum cm_stat {
  CM_FOOD,
  CM_SHIELD,
  CM_TRADE,
  CM_GOLD,
  CM_LUXURY,
  CM_SCIENCE,
  CM_NUM_STATS
};

/* A description of the goal. */
struct cm_parameter {
  int minimal_surplus[CM_NUM_STATS];
  bool require_happy;
  bool allow_disorder;
  bool allow_specialists;

  int factor[CM_NUM_STATS];
  int happy_factor;
};

/* Link definitions */
#define LINK_PREFIX '@'
#define TILE_LINK_LETTER 'L'
#define TILE_LINK_PREFIX "@L"
#define CITY_NAME_LINK_LETTER 'C'
#define CITY_NAME_LINK_PREFIX "@C"
#define CITY_ID_LINK_LETTER 'I'
#define CITY_ID_LINK_PREFIX "@I"
#define CITY_LINK_LETTER 'F'
#define CITY_LINK_PREFIX "@F"
#define UNIT_LINK_LETTER 'U'
#define UNIT_LINK_PREFIX "@U"

#endif  /* WC_COMMON_FC_TYPES_H */
