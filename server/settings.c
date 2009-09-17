/********************************************************************** 
 Freeciv - Copyright (C) 1996-2004 - The Freeciv Project
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
#include <config.h>
#endif

#include "fcintl.h"
#include "log.h"
#include "support.h"

#include "connection.h"
#include "map.h"

#include "commands.h"
#include "gamelog.h"
#include "report.h"
#include "settings.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "vote.h"

/* Category names must match the values in enum sset_category. */
const char *sset_category_names[] = {N_("Geological"),
				     N_("Ecological"),
				     N_("Sociological"),
				     N_("Economic"),
				     N_("Military"),
				     N_("Scientific"),
				     N_("Internal"),
				     N_("Networking")};

/* Level names must match the values in enum sset_level. */
const char *sset_level_names[SSET_NUM_LEVELS] = {
  N_("None"),
  N_("All"),
  N_("Vital"),
  N_("Situational"),
  N_("Rare"),
  N_("Changed")
};


/**************************************************************************
  A callback invoked when autotoggle is set.
**************************************************************************/
static bool autotoggle_callback(bool value, const char **reject_message)
{
  reject_message = NULL;
  if (!value) {
    return TRUE;
  }

  players_iterate(pplayer) {
    if (!pplayer->ai.control && !pplayer->is_connected) {
      toggle_ai_player_direct(NULL, pplayer);
    }
  } players_iterate_end;

  return TRUE;
}

/*************************************************************************
  Verify that a given startunits string is valid.  See
  game.server.start_units.
*************************************************************************/
static bool startunits_callback(const char *value, const char **error_string)
{
  int len = strlen(value), i;
  bool have_founder = FALSE;

  /* We check each character individually to see if it's valid, and
   * also make sure there is at least one city founder. */

  for (i = 0; i < len; i++) {
    /* Check for a city founder */
    if (value[i] == 'c') {
      have_founder = TRUE;
      continue;
    }
    if (strchr("cwxksdDaA", value[i])) {
      continue;
    }

    /* Looks like the character was invalid. */
    *error_string = _("Starting units string contains invalid\n"
		      "characters.  Try \"help startunits\".");
    return FALSE;
  }

  if (!have_founder) {
    *error_string = _("Starting units string does not contain\n"
		      "at least one city founder.  Try \n"
		      "\"help startunits\".");
    return FALSE;
  }
  /* All characters were valid. */
  *error_string = NULL;
  return TRUE;
}

/*************************************************************************
  Verify that a given maxplayers string is valid.
*************************************************************************/
static bool maxplayers_callback(int value, const char **error_string)
{
  if (value < game.info.nplayers) {
    *error_string =_("Number of players is higher than requested value; "
		     "Keeping old value.");
    return FALSE;
  }

  error_string = NULL;
  return TRUE;
}

/*************************************************************************
  Verify that a given idlecut value is valid.
*************************************************************************/
static bool idlecut_callback(int value, const char **error_string)
{
  if (0 < value && value < 30) {
    *error_string =_("Idlecut cannot be less than 30 seconds.");
    return FALSE;
  }
  *error_string = NULL;
  return TRUE;
}

#ifdef HAVE_MYSQL
/*************************************************************************
  Verify that a given 'rated' value is valid.
*************************************************************************/
static bool rated_callback(bool value, const char **error)
{
  int gtype = game_determine_type();

  if (value && !game_type_supports_rating(gtype)) {
    *error = _("The current game type cannot be rated.");
    return FALSE;
  }
  *error = NULL;
  return TRUE;
}
#endif /* HAVE_MYSQL */



#define GEN_BOOL(name, value, sclass, scateg, slevel, to_client,        \
                 short_help, extra_help, func, default)                 \
  {name, sclass, to_client, short_help, extra_help, SSET_BOOL,          \
      scateg, slevel, "",                                               \
      &value, default, func,                                            \
      NULL, 0, NULL, 0, 0,                                              \
      NULL, NULL, NULL, 0,                                              \
      VCF_NONE, 0, -1, -1},

#define GEN_BOOL_FULL(name, value, sclass, scateg, slevel, to_client,   \
                      short_help, extra_help, func, default,            \
                      vote_flags, vote_percent, pregame_level,          \
                      game_level)                                       \
  {name, sclass, to_client, short_help, extra_help, SSET_BOOL,          \
      scateg, slevel, "",                                               \
      &value, default, func,                                            \
      NULL, 0, NULL, 0, 0,                                              \
      NULL, NULL, NULL, 0,                                              \
      vote_flags, vote_percent,                                         \
      pregame_level, game_level},

#define GEN_INT(name, value, sclass, scateg, slevel, to_client,         \
                short_help, extra_help, func, min, max, default)        \
  {name, sclass, to_client, short_help, extra_help, SSET_INT,           \
      scateg, slevel, "",                                               \
      NULL, FALSE, NULL,                                                \
      &value, default, func, min, max,                                  \
      NULL, NULL, NULL, 0,                                              \
      VCF_NONE, 0, -1, -1},

#define GEN_INT_FULL(name, value, sclass, scateg, slevel, to_client,    \
                     reqcap, short_help, extra_help, func, min,         \
                     max, default, vote_flags, vote_percent,            \
                     pregame_level, game_level)                         \
  {name, sclass, to_client, short_help, extra_help, SSET_INT,           \
      scateg, slevel, reqcap,                                           \
      NULL, FALSE, NULL,                                                \
      &value, default, func, min, max,                                  \
      NULL, NULL, NULL, 0,                                              \
      vote_flags, vote_percent,                                         \
      pregame_level, game_level},

#define GEN_STRING(name, value, sclass, scateg, slevel, to_client,      \
                   short_help, extra_help, func, default)               \
  {name, sclass, to_client, short_help, extra_help, SSET_STRING,        \
      scateg, slevel, "",                                               \
      NULL, FALSE, NULL,                                                \
      NULL, 0, NULL, 0, 0,                                              \
      value, default, func, sizeof(value),                              \
      VCF_NONE, 0, -1, -1},

#define GEN_STRING_FULL(name, value, sclass, scateg, slevel, to_client, \
                        short_help, extra_help, func, default,          \
                        vote_flags, vote_pc, pregame_level, game_level) \
  {name, sclass, to_client, short_help, extra_help, SSET_STRING,        \
      scateg, slevel, "",                                               \
      NULL, FALSE, NULL,                                                \
      NULL, 0, NULL, 0, 0,                                              \
      value, default, func, sizeof(value),                              \
      vote_flags, vote_pc, pregame_level, game_level},

#define GEN_END                                                         \
  {NULL, SSET_LAST, SSET_SERVER_ONLY, NULL, NULL, SSET_INT,             \
      SSET_NUM_CATEGORIES, SSET_NONE, "",                               \
      NULL, FALSE, NULL,                                                \
      NULL, 0, NULL, 0, 0,                                              \
      NULL, NULL, NULL, VCF_NONE, 0, -1, -1},
      
struct settings_s settings[] = {

  /* These should be grouped by sclass */
  
  /* Map size parameters: adjustable if we don't yet have a map */  
  GEN_INT("size", map.server.size, SSET_MAP_SIZE,
	  SSET_GEOLOGY, SSET_VITAL, SSET_TO_CLIENT,
          N_("Map size (in thousands of tiles)"),
          N_("This value is used to determine the map dimensions.\n"
             "  size = 4 is a normal map of 4,000 tiles (default)\n"
             "  size = 20 is a huge map of 20,000 tiles"), NULL,
          MAP_MIN_SIZE, MAP_MAX_SIZE, MAP_DEFAULT_SIZE)
  GEN_INT("autosize", map.server.autosize, SSET_MAP_SIZE,
	  SSET_GEOLOGY, SSET_VITAL, SSET_TO_CLIENT,
          N_("Tile number for each player"),
          N_("This value is used to determine the map dimensions.\n"
             "It calculs the size from the player number and the\n"
             "landmass. If this option is set to 0, it will be\n"
             "ignored. This option is usefull for islands\n"
             "generators."), NULL,
          MAP_MIN_AUTOSIZE, MAP_MAX_AUTOSIZE, MAP_DEFAULT_AUTOSIZE)
  GEN_INT("topology", map.info.topology_id, SSET_MAP_SIZE,
	  SSET_GEOLOGY, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Map topology index"),
	  /* TRANS: do not edit the ugly ASCII art */
	  N_("Freeciv maps are always two-dimensional. They may wrap at "
	     "the north-south and east-west directions to form a flat map, "
	     "a cylinder, or a torus (donut). Individual tiles may be "
	     "rectangular or hexagonal, with either a classic or isometric "
	     "alignment - this should be set based on the tileset being "
	     "used.\n"
             "   0 Flat Earth (unwrapped)\n"
             "   1 Earth (wraps E-W)\n"
             "   2 Uranus (wraps N-S)\n"
             "   3 Donut World (wraps N-S, E-W)\n"
	     "   4 Flat Earth (isometric)\n"
	     "   5 Earth (isometric)\n"
	     "   6 Uranus (isometric)\n"
	     "   7 Donut World (isometric)\n"
	     "   8 Flat Earth (hexagonal)\n"
	     "   9 Earth (hexagonal)\n"
	     "  10 Uranus (hexagonal)\n"
	     "  11 Donut World (hexagonal)\n"
	     "  12 Flat Earth (iso-hex)\n"
	     "  13 Earth (iso-hex)\n"
	     "  14 Uranus (iso-hex)\n"
	     "  15 Donut World (iso-hex)\n"
	     "Classic rectangular:       Isometric rectangular:\n"
	     "      _________               /\\/\\/\\/\\/\\ \n"
	     "     |_|_|_|_|_|             /\\/\\/\\/\\/\\/ \n"
	     "     |_|_|_|_|_|             \\/\\/\\/\\/\\/\\\n"
	     "     |_|_|_|_|_|             /\\/\\/\\/\\/\\/ \n"
	     "                             \\/\\/\\/\\/\\/  \n"
	     "Hex tiles:                 Iso-hex:\n"
	     "  /\\/\\/\\/\\/\\/\\               _   _   _   _   _       \n"
	     "  | | | | | | |             / \\_/ \\_/ \\_/ \\_/ \\      \n"
	     "  \\/\\/\\/\\/\\/\\/\\             \\_/ \\_/ \\_/ \\_/ \\_/  \n"
	     "   | | | | | | |            / \\_/ \\_/ \\_/ \\_/ \\      \n"
	     "   \\/\\/\\/\\/\\/\\/             \\_/ \\_/ \\_/ \\_/ \\_/    \n"
          ), NULL,
	  MAP_MIN_TOPO, MAP_MAX_TOPO, MAP_DEFAULT_TOPO)

