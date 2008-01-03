/**********************************************************************
 Freeciv - Copyright (C) 2003 - The Freeciv Project
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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "database.h"
#include "fcintl.h"
#include "game.h"
#include "improvement.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "player.h"
#include "plrhand.h"
#include "srv_main.h"
#include "shared.h"
#include "support.h"
#include "terrain.h"
#include "score.h"
#include "unit.h"

/* Calibrated so that if a player with RD 50 does
 * not play for 3 years, his RD will become 350.
 * A 'rating period' is assumed to be a typical
 * game length, i.e. 3 hours. */
#define RATING_CONSTANT_SECONDS_PER_RATING_PERIOD 10800
#define RATING_CONSTANT_C 3.69993869
#define RATING_CONSTANT_Q 0.00575646273 /* ln(10)/400 */

/* The RD given to new players. */
#define RATING_CONSTANT_MAXIMUM_RD 350

/* To ensure that ratings do not update too slowly
 * if you play lots of games, RDs will not fall below this
 * value. */
#define RATING_CONSTANT_MINIMUM_RD 30

/* How much should the difference between a team member's
 * rating and the average player rating affect the rating
 * of the team as a whole?
 * (See the relevant section in update_ratings for
 * how it is used). */
#define RATING_CONSTANT_EXTRA_TEAMMATE_INFLUENCE 0.3

/* The rating given to new players. */
#define RATING_CONSTANT_AVERAGE_PLAYER_RATING 1500

/* The rating at which a player becomes more of a
 * liability to his teammates rather than a boon. */
#define RATING_CONSTANT_BAD_PLAYER_RATING 1000

/* Used to determine the 'rating' of the 'opponent' for
 * solo games. Calibrated so that an average player (1500)
 * can win a solo game at turn 100 with score 300. */
#define RATING_CONSTANT_SOLO_RATING_COEFFICIENT 505

/* Scores differing by less than this are considered equal. */
#define MINIMUM_SCORE_DIFFERENCE 1.0

/* Avoid having to recalculate civ scores all the time. */
static int score_cache[MAX_NUM_PLAYERS+MAX_NUM_BARBARIANS];

static struct grouping groupings[MAX_NUM_PLAYERS];
static int num_groupings = 0;

/**************************************************************************
  Allocates, fills and returns a land area claim map.
  Call free_landarea_map(&cmap) to free allocated memory.
**************************************************************************/

#define USER_AREA_MULT 1000

struct claim_cell {
  int when;
  int whom;
  int know;
  int cities;
};

struct claim_map {
  struct claim_cell *claims;
  int *player_landarea;
  int *player_owndarea;
  struct tile **edges;
};

/**************************************************************************
Land Area Debug...
**************************************************************************/

#define LAND_AREA_DEBUG 0

#if LAND_AREA_DEBUG >= 2

static char when_char(int when)
{
  static char list[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
    'y', 'z'
  };

  return (when >= 0 && when < sizeof(list)) ? list[when] : '?';
}

/* 
 * Writes the map_char_expr expression for each position on the map.
 * map_char_expr is provided with the variables x,y to evaluate the
 * position. The 'type' argument is used for formatting by printf; for
 * instance it should be "%c" for characters.  The data is printed in a
 * native orientation to make it easier to read.  
 */
#define WRITE_MAP_DATA(type, map_char_expr)        \
{                                                  \
  int nat_x, nat_y;                                \
  for (nat_x = 0; nat_x < map.xsize; nat_x++) {    \
    printf("%d", nat_x % 10);                      \
  }                                                \
  putchar('\n');                                   \
  for (nat_y = 0; nat_y < map.ysize; nat_y++) {    \
    printf("%d ", nat_y % 10);                     \
    for (nat_x = 0; nat_x < map.xsize; nat_x++) {  \
      int x, y;                                    \
      NATIVE_TO_MAP_POS(&x, &y, nat_x,nat_y);      \
      printf(type, map_char_expr);                 \
    }                                              \
    printf(" %d\n", nat_y % 10);                   \
  }                                                \
}

/**************************************************************************
  Prints the landarea map to stdout (a debugging tool).
**************************************************************************/
static void print_landarea_map(struct claim_map *pcmap, int turn)
{
  int p;

  if (turn == 0) {
    putchar('\n');
  }

  if (turn == 0) {
    printf("Player Info...\n");

    for (p = 0; p < game.nplayers; p++) {
      printf(".know (%d)\n  ", p);
      WRITE_MAP_DATA("%c",
		     TEST_BIT(pcmap->claims[map_pos_to_index(x, y)].know,
			      p) ? 'X' : '-');
      printf(".cities (%d)\n  ", p);
      WRITE_MAP_DATA("%c",
		     TEST_BIT(pcmap->
			      claims[map_pos_to_index(x, y)].cities,
			      p) ? 'O' : '-');
    }
  }

  printf("Turn %d (%c)...\n", turn, when_char (turn));

  printf(".whom\n  ");
  WRITE_MAP_DATA((pcmap->claims[map_pos_to_index(x, y)].whom ==
		  32) ? "%c" : "%X",
		 (pcmap->claims[map_pos_to_index(x, y)].whom ==
		  32) ? '-' : pcmap->claims[map_pos_to_index(x, y)].whom);

  printf(".when\n  ");
  WRITE_MAP_DATA("%c", when_char(pcmap->claims[map_pos_to_index(x, y)].when));
}

#endif

static int no_owner = MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS;

