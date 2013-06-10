/**********************************************************************
 Freeciv - Copyright (C) 2002 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
// cm_ is city management

#ifdef HAVE_CONFIG_H
# include "../../config.h"
#endif

#include <assert.h>
#include <stdio.h> /* for sprintf */
#include <stdlib.h>
#include <string.h>

#include "../city.h"
#include "wc_intl.h"
#include "../game.h"
#include "../government.h"
#include "hash.h"
#include "log.h"
#include "../map.h"
#include "mem.h"
#include "shared.h"
#include "support.h"
#include "timing.h"

#include "cm.h"

/*
 * Terms used
 * ==========
 *
 * Stats: food, shields, trade, luxury, science, and gold.
 * Production: amount of stats you get directly from farming tiles or
 *             having specialists.
 * Surplus: amount of stats you get, taking buildings, taxes, disorder, and
 *          any other effects into account.
 *
 * Happy City Management State: disorder (unhappy), content (!unhappy && !happy)
 *                              and happy (happy)
 *
 * Tile type: usually, many tiles produce the same food/shield/trade.
 * So we group the tiles into tile types.  Specialists are also a 'tile type.'
 *
 * Unlike the original CM code, which used a dynamic programming approach,
 * this code uses a branch-and-bound approach.  The dynamic programmin approach
 * allowed cacheing, but it was hard to guarantee the correctness of the cache,
 * so it was usually tossed and recomputed.
 *
 * The branch and bound approach also allows a very simple greedy search,
 * whereas the dynamic programming approach required a lot of pre-computing.
 * And, it appears to be very slightly faster.
 * It evaluates about half as many solutions, but each candidate solution is
 * more expensive due to the lack of cacheing.
 *
 * We use highly specific knowledge about how the city computes its stats
 * in two places:
 * - setting the min_production array.   Ideally the city should tell us.
 * - computing the weighting for tiles.  Ditto.
 */


/****************************************************************************
 Probably these declarations should be in cm.h in the long term.
*****************************************************************************/
struct cm_state;

static struct cm_state *cm_init_cmstate(city_t *pcity);
static void             cm_find_best_solution(struct cm_state *cmstate,
                            const struct cm_parameter *const parameter,
                            struct cm_result *result);

/****************************************************************************
 defines, structs, globals, forward declarations
*****************************************************************************/

#ifdef DEBUG
//#  define GATHER_TIME_STATS   gilles FIXME
#  define CM_DEBUG
#endif

/* whether to print every query, or just at cm_free ; matters only
   if GATHER_TIME_STATS is on */
#define PRINT_TIME_STATS_EVERY_QUERY

#define LOG_TIME_STATS                                  LOG_DEBUG
#define LOG_CM_STATE                                    LOG_DEBUG
#define LOG_LATTICE                                     LOG_DEBUG
#define LOG_REACHED_LEAF                                LOG_DEBUG
#define LOG_BETTER_LEAF                                 LOG_DEBUG
#define LOG_PRUNE_BRANCH                                LOG_DEBUG

#ifdef GATHER_TIME_STATS
static struct performance_s {
  struct one_perf {
    struct timer *wall_timer;
    int query_count;
    int apply_count;
    const char *name;
  } greedy;
  struct one_perf opt;

  struct one_perf *current;
} performance;

static void print_performance(struct one_perf *counts);
#endif

/* Fitness of a solution.  */
struct cm_fitness {
  int  weighted; /* weighted sum */
  bool sufficient; /* false => doesn't meet constraints */
};


/*
 * We have a cyclic structure here, so we need to forward-declare the
 * structs
 */
struct cm_tile_type;

/*
 * A tile.  Has a pointer to the type, and the x/y coords.
 * Used mostly just for converting to cm_result.
 */
struct cm_tile {
  const struct cm_tile_type *type;
  int x;
  int y; /* valid only if !is_specialist */
};

/* define the cm_tile_vector as array<struct cm_tile> */
#define SPECVEC_TAG cm_tile
#define SPECVEC_TYPE struct cm_tile
#include "specvec.h"