  /* Map generation parameters: once we have a map these are of historical
   * interest only, and cannot be changed.
   */
  GEN_INT("generator", map.server.generator,
	  SSET_MAP_GEN, SSET_GEOLOGY, SSET_VITAL,  SSET_TO_CLIENT,
	  N_("Method used to generate map"),
	  N_("0 = Scenario map - no generator;\n"
	     "1 = Fully random height generator; [4]\n"
	     "2 = Pseudo-fractal height generator; [3]\n"
	     "3 = Island-based generator (fairer but boring) [1]\n"
             "4 = equally sized peninsulas with one player each surrounding\n"
             "    an inland sea.  An isthmus connects the polar regions.\n"
             "5 = same as 4 except roads are generated in the polar regions\n"
             "    and on the connecting isthmus;\n"
             "6 = fair island generator, each player start on the same island\n"
             "    with the same start position; [1]\n"
             "7 = same as 6 except the map generated with team placement\n"
             "    if \"teamplacement\" is turned to on. [1]\n"
	     "\n"
	     "Numbers in [] give the default values for placement of "
	     "starting positions.  If the default value of startpos is "
	     "used then a startpos setting will be chosen based on the "
	     "generator.  See the \"startpos\" setting."), NULL,
	  MAP_MIN_GENERATOR, MAP_MAX_GENERATOR, MAP_DEFAULT_GENERATOR)

  GEN_INT("startpos", map.server.startpos,
	  SSET_MAP_GEN, SSET_GEOLOGY, SSET_VITAL,  SSET_TO_CLIENT,
	  N_("Method used to choose start positions"),
	  N_("0 = Generator's choice.  Selecting this setting means\n"
	     "    the default value will be picked based on the generator\n"
	     "    chosen.  See the \"generator\" setting.\n"
	     "1 = Try to place one player per continent.\n"
	     "2 = Try to place two players per continent.\n"
	     "3 = Try to place all players on a single continent.\n"
	     "4 = Place players depending on size of continents.\n"
	     "Note: generators try to create the right number of continents "
	     "for the choice of start pos and to the number of players"),
	  NULL, MAP_MIN_STARTPOS, MAP_MAX_STARTPOS, MAP_DEFAULT_STARTPOS)

  GEN_BOOL("tinyisles", map.server.tinyisles,
	   SSET_MAP_GEN, SSET_GEOLOGY, SSET_RARE, SSET_TO_CLIENT,
	   N_("Presence of 1x1 islands"),
	   N_("0 = no 1x1 islands; 1 = some 1x1 islands"), NULL,
	   MAP_DEFAULT_TINYISLES)

  GEN_BOOL("separatepoles", map.server.separatepoles,
	   SSET_MAP_GEN, SSET_GEOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	   N_("Whether the poles are separate continents"),
	   N_("0 = continents may attach to poles; 1 = poles will "
	      "usually be separate"), NULL, 
	   MAP_DEFAULT_SEPARATE_POLES)

  GEN_BOOL("alltemperate", map.server.alltemperate, 
           SSET_MAP_GEN, SSET_GEOLOGY, SSET_RARE, SSET_TO_CLIENT,
	   N_("All the map is temperate"),
	   N_("0 = normal Earth-like planet; 1 = all-temperate planet "),
	   NULL, MAP_DEFAULT_ALLTEMPERATE)

  GEN_INT("temperature", map.server.temperature,
 	  SSET_MAP_GEN, SSET_GEOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
 	  N_("Average temperature of the planet"),
 	  N_("Small values will give a cold map, while larger values will "
             "give a hotter map.\n"
	     "\n"
	     "100 means a very dry and hot planet with no polar arctic "
	     "zones, only tropical and dry zones.\n\n"
	     "70 means a hot planet with little polar ice.\n\n"
             "50 means a temperate planet with normal polar, cold, "
	     "temperate, and tropical zones; a desert zone overlaps "
	     "tropical and temperate zones.\n\n"
	     "30 means a cold planet with small tropical zones.\n\n"
	     "0 means a very cold planet with large polar zones and no "
	     "tropics"), 
          NULL,
  	  MAP_MIN_TEMPERATURE, MAP_MAX_TEMPERATURE, MAP_DEFAULT_TEMPERATURE)
 
  GEN_INT("landmass", map.server.landpercent,
	  SSET_MAP_GEN, SSET_GEOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Percentage of the map that is land"),
	  N_("This setting gives the approximate percentage of the map "
	     "that will be made into land."), NULL,
	  MAP_MIN_LANDMASS, MAP_MAX_LANDMASS, MAP_DEFAULT_LANDMASS)

  GEN_INT("steepness", map.server.steepness,
	  SSET_MAP_GEN, SSET_GEOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Amount of hills/mountains"),
	  N_("Small values give flat maps, while higher values give a "
	     "steeper map with more hills and mountains."), NULL,
	  MAP_MIN_STEEPNESS, MAP_MAX_STEEPNESS, MAP_DEFAULT_STEEPNESS)

  GEN_INT("wetness", map.server.wetness,
 	  SSET_MAP_GEN, SSET_GEOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
 	  N_("Amount of water on lands"), 
	  N_("Small values mean lots of dry, desert-like land; "
	     "higher values give a wetter map with more swamps, "
	     "jungles, and rivers."), NULL, 
 	  MAP_MIN_WETNESS, MAP_MAX_WETNESS, MAP_DEFAULT_WETNESS)

  GEN_INT("mapseed", map.server.seed,
	  SSET_MAP_GEN, SSET_INTERNAL, SSET_RARE, SSET_SERVER_ONLY,
	  N_("Map generation random seed"),
	  N_("The same seed will always produce the same map; "
	     "for zero (the default) a seed will be chosen based on "
	     "the time to give a random map. This setting is usually "
	     "only of interest while debugging the game."), NULL, 
	  MAP_MIN_SEED, MAP_MAX_SEED, MAP_DEFAULT_SEED)

  /* Map additional stuff: huts and specials.  gameseed also goes here
   * because huts and specials are the first time the gameseed gets used (?)
   * These are done when the game starts, so these are historical and
   * fixed after the game has started.
   */
  GEN_INT("gameseed", game.server.seed,
	  SSET_MAP_ADD, SSET_INTERNAL, SSET_RARE, SSET_SERVER_ONLY,
	  N_("Game random seed"),
	  N_("For zero (the default) a seed will be chosen based "
	     "on the time. This setting is usually "
	     "only of interest while debugging the game"), NULL, 
	  GAME_MIN_SEED, GAME_MAX_SEED, GAME_DEFAULT_SEED)

