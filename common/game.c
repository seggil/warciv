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
#include <config.h>
#endif

#include <assert.h>

#include "capstr.h"
#include "city.h"
#include "cm.h"
#include "connection.h"
#include "fcintl.h"
#include "government.h"
#include "idex.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "nation.h"
#include "packets.h"
#include "player.h"
#include "shared.h"
#include "spaceship.h"
#include "support.h"
#include "tech.h"
#include "unit.h"

#include "game.h"

void dealloc_id(int id);
struct civ_game game;

/* Must match enum game_outcomes in game.h */
static const char *game_outcome_strings[GOC_NUM_OUTCOMES] = {
  N_("none"),
  N_("drawn by endyear"),
  N_("drawn by mutual destruction"),
  N_("ended by lone survival"),
  N_("ended by spaceship arrival"),
  N_("ended by vote"),
  N_("ended by team victory"),
  N_("ended by allied victory")
};

/* Must match enum game_types in game.h. */
static const char *game_type_strings[GT_NUM_TYPES] = {
  N_("ffa"),
  N_("team"),
  N_("duel"),
  N_("solo"),
  N_("mixed")
};

/*
struct player_score {
  int happy;
  int content;
  int unhappy;
  int angry;
  int taxmen;
  int scientists;
  int elvis;
  int wonders;
  int techs;
  int landarea;
  int settledarea;
  int population;
  int cities;
  int units;
  int pollution;
  int literacy;
  int bnp;
  int mfg;
  int spaceship;
};
*/

/**************************************************************************
Count the # of thousand citizen in a civilisation.
**************************************************************************/
int civ_population(struct player *pplayer)
{
  int ppl=0;
  city_list_iterate(pplayer->cities, pcity)
    ppl+=city_population(pcity);
  city_list_iterate_end;
  return ppl;
}


/**************************************************************************
...
**************************************************************************/
struct city *game_find_city_by_name(const char *name)
{
  players_iterate(pplayer) {
    struct city *pcity = city_list_find_name(pplayer->cities, name);

    if (pcity) {
      return pcity;
    }
  } players_iterate_end;

  return NULL;
}


/**************************************************************************
  Often used function to get a city pointer from a city ID.
  City may be any city in the game.  This now always uses fast idex
  method, instead of looking through all cities of all players.
**************************************************************************/
struct city *find_city_by_id(int id)
{
  return idex_lookup_city(id);
}

/**************************************************************************
  ...
**************************************************************************/
struct city *find_city_by_name_fast(const char *name)
{
  return idex_lookup_city_by_name(name);
}

/**************************************************************************
  Find unit out of all units in game: now uses fast idex method,
  instead of looking through all units of all players.
**************************************************************************/
struct unit *find_unit_by_id(int id)
{
  return idex_lookup_unit(id);
}

/**************************************************************************
  In the server call wipe_unit(), and never this function directly.
**************************************************************************/
void game_remove_unit(struct unit *punit)
{
  struct city *pcity;

  freelog(LOG_DEBUG, "game_remove_unit %d", punit->id);
  freelog(LOG_DEBUG, "removing unit %d, %s %s (%d %d) hcity %d",
	  punit->id, get_nation_name(unit_owner(punit)->nation),
	  unit_name(punit->type), punit->tile->x, punit->tile->y,
	  punit->homecity);

  pcity = player_find_city_by_id(unit_owner(punit), punit->homecity);
  if (pcity) {
    unit_list_unlink(pcity->units_supported, punit);
  }

  if (pcity) {
    freelog(LOG_DEBUG, "home city %s, %s, (%d %d)", pcity->name,
	    get_nation_name(city_owner(pcity)->nation), pcity->tile->x,
	    pcity->tile->y);
  }

  unit_list_unlink(punit->tile->units, punit);
  unit_list_unlink(unit_owner(punit)->units, punit);

  idex_unregister_unit(punit);

  if (is_server) {
    dealloc_id(punit->id);
  }
  destroy_unit_virtual(punit);
}

