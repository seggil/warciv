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

#ifdef HAVE_CONFIG_H
#  include "../config.hh"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wc_intl.hh"
#include "log.hh"
#include "mem.hh"
#include "rand.hh"
#include "registry.hh"
#include "shared.hh"
#include "support.hh"

#include "capability.hh"
#include "city.hh"
#include "game.hh"
#include "government.hh"
#include "map.hh"
#include "traderoute.hh"
#include "unit.hh"
#include "version.hh"

#include "aicity.hh"
#include "aidata.hh"
#include "aiunit.hh"

#include "cityturn.hh"
#include "connecthand.hh" /* server_assign_random_nation */
#include "diplhand.hh"
#include "generator/mapgen.hh"
#include "maphand.hh"
#include "meta.hh"
#include "plrhand.hh"
#include "ruleset.hh"
#include "load_save_aux.hh"
#include "game_save.hh"
#include "srv_main.hh"
#include "stdinhand.hh"
#include "tradehand.hh"
#include "load_save_aux.hh"

/*
 * This loops over the entire map to save data. It collects all the data of
 * a line using GET_XY_CHAR and then executes the macro SECFILE_INSERT_LINE.
 * Internal variables map_x, map_y, nat_x, nat_y, and line are allocated
 * within the macro but definable by the caller of the macro.
 *
 * Parameters:
 *   line: buffer variable to hold a line of chars
 *   map_x, map_y: variables for internal map coordinates
 *   nat_x, nat_y: variables for output/native coordinates
 *   GET_XY_CHAR: macro returning the map character for each position
 *   SECFILE_INSERT_LINE: macro to output each processed line (line nat_y)
 *
 * Note: don't use this macro DIRECTLY, use
 * SAVE_NORMAL_MAP_DATA or SAVE_PLAYER_MAP_DATA instead.
 */
#define SAVE_MAP_DATA(ptile, line,                                          \
                      GET_XY_CHAR, SECFILE_INSERT_LINE)                     \
{                                                                           \
  char line[map.info.xsize + 1];                                            \
  unsigned int _nat_x, _nat_y;                                              \
                                                                            \
  for (_nat_y = 0; _nat_y < map.info.ysize; _nat_y++) {                     \
    for (_nat_x = 0; _nat_x < map.info.xsize; _nat_x++) {                   \
      tile_t *ptile = native_pos_to_tile(_nat_x, _nat_y);                   \
                                                                            \
      line[_nat_x] = (GET_XY_CHAR);                                         \
      if (!my_isprint(line[_nat_x] & 0x7f)) {                               \
          die("Trying to write invalid map "                                \
              "data: '%c' %d", line[_nat_x], line[_nat_x]);                 \
      }                                                                     \
    }                                                                       \
    line[map.info.xsize] = '\0';                                            \
    (SECFILE_INSERT_LINE);                                                  \
  }                                                                         \
}

/*
 * Wrappers for SAVE_MAP_DATA.
 *
 * SAVE_NORMAL_MAP_DATA saves a standard line of map data.
 *
 * SAVE_PLAYER_MAP_DATA saves a line of map data from a playermap.
 */
#define SAVE_NORMAL_MAP_DATA(ptile, secfile, secname, GET_XY_CHAR)          \
  SAVE_MAP_DATA(ptile, _line, GET_XY_CHAR,                                  \
                secfile_insert_str(secfile, _line, secname, _nat_y))

#define SAVE_PLAYER_MAP_DATA(ptile, secfile, secname, plrno,                \
                             GET_XY_CHAR)                                   \
  SAVE_MAP_DATA(ptile, _line, GET_XY_CHAR,                                  \
                secfile_insert_str(secfile, _line, secname, plrno, _nat_y))

/* The following should be removed when compatibility with
   pre-1.13.0 savegames is broken: startoptions, spacerace2
   and rulesets */
#define SAVEFILE_OPTIONS "startoptions spacerace2 rulesets" \
" diplchance_percent worklists2 map_editor known32fix turn " \
"attributes watchtower rulesetdir client_worklists orders " \
"startunits turn_last_built improvement_order technology_order " \
"wcdb_save"

/****************************************************************************
  References the terrain character.  See tile_types[].identifier
    example: terrain2char(T_ARCTIC) => 'a'
****************************************************************************/
static char terrain2char(Terrain_type_id terr)
{
  if (terr == OLD_TERRAIN_UNKNOWN) {
    return UNKNOWN_TERRAIN_IDENTIFIER;
  } else {
    assert(terr >= OLD_TERRAIN_FIRST && terr < OLD_TERRAIN_COUNT);
    return get_tile_type(terr)->identifier;
  }
}

/****************************************************************************
  Returns a character identifier for an order.  See also char2order.
****************************************************************************/
static char order2char(enum unit_orders order)
{
  switch (order) {
  case ORDER_MOVE:
    return 'm';
  case ORDER_FULL_MP:
    return 'w';
  case ORDER_ACTIVITY:
    return 'a';
  case ORDER_LAST:
    break;
  }

  assert(0);
  return '?';
}

/****************************************************************************
  Returns a character identifier for a direction.  See also char2dir.
****************************************************************************/
static char dir2char(enum direction8 dir)
{
  /* Numberpad values for the directions. */
  switch (dir) {
  case DIR8_NORTH:
    return '8';
  case DIR8_SOUTH:
    return '2';
  case DIR8_EAST:
    return '6';
  case DIR8_WEST:
    return '4';
  case DIR8_NORTHEAST:
    return '9';
  case DIR8_NORTHWEST:
    return '7';
  case DIR8_SOUTHEAST:
    return '3';
  case DIR8_SOUTHWEST:
    return '1';
  }

  assert(0);
  return '?';
}

/***************************************************************
Quote the memory block denoted by data and length so it consists only
of " a-f0-9:". The returned string has to be freed by the caller using
free().
***************************************************************/
static char *quote_block(const void *const data, int length)
{
  char *buffer = (char*)wc_malloc(length * 3 + 10);
  size_t offset;
  int i;

  sprintf(buffer, "%d:", length);
  offset = strlen(buffer);

  for (i = 0; i < length; i++) {
    sprintf(buffer + offset, "%02x ", ((unsigned char *) data)[i]);
    offset += 3;
  }
  return buffer;
}

/***************************************************************
...
***************************************************************/
static void map_save(struct section_file *file)
{
  unsigned int i;

  /* map.xsize and map.ysize (saved as map.width and map.height)
   * are now always saved in game_save()
   */

  /* Old warcivs expect map.is_earth to be present in the savegame. */
  secfile_insert_bool(file, FALSE, "map.is_earth");

  secfile_insert_bool(file, game.server.save_options.save_starts,
                      "game.save_starts");
  if (game.server.save_options.save_starts) {
    for (i = 0; i < map.server.num_start_positions; i++) {
      tile_t *ptile = map.server.start_positions[i].tile;

      secfile_insert_int(file, ptile->nat_x, "map.r%dsx", i);
      secfile_insert_int(file, ptile->nat_y, "map.r%dsy", i);

      if (map.server.start_positions[i].nation != NO_NATION_SELECTED) {
        const char *nation =
          get_nation_name_orig(map.server.start_positions[i].nation);

        secfile_insert_str(file, nation, "map.r%dsnation", i);
      }
    }
  }

  whole_map_iterate(ptile) {
    if (ptile->spec_sprite) {
      secfile_insert_str(file, ptile->spec_sprite, "map.spec_sprite_%d_%d",
                         ptile->nat_x, ptile->nat_y);
    }
  } whole_map_iterate_end;

  /* put the terrain type */
  SAVE_NORMAL_MAP_DATA(ptile, file, "map.t%03d",
                       terrain2char(ptile->terrain));

  if (!map.server.have_specials) {
    if (map.server.have_rivers_overlay) {
      /*
       * Save the rivers overlay map; this is a special case to allow
       * re-saving scenarios which have rivers overlay data.  This only
       * applies if don't have rest of specials.
       */

      /* bits 8-11 of special flags field */
      SAVE_NORMAL_MAP_DATA(ptile, file, "map.n%03d",
                           bin2ascii_hex(ptile->alteration, 2));
    }
    return;
  }

  /* put 4-bit segments of 12-bit "special flags" field */
  SAVE_NORMAL_MAP_DATA(ptile, file, "map.l%03d",
                       bin2ascii_hex(ptile->alteration, 0));
  SAVE_NORMAL_MAP_DATA(ptile, file, "map.u%03d",
                       bin2ascii_hex(ptile->alteration, 1));
  SAVE_NORMAL_MAP_DATA(ptile, file, "map.n%03d",
                       bin2ascii_hex(ptile->alteration, 2));

  secfile_insert_bool(file, game.server.save_options.save_known, "game.save_known");
  if (game.server.save_options.save_known) {
    /* put the top 4 bits (bits 12-15) of special flags */
    SAVE_NORMAL_MAP_DATA(ptile, file, "map.f%03d",
                         bin2ascii_hex(ptile->alteration, 3));

    /* put 4-bit segments of the 32-bit "known" field */
    SAVE_NORMAL_MAP_DATA(ptile, file, "map.a%03d",
                         bin2ascii_hex(ptile->u.server.known, 0));
    SAVE_NORMAL_MAP_DATA(ptile, file, "map.b%03d",
                         bin2ascii_hex(ptile->u.server.known, 1));
    SAVE_NORMAL_MAP_DATA(ptile, file, "map.c%03d",
                         bin2ascii_hex(ptile->u.server.known, 2));
    SAVE_NORMAL_MAP_DATA(ptile, file, "map.d%03d",
                         bin2ascii_hex(ptile->u.server.known, 3));
    SAVE_NORMAL_MAP_DATA(ptile, file, "map.e%03d",
                         bin2ascii_hex(ptile->u.server.known, 4));
    SAVE_NORMAL_MAP_DATA(ptile, file, "map.g%03d",
                         bin2ascii_hex(ptile->u.server.known, 5));
    SAVE_NORMAL_MAP_DATA(ptile, file, "map.h%03d",
                         bin2ascii_hex(ptile->u.server.known, 6));
    SAVE_NORMAL_MAP_DATA(ptile, file, "map.i%03d",
                         bin2ascii_hex(ptile->u.server.known, 7));
  }
}