/**************************************************************************
  allocate and clear claim map; determine city radii
**************************************************************************/
static void build_landarea_map_new(struct claim_map *pcmap)
{
  int nbytes;

  nbytes = map.xsize * map.ysize * sizeof(struct claim_cell);
  pcmap->claims = fc_malloc(nbytes);
  memset(pcmap->claims, 0, nbytes);

  nbytes = game.nplayers * sizeof(int);
  pcmap->player_landarea = fc_malloc(nbytes);
  memset(pcmap->player_landarea, 0, nbytes);

  nbytes = game.nplayers * sizeof(int);
  pcmap->player_owndarea = fc_malloc(nbytes);
  memset(pcmap->player_owndarea, 0, nbytes);

  nbytes = 2 * map.xsize * map.ysize * sizeof(*pcmap->edges);
  pcmap->edges = fc_malloc(nbytes);

  players_iterate(pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
      map_city_radius_iterate(pcity->tile, tile1) {
	pcmap->claims[tile1->index].cities |= (1u << pcity->owner);
      } map_city_radius_iterate_end;
    } city_list_iterate_end;
  } players_iterate_end;
}

/**************************************************************************
  0th turn: install starting points, counting their tiles
**************************************************************************/
static void build_landarea_map_turn_0(struct claim_map *pcmap)
{
  int turn, owner;
  struct tile **nextedge;
  struct claim_cell *pclaim;

  turn = 0;
  nextedge = pcmap->edges;

  whole_map_iterate(ptile) {
    int i = ptile->index;

    pclaim = &pcmap->claims[i];
    ptile = &map.tiles[i];

    if (is_ocean(ptile->terrain)) {
      /* pclaim->when = 0; */
      pclaim->whom = no_owner;
      /* pclaim->know = 0; */
    } else if (ptile->city) {
      owner = ptile->city->owner;
      pclaim->when = turn + 1;
      pclaim->whom = owner;
      *nextedge = ptile;
      nextedge++;
      pcmap->player_landarea[owner]++;
      pcmap->player_owndarea[owner]++;
      pclaim->know = ptile->known;
    } else if (ptile->worked) {
      owner = ptile->worked->owner;
      pclaim->when = turn + 1;
      pclaim->whom = owner;
      *nextedge = ptile;
      nextedge++;
      pcmap->player_landarea[owner]++;
      pcmap->player_owndarea[owner]++;
      pclaim->know = ptile->known;
    } else if (unit_list_size(ptile->units) > 0) {
      owner = (unit_list_get(ptile->units, 0))->owner;
      pclaim->when = turn + 1;
      pclaim->whom = owner;
      *nextedge = ptile;
      nextedge++;
      pcmap->player_landarea[owner]++;
      if (TEST_BIT(pclaim->cities, owner)) {
	pcmap->player_owndarea[owner]++;
      }
      pclaim->know = ptile->known;
    } else {
      /* pclaim->when = 0; */
      pclaim->whom = no_owner;
      pclaim->know = ptile->known;
    }
  } whole_map_iterate_end;

  *nextedge = NULL;

#if LAND_AREA_DEBUG >= 2
  print_landarea_map(pcmap, turn);
#endif
}

/**************************************************************************
  expand outwards evenly from each starting point, counting tiles
**************************************************************************/
static void build_landarea_map_expand(struct claim_map *pcmap)
{
  struct tile **midedge;
  int turn, accum, other;
  struct tile **thisedge;
  struct tile **nextedge;

  midedge = &pcmap->edges[map.xsize * map.ysize];

  for (accum = 1, turn = 1; accum > 0; turn++) {
    thisedge = ((turn & 0x1) == 1) ? pcmap->edges : midedge;
    nextedge = ((turn & 0x1) == 1) ? midedge : pcmap->edges;

    for (accum = 0; *thisedge; thisedge++) {
      struct tile *ptile = *thisedge;
      int i = ptile->index;
      int owner = pcmap->claims[i].whom;

      if (owner != no_owner) {
	adjc_iterate(ptile, tile1) {
	  int j = tile1->index;
	  struct claim_cell *pclaim = &pcmap->claims[j];

	  if (TEST_BIT(pclaim->know, owner)) {
	    if (pclaim->when == 0) {
	      pclaim->when = turn + 1;
	      pclaim->whom = owner;
	      *nextedge = tile1;
	      nextedge++;
	      pcmap->player_landarea[owner]++;
	      if (TEST_BIT(pclaim->cities, owner)) {
		pcmap->player_owndarea[owner]++;
	      }
	      accum++;
	    } else if (pclaim->when == turn + 1
		       && pclaim->whom != no_owner
		       && pclaim->whom != owner) {
	      other = pclaim->whom;
	      if (TEST_BIT(pclaim->cities, other)) {
		pcmap->player_owndarea[other]--;
	      }
	      pcmap->player_landarea[other]--;
	      pclaim->whom = no_owner;
	      accum--;
	    }
	  }
	} adjc_iterate_end;
      }
    }

    *nextedge = NULL;

#if LAND_AREA_DEBUG >= 2
    print_landarea_map(pcmap, turn);
#endif
  }
}

/**************************************************************************
  this just calls the three worker routines
**************************************************************************/
static void build_landarea_map(struct claim_map *pcmap)
{
  build_landarea_map_new(pcmap);
  build_landarea_map_turn_0(pcmap);
  build_landarea_map_expand(pcmap);
}

/**************************************************************************
  Frees and NULLs an allocated claim map.
**************************************************************************/
static void free_landarea_map(struct claim_map *pcmap)
{
  if (pcmap) {
    if (pcmap->claims) {
      free(pcmap->claims);
      pcmap->claims = NULL;
    }
    if (pcmap->player_landarea) {
      free(pcmap->player_landarea);
      pcmap->player_landarea = NULL;
    }
    if (pcmap->player_owndarea) {
      free(pcmap->player_owndarea);
      pcmap->player_owndarea = NULL;
    }
    if (pcmap->edges) {
      free(pcmap->edges);
      pcmap->edges = NULL;
    }
  }
}