/**************************************************************************
...
**************************************************************************/
void game_remove_city(struct city *pcity)
{
  freelog(LOG_DEBUG, "game_remove_city %d", pcity->id);
  freelog(LOG_DEBUG, "removing city %s, %s, (%d %d)", pcity->name,
	   get_nation_name(city_owner(pcity)->nation), pcity->tile->x,
	  pcity->tile->y);

  city_map_checked_iterate(pcity->tile, x, y, map_tile) {
    set_worker_city(pcity, x, y, C_TILE_EMPTY);
  } city_map_checked_iterate_end;
  city_list_unlink(city_owner(pcity)->cities, pcity);
  map_set_city(pcity->tile, NULL);
  idex_unregister_city(pcity);
  remove_city_virtual(pcity);
}

/***************************************************************
  ...
***************************************************************/
void game_init(void)
{
  int i;

  /* Info */
  game.info.globalwarming = 0;
  game.info.nuclearwinter = 0;
  game.info.gold = GAME_DEFAULT_GOLD;
  game.info.tech = GAME_DEFAULT_TECHLEVEL;
  game.info.skill_level = GAME_DEFAULT_SKILL_LEVEL;
  game.info.timeout = GAME_DEFAULT_TIMEOUT;
  game.info.end_year = GAME_DEFAULT_END_YEAR;
  game.info.year = GAME_START_YEAR;
  game.info.turn = 0;
  game.info.min_players = GAME_DEFAULT_MIN_PLAYERS;
  game.info.max_players = GAME_DEFAULT_MAX_PLAYERS;
  game.info.researchcost = GAME_DEFAULT_RESEARCHCOST;
  game.info.diplcost = GAME_DEFAULT_DIPLCOST;
  game.info.diplomacy = GAME_DEFAULT_DIPLOMACY;
  game.info.freecost = GAME_DEFAULT_FREECOST;
  game.info.conquercost = GAME_DEFAULT_CONQUERCOST;
  game.info.cityfactor = GAME_DEFAULT_CITYFACTOR;
  game.info.unhappysize = GAME_DEFAULT_UNHAPPYSIZE;
  game.info.angrycitizen = GAME_DEFAULT_ANGRYCITIZEN;
  game.info.foodbox = GAME_DEFAULT_FOODBOX;
  game.info.techpenalty = GAME_DEFAULT_TECHPENALTY;
  game.info.civstyle = GAME_DEFAULT_CIVSTYLE;
  game.info.spacerace = GAME_DEFAULT_SPACERACE;
  game.info.diplomacy = GAME_DEFAULT_DIPLOMACY;
  game.info.heating = 0;
  game.info.cooling = 0;
  game.info.nplayers = 0;
  for (i = 0; i < A_LAST; i++) {
    /* game.ruleset_control.num_tech_types = 0 here */
    game.info.global_advances[i] = 0;
  }
  for (i = 0; i < B_LAST; i++) {
    /* game.ruleset_control.num_impr_types = 0 here */
    game.info.global_wonders[i] = 0;
  }

  /* Traderoute-info --- Warserver */
  game.traderoute_info.traderevenuestyle = GAME_DEFAULT_TRADEREVENUESTYLE;
  game.traderoute_info.traderevenuepct = GAME_DEFAULT_TRADEREVENUEPCT;
  game.traderoute_info.caravanbonusstyle = GAME_DEFAULT_CARAVANBONUSSTYLE;
  game.traderoute_info.trademindist = GAME_DEFAULT_TRADEMINDIST;

  /* Ext-info --- Warserver */
  game.ext_info.futuretechsscore = GAME_DEFAULT_FUTURETECHSSCORE;
  game.ext_info.ignoreruleset = GAME_DEFAULT_IGNORERULESET;
  game.ext_info.goldtrading = GAME_DEFAULT_GOLDTRADING;
  game.ext_info.techtrading = GAME_DEFAULT_TECHTRADING;
  game.ext_info.improvedautoattack = GAME_DEFAULT_IMPROVEDAUTOATTACK;
  game.ext_info.stackbribing = GAME_DEFAULT_STACKBRIBING;
  game.ext_info.experimentalbribingcost = GAME_DEFAULT_EXPERIMENTALBRIBINGCOST;
  game.ext_info.citytrading = GAME_DEFAULT_CITYTRADING;
  game.ext_info.airliftingstyle = GAME_DEFAULT_AIRLIFTINGSTYLE;
  game.ext_info.teamplacement = GAME_DEFAULT_TEAMPLACEMENT;
  game.ext_info.globalwarmingon = GAME_DEFAULT_GLOBALWARMINGON;
  game.ext_info.nuclearwinteron = GAME_DEFAULT_NUCLEARWINTERON;
  game.ext_info.techleakagerate = GAME_DEFAULT_TECHLEAKAGERATE;
  game.ext_info.maxallies = GAME_DEFAULT_MAXALLIES;

  /* Ruleset */
  game.ruleset_game.tech_leakage = GAME_DEFAULT_TECHLEAKAGE;
  game.ruleset_game.tech_cost_style = GAME_DEFAULT_TECHCOSTSTYLE;
  game.ruleset_game.killstack = GAME_DEFAULT_KILLSTACK;
  game.ruleset_control.slow_invasions = GAME_DEFAULT_SLOWINVASIONS;
  game.ruleset_control.borders = GAME_DEFAULT_BORDERS;
  game.ruleset_control.happyborders = GAME_DEFAULT_HAPPYBORDERS;
  game.ruleset_control.notradesize = GAME_DEFAULT_NOTRADESIZE;
  game.ruleset_control.fulltradesize = GAME_DEFAULT_FULLTRADESIZE;
  game.ruleset_control.num_unit_types = 0;
  game.ruleset_control.num_impr_types = 0;
  game.ruleset_control.num_tech_types = 0;
  game.ruleset_control.nation_count = 0;
  game.ruleset_control.government_count = 0;
  game.ruleset_control.default_government = G_MAGIC;        /* flag */
  game.ruleset_control.government_when_anarchy = G_MAGIC;   /* flag */
  game.ruleset_control.default_building = B_LAST;

  /* Server */
  if (is_server) {
    game.server.is_new_game = TRUE;
    game.server.warminglevel = 8;
    game.server.coolinglevel = 8;
    game.server.bruteforcethreshold = GAME_DEFAULT_BRUTEFORCETHRESHOLD;
    game.server.iterplacementcoefficient
        = GAME_DEFAULT_ITERPLACEMENTCOEFFICIENT;
    game.server.maxconnections = GAME_DEFAULT_MAXCONNECTIONS;
    game.server.maxconnectionsperhost = GAME_DEFAULT_MAXCONNECTIONSPERHOST;
    game.server.teamplacementtype = GAME_DEFAULT_TEAMPLACEMENTTYPE;
    game.server.rated = GAME_DEFAULT_RATED;
    game.server.no_public_links = GAME_DEFAULT_NO_PUBLIC_LINKS;
    game.server.ruleset_loaded = FALSE;
    game.server.timeoutint = GAME_DEFAULT_TIMEOUTINT;
    game.server.timeoutintinc = GAME_DEFAULT_TIMEOUTINTINC;
    game.server.timeoutinc = GAME_DEFAULT_TIMEOUTINC;
    game.server.timeoutincmult = GAME_DEFAULT_TIMEOUTINCMULT;
    game.server.timeoutcounter = 1;
    game.server.timeoutaddenemymove = GAME_DEFAULT_TIMEOUTADDEMOVE; 
    game.server.tcpwritetimeout = GAME_DEFAULT_TCPWRITETIMEOUT;
    game.server.netwait = GAME_DEFAULT_NETWAIT;
    game.server.last_ping = 0;
    game.server.pingtimeout = GAME_DEFAULT_PINGTIMEOUT;
    game.server.pingtime = GAME_DEFAULT_PINGTIME;
    game.server.aifill = GAME_DEFAULT_AIFILL;
    game.server.diplchance = GAME_DEFAULT_DIPLCHANCE;
    game.server.dipldefchance = GAME_DEFAULT_DIPLDEFCHANCE;
    game.server.spyreturnchance = GAME_DEFAULT_SPYRETURNCHANCE;
    game.server.diplbribechance = GAME_DEFAULT_DIPLBRIBECHANCE;
    sz_strlcpy(game.server.start_units, GAME_DEFAULT_START_UNITS);
    game.server.dispersion  = GAME_DEFAULT_DISPERSION;
    game.server.citymindist = GAME_DEFAULT_CITYMINDIST;
    game.server.civilwarsize = GAME_DEFAULT_CIVILWARSIZE;
    game.server.contactturns = GAME_DEFAULT_CONTACTTURNS;
    game.server.rapturedelay= GAME_DEFAULT_RAPTUREDELAY;
    game.server.savepalace = GAME_DEFAULT_SAVEPALACE;
    game.server.natural_city_names = GAME_DEFAULT_NATURALCITYNAMES;
    game.server.aqueductloss= GAME_DEFAULT_AQUEDUCTLOSS;
    game.server.killcitizen = GAME_DEFAULT_KILLCITIZEN;
    game.server.scorelog = GAME_DEFAULT_SCORELOG;
    game.server.razechance = GAME_DEFAULT_RAZECHANCE;
    game.server.turnblock = GAME_DEFAULT_TURNBLOCK;
    game.server.fogofwar = GAME_DEFAULT_FOGOFWAR;
    game.server.fogofwar_old = game.server.fogofwar;
    game.server.auto_ai_toggle = GAME_DEFAULT_AUTO_AI_TOGGLE;
    game.server.barbarianrate = GAME_DEFAULT_BARBARIANRATE;
    game.server.onsetbarbarian = GAME_DEFAULT_ONSETBARBARIAN;
    game.server.nbarbarians = 0;
    game.server.occupychance = GAME_DEFAULT_OCCUPYCHANCE;
    game.server.revolution_length = GAME_DEFAULT_REVOLUTION_LENGTH;
    sz_strlcpy(game.server.save_name, GAME_DEFAULT_SAVE_NAME);
    game.server.save_nturns = 10;
#ifdef HAVE_LIBZ
    game.server.save_compress_level = GAME_DEFAULT_COMPRESS_LEVEL;
#else
    game.server.save_compress_level = GAME_NO_COMPRESS_LEVEL;
#endif
    game.server.seed = GAME_DEFAULT_SEED;
    game.server.watchtower_vision = GAME_DEFAULT_WATCHTOWER_VISION;
    game.server.watchtower_extra_vision = GAME_DEFAULT_WATCHTOWER_EXTRA_VISION;
    game.server.allowed_city_names = GAME_DEFAULT_ALLOWED_CITY_NAMES;
    sz_strlcpy(game.server.rulesetdir, GAME_DEFAULT_RULESETDIR);
    game.server.ai_goal_government = G_MAGIC;        /* flag */
    game.palace_building = B_LAST;
    game.land_defend_building = B_LAST;
    sz_strlcpy(game.server.demography, GAME_DEFAULT_DEMOGRAPHY);
    sz_strlcpy(game.server.allow_take, GAME_DEFAULT_ALLOW_TAKE);
    game.server.save_options.save_random = TRUE;
    game.server.save_options.save_players = TRUE;
    game.server.save_options.save_known = TRUE;
    game.server.save_options.save_starts = TRUE;
    game.server.save_options.save_private_map = TRUE;

    game.server.meta_info.user_message_set = FALSE;
    game.server.meta_info.user_message[0] = '\0';

    game.server.fcdb.id = 0;
    game.server.fcdb.outcome = GOC_NONE;
    game.server.fcdb.type = GT_FFA;
    game.server.fcdb.termap = NULL;
  }

  init_our_capability();

  idex_init();
  cm_init();
  team_init();
  
  /* XXX Why is this here? */
  terrain_control.river_help_text[0] = '\0';

  for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    player_init(&game.players[i]);
  }

  map_init();
}