/****************************************************************************
  Nowadays unit types are saved by name, but old servers need the
  unit_type_id.  This function tries to find the correct _old_ id for the
  unit's type.  It is used when the unit is saved.
****************************************************************************/
static int old_unit_type_id(Unit_Type_id type)
{
  const char** types;
  int num_types, i;

  if (strcmp(game.server.rulesetdir, "civ1") == 0) {
    types = old_civ1_unit_types;
    num_types = old_civ1_unit_types_size;
  } else {
    types = old_default_unit_types;
    num_types = old_default_unit_types_size;
  }

  for (i = 0; i < num_types; i++) {
    if (mystrcasecmp(unit_name_orig(type), types[i]) == 0) {
      return i;
    }
  }

  /* It's a new unit. Savegame cannot be forward compatible so we can
   * return anything */
  return type;
}

/****************************************************************************
  Nowadays improvement types are saved by name, but old servers need the
  Impr_type_id.  This function tries to find the correct _old_ id for the
  improvements's type.  It is used when the improvement is saved.
****************************************************************************/
static int old_impr_type_id(Impr_Type_id type)
{
  int i;

  for (i = 0; i < old_impr_types_size; i++) {
    if (mystrcasecmp(get_improvement_name_orig(type),
                     old_impr_types[i]) == 0) {
      return i;
    }
  }

  /* It's a new improvement. Savegame cannot be forward compatible so we can
   * return anything */
  return type;
}

/****************************************************************************
  Initialize the old-style improvement bitvector so that all improvements
  are marked as not present.
****************************************************************************/
static void init_old_improvement_bitvector(char* bitvector)
{
  int i;

  for (i = 0; i < old_impr_types_size; i++) {
    bitvector[i] = '0';
  }
  bitvector[old_impr_types_size] = '\0';
}

/****************************************************************************
  Insert improvement into old-style bitvector

  Improvement lists in cities and destroyed_wonders are saved as a
  bitvector in a string array.  New bitvectors do not depend on ruleset
  order. However, we want to create savegames which can be read by
  1.14.x and earlier servers.  This function adds an improvement into the
  bitvector string according to the 1.14.1 improvement ordering.
****************************************************************************/
static void add_improvement_into_old_bitvector(char* bitvector,
                                               Impr_Type_id id)
{
  int old_id;

  old_id = old_impr_type_id(id);
  if (old_id < 0 || old_id >= old_impr_types_size) {
    return;
  }
  bitvector[old_id] = '1';
}

/****************************************************************************
  Nowadays techs are saved by name, but old servers need numbers
  This function tries to find the correct _old_ id for the
  technology. It is used when the technology is saved.
****************************************************************************/
static int old_tech_id(Tech_Type_id tech)
{
  const char* technology_name;
  int i;

  /* old (1.14.1) servers used to save it as 0 and interpret it from context */
  if (is_future_tech(tech)) {
    return 0;
  }

  /* old (1.14.1) servers used to save it as 0 and interpret it from context */
  if (tech == A_UNSET) {
    return 0;
  }

  technology_name = advances[tech].name_orig;

  /* this is the only place where civ1 was different from 1.14.1 defaults */
  if (strcmp(game.server.rulesetdir, "civ1") == 0
      && mystrcasecmp(technology_name, "Religion") == 0) {
    return 83;
  }

  for (i = 0; i < old_default_techs_size; i++) {
    if (mystrcasecmp(technology_name, old_default_techs[i]) == 0) {
      return i;
    }
  }

  /* It's a new technology. Savegame cannot be forward compatible so we can
   * return anything */
  return tech;
}

/****************************************************************************
  Initialize the old-style technology bitvector so that all advances
  are marked as not present.
****************************************************************************/
static void init_old_technology_bitvector(char* bitvector)
{
  int i;

  for (i = 0; i < old_default_techs_size; i++) {
    bitvector[i] = '0';
  }
  bitvector[old_default_techs_size] = '\0';
}

/****************************************************************************
  Insert technology into old-style bitvector

  New bitvectors do not depend on ruleset order. However, we want to create
  savegames which can be read by 1.14.x and earlier servers.
  This function adds a technology into the bitvector string according
  to the 1.14.1 technology ordering.
****************************************************************************/
static void add_technology_into_old_bitvector(char* bitvector,
                                              Tech_Type_id id)
{
  int old_id;

  old_id = old_tech_id(id);
  if (old_id < 0 || old_id >= old_default_techs_size) {
    return;
  }
  bitvector[old_id] = '1';
}

/*****************************************************************************
  Save technology in secfile entry called path_name and for forward
  compatibility in path(by number).
*****************************************************************************/
static void save_technology(struct section_file *file,
                            const char* path, int plrno, Tech_Type_id tech)
{
  char path_with_name[128];
  const char* name;

  my_snprintf(path_with_name, sizeof(path_with_name),
              "%s_name", path);

  switch (tech) {
    case -1: /* used in changed_from */
       name = "";
       break;
    case A_NONE:
      name = "A_NONE";
      break;
    case A_UNSET:
      name = "A_UNSET";
      break;
    case A_FUTURE:
      name = "A_FUTURE";
      break;
    default:
      name = advances[tech].name_orig;
      break;
  }
  secfile_insert_str(file, name, path_with_name, plrno);
  secfile_insert_int(file, old_tech_id(tech), path, plrno);
}

/****************************************************************************
  Nowadays governments are saved by name, but old servers need the
  index.  This function tries to find the correct _old_ id for the
  government. It is used when the government is saved.
****************************************************************************/
static int old_government_id(struct government *gov)
{
  const char** names;
  int num_names, i;

  if (strcmp(game.server.rulesetdir, "civ2") == 0) {
    names = old_civ2_governments;
    num_names = old_civ2_governments_size;
  } else {
    names = old_default_governments;
    num_names = old_civ2_governments_size;
  }

  for (i = 0; i < num_names; i++) {
    if (mystrcasecmp(gov->name_orig, names[i]) == 0) {
      return i;
    }
  }

  /* It's a new government. Savegame cannot be forward compatible so we can
   * return anything */
  return gov->index;
}

/***************************************************************
Save the worklist elements specified by path, given the arguments
plrno and wlinx, from the worklist pointed to by pwl.
***************************************************************/
static void worklist_save(struct section_file *file,
                          const char *path, int plrno, int wlinx,
                          struct worklist *pwl)
{
  char efpath[64];
  char idpath[64];
  char namepath[64];
  int i;