/* define the cm_tile_type_vector as array <cm_tile_type*> */
#define SPECVEC_TAG cm_tile_type
#define SPECVEC_TYPE struct cm_tile_type *
#include "specvec.h"
#define tile_type_vector_iterate(vector, var) { \
    struct cm_tile_type *var; \
    TYPED_VECTOR_ITERATE(struct cm_tile_type*, vector, var##p) { \
      var = *var##p; \
      {
#define tile_type_vector_iterate_end }} VECTOR_ITERATE_END; }

static inline void cm_tile_type_vector_add(struct cm_tile_type_vector *tthis,
                                           struct cm_tile_type *toadd)
{
  cm_tile_type_vector_append(tthis, &toadd);
}


/*
 * A tile type.
 * Holds the production (a hill produces 1/0/0);
 * Holds a list of which tiles match this (all the hills and tundra);
 * Holds a list of other types that are strictly better
 * (grassland 2/0/0, plains 1/1/0, but not gold mine 0/1/6).
 * Holds a list of other types that are strictly worse
 * (the grassland and plains hold a pointer to the hill).
 *
 * Specialists are special types; is_specialist is set, and the tile
 * vector is empty.  We can never run out of specialists.
 */
struct cm_tile_type {
  int    production[CM_NUM_STATS];
  double estimated_fitness;    /* weighted sum of production */
  bool   is_specialist;
  enum   specialist_type spec; /* valid only if is_specialist */
  struct cm_tile_vector tiles; /* valid only if !is_specialist */
  struct cm_tile_type_vector better_types;
  struct cm_tile_type_vector worse_types;
  int    lattice_index;        /* index in cmstate->lattice */
  int    lattice_depth;        /* depth = sum(#tiles) over all better types */
};


/*
 * A partial solution.
 * Has the count of workers assigned to each lattice position, and
 * a count of idle workers yet unassigned.
 */
struct partial_solution {
  /* indices for these two match the lattice indices */
  int *worker_counts;   /* number of workers on each type */
  int *prereqs_filled;  /* number of better types filled up */

  int production[CM_NUM_STATS]; /* raw production, cached for the heuristic */
  int idle;             /* number of idle workers */
};

/*
 * City management state of the search.
 * This holds all the information needed to do the search, all in one
 * struct, in order to clean up the function calls.
 */
struct cm_state {
  /* input from the caller */
  struct cm_parameter parameter;
  /*mutable*/ city_t *pcity;

  /* the tile lattice */
  struct cm_tile_type_vector lattice;
  struct cm_tile_type_vector lattice_by_prod[CM_NUM_STATS];

  /* the best known solution, and its fitness */
  struct partial_solution best;
  struct cm_fitness       best_value;

  /* hard constraints on production: any solution with less production than
   * this fails to satisfy the constraints, so we can stop investigating
   * this branch.  A solution with more production than this may still
   * fail (for being unhappy, for instance). */
  int min_production[CM_NUM_STATS];

  /* the current solution we're examining. */
  struct partial_solution current;

  /*
   * Where we are in the search.  When we add a worker to the current
   * partial solution, we also push the tile type index on the stack.
   */
  struct choice_s {
    int *stack;
    int size;
  } choice;
};


/* return #fields + specialist types */
static int num_types(const struct cm_state *cmstate);


/* debugging functions */
#ifdef CM_DEBUG
static void print_tile_type(int loglevel,
                            const struct cm_tile_type *ptype,
                            const char *prefix);
static void print_lattice(int loglevel,
                          const struct tile_type_vector *lattice);
static void print_partial_solution(int loglevel,
                                   const struct partial_solution *soln,
                                   const struct cm_state         *cmstate);
#else
#  define print_tile_type(loglevel, ptype, prefix)
#  define print_lattice(loglevel, lattice)
#  define print_partial_solution(loglevel, soln, cmstate)
#endif


#ifdef GATHER_TIME_STATS
/****************************************************************************
  Initialize the CM data at the start of each game.  Note the citymap
  indices will not have been initialized yet (cm_init_citymap is called
  when they are).
****************************************************************************/
void cm_init(void)
{
  /* In the B&B algorithm there's not really anything to initialize. */
  memset(&performance, 0, sizeof(performance));

  if (!performance.greedy.wall_timer) {
    performance.greedy.wall_timer = new_timer(TIMER_USER, TIMER_ACTIVE);
    performance.greedy.name = "greedy";
  }
  if (!performance.opt.wall_timer) {
    performance.opt.wall_timer = new_timer(TIMER_USER, TIMER_ACTIVE);
    performance.opt.name = "opt";
  }
}
#endif

/****************************************************************************
  Initialize the CM citymap data.  This function is called when the
  city map indices are generated (basically when the topology is set,
  shortly after the start of the game).
****************************************************************************/
void cm_init_citymap(void)
{
  /* In the B&B algorithm there's not really anything to initialize. */
}

/****************************************************************************
  Clear the cache for a city.
****************************************************************************/
void cm_clear_cache(city_t *pcity)
{
  /* The B&B algorithm doesn't have city caches so there's nothing to do. */
}

/****************************************************************************
  Called at the end of a game to free any CM data.
****************************************************************************/
void cm_free(void)
{
#ifdef GATHER_TIME_STATS
  print_performance(&performance.greedy);
  print_performance(&performance.opt);

  free_timer(performance.greedy.wall_timer);
  free_timer(performance.opt.wall_timer);
  memset(&performance, 0, sizeof(performance));
#endif
}


/***************************************************************************
  Iterate over all valid city tiles (that is, don't iterate over tiles
  off the edge of the world).
 ***************************************************************************/
#define my_city_map_iterate(pcity, cx, cy) \
  city_map_checked_iterate(pcity->common.tile, cx, cy, itr_tile)

#define my_city_map_iterate_end city_map_checked_iterate_end;


/***************************************************************************
  Functions of tile-types.
 ***************************************************************************/

/****************************************************************************
  Set all production to zero and initialize the vectors for this tile type.
****************************************************************************/
static void cm_tile_type_init(struct cm_tile_type *type)
{
  memset(type, 0, sizeof(struct cm_tile_type));
  //tile_vector_init(&type->tiles);
  //tile_type_vector_init(&type->better_types);
  //tile_type_vector_init(&type->worse_types);
}

/****************************************************************************
  Duplicate a tile type, except for the vectors - the vectors of the new tile
  type will be empty.
****************************************************************************/
static struct cm_tile_type *
cm_tile_type_dup(const struct cm_tile_type *oldtype)
{
  struct cm_tile_type *newtype = wc_malloc(sizeof(struct cm_tile_type));

  memcpy(newtype, oldtype, sizeof(*oldtype));
  cm_tile_vector_init(&newtype->tiles);
  cm_tile_type_vector_init(&newtype->better_types);
  cm_tile_type_vector_init(&newtype->worse_types);
  return newtype;
}

/****************************************************************************
  Free all the storage in the tile type (but don't free the type itself).
****************************************************************************/
static void cm_tile_type_destroy(struct cm_tile_type *type)
{
  /* The call to vector_free() will magically free all the tiles in the
   * vector. */
  cm_tile_vector_free(&type->tiles);
  cm_tile_type_vector_free(&type->better_types);
  cm_tile_type_vector_free(&type->worse_types);
}

/****************************************************************************
  Destroy and free all types in the vector, and the vector itself.  This
  will free all memory associated with the vector.
****************************************************************************/
static void cm_tile_type_vector_free_all(struct cm_tile_type_vector *vec)
{
  tile_type_vector_iterate(vec, type) {
    /* Destroy all data in the type, and free the type itself. */
    cm_tile_type_destroy(type);
    free(type);
  } tile_type_vector_iterate_end;

  cm_tile_type_vector_free(vec);
}

/****************************************************************************
  Return TRUE iff all categories of the two types are equal.  This means
  all production outputs are equal and the is_specialist fields are also
  equal.
****************************************************************************/
static bool cm_tile_type_equal(const struct cm_tile_type *a,
                               const struct cm_tile_type *b)
{
  enum cm_stat stat;

  for (stat = 0; stat < CM_NUM_STATS; stat++) {
    if (a->production[stat] != b->production[stat])  {
      return FALSE;
    }
  }

  if (a->is_specialist != b->is_specialist) {
    return FALSE;
  }

  return TRUE;
}

/****************************************************************************
  Return TRUE if tile a is better or equal to tile b in all categories.

  Specialists are considered better than workers (all else being equal)
  since we have an unlimited number of them.
****************************************************************************/
static bool cm_tile_type_better(const struct cm_tile_type *a,
                                const struct cm_tile_type *b)
{
  enum cm_stat stat;

  for (stat = 0; stat < CM_NUM_STATS; stat++) {
    if (a->production[stat] < b->production[stat])  {
      return FALSE;
    }
  }

  if (a->is_specialist && !b->is_specialist) {
    /* If A is a specialist and B isn't, and all of A's production is at
     * least as good as B's, then A is better because it doesn't tie up
     * the map tile. */
    return TRUE;
  } else if (!a->is_specialist && b->is_specialist) {
    /* Vice versa. */
    return FALSE;
  }

  return TRUE;
}

/****************************************************************************
  If there is a tile type in the vector that is equivalent to the given
  type, return its index.  If not, return -1.

  Equivalence is defined in tile_type_equal().
****************************************************************************/
static int cm_tile_type_vector_find_equivalent(
                                const struct cm_tile_type_vector *vec,
                                const struct cm_tile_type     *ptype)
{
  int i;

  for (i = 0; i < vec->size; i++) {
    if (cm_tile_type_equal(vec->p[i], ptype)) {
      return i;
    }
  }

  return -1;
}

/****************************************************************************
  Return the number of tiles of this type that can be worked.  For
  is_specialist types this will always be infinite but for other types of
  tiles it is limited by what's available in the citymap.
****************************************************************************/
static int cm_tile_type_num_tiles(const struct cm_tile_type *type)
{
  if(type->is_specialist) {
    return WC_INFINITY;
  } else {
    return cm_tile_vector_size(&type->tiles);
  }
}

/****************************************************************************
  Return the number of tile types that are better than this type.

  Note this isn't the same as the number of *tiles* that are better.  There
  may be more than one tile of each type (see cm_tile_type_num_tiles).
****************************************************************************/
static int cm_tile_type_num_prereqs(const struct cm_tile_type *ptype)
{
  return ptype->better_types.size;
}

/****************************************************************************
  Retrieve a tile type by index.  For a given cmstate there are a certain
  number of tile types, which may be iterated over using this function
  as a lookup.
****************************************************************************/
static const struct cm_tile_type *cm_tile_type_get(const struct cm_state *cmstate,
                                                   int type)
{
  /* Sanity check the index. */
  assert(type >= 0);
  assert(type < cmstate->lattice.size);
  return cmstate->lattice.p[type];
}

/****************************************************************************
  Retrieve a tile of a particular type by index.  For a given tile type
  there are a certain number of tiles (1 or more), which may be iterated
  over using this function for index.  Don't call this for is_specialist
  types.  See also cm_tile_type_num_tiles().
****************************************************************************/
static const struct cm_tile *cm_tile_get(const struct cm_tile_type *ptype, int j)
{
  assert(j >= 0);
  assert(j < ptype->tiles.size);
  return &ptype->tiles.p[j];
}


/**************************************************************************
  Functions on the cm_fitness struct.
**************************************************************************/

/****************************************************************************
  Return TRUE iff fitness A is strictly better than fitness B.
****************************************************************************/
static bool fitness_better(struct cm_fitness *a, struct cm_fitness *b)
{
  if (a->sufficient != b->sufficient) {
    return a->sufficient;
  }
  return a->weighted > b->weighted;
}

#if 0
/****************************************************************************
  Return a fitness struct that is the worst possible result we can
  represent.
****************************************************************************/
static void worst_fitness(struct cm_fitness *fitness)
{
  fitness->sufficient = FALSE;
  fitness->weighted = -WC_INFINITY;
}
#endif

/****************************************************************************
  Compute the fitness of the given surplus (and disorder/happy status)
  according to the weights and minimums given in the parameter.
****************************************************************************/
static void compute_fitness(const int surplus[CM_NUM_STATS],
                            bool disorder,
                            bool happy,
                            const struct cm_parameter *parameter,
                            struct cm_fitness         *fitness)
{
  enum cm_stat stat;
  /* struct cm_fitness *fitness = malloc(sizeof(struct cm_fitness)); */

  fitness->sufficient = TRUE;
  fitness->weighted = 0;

  for (stat = 0; stat < CM_NUM_STATS; stat++) {
    fitness->weighted += surplus[stat] * parameter->factor[stat];
    if (surplus[stat] < parameter->minimal_surplus[stat]) {
      fitness->sufficient = FALSE;
    }
  }

  if (happy) {
    fitness->weighted += parameter->happy_factor;
  } else if (parameter->require_happy) {
    fitness->sufficient = FALSE;
  }

  if (disorder && !parameter->allow_disorder) {
    fitness->sufficient = FALSE;
  }
}

/***************************************************************************
  Handle struct partial_solution.
  - perform a deep copy
  - convert to city
  - convert to cm_result
 ***************************************************************************/

/****************************************************************************
  Allocate and initialize an empty solution.
****************************************************************************/
static void init_partial_solution(struct partial_solution *into,
                                  int ntypes,
                                  int idle)
{
  into->worker_counts  = wc_calloc(ntypes, sizeof(int));	/* sizeof(*into->worker_counts) */
  into->prereqs_filled = wc_calloc(ntypes, sizeof(int));	/* sizeof(*into->prereqs_filled) */
  memset(into->production, 0, sizeof(into->production));
  into->idle = idle;
}

#if 0
/****************************************************************************
  Free all storage associated with the solution.  This is basically the
  opposite of init_partial_solution.
****************************************************************************/
static void destroy_partial_solution(struct partial_solution *into)
{
  free(into->worker_counts);
  free(into->prereqs_filled);
}
#endif

/****************************************************************************
  Copy the source solution into the destination one (the destination
  solution must already be allocated).
****************************************************************************/
static void copy_partial_solution(struct partial_solution       *dst,
                                  const struct partial_solution *src,
                                  const struct cm_state         *cmstate)
{
  memcpy(dst->worker_counts, src->worker_counts,
         sizeof(*dst->worker_counts) * num_types(cmstate));
  memcpy(dst->prereqs_filled, src->prereqs_filled,
         sizeof(*dst->prereqs_filled) * num_types(cmstate));
  memcpy(dst->production, src->production, sizeof(dst->production));
  dst->idle = src->idle;
}


/**************************************************************************
  Evaluating a completed solution.
**************************************************************************/

/****************************************************************************
  Apply the solution to the city.

  Note this function writes directly into the city's citymap, unlike most
  other code which uses accessor functions.
****************************************************************************/
static void apply_solution(struct cm_state *cmstate,
                           const struct partial_solution *soln)
{
  city_t *pcity = cmstate->pcity;
  int i;
  int sumworkers = 0;

#ifdef GATHER_TIME_STATS
  performance.current->apply_count++;
#endif

  assert(soln->idle == 0);

  /* Clear all specialists, and remove all workers from fields (except
   * the city center). */
  memset(&pcity->common.specialists, 0, sizeof(pcity->common.specialists));
  city_map_iterate(x, y) {
    if (is_city_center(x, y)) {
      continue;
    }
    if (pcity->common.city_map[x][y] == C_TILE_WORKER) {
      pcity->common.city_map[x][y] = C_TILE_EMPTY;
    }
  } city_map_iterate_end;

  /* Now for each tile type, find the right number of such tiles and set them
   * as worked.  For specialists we just increase the number of specialists
   * of that type. */
  for (i = 0 ; i < num_types(cmstate); i++) {
    int nworkers = soln->worker_counts[i];
    const struct cm_tile_type *type;

    if (nworkers == 0) {
      /* No citizens of this type. */
      continue;
    }
    sumworkers += nworkers;

    type = cm_tile_type_get(cmstate, i);

    if (type->is_specialist) {
      /* Just increase the number of specialists. */
      pcity->common.specialists[type->spec] += nworkers;
    } else {
      int j;

      /* Place citizen workers onto the citymap tiles. */
      for (j = 0; j < nworkers; j++) {
        const struct cm_tile *tile = cm_tile_get(type, j);

        pcity->common.city_map[tile->x][tile->y] = C_TILE_WORKER;
      }
    }
  }

  /* Finally we must refresh the city to reset all the precomputed fields. */
  generic_city_refresh(pcity, FALSE, 0);
  assert(sumworkers == pcity->common.pop_size);
}

/****************************************************************************
  Convert the city's surplus numbers into an array.  Get the happy/disorder
  values, too.  This fills in the surplus array and disorder and happy
  values based on the city's data.
****************************************************************************/
static void get_city_surplus(const city_t *pcity,
                             int (*surplus)[CM_NUM_STATS],
                             bool *disorder,
                             bool *happy)
{
  (*surplus)[CM_FOOD]    = pcity->common.food_surplus;
  (*surplus)[CM_SHIELD]  = pcity->common.shield_surplus;
  (*surplus)[CM_TRADE]   = pcity->common.trade_prod;
  (*surplus)[CM_GOLD]    = city_gold_surplus(pcity, pcity->common.tax_total);
  (*surplus)[CM_LUXURY]  = pcity->common.luxury_total;
  (*surplus)[CM_SCIENCE] = pcity->common.science_total;

  *disorder = city_unhappy(pcity);
  *happy = city_happy(pcity);
}

/****************************************************************************
  Compute the fitness of the solution.  This is a fairly expensive operation.
****************************************************************************/
static struct cm_fitness *
evaluate_solution(struct cm_state *cmstate,
                  const struct partial_solution *soln)
{
  city_t *pcity = cmstate->pcity;
  city_t backup;
  int surplus[CM_NUM_STATS];
  bool disorder, happy;
  struct cm_fitness *fitness3 = malloc(sizeof(struct cm_fitness));

  /* make a backup, apply and evaluate the solution, and restore.  This costs
   * one "apply". */
  memcpy(&backup, pcity, sizeof(backup));
  apply_solution(cmstate, soln);
  get_city_surplus(pcity, &surplus, &disorder, &happy);
  memcpy(pcity, &backup, sizeof(backup));
  compute_fitness(surplus, disorder, happy, &cmstate->parameter, fitness3);
  return fitness3;
}

/****************************************************************************
  Convert the solution into a cm_result.  This is a fairly expensive
  operation.
****************************************************************************/
static void convert_solution_to_result(struct cm_state *cmstate,
                                       const struct partial_solution *soln,
                                       struct cm_result *result)
{
  city_t backup;
  struct cm_fitness fitness;

  if (soln->idle != 0) {
    /* If there are unplaced citizens it's not a real solution, so the
     * result is invalid. */
    result->found_a_valid = FALSE;
    return;
  }

  /* make a backup, apply and evaluate the solution, and restore */
  memcpy(&backup, cmstate->pcity, sizeof(backup));
  apply_solution(cmstate, soln);
  cm_copy_result_from_city(cmstate->pcity, result);
  memcpy(cmstate->pcity, &backup, sizeof(backup));

  /* result->found_a_valid should be only true if it matches the
   * parameter ; figure out if it does */
  compute_fitness(result->surplus,
                  result->disorder, result->happy,
		  &cmstate->parameter,
		  &fitness);
  result->found_a_valid = fitness.sufficient;
}

/***************************************************************************
  Compare functions to allow sorting lattice vectors.
 ***************************************************************************/

/****************************************************************************
  All the sorting in this code needs to respect the partial order
  in the lattice: if a is a parent of b, then a must sort before b.
  This routine enforces that relatively cheaply (without looking through
  the worse_types vectors of a and b), but requires that lattice_depth
  has already been computed.
****************************************************************************/
static int compare_tile_type_by_lattice_order(const struct cm_tile_type *a,
                                              const struct cm_tile_type *b)
{
  enum cm_stat stat;

  if (a == b) {
    return 0;
  }

  /* Least depth first */
  if (a->lattice_depth != b->lattice_depth) {
    return a->lattice_depth - b->lattice_depth;
  }

  /* With equal depth, break ties arbitrarily, more production first. */
  for (stat = 0; stat < CM_NUM_STATS; stat++) {
    if (a->production[stat] != b->production[stat]) {
      return b->production[stat] - a->production[stat];
    }
  }

  /* If we get here, then we have two copies of identical types, an error */
  assert(0);
  return 0;
}

/****************************************************************************
  Sort by fitness.  Since fitness is monotone in the production,
  if a has higher fitness than b, then a cannot be a child of b, so
  this respects the partial order -- unless a and b have equal fitness.
  In that case, use compare_tile_type_by_lattice_order.
****************************************************************************/
static int compare_tile_type_by_fitness(const void *va, const void *vb)
{
  struct cm_tile_type * const *a = va;
  struct cm_tile_type * const *b = vb;
  double diff;

  if (*a == *b) {
    return 0;
  }

  /* To avoid double->int roundoff problems, we call a result non-zero only
   * if it's larger than 0.5. */
  diff = (*b)->estimated_fitness - (*a)->estimated_fitness;
  if (diff > 0.5) {
    return 1; /* return value is int; don't round down! */
  }
  if (diff < -0.5) {
    return -1;
  }

  return compare_tile_type_by_lattice_order(*a, *b);
}

static enum cm_stat compare_key;

/****************************************************************************
  Compare by the production of type compare_key.
  If a produces more food than b, then a cannot be a child of b, so
  this respects the partial order -- unless a and b produce equal food.
  In that case, use compare_tile_type_by_lattice_order.
****************************************************************************/
static int compare_tile_type_by_stat(const void *va, const void *vb)
{
  struct cm_tile_type * const *a = va;
  struct cm_tile_type * const *b = vb;

  if (*a == *b) {
    return 0;
  }

  /* most production of what we care about goes first */
  if ((*a)->production[compare_key] != (*b)->production[compare_key]) {
    /* b-a so we sort big numbers first */
    return (*b)->production[compare_key] - (*a)->production[compare_key];
  }

  return compare_tile_type_by_lattice_order(*a, *b);
}

/***************************************************************************
  Compute the tile-type lattice.
 ***************************************************************************/

/****************************************************************************
  Compute the production of tile [x,y] and stuff it into the tile type.
  Doesn't touch the other fields.
****************************************************************************/
static void compute_tile_production(const city_t *pcity, int x, int y,
                                    struct cm_tile_type *out)
{
  bool is_celebrating = base_city_celebrating(pcity);

  out->production[CM_FOOD]
    = base_city_get_food_tile(x, y, pcity, is_celebrating);
  out->production[CM_SHIELD]
    = base_city_get_shields_tile(x, y, pcity, is_celebrating);
  out->production[CM_TRADE]
    = base_city_get_trade_tile(x, y, pcity, is_celebrating);
  out->production[CM_GOLD] = out->production[CM_SCIENCE]
    = out->production[CM_LUXURY] = 0;
}

/****************************************************************************
  Add the tile [x,y], with production indicated by type, to
  the tile-type lattice.  'newtype' can be on the stack.
  x/y are ignored if the type is a specialist.
  If the type is new, it is linked in and the lattice_index set.
  The lattice_depth is not set.
****************************************************************************/
static void cm_tile_type_lattice_add(struct cm_tile_type_vector *lattice,
                                     const struct cm_tile_type *newtype,
                                     int x, int y)
{
  struct cm_tile_type *type;
  int i;

  i = cm_tile_type_vector_find_equivalent(lattice, newtype);
  if(i >= 0) {
    /* We already have this type of tile; use it. */
    type = lattice->p[i];
  } else {
    /* This is a new tile type; add it to the lattice. */
    type = cm_tile_type_dup(newtype);

    /* link up to the types we dominate, and those that dominate us */
    tile_type_vector_iterate(lattice, other) {
      if (cm_tile_type_better(other, type)) {
        cm_tile_type_vector_add(&type->better_types, other);
        cm_tile_type_vector_add(&other->worse_types, type);
      } else if (cm_tile_type_better(type, other)) {
        cm_tile_type_vector_add(&other->better_types, type);
        cm_tile_type_vector_add(&type->worse_types, other);
      }
    } tile_type_vector_iterate_end;

    /* insert into the list */
    type->lattice_index = lattice->size;
    cm_tile_type_vector_add(lattice, type);
  }

  /* Finally, add the tile to the tile type. */
  if (!type->is_specialist) {
    struct cm_tile tile;

    tile.type = type;
    tile.x = x;
    tile.y = y;
    cm_tile_vector_append(&type->tiles, &tile);
  }
}

/*
 * Add the specialist types to the lattice.
 * This structure is necessary for now because each specialist
 * creates only one type of production and we need to map
 * indices from specialist_type to cm_stat.
 */
struct spec_stat_pair {
  enum specialist_type spec;
  enum cm_stat stat;
};
const static struct spec_stat_pair pairs[SP_COUNT] =  {
  { SP_ELVIS, CM_LUXURY },
  { SP_SCIENTIST, CM_SCIENCE },
  { SP_TAXMAN, CM_GOLD }
};

/****************************************************************************
  Create lattice nodes for each type of specialist.  This adds a new
  tile_type for each specialist type.
****************************************************************************/
static void init_specialist_lattice_nodes(struct cm_tile_type_vector *lattice,
                                          const city_t *pcity)
{
  struct cm_tile_type type;

  cm_tile_type_init(&type);
  type.is_specialist = TRUE;

  /* for each specialist type, create a tile_type that has as production
   * the bonus for the specialist (if the city is allowed to use it) */
  specialist_type_iterate(i) {
    if (city_can_use_specialist(pcity, pairs[i].spec)) {
      type.spec = pairs[i].spec;
      type.production[pairs[i].stat]
        = game.ruleset_game.specialist_bonus[pairs[i].spec];

      cm_tile_type_lattice_add(lattice, &type, 0, 0);

      type.production[pairs[i].stat] = 0;
    }
  } specialist_type_iterate_end;
}

/****************************************************************************
  Topologically sort the lattice.
  Sets the lattice_depth field.
  Important assumption in this code: we've computed the transitive
  closure of the lattice. That is, better_types includes all types that
  are better.
****************************************************************************/
static void top_sort_lattice(struct cm_tile_type_vector *lattice)
{
  int i;
  bool marked[lattice->size];
  bool will_mark[lattice->size];
  struct cm_tile_type_vector vectors[2];
  struct cm_tile_type_vector *current, *next;

  memset(marked, 0, sizeof(marked));
  memset(will_mark, 0, sizeof(will_mark));

  cm_tile_type_vector_init(&vectors[0]);
  cm_tile_type_vector_init(&vectors[1]);
  current = &vectors[0];
  next = &vectors[1];

  /* fill up 'next' */
  tile_type_vector_iterate(lattice, ptype) {
    if (cm_tile_type_num_prereqs(ptype) == 0) {
      cm_tile_type_vector_add(next, ptype);
    }
  } tile_type_vector_iterate_end;

  /* while we have nodes to process: mark the nodes whose prereqs have
   * all been visited.  Then, store all the new nodes on the frontier. */
  while (next->size != 0) {
    /* what was the next frontier is now the current frontier */
    struct cm_tile_type_vector *vtmp = current;

    current = next;
    next = vtmp;
    next->size = 0; /* clear out the contents */

    /* look over the current frontier and process everyone */
    tile_type_vector_iterate(current, ptype) {
      /* see if all prereqs were marked.  If so, decide to mark this guy,
         and put all the descendents on 'next'.  */
      bool can_mark = TRUE;
      int sumdepth = 0;

      if (will_mark[ptype->lattice_index]) {
        continue; /* we've already been processed */
      }
      tile_type_vector_iterate(&ptype->better_types, better) {
        if (!marked[better->lattice_index]) {
          can_mark = FALSE;
          break;
        } else {
          sumdepth += cm_tile_type_num_tiles(better);
          if (sumdepth >= WC_INFINITY) {
            /* if this is the case, then something better could
               always be used, and the same holds for our children */
            sumdepth = WC_INFINITY;
            can_mark = TRUE;
            break;
          }
        }
      } tile_type_vector_iterate_end;
      if (can_mark) {
        /* mark and put successors on the next frontier */
        will_mark[ptype->lattice_index] = TRUE;
        tile_type_vector_iterate(&ptype->worse_types, worse) {
          cm_tile_type_vector_add(next, worse);
        } tile_type_vector_iterate_end;

        /* this is what we spent all this time computing. */
        ptype->lattice_depth = sumdepth;
      }
    } tile_type_vector_iterate_end;

    /* now, actually mark everyone and get set for next loop */
    for (i = 0; i < lattice->size; i++) {
      marked[i] = marked[i] || will_mark[i];
      will_mark[i] = FALSE;
    }
  }

  cm_tile_type_vector_free(&vectors[0]);
  cm_tile_type_vector_free(&vectors[1]);
}

/****************************************************************************
  Remove unreachable lattice nodes to speed up processing later.
  This doesn't reduce the number of evaluations we do, it just
  reduces the number of times we loop over and reject tile types
  we can never use.

  A node is unreachable if there are fewer available workers
  than are needed to fill up all predecessors.  A node at depth
  two needs three workers to be reachable, for example (two to fill
  the predecessors, and one for the tile).  We remove a node if
  its depth is equal to the city size, or larger.

  We could clean up the tile arrays in each type (if we have two workers,
  we can use only the first tile of a depth 1 tile type), but that
  wouldn't save us anything later.
****************************************************************************/
static void clean_lattice(struct cm_tile_type_vector *lattice,
                          const city_t *pcity)
{
  int i, j; /* i is the index we read, j is the index we write */
  struct cm_tile_type_vector tofree;
  bool forced_loop = FALSE;

  /* We collect the types we want to remove and free them in one fell
     swoop at the end, in order to avoid memory errors.  */
  cm_tile_type_vector_init(&tofree);

  /* forced_loop is workaround for what seems like gcc optimization
   * bug.
   * This applies to -O2 optimization on some distributions. */
  if (lattice->size > 0) {
    forced_loop = TRUE;
  }
  for (i = 0, j = 0; i < lattice->size || forced_loop; i++) {
    struct cm_tile_type *ptype = lattice->p[i];

    forced_loop = FALSE;

    if (ptype->lattice_depth >= pcity->common.pop_size) {
      cm_tile_type_vector_add(&tofree, ptype);
    } else {
      /* Remove links to children that are being removed. */

      int ci, cj; /* 'c' for 'child'; read from ci, write to cj */

      lattice->p[j] = ptype;
      lattice->p[j]->lattice_index = j;
      j++;

      for (ci = 0, cj = 0; ci < ptype->worse_types.size; ci++) {
        const struct cm_tile_type *ptype2 = ptype->worse_types.p[ci];

        if (ptype2->lattice_depth < pcity->common.pop_size) {
          ptype->worse_types.p[cj] = ptype->worse_types.p[ci];
          cj++;
        }
      }
      ptype->worse_types.size = cj;
    }
  }
  lattice->size = j;

  cm_tile_type_vector_free_all(&tofree);
}

/****************************************************************************
  Determine the estimated_fitness fields, and sort by that.
  estimate_fitness is later, in a section of code that isolates
  much of the domain-specific knowledge.
****************************************************************************/
static double estimate_fitness(const struct cm_state *cmstate,
                               int (*production)[CM_NUM_STATS]);

static void sort_lattice_by_fitness(const struct cm_state *cmstate,
                                    struct cm_tile_type_vector *lattice)
{
  int i;

  /* compute fitness */
  tile_type_vector_iterate(lattice, ptype) {
    ptype->estimated_fitness = estimate_fitness(cmstate, &ptype->production);
  } tile_type_vector_iterate_end;

  /* sort by it */
  qsort(lattice->p, lattice->size, sizeof(*lattice->p),
        compare_tile_type_by_fitness);

  /* fix the lattice indices */
  for (i = 0; i < lattice->size; i++) {
    lattice->p[i]->lattice_index = i;
  }

  freelog(LOG_LATTICE, "sorted lattice:");
  print_lattice(LOG_LATTICE, lattice);
}

/****************************************************************************
  Create the lattice.
****************************************************************************/
static void init_tile_lattice(const city_t *pcity,
                              struct cm_tile_type_vector *lattice)
{
  struct cm_tile_type type;

  /* add all the fields into the lattice */
  cm_tile_type_init(&type); /* init just once */
  my_city_map_iterate(pcity, x, y) {
    if (pcity->common.city_map[x][y] == C_TILE_UNAVAILABLE) {
      continue;
    }
    if (!is_city_center(x, y)) {
      compute_tile_production(pcity, x, y, &type); /* clobbers type */
      cm_tile_type_lattice_add(lattice, &type, x, y); /* copy type if needed */
    }
  } my_city_map_iterate_end;

  /* Add all the specialists into the lattice.  */
  init_specialist_lattice_nodes(lattice, pcity);

  /* Set the lattice_depth fields, and clean up unreachable nodes. */
  top_sort_lattice(lattice);
  clean_lattice(lattice, pcity);

  /* All done now. */
  print_lattice(LOG_LATTICE, lattice);
}


/****************************************************************************

               Handling the choice stack for the bb algorithm.

****************************************************************************/


/****************************************************************************
  Return TRUE iff the stack is empty.
****************************************************************************/
static bool choice_stack_empty(struct cm_state *cmstate)
{
  return cmstate->choice.size == 0;
}

/****************************************************************************
  Return the last choice in the stack.
****************************************************************************/
static int last_choice(struct cm_state *cmstate)
{
  assert(!choice_stack_empty(cmstate));
  return cmstate->choice.stack[cmstate->choice.size - 1];
}

/****************************************************************************
  Return the number of different tile types.  There is one tile type for
  each type specialist, plus one for each distinct (different amounts of
  production) citymap tile.
****************************************************************************/
static int num_types(const struct cm_state *cmstate)
{
  return cm_tile_type_vector_size(&cmstate->lattice);
}

/****************************************************************************
  Update the solution to assign 'number' more workers on to tiles of the
  given type.  'number' may be negative, in which case we're removing
  workers.
  We do lots of sanity checking, since many bugs can get caught here.
****************************************************************************/
static void add_workers(struct partial_solution *soln,
                        int itype, int number,
                        const struct cm_state *cmstate)
{
  enum cm_stat stat;
  const struct cm_tile_type *ptype = cm_tile_type_get(cmstate, itype);
  int newcount;
  int old_worker_count = soln->worker_counts[itype];

  if (number == 0) {
    return;
  }

  /* update the number of idle workers */
  newcount = soln->idle - number;
  assert(newcount >= 0);
  assert(newcount <= cmstate->pcity->common.pop_size);
  soln->idle = newcount;

  /* update the worker counts */
  newcount = soln->worker_counts[itype] + number;
  assert(newcount >= 0);
  assert(newcount <= cm_tile_type_num_tiles(ptype));
  soln->worker_counts[itype] = newcount;

  /* update prereqs array: if we are no longer full but we were,
   * we need to decrement the count, and vice-versa. */
  if (old_worker_count == cm_tile_type_num_tiles(ptype)) {
    assert(number < 0);
    tile_type_vector_iterate(&ptype->worse_types, other) {
      soln->prereqs_filled[other->lattice_index]--;
      assert(soln->prereqs_filled[other->lattice_index] >= 0);
    } tile_type_vector_iterate_end;
  } else if (soln->worker_counts[itype] == cm_tile_type_num_tiles(ptype)) {
    assert(number > 0);
    tile_type_vector_iterate(&ptype->worse_types, other) {
      soln->prereqs_filled[other->lattice_index]++;
      assert(soln->prereqs_filled[other->lattice_index]
          <= cm_tile_type_num_prereqs(other));
    } tile_type_vector_iterate_end;
  }

  /* update production */
  for (stat = 0 ; stat < CM_NUM_STATS; stat++) {
    newcount = soln->production[stat] + number * ptype->production[stat];
    assert(newcount >= 0);
    soln->production[stat] = newcount;
  }
}

/****************************************************************************
  Add just one worker to the solution.
****************************************************************************/
static void add_worker(struct partial_solution *soln,
                       int itype,
                       const struct cm_state *cmstate)
{
  add_workers(soln, itype, 1, cmstate);
}

/****************************************************************************
  Remove just one worker from the solution.
****************************************************************************/
static void remove_worker(struct partial_solution *soln,
                          int itype,
                          const struct cm_state *cmstate)
{
  add_workers(soln, itype, -1, cmstate);
}

/****************************************************************************
  Remove a worker from the current solution, and pop once off the
  choice stack.
****************************************************************************/
static void pop_choice(struct cm_state *cmstate)
{
  assert(!choice_stack_empty(cmstate));
  remove_worker(&cmstate->current, last_choice(cmstate), cmstate);
  cmstate->choice.size--;
}

/****************************************************************************
  True if all tiles better than this type have been used.
****************************************************************************/
static bool prereqs_filled(const struct partial_solution *soln,
                           int type,
                           const struct cm_state *cmstate)
{
  const struct cm_tile_type *ptype = cm_tile_type_get(cmstate, type);
  int prereqs = cm_tile_type_num_prereqs(ptype);

  return soln->prereqs_filled[type] == prereqs;
}

/****************************************************************************
  Return the next choice to make after oldchoice.
  A choice can be made if:
  - we haven't used all the tiles
  - we've used up all the better tiles
  - using that choice, we have a hope of doing better than the best
    solution so far.
  If oldchoice == -1 then we return the first possible choice.
****************************************************************************/
static bool choice_is_promising(struct cm_state *cmstate, int newchoice);

static int next_choice(struct cm_state *cmstate, int oldchoice)
{
  int newchoice;

  for (newchoice = oldchoice + 1;
       newchoice < num_types(cmstate);
       newchoice++)
  {
    const struct cm_tile_type *ptype = cm_tile_type_get(cmstate, newchoice);

    if(!ptype->is_specialist
       && (cmstate->current.worker_counts[newchoice] == cm_tile_vector_size(&ptype->tiles)))
    {
      /* we've already used all these tiles */
      continue;
    }
    if (!prereqs_filled(&cmstate->current, newchoice, cmstate)) {
      /* we could use a strictly better tile instead */
      continue;
    }
    if (!choice_is_promising(cmstate, newchoice)) {
      /* heuristic says we can't beat the best going this way */
      freelog(LOG_PRUNE_BRANCH, "--- pruning branch ---");
      print_partial_solution(LOG_PRUNE_BRANCH, &cmstate->current, cmstate);
      print_tile_type(LOG_PRUNE_BRANCH, tile_type_get(cmstate, newchoice),
                      " + worker on ");
      freelog(LOG_PRUNE_BRANCH, "--- branch pruned ---");
      continue;
    }
    break;
  }

  /* returns num_types if no next choice was available. */
  return newchoice;
}


/****************************************************************************
  Pick a sibling choice to the last choice.  This works down the branch to
  see if a choice that actually looks worse may actually be better.
****************************************************************************/
static bool take_sibling_choice(struct cm_state *cmstate)
{
  int oldchoice = last_choice(cmstate);
  int newchoice;

  /* need to remove first, to run the heuristic */
  remove_worker(&cmstate->current, oldchoice, cmstate);
  newchoice = next_choice(cmstate, oldchoice);

  if (newchoice == num_types(cmstate)) {
    /* add back in so the caller can then remove it again. */
    add_worker(&cmstate->current, oldchoice, cmstate);
    return FALSE;
  } else {
    add_worker(&cmstate->current, newchoice, cmstate);
    cmstate->choice.stack[cmstate->choice.size-1] = newchoice;
    /* choice.size is unchanged */
    return TRUE;
  }
}

/****************************************************************************
  Go down from the current branch, if we can.
  Thanks to the fact that the lattice is sorted by depth, we can keep the
  choice stack sorted -- that is, we can start our next choice as
  last_choice-1.  This keeps us from trying out all permutations of the
  same combination.
****************************************************************************/
static bool take_child_choice(struct cm_state *cmstate)
{
  int oldchoice, newchoice;

  if (cmstate->current.idle == 0) {
    return FALSE;
  }

  if (cmstate->choice.size == 0) {
    oldchoice = 0;
  } else {
    oldchoice = last_choice(cmstate);
  }

  /* oldchoice-1 because we can use oldchoice again */
  newchoice = next_choice(cmstate, oldchoice - 1);

  /* did we fail? */
  if (newchoice == num_types(cmstate)) {
    return FALSE;
  }

  /* now push the new choice on the choice stack */
  add_worker(&cmstate->current, newchoice, cmstate);
  cmstate->choice.stack[cmstate->choice.size] = newchoice;
  cmstate->choice.size++;
  return TRUE;
}


/****************************************************************************
  Complete the solution by choosing tiles in order off the given
  tile lattice.
****************************************************************************/
static void complete_solution(struct partial_solution *soln,
                              const struct cm_state *cmstate,
                              const struct cm_tile_type_vector *lattice)
{
  int last_choice = -1;
  int i;

  if (soln->idle == 0) {
    return;
  }

  /* find the last worker type added (-1 if none) */
  for (i = 0; i < num_types(cmstate); i++) {
    if (soln->worker_counts[i] != 0) {
      last_choice = i;
    }
  }

  /* While there are idle workers, pick up the next-best kind of tile,
   * and assign the workers to the tiles.
   * Respect lexicographic order and prerequisites.  */
  tile_type_vector_iterate(lattice, ptype) {
    int used = soln->worker_counts[ptype->lattice_index];
    int total = cm_tile_type_num_tiles(ptype);
    int touse;

    if (ptype->lattice_index < last_choice) {
      /* lex-order: we can't use ptype (some other branch
         will check this combination, or already did) */
        continue;
    }
    if (!prereqs_filled(soln, ptype->lattice_index, cmstate)) {
      /* don't bother using this tile before all better tiles are used */
      continue;
    }

    touse = total - used;
    if (touse > soln->idle) {
      touse = soln->idle;
    }
    add_workers(soln, ptype->lattice_index, touse, cmstate);

    if (soln->idle == 0) {
      /* nothing left to do here */
      return;
    }
  } tile_type_vector_iterate_end;
}

/****************************************************************************
  The heuristic:
  A partial solution cannot produce more food than the amount of food it
  currently generates plus then placing all its workers on the best food
  tiles.  Similarly with all the other stats.
  If we take the max along each production, and it's not better than the
  best in at least one stat, the partial solution isn't worth anything.

  This function computes the max-stats produced by a partial solution.
****************************************************************************/
static void
compute_max_stats_heuristic(const struct cm_state *cmstate,
                            const struct partial_solution *soln,
                            int (*production)[CM_NUM_STATS],
                            int check_choice)
{
  enum cm_stat stat;
  struct partial_solution solnplus; /* will be soln, plus some tiles */

  /* Production is whatever the solution produces, plus the
     most possible of each kind of production the idle workers could
     produce */

  if (soln->idle == 1) {
    /* Then the total solution is soln + this new worker.  So we know the
       production exactly, and can shortcut the later code. */
    enum cm_stat stat;
    const struct cm_tile_type *ptype = cm_tile_type_get(cmstate, check_choice);

    memcpy(production, &soln->production, sizeof(soln->production));
    for (stat = 0; stat < CM_NUM_STATS; stat++) {
      (*production)[stat] += ptype->production[stat];
    }
    return;
  }

  /* initialize solnplus here, after the shortcut check */
  init_partial_solution(&solnplus, num_types(cmstate), cmstate->pcity->common.pop_size);

  for (stat = 0; stat < CM_NUM_STATS; stat++) {
    /* compute the solution that has soln, then the check_choice,
       then complete it with the best available tiles for the stat. */
    copy_partial_solution(&solnplus, soln, cmstate);
    add_worker(&solnplus, check_choice, cmstate);
    complete_solution(&solnplus, cmstate, &cmstate->lattice_by_prod[stat]);

    (*production)[stat] = solnplus.production[stat];
  }

  /*destroy_partial_solution(&solnplus);*/
  free(solnplus.worker_counts);
  free(solnplus.prereqs_filled);
}

/****************************************************************************
  A choice is unpromising if isn't better than the best in at least
  one way.
  A choice is also unpromising if any of the stats is less than the
  absolute minimum (in practice, this matters a lot more).
****************************************************************************/
static bool choice_is_promising(struct cm_state *cmstate, int newchoice)
{
  int production[CM_NUM_STATS];
  enum cm_stat stat;
  bool beats_best = FALSE;

  compute_max_stats_heuristic(cmstate, &cmstate->current, &production, newchoice);

  for (stat = 0; stat < CM_NUM_STATS; stat++) {
    if (production[stat] < cmstate->min_production[stat]) {
      freelog(LOG_PRUNE_BRANCH, "--- pruning: insufficient %s (%d < %d)",
              cm_get_stat_name(stat), production[stat],
              cmstate->min_production[stat]);
      return FALSE;
    }
    if (production[stat] > cmstate->best.production[stat]) {
      beats_best = TRUE;
      /* may still fail to meet min at another production type, so
       * don't short-circuit */
    }
  }
  if (!beats_best) {
    freelog(LOG_PRUNE_BRANCH, "--- pruning: best is better in all ways");
  }
  return beats_best;
}

/****************************************************************************
  These two functions are very specific for the default civ2-like ruleset.
  These require the parameter to have been set.
  FIXME -- this should be more general.
****************************************************************************/
static void init_min_production(struct cm_state *cmstate)
{
  int x = CITY_MAP_RADIUS;
  int y = CITY_MAP_RADIUS;
  int usage[CM_NUM_STATS];
  city_t *pcity = cmstate->pcity;
  bool is_celebrating = base_city_celebrating(pcity);
  city_t backup;

  /* make sure the city's numbers make sense (sometimes they don't,
   * somehow) */
  memcpy(&backup, pcity, (sizeof(city_t) + 8) & ~7);  //valgrind
  generic_city_refresh(pcity, FALSE, NULL);

  memset(cmstate->min_production, 0, sizeof(cmstate->min_production));

  /* If the city is content, then we know the food usage is just
   * prod-surplus; otherwise, we know it's at least 2*size but we
   * can't easily compute the settlers. */
  if (!city_unhappy(pcity)) {
    usage[CM_FOOD] = pcity->common.food_prod - pcity->common.food_surplus;
  } else {
    usage[CM_FOOD] = pcity->common.pop_size * 2;
  }
  cmstate->min_production[CM_FOOD] = usage[CM_FOOD]
      + cmstate->parameter.minimal_surplus[CM_FOOD]
      - base_city_get_food_tile(x, y, pcity, is_celebrating);

  /* surplus = (factories-waste) * production - shield_usage, so:
   *   production = (surplus + shield_usage)/(factories-waste)
   * waste >= 0, so:
   *   production >= (surplus + usage)/factories
   * Solving with surplus >= min_surplus, we get:
   *   production >= (min_surplus + usage)/factories
   * 'factories' is the pcity->shield_bonus/100.  Increase it a bit to avoid
   * rounding errors.
   *
   * pcity->shield_prod = (factories-waste) * production.
   * Therefore, shield_usage = pcity->shield_prod - pcity->shield_surplus
   */
  if (!city_unhappy(pcity)) {
    double sbonus;

    usage[CM_SHIELD] = pcity->common.shield_prod - pcity->common.shield_surplus;

    sbonus = ((double)pcity->common.shield_bonus) / 100.0;
    sbonus += .1;
    cmstate->min_production[CM_SHIELD] =
        (usage[CM_SHIELD] + cmstate->parameter.minimal_surplus[CM_SHIELD]) / sbonus;
    cmstate->min_production[CM_SHIELD] -=
        base_city_get_shields_tile(x, y, pcity, is_celebrating);
  } else {
    /* Dunno what the usage is, so it's pointless to set the
     * min_production */
    usage[CM_SHIELD] = 0;
    cmstate->min_production[CM_SHIELD] = 0;
  }

  /* we should be able to get a min_production on gold and trade, too;
     also, lux, if require_happy, but not otherwise */

  /* undo any effects from the refresh */
  memcpy(pcity, &backup, sizeof(*pcity));
}

/****************************************************************************
  Estimate the fitness of a tile.  Tiles are put into the lattice in
  fitness order, so that we start off choosing better tiles.
  The estimate MUST be monotone in the inputs; if it isn't, then
  the BB algorithm will fail.

  The only fields of the cmstate used are the city and parameter.
****************************************************************************/
static double estimate_fitness(const struct cm_state *cmstate,
                               int (*production)[CM_NUM_STATS])
{
  const city_t *pcity = cmstate->pcity;
  const struct player *pplayer = get_player(pcity->common.owner);
  enum cm_stat stat;
  double estimates[CM_NUM_STATS];
  double sum = 0;

  for (stat = 0; stat < CM_NUM_STATS; stat++) {
    estimates[stat] = (*production)[stat];
  }

  /* sci/lux/gold get benefit from the tax rates (in percentage) */
  estimates[CM_SCIENCE] += pplayer->economic.science * estimates[CM_TRADE] / 100.0;
  estimates[CM_LUXURY] += pplayer->economic.luxury  * estimates[CM_TRADE] / 100.0;
  estimates[CM_GOLD] += pplayer->economic.tax     * estimates[CM_TRADE] / 100.0;

  /* now add in the bonuses (none for food or trade) (in percentage) */
  estimates[CM_SHIELD] *= pcity->common.shield_bonus / 100.0;
  estimates[CM_LUXURY] *= pcity->common.luxury_bonus / 100.0;
  estimates[CM_GOLD] *= pcity->common.tax_bonus / 100.0;
  estimates[CM_SCIENCE] *= pcity->common.science_bonus / 100.0;

  /* finally, sum it all up, weighted by the parameter, but give additional
   * weight to luxuries to take account of disorder/happy constraints */
  for (stat = 0; stat < CM_NUM_STATS; stat++) {
    sum += estimates[stat] * cmstate->parameter.factor[stat];
  }
  sum += estimates[CM_LUXURY];
  return sum;
}



/****************************************************************************
  The high-level algorithm is:

  for each idle worker,
      non-deterministically choose a position for the idle worker to use

  To implement this, we keep a stack of the choices we've made.
  When we want the next node in the tree, we see if there are any idle
  workers left.  We push an idle worker, and make it take the first field
  in the lattice.  If there are no idle workers left, then we pop out
  until we can make another choice.
****************************************************************************/
static bool bb_next(struct cm_state *cmstate)
{
  /* if no idle workers, then look at our solution. */
  if (cmstate->current.idle == 0) {
    struct cm_fitness *fitness_ = evaluate_solution(cmstate, &cmstate->current);

    print_partial_solution(LOG_REACHED_LEAF, &cmstate->current, cmstate);
    if (fitness_better(fitness_, &cmstate->best_value)) {
      freelog(LOG_BETTER_LEAF, "-> replaces previous best");
      copy_partial_solution(&cmstate->best, &cmstate->current, cmstate);
      cmstate->best_value.weighted = fitness_->weighted;
      cmstate->best_value.sufficient = fitness_->sufficient;
    }
    free(fitness_);
  }

  /* try to move to a child branch, if we can.  If not (including if we're
     at a leaf), then move to a sibling. */
  if (!take_child_choice(cmstate)) {
    /* keep trying to move to a sibling branch, or popping out a level if
       we're stuck (fully examined the current branch) */
    while ((!choice_stack_empty(cmstate)) && !take_sibling_choice(cmstate)) {
      pop_choice(cmstate);
    }

    /* if we popped out all the way, we're done */
    if (choice_stack_empty(cmstate)) {
      return TRUE;
    }
  }

  /* if we didn't detect that we were done, we aren't */
  return FALSE;
}

/****************************************************************************
  Initialize the cmstate for the branch-and-bound algorithm.
****************************************************************************/
struct cm_state *cm_init_cmstate(city_t *pcity)
{
  int numtypes;
  enum cm_stat stat;
  struct cm_state *cmstate = wc_malloc(sizeof(struct cm_state));

  freelog(LOG_CM_STATE, "creating cm_state for %s (size %d)",
          pcity->common.name, pcity->common.pop_size);

  /* copy the arguments */
  cmstate->pcity = pcity;

  /* create the lattice */
  cm_tile_type_vector_init(&cmstate->lattice);
  init_tile_lattice(pcity, &cmstate->lattice);
  numtypes = cm_tile_type_vector_size(&cmstate->lattice);

  /* For the heuristic, make sorted copies of the lattice */
  for (stat = 0; stat < CM_NUM_STATS; stat++) {
    cm_tile_type_vector_init(&cmstate->lattice_by_prod[stat]);
    cm_tile_type_vector_copy(&cmstate->lattice_by_prod[stat], &cmstate->lattice);
    compare_key = stat;
    qsort(cmstate->lattice_by_prod[stat].p, cmstate->lattice_by_prod[stat].size,
          sizeof(*cmstate->lattice_by_prod[stat].p),
          compare_tile_type_by_stat);
  }

  /* We have no best solution yet, so its value is the worst possible. */
  init_partial_solution(&cmstate->best, numtypes, pcity->common.pop_size);
  /*worst_fitness(&cmstate->best_value);*/
  cmstate->best_value.sufficient = FALSE;
  cmstate->best_value.weighted = -WC_INFINITY;

  /* Initialize the current solution and choice stack to empty */
  init_partial_solution(&cmstate->current, numtypes, pcity->common.pop_size);
  cmstate->choice.stack = wc_malloc(pcity->common.pop_size
                                    * sizeof(*cmstate->choice.stack));
  cmstate->choice.size = 0;

  return cmstate;
}

/****************************************************************************
  Set the parameter for the cmstate.  This is the first step in actually
  solving anything.
****************************************************************************/
static void begin_search(struct cm_state *cmstate,
                         const struct cm_parameter *parameter)
{
#ifdef GATHER_TIME_STATS
  start_timer(performance.current->wall_timer);
  performance.current->query_count++;
#endif

  /* copy the parameter and sort the main lattice by it */
  /*cm_copy_parameter(&cmstate->parameter, parameter);*/
  memcpy(&cmstate->parameter, parameter, sizeof(struct cm_parameter));
  sort_lattice_by_fitness(cmstate, &cmstate->lattice);
  init_min_production(cmstate);

  /* clear out the old solution */
  /*worst_fitness(&cmstate->best_value);*/
  cmstate->best_value.sufficient = FALSE;
  cmstate->best_value.weighted = -WC_INFINITY;
  /*destroy_partial_solution(&cmstate->current);*/
  free(cmstate->current.worker_counts);
  free(cmstate->current.prereqs_filled);
  init_partial_solution(&cmstate->current, num_types(cmstate),
                        cmstate->pcity->common.pop_size);
  cmstate->choice.size = 0;
}


#ifdef GATHER_TIME_STATS
/****************************************************************************
  Clean up after a search.
  Currently, does nothing except stop the timer and output.
****************************************************************************/
static void end_search(/*struct cm_state *cmstate*/)
{
  stop_timer(performance.current->wall_timer);

#ifdef PRINT_TIME_STATS_EVERY_QUERY
  print_performance(performance.current);
#endif

  performance.current = NULL;
}
#endif

/****************************************************************************
  Release all the memory allocated for cmstate.
****************************************************************************/
static void cm_free_cmstate(struct cm_state *cmstate)
{
  enum cm_stat cmstat;

  cm_tile_type_vector_free_all(&cmstate->lattice);
  for (cmstat = 0; cmstat < CM_NUM_STATS; cmstat++) {
    cm_tile_type_vector_free(&cmstate->lattice_by_prod[cmstat]);
  }
  /*destroy_partial_solution(&cmstate->best);*/
  free(cmstate->best.worker_counts);
  free(cmstate->best.prereqs_filled);
  /*destroy_partial_solution(&cmstate->current);*/
  free(cmstate->current.worker_counts);
  free(cmstate->current.prereqs_filled);
  free(cmstate->choice.stack);
  free(cmstate);
}


/****************************************************************************
  Run "branch and bound" until we find the best solution.
****************************************************************************/
void cm_find_best_solution(struct cm_state *cmstate,
                           const struct cm_parameter *const parameter,
                           struct cm_result *result)
{
#ifdef GATHER_TIME_STATS
  performance.current = &performance.opt;
#endif

  begin_search(cmstate, parameter);

  /* search until we find a feasible solution */
  while (!bb_next(cmstate)) {
    ; /* nothing */
  }

  /* convert to the caller's format */
  convert_solution_to_result(cmstate, &cmstate->best, result);

#ifdef GATHER_TIME_STATS
  end_search(/*cmstate*/);
#endif
}

/***************************************************************************
  Wrapper that actually runs the branch & bound, and returns the best
  solution.
  call with empty structure for result
 ***************************************************************************/
void cm_query_result(city_t *pcity,
                     const struct cm_parameter *param,
                     struct cm_result *result)
{
  struct cm_state *cmstate;
  cmstate = cm_init_cmstate(pcity);

  /* Refresh the city.  Otherwise the CM can give wrong results or just be
   * slower than necessary.  Note that cities are often passed in in an
   * unrefreshed cmstate (which should probably be fixed). */
  generic_city_refresh(pcity, TRUE, NULL);

  cm_find_best_solution(cmstate, param, result);
  cm_free_cmstate(cmstate);
}


/****************************************************************************
  Return a translated name for the stat type.
*****************************************************************************/
const char *cm_get_stat_name(enum cm_stat stat)
{
  switch (stat) {
  case CM_FOOD:
    return _("Food");
  case CM_SHIELD:
    return _("Shield");
  case CM_TRADE:
    return _("Trade");
  case CM_GOLD:
    return _("Gold");
  case CM_LUXURY:
    return _("Luxury");
  case CM_SCIENCE:
    return _("Science");
  case CM_NUM_STATS:
    break;
  }
  die("Unknown stat value in cm_get_stat_name: %d", stat);
  return NULL;
}

/**************************************************************************
  Returns true if the two cm_parameters are equal.
**************************************************************************/
bool cm_are_parameter_equal(const struct cm_parameter *const p1,
                            const struct cm_parameter *const p2)
{
  int i;

  for (i = 0; i < CM_NUM_STATS; i++) {
    if (p1->minimal_surplus[i] != p2->minimal_surplus[i]) {
      return FALSE;
    }
    if (p1->factor[i] != p2->factor[i]) {
      return FALSE;
    }
  }
  if (p1->require_happy != p2->require_happy) {
    return FALSE;
  }
  if (p1->allow_disorder != p2->allow_disorder) {
    return FALSE;
  }
  if (p1->allow_specialists != p2->allow_specialists) {
    return FALSE;
  }
  if (p1->happy_factor != p2->happy_factor) {
    return FALSE;
  }

  return TRUE;
}

#if 0
/**************************************************************************
  Copy the parameter from the source to the destination field.
**************************************************************************/
void cm_copy_parameter(struct cm_parameter *dest,
                       const struct cm_parameter *const src)
{
  memcpy(dest, src, sizeof(struct cm_parameter));
}
#endif

/**************************************************************************
  Initialize the parameter to sane default values.
**************************************************************************/
void cm_init_parameter(struct cm_parameter *dest)
{
  enum cm_stat stat;

  for (stat = 0; stat < CM_NUM_STATS; stat++) {
    dest->minimal_surplus[stat] = 0;
    dest->factor[stat] = 1;
  }

  dest->happy_factor = 1;
  dest->require_happy = FALSE;
  dest->allow_disorder = FALSE;
  dest->allow_specialists = TRUE;
}

/***************************************************************************
  Initialize the parameter to sane default values that will always produce
  a result.
***************************************************************************/
void cm_init_emergency_parameter(struct cm_parameter *dest)
{
  enum cm_stat stat;

  for (stat = 0; stat < CM_NUM_STATS; stat++) {
    dest->minimal_surplus[stat] = -WC_INFINITY;
    dest->factor[stat] = 1;
  }

  dest->happy_factor = 1;
  dest->require_happy = FALSE;
  dest->allow_disorder = TRUE;
  dest->allow_specialists = TRUE;
}

/****************************************************************************
  cm_result routines.
****************************************************************************/
int cm_count_worker(const city_t * pcity,
                    const struct cm_result *result)
{
  int count = 0;

  city_map_iterate(x, y) {
    if(result->worker_positions_used[x][y] && !is_city_center(x, y)) {
      count++;
    }
  } city_map_iterate_end;
  return count;
}

/****************************************************************************
  Count the total number of specialists in the result.
****************************************************************************/
int cm_count_specialist(const city_t *pcity,
                        const struct cm_result *result)
{
  int count = 0;

  specialist_type_iterate(spec) {
    count += result->specialists[spec];
  } specialist_type_iterate_end;

  return count;
}

/****************************************************************************
  Copy the city's current setup into the cm result structure.
****************************************************************************/
void cm_copy_result_from_city(const city_t *pcity,
                              struct cm_result *result)
{
  /* copy the map of where workers are */
  city_map_iterate(x, y) {
    result->worker_positions_used[x][y] =
        (pcity->common.city_map[x][y] == C_TILE_WORKER);
  } city_map_iterate_end;

  /* copy the specialist counts */
  specialist_type_iterate(spec) {
    result->specialists[spec] = pcity->common.specialists[spec];
  } specialist_type_iterate_end;

  /* find the surplus production numbers */
  get_city_surplus(pcity, &result->surplus,
                   &result->disorder, &result->happy);

  /* this is a valid result, in a sense */
  result->found_a_valid = TRUE;
}

/****************************************************************************
  Debugging routines.
****************************************************************************/
#ifdef CM_DEBUG
static void snprint_production(char *buffer,
                               size_t bufsz,
                               const int (production*)[CM_NUM_STATS])
{
  int nout;

  nout = my_snprintf(buffer, bufsz, "[%d %d %d %d %d %d]",
                     (production*)[CM_FOOD], (production*)[CM_SHIELD],
                     (production*)[CM_TRADE], (production*)[CM_GOLD],
                     (production*)[CM_LUXURY], (production*)[CM_SCIENCE]);

  assert(nout >= 0 && nout <= bufsz);
}

/****************************************************************************
  Print debugging data about a particular tile type.
****************************************************************************/
static void print_tile_type(int loglevel,
                            const struct cm_tile_type *ptype,
                            const char *prefix)
{
  char prodstr[256];

  snprint_production(prodstr, sizeof(prodstr), &ptype->production);
  freelog(loglevel, "%s%s fitness %g depth %d, idx %d; %d tiles",
          prefix,
          prodstr, ptype->estimated_fitness,
          ptype->lattice_depth,
          ptype->lattice_index,
          cm_tile_type_num_tiles(ptype));
}

/****************************************************************************
  Print debugging data about a whole B&B lattice.
****************************************************************************/
static void print_lattice(int loglevel,
                          const struct tile_type_vector *lattice)
{
  freelog(loglevel, "lattice has %u terrain types", (unsigned)lattice->size);
  tile_type_vector_iterate(lattice, ptype) {
    print_tile_type(loglevel, ptype, "  ");
  } tile_type_vector_iterate_end;
}

/****************************************************************************
  Print debugging data about a partial CM solution.
****************************************************************************/
static void print_partial_solution(int loglevel,
                                   const struct partial_solution *soln,
                                   const struct cm_state *cmstate)
{
  int i;
  int last_type = 0;
  char buf[256];

  if(soln->idle != 0) {
    freelog(loglevel, "** partial solution has %d idle workers", soln->idle);
  } else {
    freelog(loglevel, "** completed solution:");
  }

  snprint_production(buf, sizeof(buf), &soln->production);
  freelog(loglevel, "production: %s", buf);

  freelog(loglevel, "tiles used:");
  for (i = 0; i < num_types(cmstate); i++) {
    if (soln->worker_counts[i] != 0) {
      my_snprintf(buf, sizeof(buf),
                  "  %d tiles of type ", soln->worker_counts[i]);
      print_tile_type(loglevel, tile_type_get(cmstate, i), buf);
    }
  }

  for (i = 0; i < num_types(cmstate); i++) {
    if (soln->worker_counts[i] != 0) {
      last_type = i;
    }
  }

  freelog(loglevel, "tiles available:");
  for (i = last_type; i < num_types(cmstate); i++) {
    const struct cm_tile_type *ptype = tile_type_get(cmstate, i);

    if (soln->prereqs_filled[i] == cm_tile_type_num_prereqs(ptype)
        && soln->worker_counts[i] < cm_tile_type_num_tiles(ptype)) {
      print_tile_type(loglevel, tile_type_get(cmstate, i), "  ");
    }
  }
}

#endif /* CM_DEBUG */

#ifdef GATHER_TIME_STATS
/****************************************************************************
  Print debugging performance data.
****************************************************************************/
static void print_performance(struct one_perf *counts)
{
  double s, ms;
  double q;
  int queries, applies;

  s = read_timer_seconds(counts->wall_timer);
  ms = 1000.0 * s;

  queries = counts->query_count;
  q = queries;

  applies = counts->apply_count;

  freelog(LOG_TIME_STATS,
      "CM-%s: overall=%fs queries=%d %fms / query, %d applies",
      counts->name, s, queries, ms / q, applies);
}
#endif

/****************************************************************************
  Print debugging inormation about one city.
****************************************************************************/
void cm_print_city(const city_t *pcity)
{
  freelog(LOG_NORMAL, "print_city(city='%s'(id=%d))",
          pcity->common.name, pcity->common.id);
  freelog(LOG_NORMAL,
          "  size=%d, entertainers=%d, scientists=%d, taxmen=%d",
          pcity->common.pop_size, pcity->common.specialists[SP_ELVIS],
          pcity->common.specialists[SP_SCIENTIST],
          pcity->common.specialists[SP_TAXMAN]);
  freelog(LOG_NORMAL, "  workers at:");
  my_city_map_iterate(pcity, x, y) {
    if (pcity->common.city_map[x][y] == C_TILE_WORKER) {
      freelog(LOG_NORMAL, "    (%2d,%2d)", x, y);
    }
  } my_city_map_iterate_end;

  freelog(LOG_NORMAL, "  food    = %3d (%+3d)",
          pcity->common.food_prod, pcity->common.food_surplus);
  freelog(LOG_NORMAL, "  shield  = %3d (%+3d)",
          pcity->common.shield_prod, pcity->common.shield_surplus);
  freelog(LOG_NORMAL, "  trade   = %3d", pcity->common.trade_prod);

  freelog(LOG_NORMAL, "  gold    = %3d (%+3d)", pcity->common.tax_total,
          city_gold_surplus(pcity, pcity->common.tax_total));
  freelog(LOG_NORMAL, "  luxury  = %3d", pcity->common.luxury_total);
  freelog(LOG_NORMAL, "  science = %3d", pcity->common.science_total);
}


/****************************************************************************
  Print debugging inormation about a full CM result.
****************************************************************************/
void cm_print_result(const city_t *pcity,
                     const struct cm_result *result)
{
  int y;
  int i;
  int worker = cm_count_worker(pcity, result);
  freelog(LOG_NORMAL, "print_result(result=%p)", result);
  freelog(LOG_NORMAL,
      "print_result:  found_a_valid=%d disorder=%d happy=%d",
      result->found_a_valid, result->disorder, result->happy);

  freelog(LOG_NORMAL, "print_result:  workers at:");
  my_city_map_iterate(pcity, x, y) {
    if (result->worker_positions_used[x][y]) {
      freelog(LOG_NORMAL, "print_result:    (%2d,%2d)", x, y);
    }
  } my_city_map_iterate_end;

  for (y = 0; y < CITY_MAP_SIZE; y++) {
    char line[CITY_MAP_SIZE + 1];
    int x;

    line[CITY_MAP_SIZE] = 0;

    for (x = 0; x < CITY_MAP_SIZE; x++) {
      if (!is_valid_city_coords(x, y)) {
        line[x] = '-';
      } else if (is_city_center(x, y)) {
        line[x] = 'c';
      } else if (result->worker_positions_used[x][y]) {
        line[x] = 'w';
      } else {
        line[x] = '.';
      }
    }
    freelog(LOG_NORMAL, "print_result: %s", line);
  }
  freelog(LOG_NORMAL,
      "print_result:  people: (workers/specialists) %d/%s",
      worker, specialists_string(result->specialists));

  for (i = 0; i < CM_NUM_STATS; i++) {
    freelog(LOG_NORMAL, "print_result:  %10s surplus=%d",
        cm_get_stat_name(i), result->surplus[i]);
  }
}