/***************************************************************
  Remove all initialized players. This is all player slots, 
  since we initialize them all on game initialization.
***************************************************************/
static void game_remove_all_players(void)
{
  int i;

  for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    game_remove_player(&game.players[i]);
  }

  game.info.nplayers = 0;
  game.server.nbarbarians = 0;
}

/***************************************************************
  Frees all memory of the game.
***************************************************************/
void game_free(void)
{
  game_remove_all_players();
  map_free();
  idex_free();
  cm_free();
  /* There is no team_free. */

  /* XXX Where is this init'd ? */
  ruleset_data_free();

  if (is_server && game.server.fcdb.termap) {
    free(game.server.fcdb.termap);
    game.server.fcdb.termap = NULL;
  }
}

/***************************************************************
 Frees all memory which in objects which are read from a ruleset.
***************************************************************/
void ruleset_data_free()
{
  techs_free();
  governments_free();
  nations_free();
  unit_types_free();
  improvements_free();
  city_styles_free();
  tile_types_free();
  ruleset_cache_free();
}

/***************************************************************
...
***************************************************************/
void initialize_globals(void)
{
  players_iterate(plr) {
    city_list_iterate(plr->cities, pcity) {
      built_impr_iterate(pcity, i) {
        if (is_wonder(i)) {
          game.info.global_wonders[i] = pcity->id;
        }
      } built_impr_iterate_end;
    } city_list_iterate_end;
  } players_iterate_end;
}