/**************************************************************************
  Returns the given player's land and settled areas from a claim map.
**************************************************************************/
static void get_player_landarea(struct claim_map *pcmap,
				struct player *pplayer,
				int *return_landarea,
				int *return_settledarea)
{
  if (pcmap && pplayer) {
#if LAND_AREA_DEBUG >= 1
    printf("%-14s", pplayer->name);
#endif
    if (return_landarea && pcmap->player_landarea) {
      *return_landarea =
	USER_AREA_MULT * pcmap->player_landarea[pplayer->player_no];
#if LAND_AREA_DEBUG >= 1
      printf(" l=%d", *return_landarea / USER_AREA_MULT);
#endif
    }
    if (return_settledarea && pcmap->player_owndarea) {
      *return_settledarea =
	USER_AREA_MULT * pcmap->player_owndarea[pplayer->player_no];
#if LAND_AREA_DEBUG >= 1
      printf(" s=%d", *return_settledarea / USER_AREA_MULT);
#endif
    }
#if LAND_AREA_DEBUG >= 1
    printf("\n");
#endif
  }
}

/**************************************************************************
  Calculates the civilization score for the player.
**************************************************************************/
void calc_civ_score(struct player *pplayer)
{
  struct city *pcity;
  int landarea = 0, settledarea = 0;
  static struct claim_map cmap = { NULL, NULL, NULL,NULL };

  pplayer->score.happy = 0;
  pplayer->score.content = 0;
  pplayer->score.unhappy = 0;
  pplayer->score.angry = 0;
  pplayer->score.taxmen = 0;
  pplayer->score.scientists = 0;
  pplayer->score.elvis = 0;
  pplayer->score.wonders = 0;
  pplayer->score.techs = 0;
  pplayer->score.techout = 0;
  pplayer->score.landarea = 0;
  pplayer->score.settledarea = 0;
  pplayer->score.population = 0;
  pplayer->score.cities = 0;
  pplayer->score.units = 0;
  pplayer->score.pollution = 0;
  pplayer->score.bnp = 0;
  pplayer->score.mfg = 0;
  pplayer->score.literacy = 0;
  pplayer->score.spaceship = 0;

  if (is_barbarian(pplayer)) {
    if (pplayer->player_no == game.nplayers - 1) {
      free_landarea_map(&cmap);
    }
    return;
  }

  city_list_iterate(pplayer->cities, pcity) {
    int bonus;

    pplayer->score.happy += pcity->ppl_happy[4];
    pplayer->score.content += pcity->ppl_content[4];
    pplayer->score.unhappy += pcity->ppl_unhappy[4];
    pplayer->score.angry += pcity->ppl_angry[4];
    pplayer->score.taxmen += pcity->specialists[SP_TAXMAN];
    pplayer->score.scientists += pcity->specialists[SP_SCIENTIST];
    pplayer->score.elvis += pcity->specialists[SP_ELVIS];
    pplayer->score.population += city_population(pcity);
    pplayer->score.cities++;
    pplayer->score.pollution += pcity->pollution;
    pplayer->score.techout += pcity->science_total;
    pplayer->score.bnp += pcity->trade_prod;
    pplayer->score.mfg += pcity->shield_surplus;

    bonus = CLIP(0, get_city_bonus(pcity, EFT_SCIENCE_BONUS), 100);
    pplayer->score.literacy += (city_population(pcity) * bonus) / 100;
  } city_list_iterate_end;

  if (pplayer->player_no == 0) {
    free_landarea_map(&cmap);
    build_landarea_map(&cmap);
  }
  get_player_landarea(&cmap, pplayer, &landarea, &settledarea);
  pplayer->score.landarea = landarea;
  pplayer->score.settledarea = settledarea;
  if (pplayer->player_no == game.nplayers - 1) {
    free_landarea_map(&cmap);
  }

  tech_type_iterate(i) {
    if (get_invention(pplayer, i)==TECH_KNOWN) {
      pplayer->score.techs++;
    }
  } tech_type_iterate_end;
  
  if(game.futuretechsscore)pplayer->score.techs += pplayer->future_tech * 5 / 2;
  
  unit_list_iterate(pplayer->units, punit) {
    if (is_military_unit(punit)) {
      pplayer->score.units++;
    }
  } unit_list_iterate_end

  impr_type_iterate(i) {
    if (is_wonder(i)
	&& (pcity = find_city_by_id(game.global_wonders[i]))
	&& player_owns_city(pplayer, pcity)) {
      pplayer->score.wonders++;
    }
  } impr_type_iterate_end;

  /* How much should a spaceship be worth?
   * This gives 100 points per 10,000 citizens. */
  if (pplayer->spaceship.state == SSHIP_ARRIVED) {
    pplayer->score.spaceship += (int)(100 * pplayer->spaceship.habitation
				      * pplayer->spaceship.success_rate);
  }
}

/**************************************************************************
  Return the civilization score (a numerical value) for the player.
**************************************************************************/
int get_civ_score(const struct player *pplayer)
{
  /* We used to count pplayer->score.happy here too, but this is too easily
   * manipulated by players at the endyear. */
  return (total_player_citizens(pplayer)
	  + pplayer->score.techs * 2
	  + pplayer->score.wonders * 5
	  + pplayer->score.spaceship);
}

/**************************************************************************
  Return the total number of citizens in the player's nation.
**************************************************************************/
int total_player_citizens(const struct player *pplayer)
{
  return (pplayer->score.happy
	  + pplayer->score.content
	  + pplayer->score.unhappy
	  + pplayer->score.angry
	  + pplayer->score.scientists
	  + pplayer->score.elvis
	  + pplayer->score.taxmen);
}