  sz_strlcpy(efpath, path);
  sz_strlcat(efpath, ".wlef%d");
  sz_strlcpy(idpath, path);
  sz_strlcat(idpath, ".wlid%d");
  sz_strlcpy(namepath, path);
  sz_strlcat(namepath, ".wlname%d");

  for (i = 0; i < MAX_LEN_WORKLIST; i++) {
    secfile_insert_int(file, pwl->wlefs[i], efpath, plrno, wlinx, i);
    if (pwl->wlefs[i] == WEF_UNIT) {
      secfile_insert_int(file, old_unit_type_id(pwl->wlids[i]), idpath,
                         plrno, wlinx, i);
      secfile_insert_str(file, unit_name_orig(pwl->wlids[i]), namepath, plrno,
                         wlinx, i);
    } else if (pwl->wlefs[i] == WEF_IMPR) {
      secfile_insert_int(file, pwl->wlids[i], idpath, plrno, wlinx, i);
      secfile_insert_str(file, get_improvement_name_orig(pwl->wlids[i]),
                         namepath, plrno, wlinx, i);
    } else {
      secfile_insert_int(file, 0, idpath, plrno, wlinx, i);
      secfile_insert_str(file, "", namepath, plrno, wlinx, i);
    }
    if (pwl->wlefs[i] == WEF_END) {
      break;
    }
  }

  /* Fill out remaining worklist entries. */
  for (i++; i < MAX_LEN_WORKLIST; i++) {
    /* These values match what worklist_load fills in for unused entries. */
    secfile_insert_int(file, WEF_END, efpath, plrno, wlinx, i);
    secfile_insert_int(file, 0, idpath, plrno, wlinx, i);
    secfile_insert_str(file, "", namepath, plrno, wlinx, i);
  }
}

/***************************************************************
...
***************************************************************/
static void player_save(player_t *plr, int plrno,
                        struct section_file *file)
{
  int i;
  char invs[A_LAST+1];
  struct player_spaceship *ship;
  struct ai_data *ai;
  struct government *gov;

  if (!plr || !file) {
    return;
  }

  ship = &plr->spaceship;
  ai = ai_data_get(plr);

  secfile_insert_str(file, plr->name, "player%d.name", plrno);
  secfile_insert_str(file, plr->username, "player%d.username", plrno);
  secfile_insert_str(file, get_nation_name_orig(plr->nation),
                     "player%d.nation", plrno);
  /* 1.15 and later won't use the race field, they key on the nation string
   * This field is kept only for forward compatibility
   * Nations can't be saved correctly because race must be < 62 */
  secfile_insert_int(file, plrno, "player%d.race", plrno);
  if (plr->team != TEAM_NONE) {
    secfile_insert_str(file, get_team_name(plr->team),
                       "player%d.team", plrno);
  }

  gov = get_government(plr->government);
  secfile_insert_str(file, gov->name_orig, "player%d.government_name", plrno);
  /* 1.15 and later won't use "government" field; it's kept for forward
   * compatibility */
  secfile_insert_int(file, old_government_id(gov),
                     "player%d.government", plrno);

  if (plr->target_government != G_MAGIC) {
    gov = get_government(plr->target_government);
    secfile_insert_str(file, gov->name_orig,
                       "player%d.target_government_name", plrno);
  }

  secfile_insert_int(file, plr->embassy, "player%d.embassy", plrno);

  /* This field won't be used; it's kept only for forward compatibility.
   * City styles are specified by name since CVS 12/01-04. */
  secfile_insert_int(file, 0, "player%d.city_style", plrno);

  /* This is the new city style field to be used */
  secfile_insert_str(file, get_city_style_name_orig(plr->city_style),
                      "player%d.city_style_by_name", plrno);

  secfile_insert_bool(file, plr->is_male, "player%d.is_male", plrno);
  secfile_insert_bool(file, plr->is_alive, "player%d.is_alive", plrno);
  secfile_insert_bool(file, plr->ai.control, "player%d.ai.control", plrno);
  for (i = 0; i < MAX_NUM_PLAYERS; i++) {
    secfile_insert_int(file, plr->ai.love[i],
                       "player%d.ai%d.love", plrno, i);
    secfile_insert_int(file, ai->diplomacy.player_intel[i].spam,
                       "player%d.ai%d.spam", plrno, i);
    secfile_insert_int(file, ai->diplomacy.player_intel[i].ally_patience,
                       "player%d.ai%d.patience", plrno, i);
    secfile_insert_int(file, ai->diplomacy.player_intel[i].warned_about_space,
                       "player%d.ai%d.warn_space", plrno, i);
    secfile_insert_int(file, ai->diplomacy.player_intel[i].asked_about_peace,
                       "player%d.ai%d.ask_peace", plrno, i);
    secfile_insert_int(file, ai->diplomacy.player_intel[i].asked_about_alliance,
                       "player%d.ai%d.ask_alliance", plrno, i);
    secfile_insert_int(file, ai->diplomacy.player_intel[i].asked_about_ceasefire,
                       "player%d.ai%d.ask_ceasefire", plrno, i);
  }
  secfile_insert_int(file,
                     ai->diplomacy.target == NULL ?
                       -1 : ai->diplomacy.target->player_no,
                     "player%d.ai.target", plrno);
  save_technology(file, "player%d.ai.tech_goal", plrno, plr->ai.tech_goal);
  secfile_insert_int(file, plr->ai.skill_level,
                     "player%d.ai.skill_level", plrno);
  secfile_insert_int(file, plr->ai.barbarian_type, "player%d.ai.is_barbarian", plrno);
  secfile_insert_int(file, plr->economic.gold, "player%d.gold", plrno);
  secfile_insert_int(file, plr->economic.tax, "player%d.tax", plrno);
  secfile_insert_int(file, plr->economic.science, "player%d.science", plrno);
  secfile_insert_int(file, plr->economic.luxury, "player%d.luxury", plrno);

  secfile_insert_int(file,plr->future_tech,"player%d.futuretech", plrno);

  secfile_insert_int(file, plr->research.bulbs_researched,
                     "player%d.researched", plrno);
  secfile_insert_int(file, plr->research.bulbs_researched_before,
                     "player%d.researched_before", plrno);
  secfile_insert_bool(file, plr->got_tech,
                      "player%d.research_got_tech", plrno);
  save_technology(file, "player%d.research_changed_from", plrno,
                  plr->research.changed_from);
  secfile_insert_int(file, plr->research.techs_researched,
                     "player%d.researchpoints", plrno);
  save_technology(file, "player%d.researching", plrno,
                  plr->research.researching);

  secfile_insert_bool(file, plr->capital, "player%d.capital", plrno);

  secfile_insert_int(file, plr->revolution_finishes,
                     "player%d.revolution_finishes", plrno);
  {
    /* Insert the old-style "revolution" value, for forward compatibility.
     * See the loading code for more explanation. */
    int revolution;

    if (plr->revolution_finishes < 0) {
      /* No revolution. */
      revolution = 0;
    } else if (plr->revolution_finishes <= game.info.turn) {
      revolution = 1; /* Approximation. */
    } else {
      revolution = plr->revolution_finishes - game.info.turn;
    }
    secfile_insert_int(file, revolution, "player%d.revolution", plrno);
  }

  /* 1.14 servers depend on technology order in ruleset. Here we are trying
   * to simulate 1.14.1 default order */
  init_old_technology_bitvector(invs);
  tech_type_iterate(tech_id) {
    if (get_invention(plr, tech_id) == TECH_KNOWN) {
      add_technology_into_old_bitvector(invs, tech_id);
    }
  } tech_type_iterate_end;
  secfile_insert_str(file, invs, "player%d.invs", plrno);

  /* Save technology lists as bitvector. Note that technology order is
   * saved in savefile.technology_order */
  tech_type_iterate(tech_id) {
    invs[tech_id] = (get_invention(plr, tech_id) == TECH_KNOWN) ? '1' : '0';
  } tech_type_iterate_end;
  invs[game.ruleset_control.num_tech_types] = '\0';
  secfile_insert_str(file, invs, "player%d.invs_new", plrno);

  secfile_insert_int(file, plr->reputation, "player%d.reputation", plrno);
  for (i = 0; i < MAX_NUM_PLAYERS+MAX_NUM_BARBARIANS; i++) {
    secfile_insert_int(file, plr->diplstates[i].type,
                       "player%d.diplstate%d.type", plrno, i);
    secfile_insert_int(file, plr->diplstates[i].turns_left,
                       "player%d.diplstate%d.turns_left", plrno, i);
    secfile_insert_int(file, plr->diplstates[i].has_reason_to_cancel,
                       "player%d.diplstate%d.has_reason_to_cancel", plrno, i);
    secfile_insert_int(file, plr->diplstates[i].contact_turns_left,
                       "player%d.diplstate%d.contact_turns_left", plrno, i);
  }