/***************************************************************
  Returns the next year in the game.
***************************************************************/
int game_next_year(int year)
{
  int spaceshipparts, space_parts[3] = {0, 0, 0};

  if (year == 1) /* hacked it to get rid of year 0 */
    year = 0;

    /* !McFred: 
       - want year += 1 for spaceship.
    */

  /* test game with 7 normal AI's, gen 4 map, foodbox 10, foodbase 0: 
   * Gunpowder about 0 AD
   * Railroad  about 500 AD
   * Electricity about 1000 AD
   * Refining about 1500 AD (212 active units)
   * about 1750 AD
   * about 1900 AD
   */

  /* Count how many of the different spaceship parts we can build.  Note this
   * operates even if Enable_Space is not active. */
  if (game.info.spacerace) {
    impr_type_iterate(impr) {
      Tech_Type_id t = improvement_types[impr].tech_req;

      if (!improvement_exists(impr)) {
	continue;
      }
      if (building_has_effect(impr, EFT_SS_STRUCTURAL)
	  && tech_exists(t) && game.info.global_advances[t] != 0) {
	space_parts[0] = 1;
      }
      if (building_has_effect(impr, EFT_SS_COMPONENT)
	  && tech_exists(t) && game.info.global_advances[t] != 0) {
	space_parts[1] = 1;
      }
      if (building_has_effect(impr, EFT_SS_MODULE)
	  && tech_exists(t) && game.info.global_advances[t] != 0) {
	space_parts[2] = 1;
      }
    } impr_type_iterate_end;
  }
  spaceshipparts = space_parts[0] + space_parts[1] + space_parts[2];

  if( year >= 1900 || ( spaceshipparts>=3 && year>0 ) )
    year += 1;
  else if( year >= 1750 || spaceshipparts>=2 )
    year += 2;
  else if( year >= 1500 || spaceshipparts>=1 )
    year += 5;
  else if( year >= 1000 )
    year += 10;
  else if( year >= 0 )
    year += 20;
  else if( year >= -1000 ) /* used this line for tuning (was -1250) */
    year += 25;
  else
    year += 50; 

  if (year == 0) 
    year = 1;

  return year;
}