  GEN_INT("specials", map.server.riches,
	  SSET_MAP_ADD, SSET_GEOLOGY, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Amount of \"special\" resource squares"), 
	  N_("Special resources improve the basic terrain type they "
	     "are on. The server variable's scale is parts per "
	     "thousand."), NULL,
	  MAP_MIN_RICHES, MAP_MAX_RICHES, MAP_DEFAULT_RICHES)

  GEN_INT("huts", map.server.huts,
	  SSET_MAP_ADD, SSET_GEOLOGY, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Amount of huts (minor tribe villages)"),
	  N_("This setting gives the exact number of huts that will be "
	     "placed on the entire map. Huts are small tribal villages "
	     "that may be investiged by units."), NULL,
	  MAP_MIN_HUTS, MAP_MAX_HUTS, MAP_DEFAULT_HUTS)

  /* Options affecting numbers of players and AI players.  These only
   * affect the start of the game and can not be adjusted after that.
   * (Actually, minplayers does also affect reloads: you can't start a
   * reload game until enough players have connected (or are AI).)
   */
  GEN_INT("minplayers", game.info.min_players,
	  SSET_PLAYERS, SSET_INTERNAL, SSET_VITAL,
          SSET_TO_CLIENT,
	  N_("Minimum number of players"),
	  N_("There must be at least this many players (connected "
	     "human players or AI players) before the game can start."),
	  NULL,
	  GAME_MIN_MIN_PLAYERS, GAME_MAX_MIN_PLAYERS, GAME_DEFAULT_MIN_PLAYERS)

  GEN_INT("maxplayers", game.info.max_players,
	  SSET_PLAYERS, SSET_INTERNAL, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Maximum number of players"),
          N_("The maximal number of human and AI players who can be in "
             "the game. When this number of players are connected in "
             "the pregame state, any new players who try to connect "
             "will become detached."), maxplayers_callback,
	  GAME_MIN_MAX_PLAYERS, GAME_MAX_MAX_PLAYERS, GAME_DEFAULT_MAX_PLAYERS)

  GEN_INT("aifill", game.server.aifill,
	  SSET_PLAYERS, SSET_INTERNAL, SSET_VITAL, SSET_SERVER_ONLY,
	  N_("Total number of players (including AI players)"),
	  N_("If there are fewer than this many players when the "
	     "game starts, extra AI players will be created to "
	     "increase the total number of players to the value of "
	     "this option."), NULL, 
	  GAME_MIN_AIFILL, GAME_MAX_AIFILL, GAME_DEFAULT_AIFILL)

  /* Game initialization parameters (only affect the first start of the game,
   * and not reloads).  Can not be changed after first start of game.
   */
  GEN_STRING("startunits", game.server.start_units,
	     SSET_GAME_INIT, SSET_SOCIOLOGY, SSET_VITAL, SSET_TO_CLIENT,
             N_("List of players' initial units"),
             N_("This should be a string of characters, each of which "
		"specifies a unit role. There must be at least one city "
		"founder in the string. The characters and their "
		"meanings are:\n"
		"    c   = City founder (eg., Settlers)\n"
		"    w   = Terrain worker (eg., Engineers)\n"
		"    x   = Explorer (eg., Explorer)\n"
		"    k   = Gameloss (eg., King)\n"
		"    s   = Diplomat (eg., Diplomat)\n"
		"    d   = Ok defense unit (eg., Warriors)\n"
		"    D   = Good defense unit (eg., Phalanx)\n"
		"    a   = Fast attack unit (eg., Horsemen)\n"
		"    A   = Strong attack unit (eg., Catapult)\n"),
		startunits_callback, GAME_DEFAULT_START_UNITS)

  GEN_INT("dispersion", game.server.dispersion,
	  SSET_GAME_INIT, SSET_SOCIOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Area where initial units are located"),
	  N_("This is the radius within "
	     "which the initial units are dispersed."), NULL,
	  GAME_MIN_DISPERSION, GAME_MAX_DISPERSION, GAME_DEFAULT_DISPERSION)

  GEN_INT("gold", game.info.gold,
	  SSET_GAME_INIT, SSET_ECONOMICS, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Starting gold per player"), 
	  N_("At the beginning of the game, each player is given this "
	     "much gold."), NULL,
	  GAME_MIN_GOLD, GAME_MAX_GOLD, GAME_DEFAULT_GOLD)

  GEN_INT("techlevel", game.info.tech,
	  SSET_GAME_INIT, SSET_SCIENCE, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Number of initial techs per player"), 
	  N_("At the beginning of the game, each player is given this "
	     "many technologies. The technologies chosen are random for "
	     "each player."), NULL,
	  GAME_MIN_TECHLEVEL, GAME_MAX_TECHLEVEL, GAME_DEFAULT_TECHLEVEL)

  GEN_INT("researchcost", game.info.researchcost,
	  SSET_RULES, SSET_SCIENCE, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Points required to gain a new tech"),
	  N_("This affects how quickly players can research new "
	     "technology. Doubling its value will make all technologies "
	     "take twice as long to research."), NULL,
	  GAME_MIN_RESEARCHCOST, GAME_MAX_RESEARCHCOST, 
	  GAME_DEFAULT_RESEARCHCOST)

  GEN_INT("techpenalty", game.info.techpenalty,
	  SSET_RULES, SSET_SCIENCE, SSET_RARE, SSET_TO_CLIENT,
	  N_("Percentage penalty when changing tech"),
	  N_("If you change your current research technology, and you have "
	     "positive research points, you lose this percentage of those "
	     "research points. This does not apply if you have just gained "
	     "a technology this turn."), NULL,
	  GAME_MIN_TECHPENALTY, GAME_MAX_TECHPENALTY,
	  GAME_DEFAULT_TECHPENALTY)

  GEN_INT("diplcost", game.info.diplcost,
	  SSET_RULES, SSET_SCIENCE, SSET_RARE, SSET_TO_CLIENT,
	  N_("Penalty when getting tech from treaty"),
	  N_("For each technology you gain from a diplomatic treaty, you "
	     "lost research points equal to this percentage of the cost to "
	     "research a new technology. You can end up with negative "
	     "research points if this is non-zero."), NULL, 
	  GAME_MIN_DIPLCOST, GAME_MAX_DIPLCOST, GAME_DEFAULT_DIPLCOST)

  GEN_INT("conquercost", game.info.conquercost,
	  SSET_RULES, SSET_SCIENCE, SSET_RARE, SSET_TO_CLIENT,
	  N_("Penalty when getting tech from conquering"),
	  N_("For each technology you gain by conquering an enemy city, you "
	     "lose research points equal to this percentage of the cost "
	     "to research a new technology. You can end up with negative "
	     "research points if this is non-zero."), NULL,
	  GAME_MIN_CONQUERCOST, GAME_MAX_CONQUERCOST,
	  GAME_DEFAULT_CONQUERCOST)

  GEN_INT("freecost", game.info.freecost,
	  SSET_RULES, SSET_SCIENCE, SSET_RARE, SSET_TO_CLIENT,
	  N_("Penalty when getting a free tech"),
	  N_("For each technology you gain \"for free\" (other than "
	     "covered by diplcost or conquercost: specifically, from huts "
	     "or from Great Library effects), you lose research points "
	     "equal to this percentage of the cost to research a new "
	     "technology. You can end up with negative research points if "
	     "this is non-zero."), 
	  NULL, 
	  GAME_MIN_FREECOST, GAME_MAX_FREECOST, GAME_DEFAULT_FREECOST)

  GEN_INT("foodbox", game.info.foodbox,
	  SSET_RULES, SSET_ECONOMICS, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Food required for a city to grow"),
	  N_("This is the base amount of food required to grow a city. "
	     "This value is multiplied by another factor that comes from "
	     "the ruleset and is dependent on the size of the city."),
	  NULL,
	  GAME_MIN_FOODBOX, GAME_MAX_FOODBOX, GAME_DEFAULT_FOODBOX)

  GEN_INT("aqueductloss", game.server.aqueductloss,
	  SSET_RULES, SSET_ECONOMICS, SSET_RARE, SSET_TO_CLIENT,
	  N_("Percentage food lost when need aqueduct"),
	  N_("If a city would expand, but it can't because it needs "
	     "an Aqueduct (or Sewer System), it loses this percentage "
	     "of its foodbox (or half that amount if it has a "
	     "Granary)."), NULL, 
	  GAME_MIN_AQUEDUCTLOSS, GAME_MAX_AQUEDUCTLOSS, 
	  GAME_DEFAULT_AQUEDUCTLOSS)

  /* Notradesize and fulltradesize used to have callbacks to prevent them
   * from being set illegally (notradesize > fulltradesize).  However this
   * provided a problem when setting them both through the client's settings
   * dialog, since they cannot both be set atomically.  So the callbacks were
   * removed and instead the game now knows how to deal with invalid
   * settings. */
  GEN_INT("fulltradesize", game.ruleset_control.fulltradesize,
	  SSET_RULES, SSET_ECONOMICS, SSET_RARE, SSET_TO_CLIENT,
	  N_("Minimum city size to get full trade"),
	  N_("There is a trade penalty in all cities smaller than this. "
	     "The penalty is 100% (no trade at all) for sizes up to "
	     "notradesize, and decreases gradually to 0% (no penalty "
	     "except the normal corruption) for size=fulltradesize. "
	     "See also notradesize."), NULL, 
	  GAME_MIN_FULLTRADESIZE, GAME_MAX_FULLTRADESIZE, 
	  GAME_DEFAULT_FULLTRADESIZE)

  GEN_INT("notradesize", game.ruleset_control.notradesize,
	  SSET_RULES, SSET_ECONOMICS, SSET_RARE, SSET_TO_CLIENT,
	  N_("Maximum size of a city without trade"),
	  N_("Cities do not produce any trade at all unless their size "
	     "is larger than this amount. The produced trade increases "
	     "gradually for cities larger than notradesize and smaller "
	     "than fulltradesize. See also fulltradesize."), NULL,
	  GAME_MIN_NOTRADESIZE, GAME_MAX_NOTRADESIZE,
	  GAME_DEFAULT_NOTRADESIZE)

  GEN_INT("unhappysize", game.info.unhappysize,
	  SSET_RULES, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
	  N_("City size before people become unhappy"),
	  N_("Before other adjustments, the first unhappysize citizens in a "
	     "city are content, and subsequent citizens are unhappy. "
	     "See also cityfactor."), NULL,
	  GAME_MIN_UNHAPPYSIZE, GAME_MAX_UNHAPPYSIZE,
	  GAME_DEFAULT_UNHAPPYSIZE)

  GEN_INT("angrycitizen", game.info.angrycitizen,
	  SSET_RULES, SSET_SOCIOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Whether angry citizens are enabled"),
	  N_("Introduces angry citizens like in civilization II. Angry "
	     "citizens have to become unhappy before any other class "
	     "of citizens may be considered. See also unhappysize, "
	     "cityfactor and governments."), NULL, 
	  0, 1, GAME_DEFAULT_ANGRYCITIZEN)

  GEN_INT("cityfactor", game.info.cityfactor,
	  SSET_RULES, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Number of cities for higher unhappiness"),
	  N_("When the number of cities a player owns is greater than "
	     "cityfactor, one extra citizen is unhappy before other "
	     "adjustments; see also unhappysize. This assumes a "
	     "Democracy; for other governments the effect occurs at "
	     "smaller numbers of cities."), NULL, 
	  GAME_MIN_CITYFACTOR, GAME_MAX_CITYFACTOR, GAME_DEFAULT_CITYFACTOR)

  GEN_INT("citymindist", game.server.citymindist,
	  SSET_RULES, SSET_SOCIOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Minimum distance between cities"),
	  N_("When a player founds a new city, it is checked if there is "
	     "no other city in citymindist distance. For example, if "
	     "citymindist is 3, there have to be at least two empty "
	     "fields between two cities in every direction. If set "
	     "to 0 (default), the ruleset value will be used."),
	  NULL,
	  GAME_MIN_CITYMINDIST, GAME_MAX_CITYMINDIST,
	  GAME_DEFAULT_CITYMINDIST)

  GEN_INT("rapturedelay", game.server.rapturedelay,
	  SSET_RULES, SSET_SOCIOLOGY, SSET_SITUATIONAL, SSET_TO_CLIENT,
          N_("Number of turns between rapture effect"),
          N_("Sets the number of turns between rapture growth of a city. "
             "If set to n a city will grow after celebrating for n+1 "
	     "turns."),
	  NULL,
          GAME_MIN_RAPTUREDELAY, GAME_MAX_RAPTUREDELAY,
          GAME_DEFAULT_RAPTUREDELAY)

  GEN_INT("razechance", game.server.razechance,
	  SSET_RULES, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Chance for conquered building destruction"),
	  N_("When a player conquers a city, each city improvement has this "
	     "percentage chance to be destroyed."), NULL, 
	  GAME_MIN_RAZECHANCE, GAME_MAX_RAZECHANCE, GAME_DEFAULT_RAZECHANCE)

  GEN_INT("civstyle", game.info.civstyle,
	  SSET_RULES, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
          N_("civstyle is an obsolete setting"),
          N_("This setting is obsolete; it does nothing in the current "
	     "version. It will be removed from future versions."), NULL,
	  GAME_MIN_CIVSTYLE, GAME_MAX_CIVSTYLE, GAME_DEFAULT_CIVSTYLE)

  GEN_INT("occupychance", game.server.occupychance,
	  SSET_RULES, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Chance of moving into tile after attack"),
	  N_("If set to 0, combat is Civ1/2-style (when you attack, "
	     "you remain in place). If set to 100, attacking units "
	     "will always move into the tile they attacked if they win "
	     "the combat (and no enemy units remain in the tile). If "
	     "set to a value between 0 and 100, this will be used as "
	     "the percent chance of \"occupying\" territory."), NULL, 
	  GAME_MIN_OCCUPYCHANCE, GAME_MAX_OCCUPYCHANCE, 
	  GAME_DEFAULT_OCCUPYCHANCE)

  GEN_INT("killcitizen", game.server.killcitizen,
	  SSET_RULES, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Reduce city population after attack"),
	  N_("This flag indicates if city population is reduced "
	     "after successful attack of enemy unit, depending on "
	     "its movement type (OR-ed):\n"
	     "  1 = land\n"
	     "  2 = sea\n"
	     "  4 = heli\n"
	     "  8 = air"), NULL,
	  GAME_MIN_KILLCITIZEN, GAME_MAX_KILLCITIZEN,
	  GAME_DEFAULT_KILLCITIZEN)

  GEN_INT("wtowervision", game.server.watchtower_vision,
	  SSET_RULES, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Range of vision for units in a fortress"),
	  N_("If set to 1, it has no effect. "
	     "If 2 or larger, the vision range of a unit inside a "
	     "fortress is set to this value, if the necessary invention "
	     "has been made. This invention is determined by the flag "
	     "'Watchtower' in the techs ruleset. See also wtowerevision."), 
	  NULL, 
	  GAME_MIN_WATCHTOWER_VISION, GAME_MAX_WATCHTOWER_VISION, 
	  GAME_DEFAULT_WATCHTOWER_VISION)

  GEN_INT("wtowerevision", game.server.watchtower_extra_vision,
	  SSET_RULES, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Extra vision range for units in a fortress"),
	  N_("If set to 0, it has no "
	     "effect. If larger than 0, the vision range of a unit is "
	     "raised by this value, if the unit is inside a fortress "
	     "and the invention determined by the flag 'Watchtower' "
	     "in the techs ruleset has been made. Always the larger "
	     "value of wtowervision and wtowerevision will be used. "
	     "Also see wtowervision."), NULL, 
	  GAME_MIN_WATCHTOWER_EXTRA_VISION, GAME_MAX_WATCHTOWER_EXTRA_VISION, 
	  GAME_DEFAULT_WATCHTOWER_EXTRA_VISION)

  GEN_INT("borders", game.ruleset_control.borders,
	  SSET_RULES, SSET_MILITARY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("National borders radius"),
	  N_("If this is set to greater than 0, then any land tiles "
	     "within the given radius of a city will be owned by that "
	     "nation. Special rules apply for ocean tiles or tiles within "
	     "range of more than one nation's cities."),
	  NULL,
	  GAME_MIN_BORDERS, GAME_MAX_BORDERS, GAME_DEFAULT_BORDERS)

  GEN_BOOL("happyborders", game.ruleset_control.happyborders,
	   SSET_RULES, SSET_MILITARY, SSET_SITUATIONAL,
	   SSET_TO_CLIENT,
	   N_("Units inside borders cause no unhappiness"),
	   N_("If this is set, units will not cause unhappiness when "
	      "inside your own borders."), NULL,
	   GAME_DEFAULT_HAPPYBORDERS)

  GEN_INT("diplomacy", game.info.diplomacy,
	  SSET_RULES, SSET_MILITARY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Ability to do diplomacy with other players"),
	  N_("0 = default; diplomacy is enabled for everyone.\n\n"
	     "1 = diplomacy is only allowed between human players.\n\n"
	     "2 = diplomacy is only allowed between AI players.\n\n"
             "3 = diplomacy is restricted to teams.\n\n"
             "4 = diplomacy is disabled for everyone.\n\n"
             "You can always do diplomacy with players on your team."), NULL,
	  GAME_MIN_DIPLOMACY, GAME_MAX_DIPLOMACY, GAME_DEFAULT_DIPLOMACY)

  GEN_INT("maxallies", game.ext_info.maxallies,
	  SSET_RULES, SSET_MILITARY, SSET_SITUATIONAL, SSET_TO_CLIENT,
	  N_("Allies maximum number"),
	  N_("When you have 'maxallies' allies or more, the diplomacy\n"
	     "with the other players is canceled.\n\n"
             "0 = unlimited."), NULL,
	  GAME_MIN_MAXALLIES, GAME_MAX_MAXALLIES, GAME_DEFAULT_MAXALLIES)

  GEN_INT("citynames", game.server.allowed_city_names,
	  SSET_RULES, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Allowed city names"),
	  N_("0 = There are no restrictions: players can have "
	     "multiple cities with the same names.\n\n"
	     "1 = City names have to be unique to a player: "
	     "one player can't have multiple cities with the same name.\n\n"
	     "2 = City names have to be globally unique: "
	     "all cities in a game have to have different names.\n\n"
	     "3 = Like setting 2, but a player isn't allowed to use a "
	     "default city name of another nations unless it is a default "
	     "for their nation also."),
	  NULL,
	  GAME_MIN_ALLOWED_CITY_NAMES, GAME_MAX_ALLOWED_CITY_NAMES, 
	  GAME_DEFAULT_ALLOWED_CITY_NAMES)
  
  /* Flexible rules: these can be changed after the game has started.
   *
   * The distinction between "rules" and "flexible rules" is not always
   * clearcut, and some existing cases may be largely historical or
   * accidental.  However some generalizations can be made:
   *
   *   -- Low-level game mechanics should not be flexible (eg, rulesets).
   *   -- Options which would affect the game "state" (city production etc)
   *      should not be flexible (eg, foodbox).
   *   -- Options which are explicitly sent to the client (eg, in
   *      packet_game_info) should probably not be flexible, or at
   *      least need extra care to be flexible.
   */
  GEN_INT("barbarians", game.server.barbarianrate,
	  SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Barbarian appearance frequency"),
	  N_("0 = no barbarians \n"
	     "1 = barbarians only in huts \n"
	     "2 = normal rate of barbarian appearance \n"
	     "3 = frequent barbarian uprising \n"
	     "4 = raging hordes, lots of barbarians\n"
	     "5 = super-barbarians\n"
	     "6 = very numerous and powerful barbarians"), NULL, 
	  GAME_MIN_BARBARIANRATE, GAME_MAX_BARBARIANRATE, 
	  GAME_DEFAULT_BARBARIANRATE)

  GEN_INT("onsetbarbs", game.server.onsetbarbarian,
	  SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Barbarian onset year"),
	  N_("Barbarians will not appear before this year."), NULL,
	  GAME_MIN_ONSETBARBARIAN, GAME_MAX_ONSETBARBARIAN, 
	  GAME_DEFAULT_ONSETBARBARIAN)

  GEN_INT("revolen", game.server.revolution_length,
	  SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Length in turns of revolution"),
	  N_("When changing governments, a period of anarchy lasting this "
	     "many turns will occur. "
             "Setting this value to 0 will give a random "
             "length of 1-6 turns."), NULL, 
	  GAME_MIN_REVOLUTION_LENGTH, GAME_MAX_REVOLUTION_LENGTH, 
	  GAME_DEFAULT_REVOLUTION_LENGTH)

  GEN_BOOL("fogofwar", game.server.fogofwar,
	   SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	   N_("Whether to enable fog of war"),
	   N_("If this is set to 1, only those units and cities within "
	      "the vision range of your own units and cities will be "
	      "revealed to you. You will not see new cities or terrain "
	      "changes in tiles not observed."), NULL, 
	   GAME_DEFAULT_FOGOFWAR)

  GEN_INT("diplincitechance", game.server.diplincitechance,
          SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_SITUATIONAL,
          SSET_TO_CLIENT,
          N_("Chance for inciting a city revolt"),
          N_("The base chance of success for inciting a revolt in "
             "an enemy city using a diplomatic unit."),
          NULL, GAME_MIN_DIPLINCITECHANCE, GAME_MAX_DIPLINCITECHANCE,
          GAME_DEFAULT_DIPLINCITECHANCE)

  GEN_INT("diplbribechance", game.server.diplbribechance,
          SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_SITUATIONAL,
          SSET_TO_CLIENT,
          N_("Chance in bribing an enemy unit"),
          N_("The base chance of a diplomatic unit bribing another unit"
             "is given by this setting."),
          NULL, GAME_MIN_DIPLBRIBECHANCE, GAME_MAX_DIPLBRIBECHANCE,
          GAME_DEFAULT_DIPLBRIBECHANCE)

  GEN_INT("spyreturnchance", game.server.spyreturnchance,
          SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_SITUATIONAL,
          SSET_TO_CLIENT,
          N_("Chance for spy return after mission"),
          N_("The base chance of a spy returning from a successful "
             "mission is spyreturnchance percent (diplomats never "
             "return)."),
          NULL, GAME_MIN_SPYRETURNCHANCE, GAME_MAX_SPYRETURNCHANCE,
          GAME_DEFAULT_SPYRETURNCHANCE)

  GEN_INT("dipldefchance", game.server.dipldefchance,
          SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_SITUATIONAL,
          SSET_TO_CLIENT,
          N_("Chance in diplomatic contests"),
          N_("A diplomatic unit acting against a city which has one or "
             "more defending diplomatic units has a dipldefchance "
             "(percent) base chance to defeat each such defender."
	     "Defending spies are generally twice as capable as "
             "diplomats."),
          NULL, GAME_MIN_DIPLDEFCHANCE, GAME_MAX_DIPLDEFCHANCE,
          GAME_DEFAULT_DIPLDEFCHANCE)

  GEN_INT("diplchance", game.server.diplchance,
	  SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_SITUATIONAL, SSET_TO_CLIENT,
          N_("Chance in diplomatic actions"),
          /* xgettext:no-c-format */
          N_("This value is the base chance of success for diplomatic "
             "units in diplomatic activities not already covered by the "
             "other diplomatic chance settings. Veteran units are "
             "generally 50% more capable than non-veteran ones."),
          NULL, GAME_MIN_DIPLCHANCE, GAME_MAX_DIPLCHANCE,
          GAME_DEFAULT_DIPLCHANCE)

  GEN_BOOL("spacerace", game.info.spacerace,
	   SSET_RULES_FLEXIBLE, SSET_SCIENCE, SSET_VITAL, SSET_TO_CLIENT,
	   N_("Whether to allow space race"),
	   N_("If this option is set to 1, players can build spaceships."),
	   NULL, 
	   GAME_DEFAULT_SPACERACE)

  GEN_INT("civilwarsize", game.server.civilwarsize,
	  SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Minimum number of cities for civil war"),
	  N_("A civil war is triggered if a player has at least this "
	     "many cities and the player's capital is captured. If "
	     "this option is set to the maximum value, civil wars are "
	     "turned off altogether."), NULL, 
	  GAME_MIN_CIVILWARSIZE, GAME_MAX_CIVILWARSIZE, 
	  GAME_DEFAULT_CIVILWARSIZE)

  GEN_INT("contactturns", game.server.contactturns,
	  SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	  N_("Turns until player contact is lost"),
	  N_("Players may meet for diplomacy this number of turns "
	     "after their units have last met, even if they do not have "
	     "an embassy. If set to zero then players cannot meet unless "
	     "they have an embassy."),
	  NULL,
	  GAME_MIN_CONTACTTURNS, GAME_MAX_CONTACTTURNS, 
	  GAME_DEFAULT_CONTACTTURNS)

  GEN_BOOL("savepalace", game.server.savepalace,
	   SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
	   N_("Rebuild palace if capital is conquered"),
	   N_("If this is set to 1, then when the capital is conquered the "
	      "palace "
	      "is automatically rebuilt for free in another randomly "
	      "choosen city. This is significant because the technology "
	      "requirement for building a palace will be ignored."),
	   NULL,
	   GAME_DEFAULT_SAVEPALACE)

  GEN_BOOL("naturalcitynames", game.server.natural_city_names,
           SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
           N_("Whether to use natural city names"),
           N_("If enabled, the default city names will be determined based "
              "on the surrounding terrain."),
           NULL, GAME_DEFAULT_NATURALCITYNAMES)

  /* Meta options: these don't affect the internal rules of the game, but
   * do affect players.  Also options which only produce extra server
   * "output" and don't affect the actual game.
   * ("endyear" is here, and not RULES_FLEXIBLE, because it doesn't
   * affect what happens in the game, it just determines when the
   * players stop playing and look at the score.)
   */

  GEN_BOOL("autotoggle", game.server.auto_ai_toggle,
	   SSET_META, SSET_NETWORK, SSET_SITUATIONAL, SSET_TO_CLIENT,
	   N_("Whether AI-status toggles with connection"),
	   N_("If this is set to 1, AI status is turned off when a player "
	      "connects, and on when a player disconnects."),
	   autotoggle_callback, GAME_DEFAULT_AUTO_AI_TOGGLE)

  GEN_INT("endyear", game.info.end_year,
	  SSET_META, SSET_SOCIOLOGY, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Year the game ends"), 
	  N_("The game will end at the end of the given year."), NULL,
	  GAME_MIN_END_YEAR, GAME_MAX_END_YEAR, GAME_DEFAULT_END_YEAR)

  GEN_INT("timeout", game.info.timeout,
	  SSET_META, SSET_INTERNAL, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Maximum seconds per turn"),
          /* xgettext:no-c-format */
	  N_("If all players have not hit \"Turn Done\" before this "
	     "time is up, then the turn ends automatically. Zero "
	     "means there is no timeout. Use this with the command "
	     "\"incrementaltimeout\" to have a dynamic timer. As a "
             "special case, voting to increase the timeout by 30 "
             "seconds or less requires only 33% in favor."), NULL,
	   GAME_MIN_TIMEOUT, GAME_MAX_TIMEOUT, GAME_DEFAULT_TIMEOUT)

  GEN_INT("timeaddenemymove", game.server.timeoutaddenemymove,
        SSET_META, SSET_INTERNAL, SSET_VITAL, SSET_TO_CLIENT,
	  N_("Timeout at least n seconds when enemy moved"),
	  N_("Any time a unit moves when in sight of an enemy player, "
	     "the remaining timeout is set to this value if it was lower."),
	  NULL, 0, GAME_MAX_TIMEOUT, GAME_DEFAULT_TIMEOUTADDEMOVE)
  
  GEN_INT("tcpwritetimeout", game.server.tcpwritetimeout,
	  SSET_META, SSET_NETWORK, SSET_RARE, SSET_TO_CLIENT,
	  N_("Maximum seconds to wait to write network data"),
	  N_("If the server ever has to wait more than the "
             "given number of seconds for a client's network "
             "connection to become able to receive data, then "
             "that connection is closed so that it no longer "
             "lags the server. Zero means there is no timeout "
             "(although connections will be automatically "
             "disconnected eventually)."),
	  NULL, GAME_MIN_TCPWRITETIMEOUT, GAME_MAX_TCPWRITETIMEOUT,
          GAME_DEFAULT_TCPWRITETIMEOUT)

  GEN_INT("netwait", game.server.netwait,
	  SSET_META, SSET_NETWORK, SSET_RARE, SSET_TO_CLIENT,
	  N_("Max seconds for network buffers to drain"),
	  N_("The server will wait for up to the value of this "
	     "parameter in seconds, for all client connection network "
	     "buffers to unblock. Zero means the server will not "
	     "wait at all."), NULL, 
	  GAME_MIN_NETWAIT, GAME_MAX_NETWAIT, GAME_DEFAULT_NETWAIT)

  GEN_INT("pingtime", game.server.pingtime,
	  SSET_META, SSET_NETWORK, SSET_RARE, SSET_TO_CLIENT,
	  N_("Seconds between PINGs"),
	  N_("The civserver will poll the clients with a PING request "
	     "each time this period elapses."), NULL, 
	  GAME_MIN_PINGTIME, GAME_MAX_PINGTIME, GAME_DEFAULT_PINGTIME)

  GEN_INT("pingtimeout", game.server.pingtimeout,
	  SSET_META, SSET_NETWORK, SSET_RARE,
          SSET_TO_CLIENT,
	  N_("Time to cut a client"),
	  N_("If a client doesn't reply to a PING in this time the "
	     "client is disconnected."), NULL, 
	  GAME_MIN_PINGTIMEOUT, GAME_MAX_PINGTIMEOUT, GAME_DEFAULT_PINGTIMEOUT)

  GEN_BOOL("turnblock", game.server.turnblock,
	   SSET_META, SSET_INTERNAL, SSET_SITUATIONAL, SSET_TO_CLIENT,
	   N_("Turn-blocking game play mode"),
	   N_("If this is set to 1 the game.info.turn is not advanced "
	      "until all players have finished their turn, including "
	      "disconnected players."), NULL, 
	   GAME_DEFAULT_TURNBLOCK)

  GEN_BOOL("fixedlength", game.server.fixedlength,
	   SSET_META, SSET_INTERNAL, SSET_SITUATIONAL, SSET_TO_CLIENT,
	   N_("Fixed-length turns play mode"),
	   N_("If this is set to 1 the game.info.turn will not advance "
	      "until the timeout has expired, even if all players "
	      "have clicked on \"Turn Done\"."), NULL,
	   FALSE)

  GEN_STRING("demography", game.server.demography,
	     SSET_META, SSET_INTERNAL, SSET_SITUATIONAL, SSET_TO_CLIENT,
	     N_("What is in the Demographics report"),
	     N_("This should be a string of characters, each of which "
		"specifies the inclusion of a line of information "
		"in the Demographics report.\n"
		"The characters and their meanings are:\n"
		"    N = include Population\n"
		"    P = include Production\n"
		"    A = include Land Area\n"
		"    L = include Literacy\n"
		"    R = include Research Speed\n"
		"    S = include Settled Area\n"
		"    E = include Economics\n"
		"    M = include Military Service\n"
		"    O = include Pollution\n"
		"Additionally, the following characters control whether "
		"or not certain columns are displayed in the report:\n"
		"    q = display \"quantity\" column\n"
		"    r = display \"rank\" column\n"
		"    b = display \"best nation\" column\n"
		"The order of characters is not significant, but "
		"their capitalization is."),
	     is_valid_demography, GAME_DEFAULT_DEMOGRAPHY)

  GEN_INT("saveturns", game.server.save_nturns,
	  SSET_META, SSET_INTERNAL, SSET_VITAL, SSET_SERVER_ONLY,
	  N_("Turns per auto-save"),
	  N_("The game will be automatically saved per this number of "
	     "turns. Zero means never auto-save."), NULL, 
	  0, 200, 10)

  /* Could undef entire option if !HAVE_LIBZ, but this way users get to see
   * what they're missing out on if they didn't compile with zlib?  --dwp
   */