  {
    char vision[MAX_NUM_PLAYERS+MAX_NUM_BARBARIANS+1];

    for (i=0; i < MAX_NUM_PLAYERS+MAX_NUM_BARBARIANS; i++)
      vision[i] = gives_shared_vision(plr, get_player(i)) ? '1' : '0';
    vision[i] = '\0';
    secfile_insert_str(file, vision, "player%d.gives_shared_vision", plrno);
  }

  /* Unit statistics. */
  secfile_insert_int(file, plr->score.units_built,
                     "player%d.units_built", plrno);
  secfile_insert_int(file, plr->score.units_killed,
                     "player%d.units_killed", plrno);
  secfile_insert_int(file, plr->score.units_lost,
                     "player%d.units_lost", plrno);

  secfile_insert_int(file, ship->state, "player%d.spaceship.state", plrno);

  if (ship->state != SSHIP_NONE) {
    char prefix[32];
    char st[NUM_SS_STRUCTURALS+1];
    int i;

    my_snprintf(prefix, sizeof(prefix), "player%d.spaceship", plrno);

    secfile_insert_int(file, ship->structurals, "%s.structurals", prefix);
    secfile_insert_int(file, ship->components, "%s.components", prefix);
    secfile_insert_int(file, ship->modules, "%s.modules", prefix);
    secfile_insert_int(file, ship->fuel, "%s.fuel", prefix);
    secfile_insert_int(file, ship->propulsion, "%s.propulsion", prefix);
    secfile_insert_int(file, ship->habitation, "%s.habitation", prefix);
    secfile_insert_int(file, ship->life_support, "%s.life_support", prefix);
    secfile_insert_int(file, ship->solar_panels, "%s.solar_panels", prefix);

    for(i=0; i<NUM_SS_STRUCTURALS; i++) {
      st[i] = (ship->structure[i]) ? '1' : '0';
    }
    st[i] = '\0';
    secfile_insert_str(file, st, "%s.structure", prefix);
    if (ship->state >= SSHIP_LAUNCHED) {
      secfile_insert_int(file, ship->launch_year, "%s.launch_year", prefix);
    }
  }

  secfile_insert_int(file, unit_list_size(plr->units), "player%d.nunits",
                     plrno);
  secfile_insert_int(file, city_list_size(plr->cities), "player%d.ncities",
                     plrno);

  i = -1;
  unit_list_iterate(plr->units, punit) {
    i++;
    secfile_insert_int(file, punit->id, "player%d.u%d.id", plrno, i);
    secfile_insert_int(file, punit->tile->nat_x, "player%d.u%d.x", plrno, i);
    secfile_insert_int(file, punit->tile->nat_y, "player%d.u%d.y", plrno, i);
    secfile_insert_int(file, punit->veteran, "player%d.u%d.veteran",
                                plrno, i);
    secfile_insert_int(file, punit->hp, "player%d.u%d.hp", plrno, i);
    secfile_insert_int(file, punit->homecity, "player%d.u%d.homecity",
                                plrno, i);
    /* .type is actually kept only for forward compatibility */
    secfile_insert_int(file, old_unit_type_id(punit->type),
                       "player%d.u%d.type",
                       plrno, i);
    secfile_insert_str(file, unit_name_orig(punit->type),
                       "player%d.u%d.type_by_name",
                       plrno, i);
    secfile_insert_int(file, punit->activity, "player%d.u%d.activity",
                                plrno, i);
    secfile_insert_int(file, punit->activity_count,
                                "player%d.u%d.activity_count",
                                plrno, i);
    secfile_insert_int(file, punit->activity_target,
                                "player%d.u%d.activity_target",
                                plrno, i);
    secfile_insert_bool(file, punit->done_moving,
                        "player%d.u%d.done_moving", plrno, i);
    secfile_insert_int(file, punit->moves_left, "player%d.u%d.moves",
                                plrno, i);
    secfile_insert_int(file, punit->fuel, "player%d.u%d.fuel",
                                plrno, i);

    if (punit->goto_tile) {
      secfile_insert_bool(file, TRUE, "player%d.u%d.go", plrno, i);
      secfile_insert_int(file, punit->goto_tile->nat_x,
                         "player%d.u%d.goto_x", plrno, i);
      secfile_insert_int(file, punit->goto_tile->nat_y,
                         "player%d.u%d.goto_y", plrno, i);
    } else {
      secfile_insert_bool(file, FALSE, "player%d.u%d.go", plrno, i);
      /* for compatibility with older servers */
      secfile_insert_int(file, 0, "player%d.u%d.goto_x", plrno, i);
      secfile_insert_int(file, 0, "player%d.u%d.goto_y", plrno, i);
    }

    secfile_insert_bool(file, punit->ai.control, "player%d.u%d.ai", plrno, i);
    secfile_insert_int(file, punit->ord_map, "player%d.u%d.ord_map", plrno, i);
    secfile_insert_int(file, punit->ord_city, "player%d.u%d.ord_city", plrno, i);
    secfile_insert_bool(file, punit->moved, "player%d.u%d.moved", plrno, i);
    secfile_insert_bool(file, punit->paradropped, "player%d.u%d.paradropped", plrno, i);
    secfile_insert_int(file, punit->transported_by,
                       "player%d.u%d.transported_by", plrno, i);

    /* Trade route */
    if (punit->ptr) {
      secfile_insert_int(file, punit->ptr->pcity1->common.id,
                         "player%d.u%d.trade_route_c1", plrno, i);
      secfile_insert_int(file, punit->ptr->pcity2->common.id,
                         "player%d.u%d.trade_route_c2", plrno, i);
    } else {
      secfile_insert_int(file, -1, "player%d.u%d.trade_route_c1", plrno, i);
      secfile_insert_int(file, -1, "player%d.u%d.trade_route_c2", plrno, i);
    }

    /* Air patrol */
    if (punit->air_patrol_tile) {
      secfile_insert_int(file, punit->air_patrol_tile->x,
                         "player%d.u%d.air_patrol_x", plrno, i);
      secfile_insert_int(file, punit->air_patrol_tile->y,
                         "player%d.u%d.air_patrol_y", plrno, i);
    } else {
      secfile_insert_int(file, -1, "player%d.u%d.air_patrol_x", plrno, i);
      secfile_insert_int(file, -1, "player%d.u%d.air_patrol_y", plrno, i);
    }

    if (punit->has_orders) {
      int len = punit->orders.length, j;
      char orders_buf[len + 1], dir_buf[len + 1], act_buf[len + 1];

      secfile_insert_int(file, len, "player%d.u%d.orders_length", plrno, i);
      secfile_insert_int(file, punit->orders.index,
                         "player%d.u%d.orders_index", plrno, i);
      secfile_insert_bool(file, punit->orders.repeat,
                          "player%d.u%d.orders_repeat", plrno, i);
      secfile_insert_bool(file, punit->orders.vigilant,
                          "player%d.u%d.orders_vigilant", plrno, i);

      for (j = 0; j < len; j++) {
        orders_buf[j] = order2char(punit->orders.list[j].order);
        dir_buf[j] = '?';
        act_buf[j] = '?';
        switch (punit->orders.list[j].order) {
        case ORDER_MOVE:
          dir_buf[j] = dir2char(punit->orders.list[j].dir);
          break;
        case ORDER_ACTIVITY:
          act_buf[j] = activity2char(punit->orders.list[j].activity);
          break;
        case ORDER_FULL_MP:
        case ORDER_LAST:
          break;
        }
      }
      orders_buf[len] = dir_buf[len] = act_buf[len] = '\0';

      secfile_insert_str(file, orders_buf,
                         "player%d.u%d.orders_list", plrno, i);
      secfile_insert_str(file, dir_buf,
                         "player%d.u%d.dir_list", plrno, i);
      secfile_insert_str(file, act_buf,
                         "player%d.u%d.activity_list", plrno, i);
    } else {
      /* Put all the same fields into the savegame - otherwise the
       * registry code can't correctly use a tabular format and the
       * savegame will be bigger. */
      secfile_insert_int(file, 0, "player%d.u%d.orders_length", plrno, i);
      secfile_insert_int(file, 0, "player%d.u%d.orders_index", plrno, i);
      secfile_insert_bool(file, FALSE,
                          "player%d.u%d.orders_repeat", plrno, i);
      secfile_insert_bool(file, FALSE,
                          "player%d.u%d.orders_vigilant", plrno, i);
      secfile_insert_str(file, "-",
                         "player%d.u%d.orders_list", plrno, i);
      secfile_insert_str(file, "-",
                         "player%d.u%d.dir_list", plrno, i);
      secfile_insert_str(file, "-",
                         "player%d.u%d.activity_list", plrno, i);
    }
  } unit_list_iterate_end;