/***************************************************************
  Advance the game.info.year.
***************************************************************/
void game_advance_year(void)
{
  game.info.year = game_next_year(game.info.year);
  game.info.turn++;
}

/***************************************************************
...
***************************************************************/
void game_remove_player(struct player *pplayer)
{
  team_remove_player(pplayer);

  if (pplayer->attribute_block.data) {
    free(pplayer->attribute_block.data);
    pplayer->attribute_block.data = NULL;
  }
  pplayer->attribute_block.length = 0;

  if (pplayer->attribute_block_buffer.data) {
    free(pplayer->attribute_block_buffer.data);
    pplayer->attribute_block_buffer.data = NULL;
  }
  pplayer->attribute_block_buffer.length = 0;

  if (pplayer->island_improv) {
    free(pplayer->island_improv);
    pplayer->island_improv = NULL;
  }

  conn_list_free(pplayer->connections);
  pplayer->connections = NULL;

  unit_list_iterate(pplayer->units, punit) {
    game_remove_unit(punit);
  } unit_list_iterate_end;
  assert(unit_list_size(pplayer->units) == 0);
  unit_list_free(pplayer->units);
  pplayer->units = NULL;

  city_list_iterate(pplayer->cities, pcity) 
    game_remove_city(pcity);
  city_list_iterate_end;
  assert(city_list_size(pplayer->cities) == 0);
  city_list_free(pplayer->cities);
  pplayer->cities = NULL;

  if (is_server && is_barbarian(pplayer)) {
    game.server.nbarbarians--;
  }

  player_init(pplayer);
}