#ifdef HAVE_LIBZ
  GEN_INT("compress", game.server.save_compress_level,
	  SSET_META, SSET_INTERNAL, SSET_RARE, SSET_SERVER_ONLY,
	  N_("Savegame compression level"),
	  N_("If non-zero, saved games will be compressed using zlib "
	     "(gzip format). Larger values will give better "
	     "compression but take longer. If the maximum is zero "
	     "this server was not compiled to use zlib."), NULL,

	  GAME_MIN_COMPRESS_LEVEL, GAME_MAX_COMPRESS_LEVEL,
	  GAME_DEFAULT_COMPRESS_LEVEL)
#else
  GEN_INT("compress", game.server.save_compress_level,
	  SSET_META, SSET_INTERNAL, SSET_RARE, SSET_SERVER_ONLY,
	  N_("Savegame compression level"),
	  N_("If non-zero, saved games will be compressed using zlib "
	     "(gzip format). Larger values will give better "
	     "compression but take longer. If the maximum is zero "
	     "this server was not compiled to use zlib."), NULL, 

	  GAME_NO_COMPRESS_LEVEL, GAME_NO_COMPRESS_LEVEL, 
	  GAME_NO_COMPRESS_LEVEL)
#endif

  GEN_STRING("savename", game.server.save_name,
	     SSET_META, SSET_INTERNAL, SSET_VITAL, SSET_SERVER_ONLY,
	     N_("Auto-save name prefix"),
	     N_("Automatically saved games will have name "
		"\"<prefix><year>.sav\". This setting sets "
		"the <prefix> part."), NULL,
	     GAME_DEFAULT_SAVE_NAME)

  GEN_BOOL("scorelog", game.server.scorelog,
	   SSET_META, SSET_INTERNAL, SSET_SITUATIONAL, SSET_SERVER_ONLY,
	   N_("Whether to log player statistics"),
	   N_("If this is set to 1, player statistics are appended to "
	      "the file \"civscore.log\" every turn. These statistics "
	      "can be used to create power graphs after the game."), NULL,
	   GAME_DEFAULT_SCORELOG)

  GEN_INT("gamelog", gamelog_level,
	  SSET_META, SSET_INTERNAL, SSET_SITUATIONAL, SSET_SERVER_ONLY,
	  N_("Detail level for logging game events"),
	  N_("Only applies if the game log feature is enabled "
	     "(with the -g command line option). "
	     "Levels: 0=no logging, 20=standard logging, 30=detailed "
	     "logging, 40=debuging logging."), NULL,
	  0, 40, 20)

  GEN_BOOL("techtrading", game.ext_info.techtrading, SSET_RULES,
	   SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
           N_("Technology trading"),
           N_("0 - off\n"
              "1 - on\n"
              "If turned off, trading technologies in diplomacy dialog\n"
              "is not allowed."), NULL,
           GAME_DEFAULT_TECHTRADING)

  GEN_BOOL("goldtrading", game.ext_info.goldtrading, SSET_RULES,
	   SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
           N_("Gold trading"),
           N_("0 - off\n"
              "1 - on\n"
              "If turned off, trading gold in diplomacy dialog\n"
              "is not allowed."), NULL,
           GAME_DEFAULT_GOLDTRADING)

  GEN_BOOL("citytrading", game.ext_info.citytrading, SSET_RULES,
	   SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
           N_("City trading"),
           N_("0 - off\n"
              "1 - on\n"
              "If turned off, trading cities in diplomacy dialog\n"
              "is not allowed."), NULL,
           GAME_DEFAULT_CITYTRADING)

  GEN_INT("airliftingstyle", game.ext_info.airliftingstyle, 
          SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
          N_("Airlifting style"),
          N_("0 - standard Freeciv, only 1 unit per turn to destination\n"
             "1 - multiple units can be airlifted into destination\n"
             "2 - like 0 but airlifts into allied cities are possible\n"
             "3 - like 1 but airlifts into allied cities are possible"),
          NULL, GAME_MIN_AIRLIFTINGSTYLE,
          GAME_MAX_AIRLIFTINGSTYLE, GAME_DEFAULT_AIRLIFTINGSTYLE)

  GEN_BOOL("teamplacement", game.ext_info.teamplacement, SSET_RULES,
	   SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
           N_("Team placement"),
           N_("0 - off\n"
              "1 - on\n"
              "If turned off, team players are placed randomly."), NULL,
           GAME_DEFAULT_TEAMPLACEMENT)

  GEN_INT("teamplacementtype", game.server.teamplacementtype,
	  SSET_RULES, SSET_SOCIOLOGY, SSET_RARE, SSET_TO_CLIENT,
          N_("Type of team placement"),
	  N_("0 - team players are placed as close as possible\n"
	     "    regardless of continents.\n"
             "1 - team players are placed on the same continent.\n"
             "2 - team players are placed horizontally. This is for\n"
             "    generator 4/5.\n"
            "3 - team players are placed vertically."), NULL,
	  GAME_MIN_TEAMPLACEMENTTYPE, GAME_MAX_TEAMPLACEMENTTYPE,
          GAME_DEFAULT_TEAMPLACEMENTTYPE)

  GEN_INT("bruteforcethreshold", game.server.bruteforcethreshold,
	  SSET_RULES, SSET_INTERNAL, SSET_RARE, SSET_SERVER_ONLY,
          N_("Brute force team placement algorithm threshold"),
	  N_("Brute force team placement algorithm will be used\n"
             "if the number of players is less or equal than this value.\n"
             "This algorithm guarantees that team players will be placed\n"
             "as close as possible. Default value is 11."), NULL,
	  GAME_MIN_BRUTEFORCETHRESHOLD, GAME_MAX_BRUTEFORCETHRESHOLD,
          GAME_DEFAULT_BRUTEFORCETHRESHOLD)

  GEN_INT("iterplacementcoefficient", game.server.iterplacementcoefficient,
	  SSET_RULES, SSET_INTERNAL, SSET_RARE, SSET_SERVER_ONLY,
          N_("Iterative team placement algorithm coefficient."),
	  N_("This value is the upper bound of the number of\n"
	    "iterations the iterative tabu search performs.\n"
            "Iterative alrogithm is much faster than brute force\n"
            "and provies acceptable team placement. Allied players\n"
            "will be placed very close, but a slighly better solution\n"
            "might exist. Default value is 400."), NULL,
	  GAME_MIN_ITERPLACEMENTCOEFFICIENT, GAME_MAX_ITERPLACEMENTCOEFFICIENT,
          GAME_DEFAULT_ITERPLACEMENTCOEFFICIENT)

  GEN_BOOL("globalwarming", game.ext_info.globalwarmingon, SSET_RULES,
	   SSET_ECOLOGY, SSET_RARE, SSET_TO_CLIENT,
           N_("Global warming"),
           N_("0 - off\n"
              "1 - on\n"
              "If turned off, global warming will not occur\n"
              "as a result of pollution. This settings does not\n"
              "affect pollution."), NULL,
           GAME_DEFAULT_GLOBALWARMINGON)

  GEN_BOOL("nuclearwinter", game.ext_info.nuclearwinteron, SSET_RULES,
           SSET_ECOLOGY, SSET_RARE, SSET_TO_CLIENT,
           N_("Nuclear winter"),
           N_("0 - off\n"
              "1 - on\n"
              "If turned off, nuclear winter will not occur\n"
              "as a result of nuclear war."), NULL,
           GAME_DEFAULT_NUCLEARWINTERON)

  GEN_INT_FULL("traderevenuestyle", game.traderoute_info.traderevenuestyle,
               SSET_RULES, SSET_ECONOMICS, SSET_RARE, SSET_TO_CLIENT,
               "extroutes", /* required capability for non-default */
               N_("Trade revenue style"),
               N_("0 - standard Freeciv\n"
                  "1 - experimental\n"
                  "2 - civ2 trade routes\n"
                  "This setting affects how much trade cities"
                  "generate after trade routes are established."), NULL,
               GAME_MIN_TRADEREVENUESTYLE, GAME_MAX_TRADEREVENUESTYLE,
               GAME_DEFAULT_TRADEREVENUESTYLE, VCF_NONE, 0, -1, -1)

  GEN_INT_FULL("traderevenuepercentage", game.traderoute_info.traderevenuepct,
               SSET_RULES, SSET_ECONOMICS, SSET_RARE, SSET_TO_CLIENT,
               "extroutes", /* required capability for non-default */
               N_("Trade revenue percentage"),
               N_("100 - default value"), NULL,
               GAME_MIN_TRADEREVENUEPCT, GAME_MAX_TRADEREVENUEPCT,
               GAME_DEFAULT_TRADEREVENUEPCT, VCF_NONE, 0, -1, -1)

  GEN_INT_FULL("caravanbonusstyle", game.traderoute_info.caravanbonusstyle,
               SSET_RULES, SSET_ECONOMICS, SSET_RARE, SSET_TO_CLIENT,
               "extroutes", /* required capability for non-default */
               N_("Caravan bonus style"),
               N_("0 - standard Freeciv\n"
                  "1 - experimental\n"
                  "This setting affects how much gold and science"
                  "you get when a caravan arrives in a city."), NULL,
               GAME_MIN_CARAVANBONUSSTYLE, GAME_MAX_CARAVANBONUSSTYLE,
               GAME_DEFAULT_CARAVANBONUSSTYLE, VCF_NONE, 0, -1, -1)

  GEN_INT_FULL("trademindist", game.traderoute_info.trademindist,
               SSET_RULES, SSET_ECONOMICS, SSET_RARE, SSET_TO_CLIENT,
               "extroutes", /* required capability for non-default */
               N_("Minimum trade distance"),
               N_("Minimum distance to establish trade route.\n"
                  "9 is default"), NULL,
               GAME_MIN_TRADEMINDIST, GAME_MAX_TRADEMINDIST,
               GAME_DEFAULT_TRADEMINDIST, VCF_NONE, 0, -1, -1)

  GEN_INT_FULL("maxtraderoutes", game.traderoute_info.maxtraderoutes,
	       SSET_RULES, SSET_ECONOMICS, SSET_RARE, SSET_TO_CLIENT,
	       "extglobalinfo", /* required capability for non-default */
	       N_("Maximum trade routes number for one city"),
	       N_("Maximum trade routes number for one city.\n"
		  "4 is default"), NULL,
	       GAME_MIN_MAXTRADEROUTES, GAME_MAX_MAXTRADEROUTES,
	       GAME_DEFAULT_MAXTRADEROUTES, VCF_NONE, 0, -1, -1)

  GEN_BOOL("futuretechsscore", game.ext_info.futuretechsscore, SSET_RULES,
	   SSET_SCIENCE, SSET_RARE, SSET_TO_CLIENT,
           N_("Score for future techs"),
           N_("0 - off\n"
              "1 - on"), NULL,
           GAME_DEFAULT_FUTURETECHSSCORE)

  GEN_BOOL("improvedautoattack", game.ext_info.improvedautoattack, SSET_RULES,
	   SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
           N_("Improved autoattack"),
           N_("0 - off\n"
              "1 - on\n"
              "If turned off, automatic units don't attack\n"
              "in the middle of turn.\n"), NULL,
           GAME_DEFAULT_IMPROVEDAUTOATTACK)

  GEN_BOOL("stackbribing", game.ext_info.stackbribing, SSET_RULES,
	   SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
           N_("Stack bribing"),
           N_("0 - off\n"
              "1 - on\n"
              "If turned on, diplomats can bribe stacks\n"
              "of units.\n"), NULL,
           GAME_DEFAULT_STACKBRIBING)

  GEN_BOOL("experimentalbribingcost", game.ext_info.experimentalbribingcost, SSET_RULES,
	   SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
           N_("Experimental bribing cost"),
           N_("0 - off\n"
              "1 - on\n"
              "If turned on, bribing cost doesn't depend on\n"
              "government, distance, reserve gold, but depends on\n"
              "build cost, veterancy, terrain, fortification."), NULL,
           GAME_DEFAULT_EXPERIMENTALBRIBINGCOST)

  GEN_BOOL("ignoreruleset", game.ext_info.ignoreruleset, SSET_RULES,
	   SSET_INTERNAL, SSET_RARE, SSET_TO_CLIENT,
           N_("Ignore ruleset"),
           N_("0 - off\n"
              "1 - on\n"
              "Ignore ruleset if ruleset and pregame settings differs\n"
              "Must be set to 1 if slow invasions are off, tech leakage\n"
	      "is on, techcoststyle isn't 1 or kill stack is off."), NULL,
           GAME_DEFAULT_IGNORERULESET)

  GEN_BOOL("slowinvasions", game.ruleset_control.slow_invasions, SSET_RULES,
	   SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
           N_("Slow invasions"),
           N_("0 - off\n"
              "1 - on\n"
              "If slow invasions are off, units with 2 movement points\n"
              "can unload from ship and attack in the same turn. If you\n"
              "change this setting, turn on 'ignoreruleset'."), NULL,
           GAME_DEFAULT_SLOWINVASIONS)

  GEN_BOOL("killstack", game.ruleset_game.killstack, SSET_RULES,
           SSET_MILITARY, SSET_RARE, SSET_TO_CLIENT,
           N_("Kill stack"),
           N_("0 - off\n"
              "1 - on\n"
              "If turned off, killing one unit in a stack doesn't\n"
              "kill the whole stack but all units have to be killed\n"
              "individually. If you change this setting, turn on\n"
              "'ignoreruleset'."), NULL,
           GAME_DEFAULT_KILLSTACK)

  GEN_INT("techleakage", game.ruleset_game.tech_leakage,
	  SSET_RULES, SSET_SCIENCE, SSET_RARE, SSET_TO_CLIENT,
	  N_("Technology leak from other civilizations"),
	  N_("0 - No reduction of the technology cost\n"
             "1 - Technology cost is reduced depending on the number of players\n"
             "    which already know the tech and you have an embassy with.\n"
             "2 - Technology cost is reduced depending on the number of all players\n"
             "    (human, AI and barbarians) which already know the tech.\n"
             "3 - Technology cost is reduced depending on the number of normal\n"
             "    players (human and AI) which already know the tech.\n\n"
             "If you change this setting, turn on 'ignoreruleset'.\n"),
	  NULL,
	  GAME_MIN_TECHLEAKAGE, GAME_MAX_TECHLEAKAGE, GAME_DEFAULT_TECHLEAKAGE)

  GEN_INT("techleakagerate", game.ext_info.techleakagerate,
	  SSET_RULES, SSET_SCIENCE, SSET_RARE, SSET_TO_CLIENT,
	  N_("Technology leak rate from other civilizations (in percent)"),
	  N_("The thecnholgy cost is reduced by this rate.\n"
	  "If this value is 100%, the last player to search a technology\n"
	  "will have only (technologycost * 1 / playernumber).\n"
	  "If this value is 0%, all players will have the same technology\n"
	  "costs."), NULL,
          GAME_MIN_TECHLEAKAGERATE, GAME_MAX_TECHLEAKAGERATE,
          GAME_DEFAULT_TECHLEAKAGERATE)

  GEN_INT("techcoststyle", game.ruleset_game.tech_cost_style,
	  SSET_RULES, SSET_SCIENCE, SSET_RARE, SSET_TO_CLIENT,
	  N_("Technology cost style"),
	  N_("0 - Civ (I|II) style. Every new tech add game.info.researchcost\n"
             "    to the cost of the next tech.\n"
             "1 - Cost of technology is like default Freeciv.\n"
             "2 - Cost are read from tech.ruleset. Missing costs are\n"
             "    generated by style 1.\n\n"
             "If you change this setting, turn on 'ignoreruleset'.\n"),
	  NULL, GAME_MIN_TECHCOSTSTYLE,
          GAME_MAX_TECHCOSTSTYLE, GAME_DEFAULT_TECHCOSTSTYLE)

  GEN_INT("maxconnections", game.server.maxconnections,
	  SSET_RULES_FLEXIBLE, SSET_NETWORK, SSET_RARE, SSET_TO_CLIENT,
	  N_("Maximum number of connections to the server"),
	  N_("New players will be rejected if the total number\n"
             "of connections exceeds maxconnections.\n"
             "0 means there is no limit."), NULL,
	  GAME_MIN_MAXCONNECTIONS, GAME_MAX_MAXCONNECTIONS,
          GAME_DEFAULT_MAXCONNECTIONS)

  GEN_INT("maxhostconnections", game.server.maxhostconnections,
	  SSET_RULES_FLEXIBLE, SSET_NETWORK, SSET_RARE, SSET_TO_CLIENT,
	  N_("Maximum number of connections per host to the server"),
	  N_("New connection will be rejected if the total number\n"
             "of connections from the same host exceeds\n"
	     "maxhostconnections.\n"
             "0 means there is no limit."), NULL,
	  GAME_MIN_MAXHOSTCONNECTIONS, GAME_MAX_MAXHOSTCONNECTIONS,
          GAME_DEFAULT_MAXHOSTCONNECTIONS)

  GEN_BOOL("multilinechat", srvarg.allow_multi_line_chat, SSET_RULES_FLEXIBLE,
	   SSET_INTERNAL, SSET_RARE, SSET_SERVER_ONLY,
           N_("Multi line chat"),
           N_("0 - off\n"
              "1 - on\n"
              "If turned on, the user can send many chat lines."), NULL, FALSE)