  i = -1;
  city_list_iterate(plr->cities, pcity) {
    int j, x, y;
    char buf[512];

    i++;
    secfile_insert_int(file, pcity->common.id, "player%d.c%d.id", plrno, i);
    secfile_insert_int(file, pcity->common.tile->nat_x, "player%d.c%d.x", plrno, i);
    secfile_insert_int(file, pcity->common.tile->nat_y, "player%d.c%d.y", plrno, i);
    secfile_insert_str(file, pcity->common.name, "player%d.c%d.name", plrno, i);
    secfile_insert_int(file, pcity->u.server.original, "player%d.c%d.original",
                       plrno, i);
    secfile_insert_int(file, pcity->common.pop_size, "player%d.c%d.size", plrno, i);
    secfile_insert_int(file, pcity->u.server.steal,
                       "player%d.c%d.steal", plrno, i);
    specialist_type_iterate(sp) {
      secfile_insert_int(file, pcity->common.specialists[sp],
                         "player%d.c%d.n%s", plrno, i,
                         game.ruleset_game.specialist_name[sp]);
    } specialist_type_iterate_end;

    /* Trade routes */
    j = 0;
    if (game.traderoute_info.maxtraderoutes <= OLD_NUM_TRADEROUTES) {
      /* Then save like the standard warciv.
       * If there are more trade routes, we cannot save or it will
       * crash the next standard server which will load this game. */
      established_trade_routes_iterate(pcity, ptr) {
        secfile_insert_int(file, ptr->pcity1 == pcity
                           ? ptr->pcity2->common.id
                           : ptr->pcity1->common.id,
                           "player%d.c%d.traderoute%d", plrno, i, j);
        j++;
      } established_trade_routes_iterate_end;
    }
    /* Fill, for compatibility */
    while (j < OLD_NUM_TRADEROUTES) {
      secfile_insert_int(file, 0, "player%d.c%d.traderoute%d", plrno, i, j);
      j++;
    }

    /* Save warserver datas for trade routes.
     * Note: don't save the unit here,
     *       because the units are loaded after the cities. */
    j = 0;
    trade_route_list_iterate(pcity->common.trade_routes, ptr) {
      secfile_insert_int(file,
                         ptr->pcity1 == pcity
                         ? ptr->pcity2->common.id
                         : ptr->pcity1->common.id,
                         "player%d.c%dw.wc_trade_route%d_city", plrno, i, j);
      secfile_insert_int(file, ptr->status,
                         "player%d.c%dw.wc_trade_route%d_status", plrno, i, j);
      j++;
    } trade_route_list_iterate_end;
    secfile_insert_int(file, j, "player%d.c%d.wc_trade_route_num", plrno, i);

    /* Rally point */
    secfile_insert_int(file, pcity->common.rally_point ? pcity->common.rally_point->x : -1,
                       "player%d.c%d.rally_point_x", plrno, i);
    secfile_insert_int(file, pcity->common.rally_point ? pcity->common.rally_point->y : -1,
                       "player%d.c%d.rally_point_y", plrno, i);

    /* City Manager parameter */
    secfile_insert_bool(file, pcity->u.server.managed, "player%d.c%d.managed",
                        plrno, i);
    for (j = 0; j < CM_NUM_STATS; j++) {
      secfile_insert_int(file, pcity->u.server.parameter.minimal_surplus[j],
                         "player%d.c%dw.minimal_surplus%d", plrno, i, j);
      secfile_insert_int(file, pcity->u.server.parameter.factor[j],
                         "player%d.c%dw.factor%d", plrno, i, j);
    }
    secfile_insert_bool(file, pcity->u.server.parameter.require_happy,
                        "player%d.c%dw.require_happy", plrno, i);
    secfile_insert_bool(file, pcity->u.server.parameter.allow_disorder,
                        "player%d.c%dw.allow_disorder", plrno, i);
    secfile_insert_bool(file, pcity->u.server.parameter.allow_specialists,
                        "player%d.c%dw.allow_specialists", plrno, i);
    secfile_insert_int(file, pcity->u.server.parameter.happy_factor,
                       "player%d.c%dw.happy_factor", plrno, i);

    secfile_insert_int(file, pcity->common.food_stock, "player%d.c%d.food_stock",
                       plrno, i);
    secfile_insert_int(file, pcity->common.shield_stock, "player%d.c%d.shield_stock",
                       plrno, i);
    secfile_insert_int(file, pcity->common.turn_last_built,
                       "player%d.c%d.turn_last_built", plrno, i);
    secfile_insert_bool(file, pcity->common.changed_from_is_unit,
                       "player%d.c%d.changed_from_is_unit", plrno, i);
    if (pcity->common.changed_from_is_unit) {
      secfile_insert_int(file, old_unit_type_id(pcity->common.changed_from_id),
                         "player%d.c%d.changed_from_id", plrno, i);
      secfile_insert_str(file, unit_name_orig(pcity->common.changed_from_id),
                         "player%d.c%d.changed_from_name", plrno, i);
    } else {
      secfile_insert_int(file, old_impr_type_id(pcity->common.changed_from_id),
                         "player%d.c%d.changed_from_id", plrno, i);
      secfile_insert_str(file, get_improvement_name_orig(
                                 pcity->common.changed_from_id),
                         "player%d.c%d.changed_from_name", plrno, i);
    }

    secfile_insert_int(file, pcity->common.before_change_shields,
                       "player%d.c%d.before_change_shields", plrno, i);
    secfile_insert_int(file, pcity->common.disbanded_shields,
                       "player%d.c%d.disbanded_shields", plrno, i);
    secfile_insert_int(file, pcity->common.caravan_shields,
                       "player%d.c%d.caravan_shields", plrno, i);
    secfile_insert_int(file, pcity->common.last_turns_shield_surplus,
                       "player%d.c%d.last_turns_shield_surplus", plrno, i);

    secfile_insert_int(file, pcity->common.anarchy, "player%d.c%d.anarchy", plrno,i);
    secfile_insert_int(file, pcity->common.rapture, "player%d.c%d.rapture", plrno,i);
    secfile_insert_bool(file, pcity->common.was_happy, "player%d.c%d.was_happy", plrno,i);
    if (pcity->common.turn_founded == game.info.turn) {
      j = -1;
    } else {
      assert(pcity->common.did_buy == TRUE || pcity->common.did_buy == FALSE);
      j = pcity->common.did_buy ? 1 : 0;
    }
    secfile_insert_int(file, j, "player%d.c%d.did_buy", plrno, i);
    secfile_insert_int(file, pcity->common.turn_founded,
                       "player%d.c%d.turn_founded", plrno, i);
    secfile_insert_bool(file, pcity->common.did_sell, "player%d.c%d.did_sell", plrno,i);
    secfile_insert_bool(file, pcity->common.airlift, "player%d.c%d.airlift", plrno,i);

    /* for auto_attack */
    secfile_insert_int(file, pcity->common.city_options,
                       "player%d.c%d.options", plrno, i);

    j=0;
    for(y=0; y<CITY_MAP_SIZE; y++) {
      for(x=0; x<CITY_MAP_SIZE; x++) {
        switch (get_worker_city(pcity, x, y)) {
          case C_TILE_EMPTY:       buf[j++] = '0'; break;
          case C_TILE_WORKER:      buf[j++] = '1'; break;
          case C_TILE_UNAVAILABLE: buf[j++] = '2'; break;
        }
      }
    }
    buf[j]='\0';
    secfile_insert_str(file, buf, "player%d.c%d.workers", plrno, i);

    secfile_insert_bool(file, pcity->common.is_building_unit,
                       "player%d.c%d.is_building_unit", plrno, i);
    if (pcity->common.is_building_unit) {
      secfile_insert_int(file, old_unit_type_id(pcity->common.currently_building),
                         "player%d.c%d.currently_building", plrno, i);
      secfile_insert_str(file, unit_name_orig(pcity->common.currently_building),
                         "player%d.c%d.currently_building_name", plrno, i);
    } else {
      secfile_insert_int(file, old_impr_type_id(pcity->common.currently_building),
                         "player%d.c%d.currently_building", plrno, i);
      secfile_insert_str(file, get_improvement_name_orig(
                                   pcity->common.currently_building),
                         "player%d.c%d.currently_building_name", plrno, i);
    }

    /* 1.14 servers depend on improvement order in ruleset. Here we
     * are trying to simulate 1.14.1 default order
     */
    init_old_improvement_bitvector(buf);
    impr_type_iterate(id) {
      if (pcity->common.improvements[id] != I_NONE) {
        add_improvement_into_old_bitvector(buf, id);
      }
    } impr_type_iterate_end;
    secfile_insert_str(file, buf, "player%d.c%d.improvements", plrno, i);

    /* Save improvement list as bitvector. Note that improvement order
     * is saved in savefile.improvement_order.
     */
    impr_type_iterate(id) {
      buf[id] = (pcity->common.improvements[id] != I_NONE) ? '1' : '0';
    } impr_type_iterate_end;
    buf[game.ruleset_control.num_impr_types] = '\0';
    secfile_insert_str(file, buf, "player%d.c%d.improvements_new", plrno, i);

    worklist_save(file, "player%d.c%d", plrno, i, &pcity->common.worklist);

    /* FIXME: remove this when the urgency is properly recalculated. */
    secfile_insert_int(file, pcity->u.server.ai.urgency,
                       "player%d.c%d.ai.urgency", plrno, i);
  } city_list_iterate_end;

