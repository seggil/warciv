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
#include <stdio.h> /* for remove() */ 

#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "registry.h"
#include "shared.h"
#include "support.h"

#include "city.h"
#include "events.h"
#include "improvement.h"
#include "map.h"
#include "packets.h"

#include "connecthand.h"
#include "maphand.h"
#include "plrhand.h"
#include "unittools.h"

#include "gamehand.h"
#include "srv_main.h"

#define CHALLENGE_ROOT "challenge"

typedef struct team_mapping_s {
    Team_Type_id team_id;
    int member_count;
}team_mapping_t;

team_mapping_t mapping[MAX_NUM_TEAMS+1];
int mappings;
bool brute_force_found_solution = FALSE;
int best_team_pos[MAX_NUM_PLAYERS];
int team_pos[MAX_NUM_PLAYERS];

int best_start_pos[MAX_NUM_PLAYERS];
int best_score = 0;
int repeat = 0;

/****************************************************************************
  prototypes
****************************************************************************/
int calculate_team_distance(struct tile *ptile1, struct tile *ptile2);
int calculate_score(int *start_pos);
void swap_int(int *a, int *b);
void shuffle_start_positions_by_iter(int *start_pos);
int calculate_delta_score(int *start_pos, int depth);
void find_pos_by_brute_force(int *positions, int a);
void clean_start_pos(int *positions);
void assign_players_to_positions(int *best_team_pos, int *best_start_pos);
void calculate_team_mapping(void);
int get_team_mapping(Team_Type_id team);
void shuffle_start_positions(int *start_pos);