#ifdef HAVE_MYSQL
  GEN_BOOL_FULL("rated", game.server.rated, SSET_RULES_FLEXIBLE, SSET_INTERNAL,
           SSET_VITAL, SSET_TO_CLIENT,
           N_("Update user ratings"),
           N_("0 - User ratings will not be affected by this game.\n"
              "1 - New ratings will be calculated based on the outcome "
              "of this game.\n"
              "Note that the only game types that can be rated are "
              "duels and FFAs."),
           rated_callback, GAME_DEFAULT_RATED, VCF_NONE, 50, -1, -1)
#endif

  GEN_BOOL("nopubliclinks", game.server.no_public_links, SSET_RULES_FLEXIBLE,
           SSET_INTERNAL, SSET_RARE, SSET_TO_CLIENT,
           N_("Disallow public chat links in game"),
           N_("0 - Public chat links are allowed.\n"
              "1 - Public chat messages with links are rejected."),
           NULL, GAME_DEFAULT_NO_PUBLIC_LINKS)

  GEN_INT_FULL("kicktime", game.server.kicktime, SSET_RULES_FLEXIBLE,
               SSET_INTERNAL, SSET_RARE, SSET_TO_CLIENT, "",
               N_("Time before a kicked user can reconnect"),
               N_("Gives the time in seconds before a user kicked "
                  "using the 'kick' command may reconnect. Changing "
                  "this setting will affect users kicked in the past."),
               NULL, GAME_MIN_KICKTIME, GAME_MAX_KICKTIME,
               GAME_DEFAULT_KICKTIME,
               VCF_NONE, 0, ALLOW_ADMIN, ALLOW_ADMIN)

  GEN_INT("spectatorchat", game.server.spectatorchat, SSET_RULES_FLEXIBLE,
           SSET_INTERNAL, SSET_RARE, SSET_TO_CLIENT,
           N_("How to handle non-player messages"),
           N_("This setting controls whether non-player messages "
              "can reach players in the game.\n"
              "0 - All messages are handled normally.\n"
              "1 - Non-player public/ally messages only go to other\n"
              "    non-players.\n"
              "2 - All non-public non-player messages are only seen\n"
              "    by other non-players.\n"
              "If you are worried that observers may be giving private "
              "hints to other players, use the value 2 for this setting."),
           NULL, GAME_MIN_SPECTATORCHAT, GAME_MAX_SPECTATORCHAT,
           GAME_DEFAULT_SPECTATORCHAT)

  GEN_INT_FULL("idlecut", game.server.idlecut, SSET_RULES_FLEXIBLE,
               SSET_INTERNAL, SSET_RARE, SSET_TO_CLIENT, "",
               N_("Time before idle user is cut"),
               N_("This settings controls how long (in seconds) a user's "
                  "connection can be idle while connected to the server. "
                  "Connections that idle for more than this time are "
                  "cut. A value of zero for this setting means that "
                  "idle connections are never cut."),
               idlecut_callback, GAME_MIN_IDLECUT, GAME_MAX_IDLECUT,
               GAME_DEFAULT_IDLECUT,
               VCF_NONE, 0, ALLOW_ADMIN, ALLOW_ADMIN)

  GEN_BOOL_FULL("emptyreset", game.server.emptyreset, SSET_RULES_FLEXIBLE,
                SSET_INTERNAL, SSET_RARE, SSET_TO_CLIENT,
                N_("Reset server settings when empty"),
                N_("If this setting is set to 1, then the server  "
                   "will automatically reset all settings when it "
                   "becomes empty (i.e. there are no more connections). "
                   "This only affects the server in pregame."),
                NULL, GAME_DEFAULT_EMPTYRESET,
                VCF_NONE, 0, ALLOW_ADMIN, ALLOW_ADMIN)

  GEN_INT("triremestyle", game.server.triremestyle, SSET_RULES_FLEXIBLE,
          SSET_MILITARY, SSET_SITUATIONAL, SSET_TO_CLIENT,
          N_("Trireme ocean loss behaviour"),
          N_("This setting controls how triremes are handled by the "
             "server. Changing this setting will affect how and why "
             "triremes will be lost when in ocean tiles.\n"
             "0 - Classic freeciv random chance when not at coast.\n"
             "1 - Experimental deterministic loss. With this mode\n"
             "    set, triremes will always die under the following\n"
             "    circumstances:\n"
             "    - Trireme tech researched: not at coast.\n"
             "    - Seafaring tech researched: not at coast and more\n"
             "      than 3 tiles away from home continent.\n"
             "    - Navigation tech researched: never.\n"
             "The \"home continent\" is the continent of the home city "
             "of the trireme, or of the owner's capital."), NULL,
          GAME_MIN_TRIREMESTYLE, GAME_MAX_TRIREMESTYLE,
          GAME_DEFAULT_TRIREMESTYLE)

  GEN_INT("fracmovestyle", game.server.fracmovestyle, SSET_GAME_INIT,
          SSET_MILITARY, SSET_SITUATIONAL, SSET_TO_CLIENT,
          N_("How to handle fractional unit moves"),
          N_("This setting controls whether units with only 1/3 or "
             "2/3 moves left are able to move to adjacent tiles.\n"
             "0 - Freeciv 2.0 random chance.\n"
             "1 - Freeciv 2.1 style where the move always succeeds."),
          NULL, GAME_MIN_FRACMOVESTYLE, GAME_MAX_FRACMOVESTYLE,
          GAME_DEFAULT_FRACMOVESTYLE)

  GEN_INT("endturn", game.server.endturn,
          SSET_META, SSET_SOCIOLOGY, SSET_VITAL, SSET_TO_CLIENT,
          N_("Turn the game ends"),
          N_("The game will end at the end of the given turn. This is "
             "equivalent to using the /endgame command with no arguments "
             "at that time. If this setting is zero, it will be ignored."),
          NULL, GAME_MIN_ENDTURN, GAME_MAX_ENDTURN, GAME_DEFAULT_ENDTURN)

  GEN_BOOL("revealmap", game.server.revealmap, SSET_GAME_INIT,
           SSET_MILITARY, SSET_SITUATIONAL, SSET_TO_CLIENT,
           N_("Reveal the map at game start"),
           N_("If this setting is enabled, the entire map will be "
              "known to all players from the start of the game, though "
              "it will still be fogged (depending on the fogofwar "
              "setting)."),
          NULL, GAME_DEFAULT_REVEALMAP)

  GEN_BOOL("civilwar", game.server.civilwar, SSET_RULES_FLEXIBLE,
           SSET_MILITARY, SSET_SITUATIONAL, SSET_TO_CLIENT,
           N_("Enables the civil war and player split"),
           N_("If this setting is disabled, losing you capital will never "
              "make a civil war, and losing the half of your ressources."),
          NULL, GAME_DEFAULT_CIVILWAR)

  GEN_END
};