  /********** Put the players private map **********/
 /* Otherwise the player can see all, and there's no reason to save the private map. */
  if (game.server.fogofwar
      && game.server.save_options.save_private_map) {

    /* put the terrain type */
    SAVE_PLAYER_MAP_DATA(ptile, file,"player%d.map_t%03d", plrno,
                         terrain2char(map_get_player_tile
                                      (ptile, plr)->terrain));

    /* put 4-bit segments of 12-bit "special flags" field */
    SAVE_PLAYER_MAP_DATA(ptile, file,"player%d.map_l%03d", plrno,
                         bin2ascii_hex(map_get_player_tile(ptile, plr)->
                                       alteration, 0));
    SAVE_PLAYER_MAP_DATA(ptile, file, "player%d.map_u%03d", plrno,
                         bin2ascii_hex(map_get_player_tile(ptile, plr)->
                                       alteration, 1));
    SAVE_PLAYER_MAP_DATA(ptile, file, "player%d.map_n%03d", plrno,
                         bin2ascii_hex(map_get_player_tile(ptile, plr)->
                                       alteration, 2));

    /* put 4-bit segments of 16-bit "updated" field */
    SAVE_PLAYER_MAP_DATA(ptile, file,"player%d.map_ua%03d", plrno,
                         bin2ascii_hex(map_get_player_tile
                                       (ptile, plr)->last_updated, 0));
    SAVE_PLAYER_MAP_DATA(ptile, file, "player%d.map_ub%03d", plrno,
                         bin2ascii_hex(map_get_player_tile
                                       (ptile, plr)->last_updated, 1));
    SAVE_PLAYER_MAP_DATA(ptile, file,"player%d.map_uc%03d", plrno,
                         bin2ascii_hex(map_get_player_tile
                                       (ptile, plr)->last_updated, 2));
    SAVE_PLAYER_MAP_DATA(ptile, file, "player%d.map_ud%03d", plrno,
                         bin2ascii_hex(map_get_player_tile
                                       (ptile, plr)->last_updated, 3));

    if (TRUE) {
      struct dumb_city *pdcity;
      i = 0;

      whole_map_iterate(ptile) {
        if ((pdcity = map_get_player_tile(ptile, plr)->city)) {
          secfile_insert_int(file, pdcity->id, "player%d.dc%d.id", plrno,
                             i);
          secfile_insert_int(file, ptile->nat_x,
                             "player%d.dc%d.x", plrno, i);
          secfile_insert_int(file, ptile->nat_y,
                             "player%d.dc%d.y", plrno, i);
          secfile_insert_str(file, pdcity->name, "player%d.dc%d.name",
                             plrno, i);
          secfile_insert_int(file, pdcity->size, "player%d.dc%d.size",
                             plrno, i);
          secfile_insert_bool(file, pdcity->has_walls,
                             "player%d.dc%d.has_walls", plrno, i);
          secfile_insert_bool(file, pdcity->occupied,
                              "player%d.dc%d.occupied", plrno, i);
          secfile_insert_bool(file, pdcity->happy,
                              "player%d.dc%d.happy", plrno, i);
          secfile_insert_bool(file, pdcity->unhappy,
                              "player%d.dc%d.unhappy", plrno, i);
          secfile_insert_int(file, pdcity->owner, "player%d.dc%d.owner",
                             plrno, i);
          i++;
        }
      } whole_map_iterate_end;
    }
    secfile_insert_int(file, i, "player%d.total_ncities", plrno);
  }

  /* This is a big heap of opaque data from the client.  Although the binary
   * format is not user editable, keep the lines short enough for debugging,
   * and hope that data compression will keep the file a reasonable size.
   * Note that the "quoted" format is a multiple of 3.
   */
#define PART_SIZE (3*256)
  if (plr->attribute_block.data) {
    char *quoted = quote_block(plr->attribute_block.data,
                               plr->attribute_block.length);
    char *quoted_at = strchr(quoted, ':');
    size_t bytes_left = strlen(quoted);
    size_t bytes_at_colon = 1 + (quoted_at - quoted);
    size_t bytes_adjust = bytes_at_colon % 3;
    int current_part_nr;
    int parts;
    char part[PART_SIZE + 64];

    secfile_insert_int(file, plr->attribute_block.length,
                       "player%d.attribute_v2_block_length", plrno);
    secfile_insert_int(file, bytes_left,
                       "player%d.attribute_v2_block_length_quoted", plrno);

    /* Try to wring some compression efficiencies out of the "quoted" format.
     * The first line has a variable length decimal, mis-aligning triples.
     */
    if ((bytes_left - bytes_adjust) > PART_SIZE) {
      /* first line can be longer */
      parts = 1 + (bytes_left - bytes_adjust - 1) / PART_SIZE;
    } else {
      parts = 1;
    }

    secfile_insert_int(file, parts,
                       "player%d.attribute_v2_block_parts", plrno);

    if (parts > 1) {
      size_t size_of_current_part = MIN(sizeof(part) - 1,
                                        PART_SIZE + bytes_adjust);

      /* first line can be longer */
      memcpy(part, quoted, size_of_current_part);
      part[size_of_current_part] = '\0';
      secfile_insert_str(file, part,
                         "player%d.attribute_v2_block_data.part%d",
                         plrno,
                         0);
      bytes_left -= size_of_current_part;
      quoted_at = &quoted[size_of_current_part];
      current_part_nr = 1;
    } else {
      quoted_at = quoted;
      current_part_nr = 0;
    }

    for (; current_part_nr < parts; current_part_nr++) {
      size_t size_of_current_part = MIN(bytes_left, PART_SIZE);

      assert(bytes_left);

      memcpy(part, quoted_at, size_of_current_part);
      part[size_of_current_part] = '\0';
      secfile_insert_str(file, part,
                         "player%d.attribute_v2_block_data.part%d",
                         plrno,
                         current_part_nr);
      bytes_left -= size_of_current_part;
      quoted_at = &quoted_at[size_of_current_part];
    }
    assert(bytes_left == 0);
    free(quoted);
  }
#undef PART_SIZE

  secfile_insert_bool(file, plr->is_civil_war_split,
                      "player%d.is_civil_war_split", plrno);

  secfile_insert_int(file, plr->wcdb.player_id,
                     "player%d.fcdb_player_id", plrno);
}


/***************************************************************
 Assign values to ord_city and ord_map for each unit, so the
 values can be saved.
***************************************************************/
static void calc_unit_ordering(void)
{
  int j;

  players_iterate(pplayer) {
    /* to avoid junk values for unsupported units: */
    unit_list_iterate(pplayer->units, punit) {
      punit->ord_city = 0;
    } unit_list_iterate_end;
    city_list_iterate(pplayer->cities, pcity) {
      j = 0;
      unit_list_iterate(pcity->common.units_supported, punit) {
        punit->ord_city = j++;
      } unit_list_iterate_end;
    } city_list_iterate_end;
  } players_iterate_end;

  whole_map_iterate(ptile) {
    j = 0;
    unit_list_iterate(ptile->units, punit) {
      punit->ord_map = j++;
    } unit_list_iterate_end;
  } whole_map_iterate_end;
}