/**************************************************************************
 save a ppm file which is a representation of the map of the current turn.
 this can later be turned into a animated gif.

 terrain type, units, and cities are saved.
**************************************************************************/
void save_ppm(void)
{
  char filename[600];
  char tmpname[600];
  FILE *fp;
  int i, j;

  /* the colors for each player. these were selected to give
   * the most differentiation between all players. YMMV. */
  int col[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS][3] = {
    {255,   0,   0}, {  0, 128,   0}, {255, 255, 255}, {255, 255,   0},
    {138,  43, 226}, {255, 140,   0}, {  0, 255, 255}, {139,  69,  19},
    {211, 211, 211}, {255, 215,   0}, {255,  20, 147}, {124, 252,   0},
    {218, 112, 214}, { 30, 144, 255}, {250, 128, 114}, {154, 205,  50},
    { 25,  25, 112}, {  0, 255, 127}, {139,   0,   0}, {100, 149, 237},
    {  0, 128, 128}, {255, 192, 203}, {255, 250, 205}, {119, 136, 153},
    {255, 127,  80}, {255,   0, 255}, {128, 128,   0}, {245, 222, 179},
    {184, 134,  11}, {173, 216, 230}, {102, 205, 170}, {255, 165,   0},
  };
  int watercol[3] = {0,0,255}; /* blue */
  int landcol[3] =  {0,0,0};   /* black */

  if (!srvarg.save_ppm) {
    return;
  }

  /* put this file in the same place we put savegames */
  my_snprintf(filename, sizeof(filename),
              "%s%+05d.int.ppm", game.save_name, game.year);

  /* Ensure the saves directory exists. */
  make_dir(srvarg.saves_pathname);

  sz_strlcpy(tmpname, srvarg.saves_pathname);
  if (tmpname[0] != '\0') {
    sz_strlcat(tmpname, "/");
  }
  sz_strlcat(tmpname, filename);
  sz_strlcpy(filename, tmpname);

  fp = fopen(filename, "w");

  if (!fp) {
    freelog(LOG_ERROR, "couldn't open file ppm save: %s\n", filename);
    return;
  }

  fprintf(fp, "P3\n# version:2\n# gameid: %s\n", game.id);
  fprintf(fp, "# An intermediate map from saved Freeciv game %s%+05d\n",
          game.save_name, game.year);


  for (i = 0; i < game.nplayers; i++) {
    struct player *pplayer = get_player(i);
    fprintf(fp, "# playerno:%d:color:#%02x%02x%02x:name:\"%s\"\n", 
            pplayer->player_no, col[i][0], col[i][1], col[i][2],
            pplayer->name);
  }

  fprintf(fp, "%d %d\n", map.xsize, map.ysize);
  fprintf(fp, "255\n");

  for (j = 0; j < map.ysize; j++) {
    for (i = 0; i < map.xsize; i++) {
       struct tile *ptile = native_pos_to_tile(i, j);
       int *color;

       /* color for cities first, then units, then land */
       if (ptile->city) {
         color = col[city_owner(ptile->city)->player_no];
       } else if (unit_list_size(ptile->units) > 0) {
         color = col[unit_owner(unit_list_get(ptile->units, 0))->player_no];
       } else if (is_ocean(ptile->terrain)) {
         color = watercol;
       } else {
         color = landcol;
       }

       fprintf(fp, "%d %d %d\n", color[0], color[1], color[2]);
    }
  }

  fclose(fp);
}

/**************************************************************************
  ...
**************************************************************************/
static void dump_grouping_players(const struct grouping *p)
{
  int j;
  struct player *pp;

  for (j = 0; j < p->num_players; j++) {
    pp = p->players[j];
    freelog(LOG_DEBUG, "  player %d @ %p: %s user=%s score=%d rank=%f "
            "result=%d player_id=%d",
            j, pp, pp->name, pp->username, get_civ_score(pp), pp->rank,
            pp->result, pp->fcdb.player_id);
    freelog(LOG_DEBUG, "      rated_user_id=%d r=%f "
            "rd=%f ts=%ld nr=%f nrd=%f",
            pp->fcdb.rated_user_id, pp->fcdb.rating,
            pp->fcdb.rating_deviation, pp->fcdb.last_rating_timestamp,
            pp->fcdb.new_rating, pp->fcdb.new_rating_deviation);
  }

}

/**************************************************************************
  ...
**************************************************************************/
static void dump_groupings(void)
{
  struct grouping *p;
  int i;

  freelog(LOG_DEBUG, "BEGIN GROUPING DUMP num_groupings=%d",
          num_groupings); 
  for (i = 0, p = groupings; i < num_groupings; i++, p++) {
    freelog(LOG_DEBUG, "grouping %d @ %p: score=%f rank=%f result=%d "
            "r=%f rd=%f",
            i, p, p->score, p->rank, p->result, p->rating,
            p->rating_deviation);
    freelog(LOG_DEBUG, "    nr=%f nrd=%f num_players=%d num_alive=%d "
            "rank_offset=%f",
            p->new_rating, p->new_rating_deviation, p->num_players,
            p->num_alive, p->rank_offset);
    dump_grouping_players(p);
  }
  freelog(LOG_DEBUG, "END GROUPING DUMP");
}