/* The number of settings, not including the END. */
const int SETTINGS_NUM = ARRAY_SIZE(settings) - 1;

/********************************************************************
  Returns whether the specified server setting (option) can currently
  be changed.  Does not indicate whether it can be changed by clients.
*********************************************************************/
bool sset_is_changeable(int idx)
{
  struct settings_s *op = &settings[idx];

  switch (op->sclass) {
    case SSET_MAP_SIZE:
    case SSET_MAP_GEN:
      /* Only change map options if we don't yet have a map: */
      return map_is_empty() && !map_is_loaded();
    case SSET_MAP_ADD:
    case SSET_PLAYERS:
    case SSET_GAME_INIT:
    case SSET_RULES:
      /* Only change start params and most rules if we don't yet have a map,
       * or if we do have a map but its a scenario one (ie, the game has
       * never actually been started).
       */
      return (map_is_empty() || game.server.is_new_game);
    case SSET_RULES_FLEXIBLE:
    case SSET_META:
      /* These can always be changed: */
      return TRUE;
    default:
      freelog(LOG_ERROR, "Unexpected case %d in %s line %d",
	      op->sclass, __FILE__, __LINE__);
      return FALSE;
  }
}

/********************************************************************
Returns whether the specified server setting (option) should be
sent to the client.
*********************************************************************/
bool sset_is_to_client(int idx)
{
  return (settings[idx].to_client == SSET_TO_CLIENT);
}