/***************************************************************
...
***************************************************************/
void game_renumber_players(int plrno)
{
  int i;

  for (i = plrno; i < game.info.nplayers - 1; i++) {
    game.players[i] = game.players[i + 1];
    game.players[i].player_no = i;
    conn_list_iterate(game.players[i].connections, pconn) {
      pconn->player = &game.players[i];
    } conn_list_iterate_end;
  }

  if (game.info.nplayers > 0) {
    game.info.nplayers--;

    /* reinit former last player*/
    player_init(&game.players[game.info.nplayers]);
  }
}

/**************************************************************************
get_player() - Return player struct pointer corresponding to player_id.
               Eg: player_id = punit->owner, or pcity->owner
**************************************************************************/
struct player *get_player(int player_id)
{
  assert(0 <= player_id && player_id < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS);
    return &game.players[player_id];
}

/**************************************************************************
  ...
**************************************************************************/
bool is_valid_player_id(int player_id)
{
  /* NB game.info.nplayers is 0 in the client until the game starts. */
  return player_id >= 0 && player_id < game.info.nplayers;
}

/**************************************************************************
This function is used by is_wonder_useful to estimate if it is worthwhile
to build the great library.
**************************************************************************/
int get_num_human_and_ai_players(void)
{
  return is_server ? game.info.nplayers
                   : game.info.nplayers - game.server.nbarbarians;
}

/***************************************************************
  For various data, copy eg .name to .name_orig and put
  translated version in .name
  (These could be in the separate modules, but since they are
  all almost the same, and all needed together, it seems a bit
  easier to just do them all here.)
***************************************************************/
void translate_data_names(void)
{
  int i;

  tech_type_iterate(tech_id) {
    struct advance *tthis = &advances[tech_id];

    tthis->name = Q_(tthis->name_orig);
  } tech_type_iterate_end;

  unit_type_iterate(i) {
    struct unit_type *tthis = &unit_types[i];

    tthis->name = Q_(tthis->name_orig);
  } unit_type_iterate_end;

  impr_type_iterate(i) {
    struct impr_type *tthis = &improvement_types[i];

    tthis->name = Q_(tthis->name_orig);
  } impr_type_iterate_end;

  terrain_type_iterate(i) {
    struct tile_type *tthis = get_tile_type(i);

    tthis->terrain_name = ((strcmp(tthis->terrain_name_orig, "") != 0)
			   ? Q_(tthis->terrain_name_orig) : "");

    tthis->special_1_name = ((strcmp(tthis->special_1_name_orig, "") != 0)
			     ? Q_(tthis->special_1_name_orig) : "");
    tthis->special_2_name = ((strcmp(tthis->special_2_name_orig, "") != 0)
			     ? Q_(tthis->special_2_name_orig) : "");
  } terrain_type_iterate_end;

  government_iterate(tthis) {
    int j;

    tthis->name = Q_(tthis->name_orig);
    for(j=0; j<tthis->num_ruler_titles; j++) {
      struct ruler_title *that = &tthis->ruler_titles[j];

      that->male_title = Q_(that->male_title_orig);
      that->female_title = Q_(that->female_title_orig);
    }
  } government_iterate_end;
  for (i = 0; i< game.ruleset_control.nation_count; i++) {
    struct nation_type *tthis = get_nation_by_idx(i);

    tthis->name = Q_(tthis->name_orig);
    tthis->name_plural = Q_(tthis->name_plural_orig);
  }
  for (i = 0; i < game.ruleset_control.style_count; i++) {
    struct citystyle *tthis = &city_styles[i];

    tthis->name = Q_(tthis->name_orig);
  }

}