/**************************************************************************
  ...
**************************************************************************/
#if 0
static int real_grouping_compare(const void *va, const void *vb);
static int grouping_compare(const void *va, const void *vb)
{
  const struct grouping *a, *b;
  int r;

  a = (const struct grouping *) va;
  b = (const struct grouping *) vb;

  r = real_grouping_compare(a, b);
  fprintf(stderr, "about to compare:\n");
  dump_grouping(stderr, a);
  dump_grouping(stderr, b);
  fprintf(stderr, "grouping_compare(%p, %p) = %d\n", a, b, r);
  return r;
}
static int real_grouping_compare(const void *va, const void *vb)
{
#else
/**************************************************************************
  ...
**************************************************************************/
static int grouping_compare(const void *va, const void *vb)
{
#endif
  const struct grouping *a, *b;
  double dscore;

  a = (const struct grouping *) va;
  b = (const struct grouping *) vb;

  if (a->result == PR_WIN && b->result != PR_WIN)
    return -1;
  if (a->result != PR_WIN && b->result == PR_WIN)
    return 1;
  if (a->result == PR_DRAW && b->result != PR_DRAW)
    return -1;
  if (a->result != PR_DRAW && b->result == PR_DRAW)
    return 1;
  
  dscore = a->score - b->score;
  if (fabs(dscore) < MINIMUM_SCORE_DIFFERENCE) {
    if (a->num_alive > 0 && b->num_alive <= 0)
      return -1;
    if (a->num_alive <= 0 && b->num_alive > 0)
      return 1;
    if (a->num_players < b->num_players)
      return -1;
    if (a->num_players > b->num_players)
      return 1;
    if (a->num_alive > b->num_alive)
      return -1;
    if (a->num_alive < b->num_alive)
      return 1;

    return 0;
  }

  return dscore > 0.0 ? -1 : (dscore < 0.0 ? 1 : 0);
}

/**************************************************************************
  NB Assumes score_cache has been filled in.
**************************************************************************/
static int player_in_grouping_compare(const void *va, const void *vb)
{
  struct player *a, *b;
  a = *((struct player **) va);
  b = *((struct player **) vb);

  if (a->is_alive && !b->is_alive) {
    return -1;
  }
  if (!a->is_alive && b->is_alive) {
    return 1;
  }
  return score_cache[b->player_no]
    - score_cache[a->player_no];
}

/**************************************************************************
  Assumes (old) player ratings have been already assigned. Checks that
  there are enough rated *users* for rating this game and if enough
  turns have elapsed. Notifies players if the game cannot be rated.
**************************************************************************/
static bool game_can_be_rated(void)
{
  int num_rated_users = 0;

  players_iterate(pplayer) {
    if (pplayer->fcdb.rating > 0.0
        && pplayer->fcdb.rating_deviation >= RATING_CONSTANT_MINIMUM_RD
        && pplayer->fcdb.rated_user_id > 0) {
      num_rated_users++;
    }
  } players_iterate_end;

  freelog(LOG_DEBUG, "game_can_be_rated num_rated_users=%d game.turn=%d",
          num_rated_users, game.turn);
  
  if ((game.fcdb.type == GT_SOLO && num_rated_users < 1)
      || (game.fcdb.type != GT_SOLO && num_rated_users < 2)) {
    notify_conn(NULL, _("Game: The game cannot be rated because there "
                        "are not enough rated users in the game."));
    return FALSE;
  }

  if (srvarg.fcdb.min_rated_turns > game.turn) {
    notify_conn(NULL, _("Game: The game cannot be rated because not "
                        "enough turns (%d) have been played."),
                srvarg.fcdb.min_rated_turns);
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static inline double glicko_g_function(double RD) {
  return 1.0/sqrt(1.0 + 3.0 * RATING_CONSTANT_Q * RATING_CONSTANT_Q
                  * RD * RD / (M_PI * M_PI));
}

/**************************************************************************
  NB Unlike Glicko's definition, we pass g(RD_j) here not RD_j.
**************************************************************************/
static inline double glicko_E_function(double r, double rj, double gRDj)
{
  return 1.0/(1.0 + pow(10.0, -gRDj * (r - rj) / 400.0));
}

/**************************************************************************
  Calculate the rating and RD for each grouping from the
  individual players' ratings and RDs. As this is not
  part of the Glicko rating system, just something I made
  up, it might be complete bullshit.
**************************************************************************/
void score_calculate_grouping_ratings(void)
{
  double K, A, B, min_r, RD, maxRD, sum, r;
  int i, j;

  K = RATING_CONSTANT_EXTRA_TEAMMATE_INFLUENCE;
  A = RATING_CONSTANT_AVERAGE_PLAYER_RATING;
  B = RATING_CONSTANT_BAD_PLAYER_RATING;
  min_r = RATING_CONSTANT_AVERAGE_PLAYER_RATING;

  for (i = 0; i < num_groupings; i++) {

    /* A grouping's base rating is the average of
     * its members' ratings. It's RD is the maximum
     * of its members' RDs. */
    sum = 0.0;
    maxRD = 0.0;
    for (j = 0; j < groupings[i].num_players; j++) {
      sum += groupings[i].players[j]->fcdb.rating;
      RD = groupings[i].players[j]->fcdb.rating_deviation;
      maxRD = MAX(RD, maxRD);
      min_r = MIN(min_r, groupings[i].players[j]->fcdb.rating);
    }
    sum /= (double) groupings[i].num_players;

    /* If there is more than 1 player in the grouping,
     * add to the base rating the approximate effect
     * each player will have. Thus better-than-average
     * players will increase the rating, while worse
     * players will decrease it. Furthermore, the more
     * members the grouping has, the smaller this
     * contribution will be. */
    if (groupings[i].num_players > 1) {
      for (j = 0; j < groupings[i].num_players; j++) {
        r = groupings[i].players[j]->fcdb.rating;
        if (r > A) {
          sum += K * (r - A);
        } else if (r < B) {
          sum += K * (r - B);
        }
      }
    }

    /* In the case of a team of mostly bad players,
     * ensure that the rating does not go too low. */
    if (sum < min_r) {
      sum = min_r;
    }

    groupings[i].rating = sum;
    groupings[i].rating_deviation = maxRD;

    if (player_is_on_team(groupings[i].players[0])) {
      /* Set corresponding fcdb fields in team structures. */
      struct team *pteam = team_get_by_id(groupings[i].players[0]->team);

      pteam->fcdb.rating = groupings[i].rating;
      pteam->fcdb.rating_deviation = groupings[i].rating_deviation;
    }
  }
}

/**************************************************************************
  ...
**************************************************************************/
void score_propagate_grouping_ratings(void)
{
  int i, j;
  double rating_change, rd_change, sum, new_r, new_RD, trank, weight;

  for (i = 0; i < num_groupings; i++) {
    rating_change = groupings[i].new_rating - groupings[i].rating;
    rd_change = groupings[i].new_rating_deviation
        - groupings[i].rating_deviation;

    /* Use n*(n+1)/2 not (n-1)*n/2 since we add 1 to 'trank' below. */
    sum = groupings[i].num_players
        * (groupings[i].num_players + 1.0) / 2.0;

    for (j = 0; j < groupings[i].num_players; j++) {
      new_r = groupings[i].players[j]->fcdb.rating;
      new_RD = groupings[i].players[j]->fcdb.rating_deviation;

      if (groupings[i].num_players == 1) {
        new_r += rating_change;
        new_RD += rd_change;
      } else {
        /* The "pie/blame" allotment scheme:
         * If the change is positive, 'better' members get a bigger
         * piece of the pie. If the change is negative, 'worse' members
         * get a bigger piece of the blame. */

        /* NB HIGHER rank means LOWER rank number,
         * i.e. 0.0 is first place. */

        /* Add 1 to the rank to avoid weight = 0. */
        trank = groupings[i].players[j]->rank + 1.0
                - groupings[i].rank_offset;
        weight = trank / sum;

        if (rating_change > 0) {
          new_r += rating_change * (1 - weight);
        } else {
          new_r += rating_change * weight;
        }

        /* The uncertainty changes, but not so much if you
         * are in a big team. */
        new_RD += rd_change / groupings[i].num_players;
      }

      /* As per Glicko's suggestion. */
      if (new_RD < RATING_CONSTANT_MINIMUM_RD) {
        new_RD = RATING_CONSTANT_MINIMUM_RD;
      }

      groupings[i].players[j]->fcdb.new_rating = new_r;
      groupings[i].players[j]->fcdb.new_rating_deviation = new_RD;
    }
  }
}

/**************************************************************************
  Glicko rating system extended to variable-sized groups of players. To
  understand what this code is trying to do, read what it is based on at
  http://math.bu.edu/people/mg/glicko/glicko.doc/glicko.html.
**************************************************************************/
static void update_ratings(void)
{
  int i, j;
  double RD, c, t, r, q, q2, RD2, sum, inv_d2;
  double rj, sj, E, new_r, new_RD, gRD[MAX_NUM_PLAYERS];

  if (game.fcdb.type == GT_SOLO) {
    assert(num_groupings == 1);
    assert(groupings[0].num_players == 1);
  }

  /* Adjust player RDs to reflect the passage
   * of time. (Glicko Step 1) */

  c = RATING_CONSTANT_C;
  players_iterate(pplayer) {
    if (pplayer->fcdb.last_rating_timestamp <= 0) {
      continue;
    }

    RD = pplayer->fcdb.rating_deviation;
    t = floor((double) (time(NULL) - pplayer->fcdb.last_rating_timestamp)
        / RATING_CONSTANT_SECONDS_PER_RATING_PERIOD);

    pplayer->fcdb.rating_deviation
      = MIN(sqrt(RD*RD + c*c*t), RATING_CONSTANT_MAXIMUM_RD);
  } players_iterate_end;


  /* Transform player ratings into team/grouping ratings. */
  score_calculate_grouping_ratings();


  /* Update ratings. (Glicko Step 2) */

  /* Fill gRD[j] table to avoid recalculation. */
  if (game.fcdb.type == GT_SOLO) {
    gRD[0] = glicko_g_function(score_get_solo_opponent_rating_deviation());
  } else {
    for (i = 0; i < num_groupings; i++) {
      gRD[i] = glicko_g_function(groupings[i].rating_deviation);
    }
  }

  q = RATING_CONSTANT_Q;
  q2 = q * q;
  for (i = 0; i < num_groupings; i++) {
    r = groupings[i].rating;
    RD = groupings[i].rating_deviation;
    RD2 = RD * RD;

    sum = 0.0;
    inv_d2 = 0.0;
    for (j = 0; j < num_groupings; j++) {

      if (game.fcdb.type == GT_SOLO) {
        rj = score_calculate_solo_opponent_rating(&groupings[0]);

        /* You only 'win' if you get to Alpha Centauri. */
        sj = game.fcdb.outcome == GOC_ENDED_BY_SPACESHIP ? 1.0 : 0.0;

      } else {
        if (i == j) {
          /* Don't update your rating against yourself! */
          continue;
        }
        rj = groupings[j].rating;

        /* There are many potential ways we can calculate sj
         * (the observed performace):
         * 1. Relative rank: higher rank means win, same rank
         *    means draw, lower rank means loss, with the results
         *    giving sj 1.0, 0.5, 0.0 respectively, as in the
         *    original Glicko system.
         * 2. Weighted rank: sj = 1/2 - (rank - rankj) / (2 * (n-1))
         *    where n is the number of groupings.
         * 3. Result only:
         *    PR_WIN  ==> sj = 1.0
         *    PR_DRAW ==> sj = 0.5
         *    PR_LOSS ==> sj = 0.0
         * 4. Weighted score: like (2) but using scores instead of
         *    ranks.
        */

        /* Method 2 */
        sj = 0.5 - (groupings[i].rank - groupings[j].rank)
            / (2.0 * ((double) num_groupings - 1.0));
      }


      E = glicko_E_function(r, rj, gRD[j]);
      sum += gRD[j] * (sj - E);
      inv_d2 += gRD[j] * gRD[j] * E * (1.0 - E);
    }
    
    inv_d2 = q2 * inv_d2;

    new_r = r + (q / (1.0 / RD2 + inv_d2)) * sum;
    new_RD = sqrt(1.0 / (1.0 / RD2 + inv_d2));

    groupings[i].new_rating = new_r;
    groupings[i].new_rating_deviation = new_RD;

    if (player_is_on_team(groupings[i].players[0])) {
      /* Set corresponding fcdb fields in team structures. */
      struct team *pteam = team_get_by_id(groupings[i].players[0]->team);

      pteam->fcdb.new_rating = groupings[i].new_rating;
      pteam->fcdb.new_rating_deviation = groupings[i].new_rating_deviation;
    }
  }

  /* Now transform the new grouping ratings and RDs into new
   * player ratings and RDs. (Not part of Glicko, caveat lector) */
  score_propagate_grouping_ratings();
}

/**************************************************************************
  ...
**************************************************************************/
void score_assign_groupings(void)
{
  int i, j;

  memset(groupings, 0, sizeof(groupings));
  num_groupings = 0;


  /* Assign all players in the same team to their own grouping. */
  i = 0;
  team_iterate(pteam) {
    j = 0;
    players_iterate(pplayer) {
      if (pplayer->team == pteam->id) {
        groupings[i].players[j++] = pplayer;
      }
    } players_iterate_end;
    groupings[i].num_players = j;
    groupings[i].num_alive = team_count_members_alive(pteam->id);
    i++;
  } team_iterate_end;

  /* Assign the individual players that are left. */
  players_iterate(pplayer) {
    if (player_is_on_team(pplayer)
        || is_barbarian(pplayer)
        || pplayer->is_observer) {
      continue;
    }
    groupings[i].players[0] = pplayer;
    groupings[i].num_players = 1;
    groupings[i].num_alive = pplayer->is_alive ? 1 : 0;
    i++;
  } players_iterate_end;

  num_groupings = i;
}

/**************************************************************************
  ...
**************************************************************************/
void score_update_grouping_results(void)
{
  int i, j, k, next_cmp;
  double ranksum, frank;
  bool force_draw, force_loss;

  assert(num_groupings > 0);

  /* Build the score cache, so we don't have to
     constantly call get_civ_score. */
  players_iterate(pplayer) {
    if (is_barbarian(pplayer) || pplayer->is_observer) {
      continue;
    }
    score_cache[pplayer->player_no] = get_civ_score(pplayer);
  } players_iterate_end;

  /* Assign grouping scores. */
  for (i = 0; i < num_groupings; i++) {
    if (groupings[i].num_players == 1) {
      groupings[i].score = score_cache[groupings[i].players[0]->player_no];
    } else {
      struct team *pteam = team_get_by_id(groupings[i].players[0]->team);

      assert(pteam != NULL);
      groupings[i].score = pteam->score;
    }
  }

  /* Assign the groupings' forced results (if any). */
  for (i = 0; i < num_groupings; i++) {
    force_draw = FALSE;
    force_loss = FALSE;
    groupings[i].result = PR_NONE;

    for (j = 0; j < groupings[i].num_players; j++) {
      if (groupings[i].result == PR_NONE) {
        if (groupings[i].players[j]->result == PR_WIN) {
          /* forced winner (e.g. spaceship, /end <name>, etc. */
          groupings[i].result = PR_WIN;
        } else if (groupings[i].players[j]->result == PR_DRAW) {
          /* forced draw may be overruled by forced win above */
          force_draw = TRUE;
        } else if (groupings[i].players[j]->result == PR_LOSE) {
          /* for completeness, handle forced loss too (not used
             in any game outcome as of yet) */
          force_loss = TRUE;
        }
      }
    }

    if (force_draw && groupings[i].result == PR_NONE) {
      groupings[i].result = PR_DRAW;
    }
    if (force_loss && groupings[i].result == PR_NONE) {
      groupings[i].result = PR_LOSE;
    }
  }

  /* Sort the groupings, and the players within the groupings. */

  qsort(groupings,
        num_groupings,
        sizeof(struct grouping),
        grouping_compare);

  for (i = 0; i < num_groupings; i++) {
    if (groupings[i].num_players < 2) {
      continue;
    }
    qsort(groupings[i].players,
          groupings[i].num_players,
          sizeof(struct player *),
          player_in_grouping_compare);
  }

#ifdef DEBUG
  freelog(LOG_DEBUG, "Groupings after qsort:");
  dump_groupings();
#endif

  /* Fill in unset results based on the order. */

  if (groupings[0].result == PR_NONE) {
    if (num_groupings > 1
        && 0 == grouping_compare(&groupings[0], &groupings[1])) {
      next_cmp = 0;
      i = 1;
      groupings[0].result = PR_DRAW;
      do {
        i++;
        next_cmp = grouping_compare(&groupings[i-1], &groupings[i]);
        groupings[i-1].result = PR_DRAW;
      } while (i < num_groupings && 0 == next_cmp);
    } else {
      if (game.fcdb.type == GT_SOLO
          && game.fcdb.outcome != GOC_ENDED_BY_SPACESHIP) {
        groupings[0].result = PR_LOSE;
      } else {
        groupings[0].result = PR_WIN;
      }
    }
  }

  for (i = 0; i < num_groupings; i++) {
    if (groupings[i].result != PR_NONE) {
      continue;
    }
    groupings[i].result = PR_LOSE;
  }

  /* Assign fractional ranks based on the order. */
  i = 0;
  groupings[0].rank = 0;
  do {
    ranksum = (double) i;
    j = i + 1;
    while (j < num_groupings
           && 0 == grouping_compare(&groupings[i], &groupings[j])) {
      ranksum += (double) j++;
    }
    frank = ranksum / (double) (j - i);
    for (k = i; k < j; k++) {
      groupings[k].rank = frank;
    }
    i = j;
  } while (i < num_groupings);
}

/**************************************************************************
  Propagate the groupings' results back to the players and teams, and
  calculate the fractional ranking for players.
**************************************************************************/
void score_propagate_grouping_results(void)
{
  int i, j, k, rank_offset, n;
  double ranksum, frank;
  struct team *pteam = NULL;

  rank_offset = 0;
  for (i = 0; i < num_groupings; i++) {
    if (groupings[i].num_players == 1) {
      groupings[i].players[0]->rank = groupings[i].rank;      
      groupings[i].players[0]->result = groupings[i].result;
    } else {
      j = 0;
      groupings[i].players[0]->rank = 0;
      do {
        ranksum = (double) j;
        for (n = j + 1; n < groupings[i].num_players
             && 0 == player_in_grouping_compare(&groupings[i].players[j],
                                                &groupings[i].players[n]);
             n++) {
          ranksum += (double) n;
        }
        frank = ranksum / (double) (n - j);
        for (k = j; k < n; k++) {
          groupings[i].players[k]->rank = frank + rank_offset;
          groupings[i].players[k]->result = groupings[i].result;
        }
        j = n;
      } while (j < groupings[i].num_players);
    }
    groupings[i].rank_offset = rank_offset;
    rank_offset += groupings[i].num_players;
  }

  /* Propagate grouping results/ranks to teams. */
  for (i = 0; i < num_groupings; i++) {
    if (!player_is_on_team(groupings[i].players[0])) {
      continue;
    }
    pteam = team_get_by_id(groupings[i].players[0]->team);
    if (!pteam) {
      continue;
    }
    pteam->rank = groupings[i].rank;
    pteam->result = groupings[i].result;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void score_evaluate_players(void)
{
  freelog(LOG_DEBUG, "Evaluating players...");

  score_calculate_team_scores();
  score_assign_groupings();
  score_update_grouping_results();
  score_propagate_grouping_results();

#ifdef DEBUG
  freelog(LOG_DEBUG, "Groupings after assign, update and propagate:");
  dump_groupings();
#endif
  
  if (!game.rated || !srvarg.fcdb.enabled || !srvarg.auth.enabled) {
    return;
  }

  /* Get the old ratings. */
  if (!fcdb_load_player_ratings(game.fcdb.type)) {
    notify_conn(NULL, _("Game: Though the server option 'rated' was set, "
        "player ratings cannot be updated because there was an error "
        "communicating with the database."));
    game.rated = FALSE;
    return;
  }

  if (!game_can_be_rated()) {
    game.rated = FALSE;
    return;
  }

  update_ratings();

#ifdef DEBUG
  freelog(LOG_DEBUG, "Groupings after rating calculation:");
  dump_groupings();
#endif
}

/***************************************************************
  The team score is the average of the members' scores.
***************************************************************/
void score_calculate_team_scores(void)
{
  double sum;
  team_iterate(pteam) {
    sum = 0.0;
    players_iterate(pplayer) {
      if (pplayer->team != pteam->id)
        continue;
      sum += (double) get_civ_score(pplayer);
    } players_iterate_end;

    pteam->score = sum / (double) pteam->member_count;
  } team_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void score_assign_ai_rating(struct player *pplayer,
                            int game_type)
{
  score_get_ai_rating(pplayer->ai.skill_level, game_type,
                      &pplayer->fcdb.rating,
                      &pplayer->fcdb.rating_deviation);
  pplayer->fcdb.new_rating = 0.0;
  pplayer->fcdb.new_rating_deviation = 0.0;
  pplayer->fcdb.last_rating_timestamp = 0;
}

/**************************************************************************
  ...
**************************************************************************/
void score_assign_new_player_rating(struct player *pplayer,
                                    int game_type)
{
  pplayer->fcdb.rating = RATING_CONSTANT_AVERAGE_PLAYER_RATING;
  pplayer->fcdb.rating_deviation = RATING_CONSTANT_MAXIMUM_RD;
  pplayer->fcdb.new_rating = 0.0;
  pplayer->fcdb.new_rating_deviation = 0.0;
  pplayer->fcdb.last_rating_timestamp = 0;
}

/**************************************************************************
  ...
**************************************************************************/
const struct grouping *score_get_groupings(int *pnum_groupings)
{
  *pnum_groupings = num_groupings;
  return groupings;
}

/**************************************************************************
  ...
**************************************************************************/
double score_get_solo_opponent_rating_deviation(void)
{
  return RATING_CONSTANT_MINIMUM_RD;
}

/**************************************************************************
  Because there is no opponent for solo players, create a 'pseudo opponent'
  whose rating depends on your score and the turn that the game ended.
**************************************************************************/
double score_calculate_solo_opponent_rating(const struct grouping *g)
{
  double K, S, T, rating;

  K = RATING_CONSTANT_SOLO_RATING_COEFFICIENT;
  S = g->score;
  T = game.turn;

  rating = K * S / (1.0 + T);

  freelog(LOG_DEBUG, "Solo game psuedo-opponent rating=%f", rating);

  return rating;
}

/**************************************************************************
  Estimate AI rating and RD.
**************************************************************************/
void score_get_ai_rating(int skill_level, int game_type,
                         double *prating, double *prd)
{
  /* I assume higher skill level implies a better ai. */
  *prating = 600 + skill_level * 50;
  if (game_type == GT_TEAM) {
    *prating -= 100;
  }
  *prd = 50;
}

/**************************************************************************
  ...
**************************************************************************/
int player_get_rated_username(const struct player *pplayer,
                              char *outbuf, int maxlen)
{
  if (pplayer->fcdb.rated_user_id <= 0
      || pplayer->fcdb.rated_user_name[0] == '\0') {
    return player_get_username(pplayer, outbuf, maxlen);
  }

  return mystrlcpy(outbuf, pplayer->fcdb.rated_user_name, maxlen);
}