/***************************************************************
...
***************************************************************/
void game_save(struct section_file *file)
{
  unsigned int i;
  int version;
  char options[512];
  char temp[B_LAST+1];
  const char *user_message;

  version = MAJOR_VERSION *10000 + MINOR_VERSION *100 + PATCH_VERSION;
  secfile_insert_int(file, version, "game.version");

  /* Game state: once the game is no longer a new game (ie, has been
   * started the first time), it should always be considered a running
   * game for savegame purposes:
   */
  secfile_insert_int(file, (int) (game.server.is_new_game ? server_state :
                                  RUN_GAME_STATE), "game.server_state");

  secfile_insert_str(file, get_meta_patches_string(), "game.metapatches");
  secfile_insert_str(file, get_meta_topic_string(), "game.metatopic");
  secfile_insert_bool(file, game.server.meta_info.user_message_set,
                      "game.user_metamessage");
  user_message = get_user_meta_message_string();
  if (user_message != NULL) {
    secfile_insert_str(file, user_message, "game.metamessage");
  }
  secfile_insert_str(file, meta_addr_port(), "game.metaserver");

  sz_strlcpy(options, SAVEFILE_OPTIONS);
  if (game.server.is_new_game) {
    if (map.server.num_start_positions > 0) {
      sz_strlcat(options, " startpos");
    }
    if (map.server.have_specials) {
      sz_strlcat(options, " specials");
    }
    if (map.server.have_rivers_overlay && !map.server.have_specials) {
      sz_strlcat(options, " riversoverlay");
    }
  }
  secfile_insert_str(file, options, "savefile.options");
  /* Save improvement order in savegame, so we are not dependent on
   * ruleset order.
   * If the game isn't started improvements aren't loaded
   * so we can not save the order.
   */
  if (game.ruleset_control.num_impr_types > 0) {
    const char* buf[game.ruleset_control.num_impr_types];
    impr_type_iterate(id) {
      buf[id] = get_improvement_name_orig(id);
    } impr_type_iterate_end;
    secfile_insert_str_vec(file, buf, game.ruleset_control.num_impr_types,
                           "savefile.improvement_order");
  }

  /* Save technology order in savegame, so we are not dependent on ruleset
   * order. If the game isn't started advances aren't loaded
   * so we can not save the order. */
  if (game.ruleset_control.num_tech_types > 0) {
    const char* buf[game.ruleset_control.num_tech_types];
    tech_type_iterate(tech) {
      if (tech == A_NONE) {
        buf[tech] = "A_NONE";
      } else {
        buf[tech] = advances[tech].name_orig;
      }
    } tech_type_iterate_end;
    secfile_insert_str_vec(file, buf, game.ruleset_control.num_tech_types,
                           "savefile.technology_order");
  }

  secfile_insert_int(file, game.info.gold, "game.gold");
  secfile_insert_int(file, game.info.tech, "game.tech");
  secfile_insert_int(file, game.info.skill_level, "game.skill_level");
  secfile_insert_int(file, game.info.timeout, "game.timeout");
  secfile_insert_int(file, game.server.timeoutint, "game.timeoutint");
  secfile_insert_int(file, game.server.timeoutintinc, "game.timeoutintinc");
  secfile_insert_int(file, game.server.timeoutinc, "game.timeoutinc");
  secfile_insert_int(file, game.server.timeoutincmult, "game.timeoutincmult");
  secfile_insert_int(file, game.server.timeoutcounter, "game.timeoutcounter");
  secfile_insert_int(file, game.server.timeoutaddenemymove,
                     "game.timeoutaddenemymove");
  secfile_insert_int(file, game.info.end_year, "game.end_year");
  secfile_insert_int(file, game.info.year, "game.year");
  secfile_insert_int(file, game.info.turn, "game.turn");
  secfile_insert_int(file, game.info.researchcost, "game.researchcost");
  secfile_insert_int(file, game.info.min_players, "game.min_players");
  secfile_insert_int(file, game.info.max_players, "game.max_players");
  secfile_insert_int(file, game.info.nplayers, "game.nplayers");
  secfile_insert_int(file, game.info.globalwarming, "game.globalwarming");
  secfile_insert_int(file, game.server.warminglevel, "game.warminglevel");
  secfile_insert_int(file, game.info.nuclearwinter, "game.nuclearwinter");
  secfile_insert_int(file, game.server.coolinglevel, "game.coolinglevel");
  secfile_insert_int(file, game.ruleset_control.notradesize, "game.notradesize");
  secfile_insert_int(file, game.ruleset_control.fulltradesize, "game.fulltradesize");
  secfile_insert_int(file, game.info.unhappysize, "game.unhappysize");
  secfile_insert_bool(file, game.info.angrycitizen, "game.angrycitizen");
  secfile_insert_int(file, game.info.cityfactor, "game.cityfactor");
  secfile_insert_int(file, game.server.citymindist, "game.citymindist");
  secfile_insert_int(file, game.server.civilwarsize, "game.civilwarsize");
  secfile_insert_int(file, game.server.contactturns, "game.contactturns");
  secfile_insert_int(file, game.server.rapturedelay, "game.rapturedelay");
  secfile_insert_int(file, game.info.diplcost, "game.diplcost");
  secfile_insert_int(file, game.info.freecost, "game.freecost");
  secfile_insert_int(file, game.info.conquercost, "game.conquercost");
  secfile_insert_int(file, game.info.foodbox, "game.foodbox");
  secfile_insert_int(file, game.info.techpenalty, "game.techpenalty");
  secfile_insert_int(file, game.server.razechance, "game.razechance");
  secfile_insert_int(file, game.info.civstyle, "game.civstyle");
  secfile_insert_int(file, game.server.save_nturns, "game.save_nturns");
  secfile_insert_str(file, game.server.save_name, "game.save_name");
  secfile_insert_int(file, game.server.aifill, "game.aifill");
  secfile_insert_bool(file, game.server.scorelog, "game.scorelog");
  secfile_insert_str(file, game.server.id, "game.id");
  secfile_insert_bool(file, game.server.fogofwar, "game.fogofwar");
  secfile_insert_bool(file, game.info.spacerace, "game.spacerace");
  secfile_insert_bool(file, game.server.auto_ai_toggle, "game.auto_ai_toggle");
  secfile_insert_int(file, game.server.diplchance, "game.diplchance");
  secfile_insert_int(file, game.server.dipldefchance, "game.dipldefchance");
  secfile_insert_int(file, game.server.spyreturnchance, "game.spyreturnchance");
  secfile_insert_int(file, game.server.diplbribechance, "game.diplbribechance");
  secfile_insert_int(file, game.server.diplincitechance, "game.diplincitechance");
  secfile_insert_int(file, game.server.aqueductloss, "game.aqueductloss");
  secfile_insert_int(file, game.server.killcitizen, "game.killcitizen");
  secfile_insert_bool(file, game.server.turnblock, "game.turnblock");
  secfile_insert_bool(file, game.server.savepalace, "game.savepalace");
  secfile_insert_bool(file, game.server.fixedlength, "game.fixedlength");
  secfile_insert_int(file, game.server.barbarianrate, "game.barbarians");
  secfile_insert_int(file, game.server.onsetbarbarian, "game.onsetbarbs");
  secfile_insert_int(file, game.server.revolution_length, "game.revolen");
  secfile_insert_int(file, game.server.occupychance, "game.occupychance");
  secfile_insert_str(file, game.server.demography, "game.demography");
  save_user_allow_behavior_state(file);
  secfile_insert_int(file, game.ruleset_control.borders, "game.borders");
  secfile_insert_bool(file, game.ruleset_control.happyborders, "game.happyborders");
  secfile_insert_int(file, game.info.diplomacy, "game.diplomacy");
  secfile_insert_int(file, game.ext_info.maxallies, "game.maxallies");
  secfile_insert_int(file, game.server.watchtower_vision, "game.watchtower_vision");
  secfile_insert_int(file, game.server.watchtower_extra_vision, "game.watchtower_extra_vision");
  secfile_insert_int(file, game.server.allowed_city_names, "game.allowed_city_names");

  /* Warserver settings. */
  secfile_insert_int(file, game.traderoute_info.trademindist,
                     "game.trademindist");
  secfile_insert_int(file, game.traderoute_info.traderevenuepct,
                     "game.traderevenuepct");
  secfile_insert_int(file, game.traderoute_info.traderevenuestyle,
                     "game.traderevenuestyle");
  secfile_insert_int(file, game.traderoute_info.caravanbonusstyle,
                     "game.caravanbonusstyle");
  secfile_insert_int(file, game.traderoute_info.maxtraderoutes,
                     "game.maxtraderoutes");
  secfile_insert_bool(file, game.ext_info.futuretechsscore,
                      "game.futuretechsscore");
  secfile_insert_bool(file, game.ext_info.improvedautoattack,
                      "game.improvedautoattack");
  secfile_insert_bool(file, game.ext_info.stackbribing,
                      "game.stackbribing");
  secfile_insert_bool(file, game.ext_info.experimentalbribingcost,
                      "game.experimentalbribingcost");
  secfile_insert_bool(file, game.ext_info.techtrading,
                      "game.techtrading");
  secfile_insert_bool(file, game.ext_info.goldtrading,
                      "game.goldtrading");
  secfile_insert_bool(file, game.ext_info.citytrading,
                      "game.citytrading");
  secfile_insert_int(file, game.ext_info.airliftingstyle,
                     "game.airliftingstyle");
  secfile_insert_bool(file, game.ext_info.teamplacement,
                      "game.teamplacement");
  secfile_insert_bool(file, game.ext_info.globalwarmingon,
                      "game.globalwarmingon");
  secfile_insert_bool(file, game.ext_info.nuclearwinteron,
                      "game.nuclearwinteron");
  secfile_insert_int(file, game.server.bruteforcethreshold,
                     "game.bruteforcethreshold");
  secfile_insert_int(file, game.server.iterplacementcoefficient,
                     "game.iterplacementcoefficient");
  secfile_insert_int(file, game.server.teamplacementtype,
                     "game.teamplacementtype");
  secfile_insert_int(file, game.ext_info.techleakagerate,
                     "game.techleakagerate");
  secfile_insert_int(file, game.server.triremestyle,
                     "game.triremestyle");
  secfile_insert_int(file, game.server.fracmovestyle,
                     "game.fracmovestyle");
  secfile_insert_int(file, game.server.endturn,
                     "game.endturn");
  secfile_insert_int(file, game.server.revealmap,
                     "game.revealmap");
  secfile_insert_int(file, game.server.civilwar,
                     "game.civilwar");

  /* Warserver 'ignoreruleset' settings. */
  secfile_insert_bool(file, game.ext_info.ignoreruleset,
                      "game.ignoreruleset");
  if (game.ext_info.ignoreruleset) {
    secfile_insert_bool(file, game.ruleset_control.slow_invasions,
                        "game.slowinvasions");
    secfile_insert_bool(file, game.ruleset_game.killstack,
                        "game.killstack");
    secfile_insert_int(file, game.ruleset_game.tech_leakage,
                       "game.techleakage");
    secfile_insert_int(file, game.ruleset_game.tech_cost_style,
                       "game.techcoststyle");
  }

  /* WCDB related data that should be restored when the game is loaded. */
  secfile_insert_int(file, game.server.wcdb.id, "game.server_fcdb_id");
  secfile_insert_int(file, game.server.wcdb.type, "game.server_fcdb_type");

  /* old (1.14.1) servers need to have these server variables.  The values
   * don't matter, though. */
  secfile_insert_int(file, 2, "game.settlers");
  secfile_insert_int(file, 1, "game.explorer");
  secfile_insert_int(file, 30, "map.mountains");
  secfile_insert_int(file, 35, "map.grass");
  secfile_insert_int(file, 5, "map.swampsize");
  secfile_insert_int(file, 5, "map.deserts");
  secfile_insert_int(file, 5, "map.riverlength");
  secfile_insert_int(file, 20, "map.forestsize");

  if (TRUE) {
    /* Now always save these, so the server options reflect the
     * actual values used at the start of the game.
     * The first two used to be saved as "map.xsize" and "map.ysize"
     * when PRE_GAME_STATE, but I'm standardizing on width,height --dwp
     */
    secfile_insert_int(file, map.info.topology_id, "map.topology_id");
    secfile_insert_int(file, map.server.size, "map.size");
    secfile_insert_int(file, map.info.xsize, "map.width");
    secfile_insert_int(file, map.info.ysize, "map.height");
    secfile_insert_str(file, game.server.start_units, "game.start_units");
    secfile_insert_int(file, game.server.dispersion, "game.dispersion");
    secfile_insert_int(file, map.server.seed, "map.seed");
    secfile_insert_int(file, map.server.landpercent, "map.landpercent");
    secfile_insert_int(file, map.server.riches, "map.riches");
    secfile_insert_int(file, map.server.wetness, "map.wetness");
    secfile_insert_int(file, map.server.steepness, "map.steepness");
    secfile_insert_int(file, map.server.huts, "map.huts");
    secfile_insert_int(file, map.server.generator, "map.generator");
    secfile_insert_bool(file, map.server.have_huts, "map.have_huts");
    secfile_insert_int(file, map.server.temperature, "map.temperature");
    secfile_insert_bool(file, map.server.alltemperate, "map.alltemperate");
    secfile_insert_bool(file, map.server.tinyisles, "map.tinyisles");
    secfile_insert_bool(file, map.server.separatepoles, "map.separatepoles");
  }

  secfile_insert_int(file, game.server.seed, "game.randseed");

  if (myrand_is_init() && game.server.save_options.save_random) {
    RANDOM_STATE rstate = get_myrand_state();
    secfile_insert_int(file, 1, "game.save_random");
    assert(rstate.is_init);

    secfile_insert_int(file, rstate.j, "random.index_J");
    secfile_insert_int(file, rstate.k, "random.index_K");
    secfile_insert_int(file, rstate.x, "random.index_X");

    for (i = 0; i < 8; i++) {
      char name[20], vec[100];

      my_snprintf(name, sizeof(name), "random.table%d", i);
      my_snprintf(vec, sizeof(vec),
                  "%8x %8x %8x %8x %8x %8x %8x", rstate.v[7 * i],
                  rstate.v[7 * i + 1], rstate.v[7 * i + 2],
                  rstate.v[7 * i + 3], rstate.v[7 * i + 4],
                  rstate.v[7 * i + 5], rstate.v[7 * i + 6]);
      secfile_insert_str(file, vec, "%s", name);
    }
  } else {
    secfile_insert_int(file, 0, "game.save_random");
  }

  secfile_insert_str(file, game.server.rulesetdir, "game.rulesetdir");

  if (!map_is_empty()) {
    map_save(file);
  }

  if ((server_state == PRE_GAME_STATE) && game.server.is_new_game) {
    return; /* want to save scenarios as well */
  }

  secfile_insert_bool(file, game.server.save_options.save_players,
                      "game.save_players");
  if (game.server.save_options.save_players) {
    /* 1.14 servers depend on improvement order in ruleset. Here we
     * are trying to simulate 1.14.1 default order
     */
    init_old_improvement_bitvector(temp);
    impr_type_iterate(id) {
      if (is_wonder(id) && game.info.global_wonders[id] != 0
          && !find_city_by_id(game.info.global_wonders[id])) {
        add_improvement_into_old_bitvector(temp, id);
      }
    } impr_type_iterate_end;
    secfile_insert_str(file, temp, "game.destroyed_wonders");

    /* Save destroyed wonders as bitvector. Note that improvement order
     * is saved in savefile.improvement_order
     */
    impr_type_iterate(id) {
      if (is_wonder(id) && game.info.global_wonders[id] != 0
          && !find_city_by_id(game.info.global_wonders[id])) {
        temp[id] = '1';
      } else {
        temp[id] = '0';
      }
    } impr_type_iterate_end;
    temp[game.ruleset_control.num_impr_types] = '\0';
    secfile_insert_str(file, temp, "game.destroyed_wonders_new");

    calc_unit_ordering();

    players_iterate(pplayer) {
      player_save(pplayer, pplayer->player_no, file);
    } players_iterate_end;

    for (i = 0; i < game.info.nplayers; i++) {
      secfile_insert_int(file, shuffled_player(i)->player_no,
                         "game.shuffled_player_%d", i);
    }
  }
}