/********************************************************************
  ...
*********************************************************************/
static void setting_set_to_default(int idx)
{
  struct settings_s *pset = &settings[idx];

  switch (pset->type) {
    case SSET_BOOL:
      (*pset->bool_value) = pset->bool_default_value;
      break;
    case SSET_INT:
      (*pset->int_value) = pset->int_default_value;
      break;
    case SSET_STRING:
      mystrlcpy(pset->string_value, pset->string_default_value,
		pset->string_value_size);
      break;
    default:
      die("Setting %d has a wrong type (%d)", idx, pset->type);
  }
}

/********************************************************************
  Set all settings to default. This function should be used only to
  initialize the server. For other cases, use settings_reset().
*********************************************************************/
void settings_init(void)
{
  int i;

  for (i = 0; i < SETTINGS_NUM; i++) {
    setting_set_to_default(i);
  }
}

/********************************************************************
  ...
*********************************************************************/
void settings_reset(void)
{
  int i;

  for (i = 0; i < SETTINGS_NUM; i++) {
    if (sset_is_changeable(i)) {
      setting_set_to_default(i);
    }
  }
}

/********************************************************************
  ...
*********************************************************************/
enum cmdlevel_id sset_access_level(int idx)
{
  struct settings_s *pset;
  
  if (!(0 <= idx && idx < SETTINGS_NUM)) {
    return ALLOW_NEVER;
  }

  pset = settings + idx;

  if (server_state == PRE_GAME_STATE) {
    if (pset->pregame_level < 0) {
      return command_access_level(CMD_SET);
    }
    return pset->pregame_level;
  }

  if (pset->game_level < 0) {
    return command_access_level(CMD_SET);
  }
  return pset->game_level;
}
