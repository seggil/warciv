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
#  include "../config.h"
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "wc_intl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

#include "city.h"
#include "game.h"
#include "improvement.h"
#include "map.h"
#include "player.h"
#include "terrain.h"
#include "unit.h"

#include "database.h"
#include "plrhand.h"
#include "srv_main.h"

#include "score.h"

/* Avoid having to recalculate civ scores all the time. */
static int score_cache[MAX_NUM_PLAYERS+MAX_NUM_BARBARIANS];

static struct grouping groupings[MAX_NUM_PLAYERS];
static int num_groupings = 0;

typedef int (*compare_func_t)(const void *, const void *);

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
  tile_t **edges;
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
#define WRITE_MAP_DATA(type, map_char_expr)             \
{                                                       \
  int nat_x, nat_y;                                     \
  for (nat_x = 0; nat_x < map.info.xsize; nat_x++) {    \
    printf("%d", nat_x % 10);                           \
  }                                                     \
  putchar('\n');                                        \
  for (nat_y = 0; nat_y < map.info.ysize; nat_y++) {    \
    printf("%d ", nat_y % 10);                          \
    for (nat_x = 0; nat_x < map.info.xsize; nat_x++) {  \
      int x, y;                                         \
      NATIVE_TO_MAP_POS(&x, &y, nat_x,nat_y);           \
      printf(type, map_char_expr);                      \
    }                                                   \
    printf(" %d\n", nat_y % 10);                        \
  }                                                     \
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

    for (p = 0; p < game.info.nplayers; p++) {
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

  nbytes = map.info.xsize * map.info.ysize * sizeof(struct claim_cell);
  pcmap->claims = wc_malloc(nbytes);
  memset(pcmap->claims, 0, nbytes);

  nbytes = game.info.nplayers * sizeof(int);
  pcmap->player_landarea = wc_malloc(nbytes);
  memset(pcmap->player_landarea, 0, nbytes);

  nbytes = game.info.nplayers * sizeof(int);
  pcmap->player_owndarea = wc_malloc(nbytes);
  memset(pcmap->player_owndarea, 0, nbytes);

  nbytes = 2 * map.info.xsize * map.info.ysize * sizeof(*pcmap->edges);
  pcmap->edges = wc_malloc(nbytes);

  players_iterate(pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
      map_city_radius_iterate(pcity->common.tile, tile1) {
        pcmap->claims[tile1->index].cities |= (1u << pcity->common.owner);
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
  tile_t **nextedge;
  struct claim_cell *pclaim;

  turn = 0;
  nextedge = pcmap->edges;

  whole_map_iterate(ptile) {
    int i = ptile->index;

    pclaim = &pcmap->claims[i];
    ptile = &map.board[i];

    if (is_ocean(ptile->terrain)) {
      /* pclaim->when = 0; */
      pclaim->whom = no_owner;
      /* pclaim->know = 0; */
    } else if (ptile->city) {
      owner = ptile->city->common.owner;
      pclaim->when = turn + 1;
      pclaim->whom = owner;
      *nextedge = ptile;
      nextedge++;
      pcmap->player_landarea[owner]++;
      pcmap->player_owndarea[owner]++;
      pclaim->know = ptile->u.server.known;
    } else if (ptile->worked) {
      owner = ptile->worked->common.owner;
      pclaim->when = turn + 1;
      pclaim->whom = owner;
      *nextedge = ptile;
      nextedge++;
      pcmap->player_landarea[owner]++;
      pcmap->player_owndarea[owner]++;
      pclaim->know = ptile->u.server.known;
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
      pclaim->know = ptile->u.server.known;
    } else {
      /* pclaim->when = 0; */
      pclaim->whom = no_owner;
      pclaim->know = ptile->u.server.known;
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
  tile_t **midedge;
  int turn, accum, other;
  tile_t **thisedge;
  tile_t **nextedge;

  midedge = &pcmap->edges[map.info.xsize * map.info.ysize];

  for (accum = 1, turn = 1; accum > 0; turn++) {
    thisedge = ((turn & 0x1) == 1) ? pcmap->edges : midedge;
    nextedge = ((turn & 0x1) == 1) ? midedge : pcmap->edges;

    for (accum = 0; *thisedge; thisedge++) {
      tile_t *ptile = *thisedge;
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
  city_t *pcity;
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
    if (pplayer->player_no == game.info.nplayers - 1) {
      free_landarea_map(&cmap);
    }
    return;
  }

  city_list_iterate(pplayer->cities, pcity) {
    int bonus;

    pplayer->score.happy += pcity->common.people_happy[4];
    pplayer->score.content += pcity->common.people_content[4];
    pplayer->score.unhappy += pcity->common.people_unhappy[4];
    pplayer->score.angry += pcity->common.people_angry[4];
    pplayer->score.taxmen += pcity->common.specialists[SP_TAXMAN];
    pplayer->score.scientists += pcity->common.specialists[SP_SCIENTIST];
    pplayer->score.elvis += pcity->common.specialists[SP_ELVIS];
    pplayer->score.population += city_population(pcity);
    pplayer->score.cities++;
    pplayer->score.pollution += pcity->common.pollution;
    pplayer->score.techout += pcity->common.science_total;
    pplayer->score.bnp += pcity->common.trade_prod;
    pplayer->score.mfg += pcity->common.shield_surplus;

    bonus = CLIP(0, get_city_bonus(pcity, EFFECT_TYPE_SCIENCE_BONUS), 100);
    pplayer->score.literacy += (city_population(pcity) * bonus) / 100;
  } city_list_iterate_end;

  if (pplayer->player_no == 0) {
    free_landarea_map(&cmap);
    build_landarea_map(&cmap);
  }
  get_player_landarea(&cmap, pplayer, &landarea, &settledarea);
  pplayer->score.landarea = landarea;
  pplayer->score.settledarea = settledarea;
  if (pplayer->player_no == game.info.nplayers - 1) {
    free_landarea_map(&cmap);
  }

  tech_type_iterate(i) {
    if (get_invention(pplayer, i)==TECH_KNOWN) {
      pplayer->score.techs++;
    }
  } tech_type_iterate_end;

  if(game.ext_info.futuretechsscore)pplayer->score.techs += pplayer->future_tech * 5 / 2;

  unit_list_iterate(pplayer->units, punit) {
    if (is_military_unit(punit)) {
      pplayer->score.units++;
    }
  } unit_list_iterate_end

  impr_type_iterate(i) {
    if (is_wonder(i)
        && (pcity = find_city_by_id(game.info.global_wonders[i]))
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
    {21155, 211, 211}, {255, 215,   0}, {255,  20, 147}, {124, 252,   0},
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
              "%s%+05d.int.ppm", game.server.save_name, game.info.year);

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

  fprintf(fp, "P3\n# version:2\n# gameid: %s\n", game.server.id);
  fprintf(fp, "# An intermediate map from saved Warciv game %s%+05d\n",
          game.server.save_name, game.info.year);


  for (i = 0; i < game.info.nplayers; i++) {
    struct player *pplayer = get_player(i);
    fprintf(fp, "# playerno:%d:color:#%02x%02x%02x:name:\"%s\"\n",
            pplayer->player_no, col[i][0], col[i][1], col[i][2],
            pplayer->name);
  }

  fprintf(fp, "%d %d\n", map.info.xsize, map.info.ysize);
  fprintf(fp, "255\n");

  for (j = 0; j < map.info.ysize; j++) {
    for (i = 0; i < map.info.xsize; i++) {
       tile_t *ptile = native_pos_to_tile(i, j);
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

#ifdef DEBUG
/**************************************************************************
  ...
**************************************************************************/
static void dump_grouping_players(const struct grouping *p)
{
  int j;
  struct player *pp;

  freelog(LOG_DEBUG, "dump_grouping_players grouping=%p", p);
  for (j = 0; j < p->num_players; j++) {
    pp = p->players[j];
    if (!pp) {
      freelog(LOG_DEBUG, "  player %d @ %p !!!", j, pp);
      continue;
    }
    freelog(LOG_DEBUG, "  player %d @ %p: %s user=%s score=%d rank=%f "
            "result=%d player_id=%d",
            j, pp, pp->name, pp->username, get_civ_score(pp), pp->rank,
            pp->result, pp->wcdb.player_id);
    freelog(LOG_DEBUG, "      rated_user_id=%d r=%f "
            "rd=%f ts=%ld nr=%f nrd=%f",
            pp->wcdb.rated_user_id, pp->wcdb.rating,
            pp->wcdb.rating_deviation, pp->wcdb.last_rating_timestamp,
            pp->wcdb.new_rating, pp->wcdb.new_rating_deviation);
  }

}

/**************************************************************************
  ...
**************************************************************************/
static void dump_grouping(const struct grouping *p, int i)
{
  freelog(LOG_DEBUG, "grouping %d @ %p: score=%f rank=%f result=%d "
          "r=%f rd=%f",
          i, p, p->score, p->rank, p->result, p->rating,
          p->rating_deviation);
  freelog(LOG_DEBUG, "    nr=%f nrd=%f num_players=%d num_alive=%d",
          p->new_rating, p->new_rating_deviation, p->num_players,
          p->num_alive);
  dump_grouping_players(p);
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
    dump_grouping(p, i);
  }
  freelog(LOG_DEBUG, "END GROUPING DUMP");
}
#endif /* DEBUG */

/**************************************************************************
  ...
**************************************************************************/
#if 0   /* FOR HARDCORE DEBUGGING ONLY */
static int real_grouping_compare(const void *va, const void *vb);
static int grouping_compare(const void *va, const void *vb)
{
  const struct grouping *a, *b;
  int r;

  a = (const struct grouping *) va;
  b = (const struct grouping *) vb;

  r = real_grouping_compare(a, b);
  freelog(LOG_DEBUG, "about to compare (%p, %p):", a, b);
  dump_grouping(a, -1);
  dump_grouping(b, -2);
  freelog(LOG_DEBUG, "grouping_compare(%p, %p) = %d", a, b, r);
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
  if (a->result == PR_LOSE && b->result != PR_LOSE)
    return 1;
  if (a->result != PR_LOSE && b->result == PR_LOSE)
    return -1;
  if (a->result == PR_DRAW && b->result == PR_DRAW)
    return 0;
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
static int player_compare(const void *va, const void *vb)
{
  struct player *a, *b;
  a = *((struct player **) va);
  b = *((struct player **) vb);

  if (a->result == PR_WIN && b->result != PR_WIN)
    return -1;
  if (a->result != PR_WIN && b->result == PR_WIN)
    return 1;
  if (a->result == PR_LOSE && b->result != PR_LOSE)
    return 1;
  if (a->result != PR_LOSE && b->result == PR_LOSE)
    return -1;
  if (a->result == PR_DRAW && b->result == PR_DRAW)
    return 0;
  if (a->result == PR_DRAW && b->result != PR_DRAW)
    return -1;
  if (a->result != PR_DRAW && b->result == PR_DRAW)
    return 1;

  if (a->is_alive && !b->is_alive)
    return -1;
  if (!a->is_alive && b->is_alive)
    return 1;

  return score_cache[b->player_no] - score_cache[a->player_no];
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
    if (pplayer->wcdb.rating > 0.0
        && pplayer->wcdb.rating_deviation >= RATING_CONSTANT_MINIMUM_RD
        && pplayer->wcdb.rated_user_id > 0) {
      num_rated_users++;
    }
  } players_iterate_end;

  freelog(LOG_DEBUG, "game_can_be_rated num_rated_users=%d game.info.turn=%d",
          num_rated_users, game.info.turn);

  if ((game.server.wcdb.type == GAME_TYPE_SOLO && num_rated_users < 1)
      || (game.server.wcdb.type != GAME_TYPE_SOLO && num_rated_users < 2)) {
    notify_conn(NULL, _("Game: The game cannot be rated because there "
                        "are not enough rated users in the game."));
    return FALSE;
  }

  if (srvarg.wcdb.min_rated_turns > game.info.turn) {
    notify_conn(NULL, _("Game: The game cannot be rated because not "
                        "enough turns (%d) have been played."),
                srvarg.wcdb.min_rated_turns);
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

  NB: Also sets rating and RD fields in team structs.
  NB: Modifies global groupings array.
**************************************************************************/
void score_calculate_grouping_ratings(void)
{
  double K, A, B, min_r, RD, avgRD, sum, r;
  int i, j;

  K = RATING_CONSTANT_EXTRA_TEAMMATE_INFLUENCE;
  A = RATING_CONSTANT_AVERAGE_PLAYER_RATING;
  B = RATING_CONSTANT_BAD_PLAYER_RATING;
  min_r = RATING_CONSTANT_AVERAGE_PLAYER_RATING;

  for (i = 0; i < num_groupings; i++) {

    /* A grouping's base rating is the average of
     * its members' ratings. Its RD is the average
     * of its members' RDs. */
    sum = 0.0;
    avgRD = 0.0;
    for (j = 0; j < groupings[i].num_players; j++) {
      sum += groupings[i].players[j]->wcdb.rating;
      RD = groupings[i].players[j]->wcdb.rating_deviation;
      avgRD += RD;
      min_r = MIN(min_r, groupings[i].players[j]->wcdb.rating);
    }
    sum /= (double) groupings[i].num_players;
    avgRD /= (double) groupings[i].num_players;

    /* If there is more than 1 player in the grouping,
     * add to the base rating the approximate effect
     * each player will have. Thus better-than-average
     * players will increase the rating, while worse
     * players will decrease it. Furthermore, the more
     * members the grouping has, the smaller this
     * contribution will be. */
    if (groupings[i].num_players > 1) {
      for (j = 0; j < groupings[i].num_players; j++) {
        r = groupings[i].players[j]->wcdb.rating;
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
    groupings[i].rating_deviation = avgRD;

    if (player_is_on_team(groupings[i].players[0])) {
      /* Set corresponding wcdb fields in team structures. */
      struct team *pteam = team_get_by_id(groupings[i].players[0]->team);

      pteam->server.wcdb.rating = groupings[i].rating;
      pteam->server.wcdb.rating_deviation = groupings[i].rating_deviation;
    }
  }
}

/**************************************************************************
  NB: Modifies global groupings and player structs.
**************************************************************************/
void score_propagate_grouping_ratings(void)
{
  int i, j;
  double rating_change, rd_change, sum, new_r, new_RD, trank, weight;

  for (i = 0; i < num_groupings; i++) {
    rating_change = groupings[i].new_rating - groupings[i].rating;
    rd_change = groupings[i].new_rating_deviation
        - groupings[i].rating_deviation;

    /* Note 'sum' is the sum of all the trank (i.e. the rank of
     * a player in his own team, with 1 being first place and
     * groupings[i].num_players being that last place value)
     * values used below. We use n*(n+1)/2 not (n-1)*n/2 since
     * the trank values start at 1 (unlike most other internal
     * rank values, which start at 0). */
    sum = groupings[i].num_players
        * (groupings[i].num_players + 1.0) / 2.0;

    for (j = 0; j < groupings[i].num_players; j++) {
      new_r = groupings[i].players[j]->wcdb.rating;
      new_RD = groupings[i].players[j]->wcdb.rating_deviation;

      if (groupings[i].num_players == 1) {
        new_r += rating_change;
        new_RD += rd_change;
      } else {
        /* The "pie/blame" allotment scheme:
         * If the rating change is positive, "better" members get a
         * bigger piece of the pie (i.e. the rating change). If the
         * change is negative, "worse" members get a bigger piece of
         * the blame. */

        /* NB: HIGHER score means LOWER rank, i.e. 0 is first place. */

        /* Add 1 to the in-team rank to avoid weight = 0. */
        trank = groupings[i].players[j]->team_rank + 1.0;

        if (rating_change > 0) {
          /* Flip the in-team rank so that players with
           * a LOWER rank (i.e. higher score) get a BIGGER
           * piece of the rating change. */
          trank = groupings[i].num_players - trank + 1;
        }

        weight = trank / sum;

        new_r += rating_change * weight;

        /* If the player ranks highly (i.e. near 1st place)
         * in his team and the team wins, 'weight' will be
         * larger. This corresponds to a higher certainty that
         * the player played a role in the team's win, hence
         * the RD change should be larger.
         *
         * Similarly, if the team loses and the player ranks
         * lowly (i.e. near last place in the team), then it
         * is probable that the player contributed more to the
         * team's loss. Hence again 'weight' will be larger and
         * the RD change larger as well. */
        new_RD += rd_change * weight;
      }


      /* As per Glicko's suggestion. */
      if (new_RD < RATING_CONSTANT_MINIMUM_RD) {
        new_RD = RATING_CONSTANT_MINIMUM_RD;
      }

      groupings[i].players[j]->wcdb.new_rating = new_r;
      groupings[i].players[j]->wcdb.new_rating_deviation = new_RD;
    }
  }
}

/**************************************************************************
  Glicko rating system extended to variable-sized groups of players. To
  understand what this code is trying to do, read what it is based on at
  http://math.bu.edu/people/mg/glicko/glicko.doc/glicko.html.

  NB: Modifies global groupings and player structs.
**************************************************************************/
static void update_ratings(void)
{
  int i, j;
  double RD, c, t, r, q, q2, RD2, sum, inv_d2;
  double rj, sj, E, new_r, new_RD, gRD[MAX_NUM_PLAYERS];
  time_t now_time;

  if (game.server.wcdb.type == GAME_TYPE_SOLO) {
    assert(num_groupings == 1);
    assert(groupings[0].num_players == 1);
  }

  /* Adjust player RDs to reflect the passage
   * of time. (Glicko Step 1) */

  c = RATING_CONSTANT_C;
  now_time = time(NULL);
  players_iterate(pplayer) {
    if (pplayer->wcdb.last_rating_timestamp <= 0) {
      continue;
    }

    RD = pplayer->wcdb.rating_deviation;
    t = (double) (now_time - pplayer->wcdb.last_rating_timestamp)
        / RATING_CONSTANT_SECONDS_PER_RATING_PERIOD;

    pplayer->wcdb.rating_deviation
      = MIN(sqrt(RD*RD + c*c*t), RATING_CONSTANT_MAXIMUM_RD);
  } players_iterate_end;


  /* Transform player ratings into team/grouping ratings. */
  score_calculate_grouping_ratings();


  /* Update ratings. (Glicko Step 2) */

  /* Fill gRD[j] table to avoid recalculation. */
  if (game.server.wcdb.type == GAME_TYPE_SOLO) {
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

      if (game.server.wcdb.type == GAME_TYPE_SOLO) {
        rj = score_calculate_solo_opponent_rating(&groupings[0]);

        /* You only 'win' if you get to Alpha Centauri. */
        sj = game.server.wcdb.outcome == GAME_ENDED_BY_SPACESHIP ? 1.0 : 0.0;

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
         *    PR_LOSE ==> sj = 0.0
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
      /* Set corresponding wcdb fields in team structures. */
      struct team *pteam = team_get_by_id(groupings[i].players[0]->team);

      pteam->server.wcdb.new_rating = groupings[i].new_rating;
      pteam->server.wcdb.new_rating_deviation = groupings[i].new_rating_deviation;
    }
  }

  /* Now transform the new grouping ratings and RDs into new
   * player ratings and RDs. (Not part of Glicko, caveat lector) */
  score_propagate_grouping_ratings();
}

/**************************************************************************
  NB: Modifies global grouping array.
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
        || pplayer->is_civil_war_split) {
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
  Calculate the fractional ranking of the 'nmemb' objects each of 'size'
  bytes given in 'base' using the supplied comparison function 'cmp' and
  recording the results into 'frac_ranks'.

  NB: The elements of 'base' are assumed to be ordered already.
  NB: The 'frac_ranks' array must have room for at least 'nmemb'
  elements.
**************************************************************************/
static void calculate_fractional_ranking(void *base,
                                         int nmemb,
                                         size_t size,
                                         float *frac_ranks,
                                         compare_func_t cmp)
{
  int i, j, k;
  float ranksum, frac_rank;
  char *a = base;

  freelog(LOG_DEBUG, "calculate_fractional_ranking base=%p nmemb=%d "
          "size=%d", base, nmemb, (int) size);

  if (nmemb < 1) {
    return;
  }

  assert(base != NULL);
  assert(frac_ranks != NULL);
  assert(cmp != NULL);

  i = 0;
  frac_ranks[0] = 0.0;
  do {
    ranksum = (float) i;
    j = i + 1;
    while (j < nmemb && 0 == cmp(a + (j-1)*size, a + j*size)) {
      ranksum += (float) j;
      j++;
    }
    frac_rank = ranksum / (float) (j - i);
    for (k = i; k < j; k++) {
      frac_ranks[k] = frac_rank;
    }
    i = j;
  } while (i < nmemb);

}
/**************************************************************************
  NB: Modifies global grouping array.
**************************************************************************/
void score_update_grouping_results(void)
{
  int i, j, next_cmp;
  float frac_ranks[MAX_NUM_PLAYERS];
  bool force_draw, force_loss;

  assert(num_groupings > 0);

  /* Build the score cache, so we don't have to
     constantly call get_civ_score. */
  players_iterate(pplayer) {
    if (is_barbarian(pplayer)) {
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
      groupings[i].score = pteam->server.score;
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
    } else if (force_loss && groupings[i].result == PR_NONE) {
      groupings[i].result = PR_LOSE;
    }
  }

#ifdef DEBUG
  freelog(LOG_DEBUG, "Groupings after result assignment:");
  dump_groupings();
#endif

  /* Sort the groupings, and the players within the groupings. */

  qsort(groupings, num_groupings, sizeof(struct grouping),
        grouping_compare);

  for (i = 0; i < num_groupings; i++) {
    if (groupings[i].num_players < 2) {
      continue;
    }
    freelog(LOG_DEBUG, "sorting players in grouping %d", i);
    qsort(groupings[i].players, groupings[i].num_players,
          sizeof(struct player *), player_compare);
  }

#ifdef DEBUG
  freelog(LOG_DEBUG, "Groupings after sort:");
  dump_groupings();
#endif

  /* Fill in unset results based on the order. */

  if (groupings[0].result == PR_NONE) {
    if (num_groupings < 2) {
      if (game.server.wcdb.type == GAME_TYPE_SOLO
          && game.server.wcdb.outcome != GAME_ENDED_BY_SPACESHIP) {
        groupings[0].result = PR_LOSE;
      } else {
        groupings[0].result = PR_WIN;
      }
    } else {
      next_cmp = grouping_compare(&groupings[0], &groupings[1]);
      groupings[0].result = PR_DRAW;
      if (next_cmp != 0) {
        groupings[0].result = PR_WIN;
      } else {
        i = 0;
        while (TRUE) {
          i++;
          if (i >= num_groupings - 1) {
            groupings[i].result = PR_DRAW;
            break;
          }
          next_cmp = grouping_compare(&groupings[i], &groupings[i+1]);
          groupings[i].result = PR_DRAW;
          if (next_cmp != 0) {
            break;
          }
        }
      }
    }
  }

  for (i = 0; i < num_groupings; i++) {
    if (groupings[i].result != PR_NONE) {
      continue;
    }
    groupings[i].result = PR_LOSE;
  }

#ifdef DEBUG
  freelog(LOG_DEBUG, "Groupings after result assignment:");
  dump_groupings();
#endif

  /* Assign fractional ranks based on the order. */
  calculate_fractional_ranking(groupings,
                               num_groupings,
                               sizeof(struct grouping),
                               frac_ranks,
                               grouping_compare);
  for (i = 0; i < num_groupings; i++) {
    groupings[i].rank = frac_ranks[i];
  }

#ifdef DEBUG
  freelog(LOG_DEBUG, "Groupings after fractional ranking:");
  dump_groupings();
#endif

}

/**************************************************************************
  Propagate the groupings' results back to the players and teams, and
  calculate the fractional ranking for players.

  NB: Assumes groupings' results and ranks have been calculated.
  NB: Modifies global groupings, player and team structs.
**************************************************************************/
void score_propagate_grouping_results(void)
{
  int i, j;
  float frac_ranks[MAX_NUM_PLAYERS];
  struct team *pteam = NULL;
  struct player *ranked[MAX_NUM_PLAYERS];
  int num_ranked = 0;

  /* Copy grouping results/ranks to teams. */
  for (i = 0; i < num_groupings; i++) {
    if (!player_is_on_team(groupings[i].players[0])) {
      continue;
    }
    pteam = team_get_by_id(groupings[i].players[0]->team);
    if (!pteam) {
      continue;
    }
    pteam->server.rank = groupings[i].rank;
    pteam->server.result = groupings[i].result;
  }


  for (i = 0; i < num_groupings; i++) {

    /* Propagate grouping results to players. */
    for (j = 0; j < groupings[i].num_players; j++) {
      groupings[i].players[j]->result = groupings[i].result;
    }

    /* Calculate fractional team rank (the rank of
     * the player only with respect to other members
     * of the grouping). */
    calculate_fractional_ranking(groupings[i].players,
                                 groupings[i].num_players,
                                 sizeof(struct player *),
                                 frac_ranks,
                                 player_compare);
    for (j = 0; j < groupings[i].num_players; j++) {
      groupings[i].players[j]->team_rank = frac_ranks[j];
    }

    /* Assemble the players into the ranked list. */
    for (j = 0; j < groupings[i].num_players; j++) {
      ranked[num_ranked++] = groupings[i].players[j];
    }
  }

  /* Now calculate the overall fractional player rank. */
  calculate_fractional_ranking(ranked,
                               num_ranked,
                               sizeof(struct player *),
                               frac_ranks,
                               player_compare);
  for (i = 0; i < num_ranked; i++) {
    ranked[i]->rank = frac_ranks[i];
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

  if (!game.server.rated || !srvarg.wcdb.enabled || !srvarg.auth.enabled) {
    return;
  }

  /* Get the old ratings. */
  if (!wcdb_load_player_ratings(game.server.wcdb.type, TRUE)) {
    notify_conn(NULL, _("Game: Though the server option 'rated' was set, "
        "player ratings cannot be updated because there was an error "
        "communicating with the database."));
    game.server.rated = FALSE;
    return;
  }

  if (!game_can_be_rated()) {
    game.server.rated = FALSE;
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

    pteam->server.score = sum / (double) pteam->member_count;
  } team_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void score_assign_ai_rating(struct player *pplayer,
                            int game_type)
{
  score_get_ai_rating(pplayer->ai.skill_level, game_type,
                      &pplayer->wcdb.rating,
                      &pplayer->wcdb.rating_deviation);
  pplayer->wcdb.new_rating = 0.0;
  pplayer->wcdb.new_rating_deviation = 0.0;
  pplayer->wcdb.last_rating_timestamp = 0;
}

/**************************************************************************
  ...
**************************************************************************/
void score_assign_new_player_rating(struct player *pplayer,
                                    int game_type)
{
  pplayer->wcdb.rating = RATING_CONSTANT_AVERAGE_PLAYER_RATING;
  pplayer->wcdb.rating_deviation = RATING_CONSTANT_MAXIMUM_RD;
  pplayer->wcdb.new_rating = 0.0;
  pplayer->wcdb.new_rating_deviation = 0.0;
  pplayer->wcdb.last_rating_timestamp = 0;
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
  T = game.info.turn;

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
  if (game_type == GAME_TYPE_TEAM) {
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
  if (pplayer->wcdb.rated_user_id <= 0
      || pplayer->wcdb.rated_user_name[0] == '\0') {
    return player_get_username(pplayer, outbuf, maxlen);
  }

  return mystrlcpy(outbuf, pplayer->wcdb.rated_user_name, maxlen);
}