/****************************************************************************
  Initialize the game.server.id variable to a random string of characters.
****************************************************************************/
static void init_game_id(void)
{
  static const char chars[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i;

  for (i = 0; i < sizeof(game.server.id) - 1; i++) {
    game.server.id[i] = chars[myrand(sizeof(chars) - 1)];
  }
  game.server.id[i] = '\0';
}

/****************************************************************************
  Place a starting unit for the player.
****************************************************************************/
static void place_starting_unit(struct tile *ptile, struct player *pplayer,
				char crole)
{
  Unit_Type_id utype;
  enum unit_flag_id role;

  assert(!is_non_allied_unit_tile(ptile, pplayer));

  /* For scenarios or dispersion, huts may coincide with player starts (in 
   * other cases, huts are avoided as start positions).  Remove any such hut,
   * and make sure to tell the client, since we may have already sent this
   * tile (with the hut) earlier: */
  if (map_has_special(ptile, S_HUT)) {
    map_clear_special(ptile, S_HUT);
    update_tile_knowledge(ptile);
    freelog(LOG_VERBOSE, "Removed hut on start position for %s",
	    pplayer->name);
  }

  /* Expose visible area. */
  circle_iterate(ptile, game.ruleset_game.init_vis_radius_sq, ctile) {
    show_area(pplayer, ctile, 0);
  } circle_iterate_end;

  switch(crole) {
  case 'c':
    role = L_CITIES;
    break;
  case 'w':
    role = L_SETTLERS;
    break;
  case 'x':
    role = L_EXPLORER;
    break;
  case 'k':
    role = L_GAMELOSS;
    break;
  case 's':
    role = L_DIPLOMAT;
    break;
  case 'f':
    role = L_FERRYBOAT;
    break;
  case 'd':
    role = L_DEFEND_OK;
    break;
  case 'D':
    role = L_DEFEND_GOOD;
    break;
  case 'a':
    role = L_ATTACK_FAST;
    break;
  case 'A':
    role = L_ATTACK_STRONG;
    break;
  default: 
    assert(FALSE);
    return;
  }

  /* Create the unit of an appropriate type, if it exists */
  if (num_role_units(role) > 0) {
    utype = first_role_unit_for_player(pplayer, role);
    if (utype == U_LAST) {
      utype = get_role_unit(role, 0);
    }

    /* We cannot currently handle sea units as start units. */
    if (unit_types[utype].move_type == SEA_MOVING) {
      freelog(LOG_ERROR, _("Sea moving start units are not yet supported, "
                           "%s not created."), unit_types[utype].name);
      notify_player(pplayer, _("Sea moving start units are not yet supported. "
                               "Nobody gets %s."), unit_types[utype].name);
      return;
    }

    (void) create_unit(pplayer, ptile, utype, FALSE, 0, -1);
  }
}

/****************************************************************************
  Calculate the distance relative to the team placement type.
****************************************************************************/
int calculate_team_distance(struct tile *ptile1, struct tile *ptile2)
{
  int dx, dy;

  map_distance_vector(&dx, &dy, ptile1, ptile2);
  switch (game.server.teamplacementtype) {
    case 1:
      if (map_get_continent(ptile1) != map_get_continent(ptile2)) {
        return NATIVE_WIDTH + NATIVE_HEIGHT
               + map_vector_to_real_distance(dx, dy);
      }
      /* break not missing here */
    case 0:
      return map_vector_to_real_distance(dx, dy);
    case 2:
      return abs(dy);
    case 3:
      return abs(dx);
    default:
      break;
  }

  freelog(LOG_ERROR, "Unkown team placement type.");
  abort();

  return -1;
}

/****************************************************************************
  Calculate score for the best starting positions with regard to teams
  If nobody is on team, score is 0.
****************************************************************************/
int calculate_score(int *start_pos)
{
  int score = 0;
  int x, y;

  for (x = 0; x < game.info.nplayers; x++ ) {
    for (y = x + 1; y < game.info.nplayers; y++ ) {
      if (start_pos[x] == start_pos[y]) {
	score += calculate_team_distance(map.server.start_positions[x].tile,
					 map.server.start_positions[y].tile);
      }
    }
  }
  return score;
}

/*****************************************************************************
  Simple swap
*****************************************************************************/
void swap_int(int *a, int *b)
{
  int c;

  c = *a;
  *a = *b;
  *b = c;
}

/****************************************************************************
    Shuffle positions so that team players end up close
    Always try to exchange 2 positions
****************************************************************************/
void shuffle_start_positions_by_iter(int *start_pos)
{
  int tabtabu[game.info.nplayers][game.info.nplayers];
  int i,x,y, found = 1;
  int tmp;
  int score, best_local_score = 0, best_local_x = 0, best_local_y = 0;

  while (found
	 && (repeat < game.info.nplayers
	     /* Limit # of iterations. */
	     * game.server.iterplacementcoefficient)) {
    repeat++;
    if (repeat % (game.info.nplayers * game.server.iterplacementcoefficient / 8)
	== 0) {
      notify_conn(NULL,
		  _("Iterative team placement in progress, %d%% complete."),
		  repeat * 100 / (game.info.nplayers
				  * game.server.iterplacementcoefficient));
    }
    found = 0;
    best_local_score = 99999999;
    for (x = 0; x < game.info.nplayers; x++ ) {
      for (y = x + 1; y < game.info.nplayers; y++ ) {
	if (start_pos[x] == start_pos[y]) {
	  /* Same teams, no reason to swap. */
	  continue;
	}
	if (tabtabu[x][y] > 0) {
	  tabtabu[x][y]--;
	}
	swap_int(&start_pos[x], &start_pos[y]);
	/* Calculate how much this swap is worth. */
	score = calculate_score(start_pos);
	swap_int(&start_pos[x], &start_pos[y]);
	if (!tabtabu[x][y] && score < best_local_score) {
	  /* Exchange starting positions. */
	  best_local_score = score;
	  best_local_x = x; /* Candidate for exchange. */
	  best_local_y = y;
	  found = 1;
	}
      }
    }
    if (found && best_local_score < best_score) {
      /* New best solution found. */
      tmp = start_pos[best_local_x];
      start_pos[best_local_x] = start_pos[best_local_y];
      start_pos[best_local_y] = tmp;
      best_score = best_local_score;
      freelog(LOG_VERBOSE, "Exchanging (%d, %d) with score %d.",
	      best_local_x, best_local_y, best_score);
      tabtabu[best_local_x][best_local_y] += game.info.nplayers;
      for (i = 0; i < game.info.nplayers; i++ ) {
	/* Remember new best solution. */
	best_team_pos[i] = start_pos[i];
      }
    } else if (found) {
      /* Other solution found which is worse than the best one. */
      tmp = start_pos[best_local_x];
      start_pos[best_local_x] = start_pos[best_local_y];
      start_pos[best_local_y] = tmp;
      /* But we do not rewrite the best solution found
       * we remember that we did this exchange. */
      tabtabu[best_local_x][best_local_y] += game.info.nplayers;
    }
  }
}


/****************************************************************************
  Calculate score for the current starting positions with regard to teams
  - optimized variant
  - used in brute force
  If nobody is in team, score is 0.
****************************************************************************/
int calculate_delta_score(int *start_pos, int depth)
{
  int score = 0;
  static int x;

  assert(start_pos != NULL && depth >= 0 && depth < MAX_NUM_PLAYERS);
  for (x = 0; x < depth; x++) {
    if(start_pos[x] == start_pos[depth]) {
      /* Then they are on the same team. */
      score += calculate_team_distance(map.server.start_positions[x].tile,
				       map.server.start_positions[depth].tile);
    }
  }
  return score;
}

/****************************************************************************
  Pruning brute force algorithm
****************************************************************************/
void find_pos_by_brute_force(int *positions, int a)
{
  static int depth = 0;
  static int score = 0;
  int tmpscore;
  int i, p;

  assert(positions != NULL && a >= 0 && a < MAX_NUM_TEAMS + 1);
  /* Add team to pos [depth]. */
  positions[depth] = a;
  mapping[a].member_count--;

  tmpscore = calculate_delta_score(positions, depth);
  score += tmpscore;

  if (depth == (game.info.nplayers - 1)) {
    /* Enough players. */
    repeat++;    
    if (score < best_score) {
      for (p = 0; p < game.info.nplayers; p++ ) {
	best_team_pos[p] = positions[p];
      }
      best_score = score;
      freelog(LOG_VERBOSE, "New best score %d.", best_score);
      brute_force_found_solution = TRUE;
    }
  } else {
    /* We continue adding players. */
    depth++;

    if (score < best_score) {
      /* Choose next team for [depth+1] position. */
      for (i = 0; i < mappings; i++ ) {
	if (mapping[i].member_count > 0) {
	  find_pos_by_brute_force(positions, i);
	}
      }
    }
    depth--;
  }
  /* Remove player from pos [depth]. */
  positions[depth] = -1;
  mapping[a].member_count++;
  score -= tmpscore;
}

/****************************************************************************
   Cleans start positions for brute force algorithm
****************************************************************************/
void clean_start_pos(int *positions)
{
  assert(positions != NULL);
  memset(positions, -1, sizeof(int) * game.info.nplayers);
}

/****************************************************************************
   Assigns players to team start positions by using team mapping
****************************************************************************/
void assign_players_to_positions(int *best_team_pos, int *best_start_pos)
{
  int i;
  bool error = FALSE;
  Team_Type_id team_id;

  freelog(LOG_VERBOSE, "entry");
  /* Starting positions were assigned to team indexes in team mapping. */
  for (i = 0; i < game.info.nplayers; i++ ) {
    assert(best_team_pos[i] >= 0 && best_team_pos[i] < MAX_NUM_TEAMS+1);
    team_id = mapping[best_team_pos[i]].team_id;
    freelog(LOG_VERBOSE, "Assigning position %d to team index %d, team_id %d.",
	    i, best_team_pos[i], mapping[best_team_pos[i]].team_id);
    error = TRUE;
    players_iterate(pplayer) {
      if (pplayer->team == team_id && pplayer->team_placement_flag == FALSE) {
	freelog(LOG_VERBOSE, "Assigning position %d to player %d",
		i, pplayer->player_no);
	pplayer->team_placement_flag = TRUE;
	best_start_pos[pplayer->player_no] = i;
	error = FALSE;
	break;
      }
    } players_iterate_end
    assert(!error);
  }
  freelog(LOG_VERBOSE, "exit");    
}

/****************************************************************************
   Calculate team mapping for brute force placement algorithm.
   Team mapping is needed because we want to consider players with TEAM_NONE
   to be in the same team and thus greatly improving algorithm speed.
****************************************************************************/
void calculate_team_mapping(void)
{
  freelog(LOG_DEBUG, "entry");
  mapping[0].team_id = TEAM_NONE;
  mapping[0].member_count = team_count_members(TEAM_NONE);
  freelog(LOG_VERBOSE, "Team TEAM_NONE has %d members",
	  mapping[0].member_count);
  mappings = 1;
  
  team_iterate(pteam) {
    mapping[mappings].team_id = pteam->id;
    mapping[mappings].member_count = pteam->member_count;
    freelog(LOG_VERBOSE, "Team %d has %d members",
	    pteam->id, pteam->member_count);
    mappings++;
  } team_iterate_end
  freelog(LOG_VERBOSE, "Total mappings %d",mappings);
  freelog(LOG_DEBUG, "exit");
}

/****************************************************************************
    Find team id from mapping
****************************************************************************/
int get_team_mapping(Team_Type_id team)
{
  int i;

  for (i = 0; i < mappings; i++) {
    if (mapping[i].team_id == team) {
      return i;
    }
  }
  assert(FALSE);
  return 0;
}

/****************************************************************************
    Shuffle start positions so team players are close
****************************************************************************/
void shuffle_start_positions(int *start_pos)
{
  int i;
  struct player *pplayer;
  assert(start_pos != NULL);

  calculate_team_mapping();
  for (i = 0; i < game.info.nplayers; i++) {
    pplayer = get_player(i);
    assert(pplayer != NULL);
    pplayer->team_placement_flag = FALSE;

    best_start_pos[i] = -1;
    team_pos[i] = best_team_pos[i] = get_team_mapping(pplayer->team);
    freelog(LOG_VERBOSE, "Setting team pos %d = %d.", i, team_pos[i]);
  }
  best_score = calculate_score(best_team_pos);
  freelog(LOG_VERBOSE, "Current best score is %d.", best_score);

  if (game.info.nplayers <= game.server.bruteforcethreshold) {
    /* Brute force for small number of players. */
    notify_conn(NULL, _("Using brute force team placement algorithm?"));
    freelog(LOG_VERBOSE, "Using brute force algorithm?");
    clean_start_pos(team_pos);
    for (i = 0; i < mappings; i++) {
      if (mapping[i].member_count > 0) {
        find_pos_by_brute_force(team_pos, i);
      }
      notify_conn(NULL,
                  _("Brute force team placement in progress, %d%% complete."),
                  (i + 1) * 100 / mappings);
    }
  } else {
    freelog(LOG_VERBOSE, "Using iterative team placement algorithm.");
    notify_conn(NULL, _("Using iterative team placement algorithm."));
    shuffle_start_positions_by_iter(team_pos);
  }
  assign_players_to_positions(best_team_pos, best_start_pos);

  freelog(LOG_VERBOSE, "Iterations: %d.", repeat);
  freelog(LOG_VERBOSE, "Final score checked: %d.",
          calculate_score(best_team_pos));
  for (i = 0; i < game.info.nplayers; i++) { //restore best solution
    start_pos[i] = best_start_pos[i];
  }
}
/****************************************************************************
  Initialize a new game: place the players' units onto the map, etc.
****************************************************************************/
void init_new_game(void)
{
  const int NO_START_POS = -1;
  int start_pos[game.info.nplayers];
  bool pos_used[map.server.num_start_positions];
  int i, num_used = 0;

  init_game_id();

  /* Shuffle starting positions around so that they match up with the
   * desired players. */

  /* First set up some data fields. */
  freelog(LOG_VERBOSE, "Placing players at start positions.");
  for (i = 0; i < map.server.num_start_positions; i++) {
    Nation_Type_id n = map.server.start_positions[i].nation;

    pos_used[i] = FALSE;
    freelog(LOG_VERBOSE, "%3d : (%2d,%2d) : %d : %s",
	    i, map.server.start_positions[i].tile->x,
	    map.server.start_positions[i].tile->y,
	    n, (n >= 0 ? get_nation_name(n) : ""));
  }
  players_iterate(pplayer) {
    start_pos[pplayer->player_no] = NO_START_POS;
  } players_iterate_end;

  /* Second, assign a nation to a start position for that nation. */
  freelog(LOG_VERBOSE, "Assigning matching nations.");
  players_iterate(pplayer) {
    for (i = 0; i < map.server.num_start_positions; i++) {
      assert(pplayer->nation != NO_NATION_SELECTED);
      if (pplayer->nation == map.server.start_positions[i].nation) {
	freelog(LOG_VERBOSE, "Start_pos %d matches player %d (%s).",
		i, pplayer->player_no, get_nation_name(pplayer->nation));
	start_pos[pplayer->player_no] = i;
	pos_used[i] = TRUE;
	num_used++;
      }
    }
  } players_iterate_end;

  /* Third, assign players randomly to the remaining start positions. */
  freelog(LOG_VERBOSE, "Assigning random nations.");
  players_iterate(pplayer) {
    if (start_pos[pplayer->player_no] == NO_START_POS) {
      int which = myrand(map.server.num_start_positions - num_used);

      for (i = 0; i < map.server.num_start_positions; i++) {
	if (!pos_used[i]) {
	  if (which == 0) {
	    freelog(LOG_VERBOSE,
		    "Randomly assigning player %d (%s) to pos %d.",
		    pplayer->player_no, get_nation_name(pplayer->nation), i);
	    start_pos[pplayer->player_no] = i;
	    pos_used[i] = TRUE;
	    num_used++;
	    break;
	  }
	  which--;
	}
      }
    }
    assert(start_pos[pplayer->player_no] != NO_START_POS);
  } players_iterate_end;

  if (game.ext_info.teamplacement && map.server.generator != 7) {
    shuffle_start_positions(start_pos);
  }

  /* Loop over all players, creating their initial units... */
  players_iterate(pplayer) {
    struct start_position pos
      = map.server.start_positions[start_pos[pplayer->player_no]];

    /* Place the first unit. */
    place_starting_unit(pos.tile, pplayer, game.server.start_units[0]);
  } players_iterate_end;

  /* Place all other units. */
  players_iterate(pplayer) {
    int i, x, y;
    struct tile *ptile;
    struct start_position p
      = map.server.start_positions[start_pos[pplayer->player_no]];

    assert(!is_ocean(map_get_terrain(p.tile)));

    for (i = 1; i < strlen(game.server.start_units); i++) {
      do {
	x = p.tile->x + myrand(2 * game.server.dispersion + 1) - game.server.dispersion;
	y = p.tile->y + myrand(2 * game.server.dispersion + 1) - game.server.dispersion;
      } while (!((ptile = map_pos_to_tile(x, y))
		 && map_get_continent(p.tile) == map_get_continent(ptile)
		 && !is_ocean(map_get_terrain(ptile))
		 && !is_non_allied_unit_tile(ptile, pplayer)));


      /* Create the unit of an appropriate type. */
      place_starting_unit(ptile, pplayer, game.server.start_units[i]);
    }
  } players_iterate_end;

  /* Initialise list of improvements with world-wide equiv_range */
  improvement_status_init(game.improvements, ARRAY_SIZE(game.improvements));
}

/**************************************************************************
...
**************************************************************************/
void send_start_turn_to_clients(void)
{
  lsend_packet_start_turn(game.game_connections);
}

/**************************************************************************
  Tell clients the year, and also update turn_done and nturns_idle fields
  for all players.
**************************************************************************/
void send_year_to_clients(int year)
{
  struct packet_new_year apacket;
  int i;
  
  for(i=0; i<game.info.nplayers; i++) {
    struct player *pplayer = &game.players[i];
    pplayer->turn_done = FALSE;
    pplayer->nturns_idle++;
  }

  apacket.year = year;
  apacket.turn = game.info.turn;
  lsend_packet_new_year(game.game_connections, &apacket);

  /* Hmm, clients could add this themselves based on above packet? */
  notify_conn_ex(game.game_connections, NULL, E_NEXT_YEAR, _("Year: %s"),
		 textyear(year));
}


/**************************************************************************
  Send specified state; should be a CLIENT_GAME_*_STATE ?
  (But note client also changes state from other events.)
**************************************************************************/
void send_game_state(struct conn_list *dest, int state)
{
  dlsend_packet_game_state(dest, state);
}


/**************************************************************************
  Send game_info packet; some server options and various stuff...
  dest==NULL means game.game_connections
**************************************************************************/
void send_game_info(struct conn_list *dest)
{
  if (!dest) {
    dest = game.game_connections;
  }

  /* the following values are computed every
     time a packet_game_info packet is created */
  if (game.info.timeout != 0) {
    game.info.seconds_to_turndone =
      game.server.turn_start + game.info.timeout - time(NULL);
  } else {
    /* unused but at least initialized */
    game.info.seconds_to_turndone = -1;
  }

  conn_list_iterate(dest, pconn) {
    game.info.player_idx = (pconn->player ? pconn->player->player_no : -1);
    send_packet_game_info(pconn, &game.info);
    if (has_capability("extroutes", pconn->capability)) {
      send_packet_traderoute_info(pconn, &game.traderoute_info);
    }
    if (has_capability("extgameinfo", pconn->capability)) {
      send_packet_extgame_info(pconn, &game.ext_info);
    }
  } conn_list_iterate_end;
}

/**************************************************************************
  adjusts game.info.timeout based on various server options

  timeoutint: adjust game.info.timeout every timeoutint turns
  timeoutinc: adjust game.info.timeout by adding timeoutinc to it.
  timeoutintinc: every time we adjust game.info.timeout, we add timeoutintinc
                 to timeoutint.
  timeoutincmult: every time we adjust game.info.timeout, we multiply timeoutinc
                  by timeoutincmult
**************************************************************************/
int update_timeout(void)
{
  /* if there's no timer or we're doing autogame, do nothing */
  if (game.info.timeout < 1 || game.server.timeoutint == 0) {
    return game.info.timeout;
  }

  if (game.server.timeoutcounter >= game.server.timeoutint) {
    game.info.timeout += game.server.timeoutinc;
    game.server.timeoutinc *= game.server.timeoutincmult;

    game.server.timeoutcounter = 1;
    game.server.timeoutint += game.server.timeoutintinc;

    if (game.info.timeout > GAME_MAX_TIMEOUT) {
      notify_conn_ex(game.game_connections, NULL, E_NOEVENT,
		     _("The turn timeout has exceeded its maximum value, "
		       "fixing at its maximum"));
      freelog(LOG_DEBUG, "game.info.timeout exceeded maximum value");
      game.info.timeout = GAME_MAX_TIMEOUT;
      game.server.timeoutint = 0;
      game.server.timeoutinc = 0;
    } else if (game.info.timeout < 0) {
      notify_conn_ex(game.game_connections, NULL, E_NOEVENT,
		     _("The turn timeout is smaller than zero, "
		       "fixing at zero."));
      freelog(LOG_DEBUG, "game.info.timeout less than zero");
      game.info.timeout = 0;
    }
  } else {
    game.server.timeoutcounter++;
  }

  freelog(LOG_DEBUG, "timeout=%d, inc=%d incmult=%d\n   "
	  "int=%d, intinc=%d, turns till next=%d",
	  game.info.timeout, game.server.timeoutinc, game.server.timeoutincmult,
	  game.server.timeoutint, game.server.timeoutintinc,
	  game.server.timeoutint - game.server.timeoutcounter);

  return game.info.timeout;
}

/**************************************************************************
  adjusts game.server.turn_start when enemy moves an unit, we see it and the 
  remaining timeout is smaller than the option
  It's possible to use a simular function to do that per player.
**************************************************************************/
void increase_timeout_because_unit_moved(void)
{
  if (game.info.timeout != 0){
    int seconds_to_turndone = game.server.turn_start + game.info.timeout - time(NULL);

    if (seconds_to_turndone < game.server.timeoutaddenemymove){
      game.server.turn_start = time(NULL) - game.info.timeout + game.server.timeoutaddenemymove;
      send_game_info(NULL);
    }	
  }
}

/************************************************************************** 
  generate challenge filename for this connection, cannot fail.
**************************************************************************/
static void gen_challenge_filename(struct connection *pc)
{
}

/************************************************************************** 
  get challenge filename for this connection.
**************************************************************************/
static const char *get_challenge_filename(struct connection *pc)
{
  static char filename[MAX_LEN_PATH];

  my_snprintf(filename, sizeof(filename), "%s_%d_%d",
      CHALLENGE_ROOT, srvarg.port, pc->id);

  return filename;
}

/************************************************************************** 
  get challenge full filename for this connection.
**************************************************************************/
static const char *get_challenge_fullname(struct connection *pc)
{
  static char fullname[MAX_LEN_PATH];

  interpret_tilde(fullname, sizeof(fullname), "~/.freeciv/");
  sz_strlcat(fullname, get_challenge_filename(pc));

  return fullname;
}

/************************************************************************** 
  find a file that we can write too, and return it's name.
**************************************************************************/
const char *new_challenge_filename(struct connection *pc)
{
  if (!has_capability("new_hack", pc->capability)) {
    return "";
  }

  gen_challenge_filename(pc);
  return get_challenge_filename(pc);
}


/************************************************************************** 
opens a file specified by the packet and compares the packet values with
the file values. Sends an answer to the client once it's done.
**************************************************************************/
void handle_single_want_hack_req(struct connection *pc,
    				 const struct packet_single_want_hack_req
				 *packet)
{
  struct section_file file;
  char *token = NULL;
  bool you_have_hack = FALSE;

  if (!has_capability("new_hack", pc->capability)
      || user_action_list_size(on_connect_user_actions) > 0) {
    dsend_packet_single_want_hack_reply(pc, FALSE);
    return ;
  }

  if (section_file_load_nodup(&file, get_challenge_fullname(pc))) {
    token = secfile_lookup_str_default(&file, NULL, "challenge.token");
    you_have_hack = (token && strcmp(token, packet->token) == 0);
    section_file_free(&file);
  }

  if (!token) {
    freelog(LOG_DEBUG, "Failed to read authentication token");
  }

  if (you_have_hack) {
    pc->server.granted_access_level = pc->server.access_level = ALLOW_HACK;
  }

  dsend_packet_single_want_hack_reply(pc, you_have_hack);
}