/****************************************************************************
  Return a prettily formatted string containing the population text.  The
  population is passed in as the number of citizens, in thousands.
****************************************************************************/
const char *population_to_text(int thousand_citizen)
{
  /* big_int_to_text can't handle negative values, and in any case we'd
   * better not have a negative population. */
  assert(thousand_citizen >= 0);
  return big_int_to_text(thousand_citizen, 3);
}

/****************************************************************************
  Determines and returns the type of game (team, freeforall, duel, etc.)
  based on the current teams/players/ais.

  NB: Because an AI player might be added during the game (e.g. from a
  civil war split), calling the function after the game has started may
  not give the expected result.
****************************************************************************/
enum game_types game_determine_type(void)
{
  int ais = 0, players = 0;
  int num_teams = team_count();
  int max_team_size = 0;

  players_iterate(pplayer) {
    if (is_barbarian(pplayer) || pplayer->is_observer) {
      continue;
    }
    if (!pplayer->is_connected
        && pplayer->ai.control
        && pplayer->was_created) {
      ais++;
    } else {
      players++;
    }
  } players_iterate_end;

  team_iterate(pteam) {
    int size = team_count_members(pteam->id);
    if (size > max_team_size) {
      max_team_size = size;
    }
  } team_iterate_end;

  if (players == 1 && ais == 0) {
    return GT_SOLO;

  } else if (players == 2
             && ais == 0
             && max_team_size < 2) {
    return GT_DUEL;

  } else if (players > 2
             && num_teams > 0
             && max_team_size > 1) {
    return GT_TEAM;

  } else if (max_team_size < 2
             && players > 2) {
    return GT_FFA;

  } else {
    return GT_MIXED;
  }

  return GT_NUM_TYPES;
}

/****************************************************************************
  ...
****************************************************************************/
enum game_types game_get_type_from_string(const char *s)
{
  int i;

  for (i = 0; i < GT_NUM_TYPES; i++) {
#ifdef ENABLE_NLS
    if (0 == mystrcasecmp(s, game_type_strings[i])
	|| 0 == mystrcasecmp(s, _(game_type_strings[i]))) {
#else
    if (0 == mystrcasecmp(s, game_type_strings[i])) {
#endif /* ENABLE_NLS */
      return i;
    }
  }
  return GT_NUM_TYPES;
}

/****************************************************************************
  ...
****************************************************************************/
const char *game_type_name_orig(enum game_types type)
{
  assert(0 <= type && type < GT_NUM_TYPES);
  return game_type_strings[type];
}

/****************************************************************************
  ...
****************************************************************************/
const char *game_type_name(enum game_types type)
{
  assert(0 <= type && type < GT_NUM_TYPES);
  return _(game_type_strings[type]);
}

/****************************************************************************
  ...
****************************************************************************/
const char *game_outcome_name_orig(enum game_outcomes outcome)
{
  assert(0 <= outcome && outcome < GOC_NUM_OUTCOMES);
  return game_outcome_strings[outcome];
}

/****************************************************************************
  ...
****************************************************************************/
const char *game_outcome_name(enum game_outcomes outcome)
{
  assert(0 <= outcome && outcome < GOC_NUM_OUTCOMES);
  return _(game_outcome_strings[outcome]);
}
