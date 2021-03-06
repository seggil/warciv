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
#include <time.h>

#ifdef HAVE_UCONTEXT_H
#  include <ucontext.h>
#else
#  ifdef WIN32_NATIVE
#    include "win32-ucontext.hh"
#  endif /* WIN32_NATIVE */
#endif /* HAVE_UCONTEXT_H */

#include "wc_intl.hh"
#include "log.hh"
#include "mem.hh"
#include "support.hh"

#include "city.hh"
#include "game.hh"
#include "aicore/pf_tools.hh"

#include "traderoute.hh"

/**************************************************************************
  Create a default parameter for caravans.
  return: PathFinding_parameter
  Hack: Create a virtual unit to obtain it. It should support
        if the game changes (no unit_type or player_type kept).
**************************************************************************/
static struct pf_parameter *get_caravan_parameter(player_t *pplayer,
                                                  tile_t *ptile)
{
  static struct pf_parameter parameter;
  static struct pf_parameter *pparameter = NULL;

  if (pparameter) {
    /* Note that pparameter can be NULL */
    parameter.start_tile = ptile;
    parameter.owner = pplayer;

    return pparameter;
  }

  Unit_Type_id utype = -1;
  unit_t *punit;

  /* Find a type */
  unit_type_iterate(ttype) {
    if (unit_type_flag(ttype, F_TRADE_ROUTE)) {
      utype = ttype;
      break;
    } else if (utype == -1 && get_unit_type(ttype)->move_type == LAND_MOVING) {
      /* By default */
      utype = ttype;
    }
  } unit_type_iterate_end

  if (utype == -1) {
    /* There isn't any unit able to trade! */
    freelog(LOG_ERROR, "There is no unit able to trade in this game!");
    return NULL;
  }

  /* Fill parameter */
  punit = create_unit_virtual(pplayer, NULL, utype, 0);
  punit->tile = ptile;
  punit->moves_left = 0;
  pparameter = &parameter;
  pft_fill_unit_parameter(pparameter, punit);
  destroy_unit_virtual(punit);

  return pparameter;
}

/**************************************************************************
  Calculate the default move cost between pcity1 to pcity2.
**************************************************************************/
static int calculate_default_move_cost(city_t *pcity1, city_t *pcity2)
{
  struct pf_parameter *pparameter = get_caravan_parameter(city_owner(pcity1),
                                                          pcity1->common.tile);
  struct path_finding_map *pmap = pf_create_map(pparameter);
  struct pf_position position;

  if (!pf_get_position(pmap, pcity2->common.tile, &position)) {
    pf_destroy_map(pmap);
    return WC_INFINITY;
  }

  pf_destroy_map(pmap);
  return position.total_MC;
}

/**************************************************************************
  Calculate the move cost for the unit for going from to ptile1 to ptile2.
**************************************************************************/
static int base_calculate_move_cost(unit_t *punit,
                                    tile_t *ptile1,
                                    tile_t *ptile2)
{
  struct pf_parameter parameter;
  struct path_finding_map *pmap;
  struct pf_position position;

  pft_fill_unit_parameter(&parameter, punit);
  parameter.start_tile = ptile1;
  pmap = pf_create_map(&parameter);

  if (!pf_get_position(pmap, ptile2, &position)) {
    pf_destroy_map(pmap);
    return WC_INFINITY;
  }

  pf_destroy_map(pmap);
  return position.total_MC;
}

/**************************************************************************
  Calculate trade move cost. For a unit between 2 cities.
**************************************************************************/
static int base_calculate_trade_move_cost(unit_t *punit,
                                          city_t *pcity1,
                                          city_t *pcity2)
{
  if (punit->homecity == pcity1->common.id) {
    return base_calculate_move_cost(punit, punit->tile, pcity2->common.tile);
  } else if (punit->owner != pcity1->common.owner) {
    return WC_INFINITY;
  } else {
    return base_calculate_move_cost(punit, punit->tile, pcity1->common.tile)
           + base_calculate_move_cost(punit, pcity1->common.tile, pcity2->common.tile);
  }
}

/****************************************************************************
  ...
****************************************************************************/
static struct trade_route *trade_route_new(city_t *pcity1,
                                           city_t *pcity2,
                                           unit_t *punit,
                                           enum trade_route_status status)
{
  struct trade_route *p_tr = static_cast<trade_route*>(wc_malloc(sizeof(struct trade_route)));

  p_tr->pcity1 = pcity1;
  p_tr->pcity2 = pcity2;
  p_tr->punit = punit;
  p_tr->status = status;

  p_tr->value = -1;
  p_tr->move_cost = -1;
  p_tr->move_turns = -1;

  return p_tr;
}

/****************************************************************************
  Make a new trade route, attached to all structures.
  Returns NULL if there is an error.
****************************************************************************/
struct trade_route *game_trade_route_add(city_t *pcity1,
                                         city_t *pcity2)
{
  if (! pcity1 || ! pcity2) {
    return NULL;
  }

  if (game_trade_route_find(pcity1, pcity2)) {
    freelog(LOG_ERROR,
            "The trade route between %s and %s was already existant",
            pcity1->common.name,
            pcity2->common.name);
    return NULL;
  }

  struct trade_route *p_tr = trade_route_new(pcity1, pcity2, NULL, TR_NONE);

  p_tr->value = trade_between_cities(pcity1, pcity2);
  trade_route_list_append(pcity1->common.trade_routes, p_tr);
  trade_route_list_append(pcity2->common.trade_routes, p_tr);

  return p_tr;
}

/****************************************************************************
  Remove a trade route, detaching from all structures.
****************************************************************************/
void game_trade_route_remove(struct trade_route *p_tr)
{
  if (!p_tr) {
    return;
  }

  assert(p_tr && p_tr->pcity1 && p_tr->pcity2);
  trade_route_list_unlink(p_tr->pcity1->common.trade_routes, p_tr);
  trade_route_list_unlink(p_tr->pcity2->common.trade_routes, p_tr);
  free(p_tr);
}

/****************************************************************************
  Check if a trade route exist between 2 cities.
****************************************************************************/
struct trade_route *game_trade_route_find(const city_t *pcity1,
                                          const city_t *pcity2)
{
  if (!pcity1 || !pcity2) {
    return NULL;
  }

  bool same_owner = pcity1->common.owner == pcity2->common.owner;

  trade_route_list_iterate(pcity1->common.trade_routes, p_tr) {
    if (p_tr->pcity2 == pcity2
        || ((same_owner
             || p_tr->status == TR_ESTABLISHED)
            && p_tr->pcity1 == pcity2))
    {
      return p_tr;
    }
  } trade_route_list_iterate_end;

  return NULL;
}

/****************************************************************************
  Calculate trade move cost. Can swap pcity1 and pcity2, if it's faster.
****************************************************************************/
int calculate_trade_move_cost(struct trade_route *p_tr)
{
  if (!p_tr->punit) {
    return calculate_default_move_cost(p_tr->pcity1, p_tr->pcity2);
  }

  if (p_tr->pcity1->common.owner != p_tr->pcity2->common.owner) {
    return base_calculate_trade_move_cost(p_tr->punit, p_tr->pcity1, p_tr->pcity2);
  }

  int mtc1, mtc2;

  mtc1 = base_calculate_trade_move_cost(p_tr->punit, p_tr->pcity1, p_tr->pcity2);
  mtc2 = base_calculate_trade_move_cost(p_tr->punit, p_tr->pcity2, p_tr->pcity1);

  if (mtc2 >= mtc1) {
    return mtc1;
  }

  /* Swap cities */
  city_t *pcity0 = p_tr->pcity1;
  p_tr->pcity1 = p_tr->pcity2;
  p_tr->pcity2 = pcity0;

  return mtc2;
}

/****************************************************************************
  Calculate trade move cost. Can swap pcity1 and pcity2, if it's faster.
****************************************************************************/
int calculate_trade_move_turns(struct trade_route *p_tr)
{
  p_tr->move_cost = calculate_trade_move_cost(p_tr);
  p_tr->move_turns = COST_TO_TURNS(p_tr);
  return p_tr->move_turns;
}

/****************************************************************************
  Return TRUE, if a path is found, else FALSE.
****************************************************************************/
static bool add_move_orders(unit_t *punit, struct unit_order *porders,
                            int *length, int max_length,
                            tile_t *otile, tile_t *dtile)
{
  struct pf_parameter parameter;
  struct path_finding_map *pmap;
  struct pf_path *ppath;
  int i;

  pft_fill_unit_parameter(&parameter, punit);
  parameter.start_tile = otile;
  pmap = pf_create_map(&parameter);
  ppath = pf_get_path(pmap, dtile);
  pf_destroy_map(pmap);

  /* Keep a free order to add at the end of the orders list */
  if (!ppath || ppath->length + *length >= max_length) {
    return FALSE;
  }

  for (i = 1; i < ppath->length; i++) {
    if (ppath->positions[i].tile == ppath->positions[i - 1].tile) {
      /* Shouldn't occur often! This is for air caravans :) */
      porders[*length].order = ORDER_FULL_MP;
      porders[*length].dir = static_cast<direction8>(-1);
    } else {
      porders[*length].order = ORDER_MOVE;
      porders[*length].dir = static_cast<direction8>(
          get_direction_for_step(ppath->positions[i - 1].tile,
                                 ppath->positions[i].tile));
    }
    porders[*length].activity = ACTIVITY_LAST;
    (*length)++;
  }
  pf_destroy_path(ppath);

  return TRUE;
}

/****************************************************************************
  Return some unit orders
****************************************************************************/
struct unit_order *make_unit_orders(struct trade_route *p_tr, int *length)
{
  if (!p_tr || !p_tr->punit) {
    return NULL;
  }

  struct unit_order orders[MAX_LEN_ROUTE], *porders;
  tile_t *ptile = p_tr->punit->tile;
  int i;

  *length = 0;
  calculate_trade_move_cost(p_tr); /* Maybe swap city1 and city2,
                                    * if it's faster */

  if (p_tr->punit->homecity != p_tr->pcity1->common.id) {
    /* Goto city1 first */
    if (!add_move_orders(p_tr->punit, orders, length, MAX_LEN_ROUTE,
                         p_tr->punit->tile, p_tr->pcity1->common.tile)
        || *length >= MAX_LEN_ROUTE) {
      /* Too long path */
      return NULL;
    }
    /* N.B.: ORDER_HOMECITY order not possible in 2.0 :( */
    ptile = p_tr->pcity1->common.tile;
  }

  if (!add_move_orders(p_tr->punit, orders, length, MAX_LEN_ROUTE,
                       ptile, p_tr->pcity2->common.tile)) {
    /* Too long path */
    return NULL;
  }

  /* Make a dynamic structure to return */
  porders = static_cast<unit_order*>(wc_malloc(*length * sizeof(struct unit_order)));
  for (i = 0; i < *length; i++) {
    porders[i] = orders[i];
  }
  /* N.B.: ORDER_TRADEROUTE order not possible in 2.0 :( */

  return porders;
}

/****************************************************************************
  Trade planning structures
****************************************************************************/
#ifdef ASYNC_TRADE_PLANNING
enum trade_plan_state {
  TPC_RUNNING,
  TPC_INTERRUPTED
};
#endif  /* ASYNC_TRADE_PLANNING */

struct trade_city {
  city_t *pcity;                   /* The pointed city */
  int free_slots;                       /* Number of free slots */
  int trade_routes_num;                 /* Number of city it can trade with */
  struct trade_route **trade_routes;    /* A list of possible trade routes */
  int *distance;
};

struct trade_configuration {
  int trade_routes_num; /* The total number of trade routes found */
  int free_slots;       /* The total number of free slots */
  int turns;            /* The total number of required turns for a caravan */
  int moves;            /* The total number of required moves for a caravan */
};

struct trade_planning_calculation {
  struct trade_city *tcities;           /* The cities data */
  size_t size;                          /* The number of the cities data */
  struct trade_route **ctlist;          /* The current trade routes */
  struct trade_configuration ctconf;    /* The current configuration */
  struct trade_route **btlist;          /* The best trade routes */
  struct trade_configuration btconf;    /* The best configuration */
#ifdef ASYNC_TRADE_PLANNING
  enum trade_plan_state state;          /* The state of the calculation */
  struct ucontext start_point;          /* The launch point of the program */
  struct ucontext interrupted_point;    /* The point it has been interrupted */
  int operations_counter;               /* The number of operations */
#else
  time_t max_time;                      /* The maximum time to calculate */
#endif  /* ASYNC_TRADE_PLANNING */
  bool destroying;                      /* TRUE if going to be destroyed */
  /* Callbacks */
  void (*destroy_callback)(const struct trade_planning_calculation *, void *);
  void (*apply_callback)(const struct trade_planning_calculation *, void *);
  void *data;                           /* User data */
};

#define SPECLIST_TAG tp_calc
#define SPECLIST_TYPE struct trade_planning_calculation
#include "speclist.hh"

/* The list of all trade planning calculations */
static struct tp_calc_list *tp_calculations = NULL;

#define tp_calc_list_iterate(pcalc)                             \
  if (tp_calculations) {                                        \
    TYPED_LIST_ITERATE(struct trade_planning_calculation,       \
                       tp_calculations, pcalc)
#define tp_calc_list_iterate_end LIST_ITERATE_END }

/* The calculation is will interputed every time that the CPU
 * will make MAX_OPERATIONS operations */
#define MAX_OPERATIONS  1000000L

void recursive_calculate_trade_planning(
    struct trade_planning_calculation *pcalc,
    int start_city,
    int start_trade);

/****************************************************************************
  Count the trade routes, but ignore the planned ones.
****************************************************************************/
static int get_real_trade_route_number(city_t *pcity)
{
  if (!pcity) {
    return 0;
  }

  int count = 0;

  trade_route_list_iterate(pcity->common.trade_routes, p_tr) {
    if (p_tr->status > TR_PLANNED) {
      count++;
    }
  } trade_route_list_iterate_end;

  return count;
}

/****************************************************************************
  Have cities trade route, or will have soon (& TR_IN_ROUTE).
****************************************************************************/
static bool cities_will_have_trade(city_t *pcity1, city_t *pcity2)
{
  struct trade_route *p_tr = game_trade_route_find(pcity1, pcity2);
  return p_tr && p_tr->status > TR_PLANNED ? TRUE : FALSE;
}

/****************************************************************************
  Precalculation function.

  ptlist: The source tiles.
  size: the number of elements of the integer array (should be
        equal to tile_list_size(ptlist)).
  free_slots: an editable integer array. This will set how many free slots there
              is for each tile index.

  Returns the total number of free slots or -1 on failure.
****************************************************************************/
int trade_planning_precalculation(const struct tile_list *ptlist,
                                  size_t size, int *free_slots)
{
  struct precalc_city {
    tile_t *ptile;
    city_t *pcity;
    int free_slots;
    int trade_routes_num;
    bool *trade_routes;
  } cities[size];
  struct precalc_city *pc1, *pc2;
  unsigned int i, j;
  int total = 0;

  if (game.traderoute_info.maxtraderoutes == 0) {
    return 0;
  }

  assert((int)size >= tile_list_size(ptlist));

  /* Initialize */
  i = 0;
  pc1 = cities;
  tile_list_iterate(ptlist, ptile) {
    pc1->ptile = ptile;
    pc1->pcity = map_get_city(ptile);
    pc1->free_slots = game.traderoute_info.maxtraderoutes
      - get_real_trade_route_number(pc1->pcity);
    pc1->trade_routes_num = 0;
    pc1->trade_routes = static_cast<bool*>(wc_malloc(size * sizeof(bool)));
    memset(pc1->trade_routes, FALSE, size * sizeof(bool));

    /* Check for earlier cities in the list */
    for (j = 0, pc2 = cities; j < i; j++, pc2++) {
      if (pc1->free_slots > 0 && pc2->free_slots > 0) {
        if (pc1->pcity && pc2->pcity) {
          if (can_cities_trade(pc1->pcity, pc2->pcity)
              && !cities_will_have_trade(pc1->pcity, pc2->pcity)) {
            pc1->trade_routes[j] = TRUE;
            pc1->trade_routes_num++;
            pc2->trade_routes[i] = TRUE;
            pc2->trade_routes_num++;
          }
        } else {
          if (map_distance(pc1->ptile, pc2->ptile)
              >= game.traderoute_info.trademindist) {
            pc1->trade_routes[j] = TRUE;
            pc1->trade_routes_num++;
            pc2->trade_routes[i] = TRUE;
            pc2->trade_routes_num++;
          }
        }
      }
    }

    i++;
    pc1++;
  } tile_list_iterate_end;

  /* Look for free slots */
  do {
    pc2 = NULL;
    j = -1;
    /* Find the hardest */
    for (i = 0, pc1 = cities; i < size; i++, pc1++) {
      if (pc1->trade_routes_num > 0
          && pc1->trade_routes_num <= pc1->free_slots) {
        if (!pc2
            || pc1->free_slots - pc1->trade_routes_num
               > pc2->free_slots - pc2->trade_routes_num) {
          j = i;
          pc2 = pc1;
        }
      }
    }
    if (pc2 && j >= 0) {
      /* Give it all its trade routes */
      for (i = 0, pc1 = cities; i < size; i++, pc1++) {
        if (pc2->trade_routes[i] && pc1->free_slots > 0) {
          pc1->free_slots--;
          pc1->trade_routes_num--;
          pc1->trade_routes[j] = FALSE;
          pc2->free_slots--;
          pc2->trade_routes_num--;
          pc2->trade_routes[i] = FALSE;

          /*
           * If the city doesn't have free slots anymore,
           * clear its possible trade routes
           */
          if (pc1->free_slots == 0) {
            struct precalc_city *pc3;
            unsigned int k;

            for (k = 0, pc3 = cities; k < size; k++, pc3++) {
              if (pc1->trade_routes[k]) {
                pc1->trade_routes[k] = FALSE;
                pc1->trade_routes_num--;
                pc3->trade_routes[i] = FALSE;
                pc3->trade_routes_num--;
              }
            }
            assert(pc1->trade_routes_num == 0);
          }
        }
      }
      assert(pc2->trade_routes_num == 0);
    }
  } while (pc2 && j >= 0);

  /* Get and free datas */
  for (i = 0, pc1 = cities; i < size; i++, pc1++) {
    if (pc1->free_slots > pc1->trade_routes_num) {
      free_slots[i] = pc1->free_slots - pc1->trade_routes_num;
      total += free_slots[i];
    } else {
      free_slots[i] = 0;
    }
    free(pc1->trade_routes);
  }

  return total;
}

/****************************************************************************
  Add a trade route in the trade planning.
  Returns the added trade route.
****************************************************************************/
static struct trade_route *add_trade_route_in_planning(
    struct trade_planning_calculation *pcalc, int c1, int c2)
{
  struct trade_route *p_tr;
  struct trade_city *tcity1 = &pcalc->tcities[c1];
  struct trade_city *tcity2 = &pcalc->tcities[c2];
  struct trade_configuration *pconf = &pcalc->ctconf;

  p_tr = tcity1->trade_routes[c2];
  if (!p_tr || p_tr != tcity2->trade_routes[c1]) {
    freelog(LOG_ERROR,
            "Trying to add a wrong trade route in the trade planning (%s - %s)",
            tcity1->pcity->common.name, tcity2->pcity->common.name);
    return NULL;
  }

  /* Change cities datas */
  tcity1->free_slots--;
  tcity1->trade_routes[c2] = NULL;
  tcity1->trade_routes_num--;
  tcity2->free_slots--;
  tcity2->trade_routes[c1] = NULL;
  tcity2->trade_routes_num--;

  /* Change configuration */
  pcalc->ctlist[pconf->trade_routes_num++] = p_tr;
  pconf->free_slots -= 2;
  if (p_tr->move_cost < WC_INFINITY) {
    pconf->moves += p_tr->move_cost;
    pconf->turns += p_tr->move_turns;
  } else {
    pconf->turns += pcalc->tcities[c1].distance[c2];
    pconf->moves += pcalc->tcities[c1].distance[c2] * SINGLE_MOVE;
  }

  return p_tr;
}

/****************************************************************************
  Calculate the trade planning...

  pcalc:       the calculation data cache.
  start_city:  begin the calculation by the city with this index.
  start_trade: begin the calculation by this city.
****************************************************************************/
void recursive_calculate_trade_planning(
    struct trade_planning_calculation *pcalc, int start_city, int start_trade)
{
  unsigned int i = 0;
  unsigned int j = 0;
  struct trade_city *tcity1, *tcity2;

  for (i = start_city; i < pcalc->size; i++) {
    tcity1 = &pcalc->tcities[i];

#ifdef ASYNC_TRADE_PLANNING
    if (++pcalc->operations_counter >= MAX_OPERATIONS) {
      pcalc->state = TPC_INTERRUPTED;
      swapcontext(&pcalc->interrupted_point, &pcalc->start_point);
    }
#else
    if (pcalc->max_time && time(NULL) > pcalc->max_time) {
      break;
    }
#endif  /* ASYNC_TRADE_PLANNING */

    if (tcity1->free_slots <= 0 || tcity1->trade_routes_num <= 0) {
      continue;
    }

    for (j = i == (unsigned int)start_city ? start_trade : i + 1;
         j < pcalc->size; j++)
    {
      tcity2 = &pcalc->tcities[j];

#ifdef ASYNC_TRADE_PLANNING
      if (++pcalc->operations_counter >= MAX_OPERATIONS) {
        pcalc->state = TPC_INTERRUPTED;
        swapcontext(&pcalc->interrupted_point, &pcalc->start_point);
      }
#else
      if (pcalc->max_time && time(NULL) > pcalc->max_time) {
        break;
      }
#endif  /* ASYNC_TRADE_PLANNING */

      if (tcity2->free_slots <= 0 || !tcity1->trade_routes[j]) {
        continue;
      }

      /* Plan this trade route */
      struct trade_configuration conf;
      struct trade_route *p_tr;

      conf = pcalc->ctconf;
      p_tr = add_trade_route_in_planning(pcalc, i, j);

      /* Do recursive calculation */
      recursive_calculate_trade_planning(pcalc, i, j + 1);

      /* Reverse */
      pcalc->ctconf = conf;
      tcity1->free_slots++;
      tcity1->trade_routes[j] = p_tr;
      tcity1->trade_routes_num++;
      tcity2->free_slots++;
      tcity2->trade_routes[i] = p_tr;
      tcity2->trade_routes_num++;
    }

    if (pcalc->btconf.free_slots <= 0) {
      /* If we continue, we will keep free slots, so it's not necessary. */
      break;
    }
  }

  /* Is the best configuration? */
  if (pcalc->ctconf.free_slots < pcalc->btconf.free_slots
      || (pcalc->ctconf.free_slots == pcalc->btconf.free_slots
          && (pcalc->ctconf.turns < pcalc->btconf.turns
              || (pcalc->ctconf.turns == pcalc->btconf.turns
                  && pcalc->ctconf.moves < pcalc->btconf.moves)))) {
    memcpy(pcalc->btlist, pcalc->ctlist,
           pcalc->ctconf.trade_routes_num * sizeof(struct trade_route *));
    pcalc->btconf = pcalc->ctconf;
  }

#ifdef ASYNC_TRADE_PLANNING
#ifdef WIN32_NATIVE
  /* uc_link not working on windows. Swap contexts at the end
   * of the calculation. */
  if (start_city == 0 && start_trade == 1) {
    swapcontext(&pcalc->interrupted_point, &pcalc->start_point);
  }
#endif  /* WIN32_NATIVE */
#endif  /* ASYNC_TRADE_PLANNING */
}

/****************************************************************************
  Initialize a trade planning calculation. Note that this function doesn't
  launch the recursive calculation, use trade_planning_calculation_resume()
  for that. However, the calcaultion could be finished before the end of this
  function (by simplification). In a such case, this function returns NULL.

  pplayer:          The player we are calculating for.
  pclist:           The list of the cities.
  time_limit:       The maximum time of the calculation (only if we don't
                                                         use a theard)
  destroy_callback: Called just before the calculation is going to be destroyed.
  apply_callback:   Called when an application is requested.
  data:             A user data passed as second argument of the callbacks.
****************************************************************************/
struct trade_planning_calculation *trade_planning_calculation_new(
    player_t *pplayer, const struct city_list *pclist,
#ifndef ASYNC_TRADE_PLANNING
    int time_limit,
#endif
    void (*destroy_callback)(const struct trade_planning_calculation *, void *),
    void (*apply_callback)(const struct trade_planning_calculation *, void *),
    void *data)
{
  assert(NULL != pclist);
  assert(NULL != destroy_callback);
  assert(NULL != apply_callback);

  if (city_list_size(pclist) <= 0) {
    return NULL;
  }

  struct trade_planning_calculation *pcalc;
  struct trade_city *tcity1, *tcity2;
  struct pf_parameter *pparameter = get_caravan_parameter(pplayer, NULL);
  struct path_finding_map *map;
  struct pf_position pos;
  unsigned int i, j;

  /* Initialize */
  pcalc = static_cast<trade_planning_calculation*>(
              wc_malloc(sizeof(struct trade_planning_calculation)));
  pcalc->size = city_list_size(pclist);
  pcalc->tcities = static_cast<trade_city*>(wc_malloc(pcalc->size * sizeof(struct trade_city)));
  pcalc->ctlist = static_cast<trade_route**>(
                      wc_malloc(pcalc->size * game.traderoute_info.maxtraderoutes
                                * sizeof(struct trade_route *) / 2));
  pcalc->btlist = static_cast<trade_route**>(
                      wc_malloc(pcalc->size * game.traderoute_info.maxtraderoutes
                                * sizeof(struct trade_route *) / 2));
#ifdef ASYNC_TRADE_PLANNING
  pcalc->state = TPC_INTERRUPTED;
#ifdef WIN32_NATIVE
  pcalc->interrupted_point.uc_stack.ss_flags = 0;
#endif  /* WIN32_NATIVE */
  pcalc->interrupted_point.uc_link = &pcalc->start_point;
  pcalc->interrupted_point.uc_stack.ss_sp = wc_malloc(SIGSTKSZ);
  pcalc->interrupted_point.uc_stack.ss_size = SIGSTKSZ;
  getcontext(&pcalc->interrupted_point);
  makecontext(&pcalc->interrupted_point,
              (void (*)(void)) recursive_calculate_trade_planning,
              3, pcalc, 0, 1);
#else
  pcalc->max_time = time_limit > 0 ? time(NULL) + time_limit : 0;
#endif  /* ASYNC_TRADE_PLANNING */
  pcalc->destroying = FALSE;
  pcalc->destroy_callback = destroy_callback;
  pcalc->apply_callback = apply_callback;
  pcalc->data = data;

  if (!tp_calculations) {
    tp_calculations = tp_calc_list_new();
  }
  tp_calc_list_append(tp_calculations, pcalc);

  i = 0;
  pcalc->ctconf.trade_routes_num = 0;
  pcalc->ctconf.free_slots = 0;
  pcalc->ctconf.turns = 0;
  pcalc->ctconf.moves = 0;
  city_list_iterate(pclist, pcity) {
    tcity1 = &pcalc->tcities[i];
    tcity1->pcity = pcity;
    tcity1->free_slots = game.traderoute_info.maxtraderoutes
                         - get_real_trade_route_number(pcity);
    tcity1->trade_routes_num = 0;
    tcity1->trade_routes = static_cast<trade_route**>(
        wc_malloc(pcalc->size * sizeof(struct trade_route *)));
    tcity1->distance = static_cast<int*>(
        wc_malloc(pcalc->size * sizeof(int)));

    map = NULL;
    /* Check if a trade route is possible with the lower index cities */
    for (j = 0; j <= i; j++) {
      tcity2 = &pcalc->tcities[j];
      if (tcity1->free_slots > 0 && tcity2->free_slots > 0
          && can_cities_trade(tcity1->pcity, tcity2->pcity)
          && !cities_will_have_trade(tcity1->pcity, tcity2->pcity)) {
        struct trade_route *p_tr = trade_route_new(tcity1->pcity, tcity2->pcity,
                                                   NULL, TR_PLANNED);
        int distance = real_map_distance(tcity1->pcity->common.tile,
                                         tcity2->pcity->common.tile);

        if (pparameter && !map) {
          pparameter->start_tile = pcity->common.tile;
          map = pf_create_map(pparameter);
        }
        if (map && pf_get_position(map, tcity2->pcity->common.tile, &pos)) {
          p_tr->move_cost = pos.total_MC;
          p_tr->move_turns = COST_TO_TURNS(p_tr);
        } else {
          p_tr->move_cost = WC_INFINITY;
          p_tr->move_turns = WC_INFINITY;
        }
        tcity1->trade_routes[j] = p_tr;
        tcity2->trade_routes[i] = p_tr;
        tcity1->distance[j] = distance;
        tcity2->distance[i] = distance;
        tcity1->trade_routes_num++;
        tcity2->trade_routes_num++;
      } else {
        tcity1->trade_routes[j] = NULL;
        tcity2->trade_routes[i] = NULL;
      }
    }

    pcalc->ctconf.free_slots += tcity1->free_slots;
    if (map) {
      pf_destroy_map(map);
    }
    i++;
  } city_list_iterate_end;

  /* Improvation of the calculation: allocate first the harder cities */
  do {
    tcity2 = NULL;
    j = -1;
    /* Find the hardest */
    for (i = 0; i < pcalc->size; i++) {
      tcity1 = &pcalc->tcities[i];
      if (tcity1->trade_routes_num > 0
          && tcity1->trade_routes_num <= tcity1->free_slots) {
        if (!tcity2
            || tcity1->free_slots - tcity1->trade_routes_num
               > tcity2->free_slots - tcity2->trade_routes_num) {
          j = i;
          tcity2 = tcity1;
        }
      }
    }
    if (tcity2 && j >= 0) {
      /* Give it all its trade routes */
      for (i = 0; i < pcalc->size; i++) {
       tcity1 = &pcalc->tcities[i];
       if (tcity2->trade_routes[i] && tcity1->free_slots > 0) {
          /* Add a trade route in the planning */
          add_trade_route_in_planning(pcalc, j, i);

          /*
           * If the city doesn't have free slots anymore,
           * clear its possible trade routes
           */
          if (tcity1->free_slots == 0) {
            struct trade_city *tcity3;
            struct trade_route *p_tr;
            unsigned int k;

            for (k = 0; k < pcalc->size; k++) {
              tcity3 = &pcalc->tcities[k];
              if ((p_tr = tcity1->trade_routes[k])) {
                tcity1->trade_routes[k] = NULL;
                tcity1->trade_routes_num--;
                tcity3->trade_routes[i] = NULL;
                tcity3->trade_routes_num--;
                free(p_tr);
              }
            }
            assert(tcity1->trade_routes_num == 0);
          }
        }
      }
      assert(tcity2->trade_routes_num == 0);
    }
  } while (tcity2 && j >= 0);

  /* Set the current configuration as best configuration */
  if (pcalc->ctconf.trade_routes_num > 0) {
    memcpy(pcalc->btlist, pcalc->ctlist,
           pcalc->ctconf.trade_routes_num * sizeof(struct trade_route *));
  }
  pcalc->btconf = pcalc->ctconf;

  if (pcalc->btconf.free_slots == 0) {
    /* Already finished */
    trade_planning_calculation_destroy(pcalc, TRUE);
    return NULL;
  }

  return pcalc;
}

/****************************************************************************
  Free a trade planning calculation and apply if needed.
****************************************************************************/
void trade_planning_calculation_destroy(
    struct trade_planning_calculation *pcalc, bool apply)
{
  assert(NULL != pcalc);

  unsigned int i, j;

  /* Avoid recursive calls */
  if (pcalc->destroying) {
    return;
  }
  pcalc->destroying = TRUE;

  /* Notifications */
  if (apply) {
    assert(NULL != pcalc->apply_callback);
    pcalc->apply_callback(pcalc, pcalc->data);
  }
  assert(NULL != pcalc->destroy_callback);
  pcalc->destroy_callback(pcalc, pcalc->data);

  if (tp_calculations) {
    tp_calc_list_unlink(tp_calculations, pcalc);
  }

  /* Free datas */
#ifdef ASYNC_TRADE_PLANNING
  free(pcalc->interrupted_point.uc_stack.ss_sp);
#endif

  for (i = 0; i < (unsigned int)pcalc->ctconf.trade_routes_num; i++) {
    free(pcalc->ctlist[i]);
  }
  free(pcalc->ctlist);

  /*
   * N.B.: The trade routes in btlist doesn't need to be free, because
   * all trade routes it could contain are already listed in tcities or ctlist.
   */
  free(pcalc->btlist);

  for (i = 0; i < pcalc->size; i++) {
    for (j = i + 1; j < pcalc->size; j++) {
      if (pcalc->tcities[i].trade_routes[j]) {
        free(pcalc->tcities[i].trade_routes[j]);
      }
    }
    free(pcalc->tcities[i].trade_routes);
    free(pcalc->tcities[i].distance);
  }
  free(pcalc->tcities);

  free(pcalc);
}

/**************************************************************************
  Resume the calculation where it has been interrupted. Returns FALSE if the
  calculation is finished.
**************************************************************************/
bool trade_planning_calculation_resume(struct trade_planning_calculation *pcalc)
{
  if (!pcalc) {
    return FALSE;
  }

#ifdef ASYNC_TRADE_PLANNING
  if (pcalc->state != TPC_INTERRUPTED) {
    return FALSE;
  }

  pcalc->state = TPC_RUNNING;
  pcalc->operations_counter = 0;

  swapcontext(&pcalc->start_point, &pcalc->interrupted_point);

  switch (pcalc->state) {
  case TPC_RUNNING:
    trade_planning_calculation_destroy(pcalc, TRUE);
    return FALSE;
  case TPC_INTERRUPTED:
    return TRUE;
  }
#else
  recursive_calculate_trade_planning(pcalc, 0, 1);
  trade_planning_calculation_destroy(pcalc, TRUE);
#endif  /* ASYNC_TRADE_PLANNING */
  return FALSE;
}

/**************************************************************************
  Get the trade routes of the calculation.
**************************************************************************/
struct trade_route_list *trade_planning_calculation_get_trade_routes(
    const struct trade_planning_calculation *pcalc,
    char *buf, size_t buf_len)
{
  struct trade_route_list *trade_planning = trade_route_list_new();
  int i;

  for (i = 0; i < pcalc->btconf.trade_routes_num; i++) {
    struct trade_route *p_tr = static_cast<trade_route*>(
        wc_malloc(sizeof(struct trade_route)));

    *p_tr = *pcalc->btlist[i];
    trade_route_list_append(trade_planning, p_tr);
  }

  if (buf) {
    if (trade_route_list_size(trade_planning) > 0) {
      my_snprintf(buf, buf_len,
                  _("%d new trade routes, with a total of %d moves"),
                  trade_route_list_size(trade_planning), pcalc->btconf.moves);
    } else {
      my_snprintf(buf, buf_len, _("Didn't find any trade routes to establish"));
    }
  }

  return trade_planning;
}

/**************************************************************************
  Get some useful datas from the trade calculation.
**************************************************************************/
void trade_planning_calculation_get_advancement(
    const struct trade_planning_calculation *pcalc,
    int *trade_routes_num, int *max_trade_routes_num, int *total_moves)
{
  assert(NULL != pcalc);

  *trade_routes_num = pcalc->btconf.trade_routes_num;
  *max_trade_routes_num = pcalc->btconf.trade_routes_num
                          + pcalc->btconf.free_slots / 2;
  *total_moves = pcalc->btconf.moves;
}

/****************************************************************************
  Try to find a trade route to establish for the unit.

  Arguments:
  best_value:       Check the best trade route by the value of this trade route,
                    else, use the faster route.
  allow_free_other: Can return a route which is already going to be established.
  internal_first:   If not, it could return an external trade route whereas
                    there are internal trade route non-used yet.
  homecity_first:   Returns a trade route of the homecity if it's possible.


  btr is the best trade route.
  etr is the best external trade route.
  htr is the best trade route from homecity.
****************************************************************************/
struct trade_route *get_next_trade_route_to_establish(unit_t *punit,
                                                      bool best_value,
                                                      bool allow_free_other,
                                                      bool internal_first,
                                                      bool homecity_first)
{
  struct trade_route *btr = NULL;
  struct trade_route *etr = NULL;
  struct trade_route *htr = NULL;
  struct trade_route **pp_tr;
  unit_t *ounit;

  city_list_iterate(unit_owner(punit)->cities, pcity) {
    trade_route_list_iterate(pcity->common.trade_routes, p_tr) {
      if (p_tr->status == TR_ESTABLISHED
          || (p_tr->pcity2->common.owner != p_tr->pcity1->common.owner
              && p_tr->pcity2 == pcity)) {
        continue;
      }

      if (p_tr->punit) {
        if (allow_free_other) {
          /* Is punit a faster? */
          struct trade_route *ntr = trade_route_new(p_tr->pcity1, p_tr->pcity2,
                                                    punit, p_tr->status);
          ntr->move_cost = calculate_trade_move_cost(ntr);
          ntr->move_turns = COST_TO_TURNS(ntr);
          p_tr->move_cost = calculate_trade_move_cost(p_tr);
          p_tr->move_turns = COST_TO_TURNS(p_tr);
          if (ntr->move_turns >= p_tr->move_turns) {
            free(ntr);
            continue;
          }
          free(ntr);
        } else {
          continue;
        }
      }

      /* Find the trade route to compare with */
      if (internal_first
          && p_tr->pcity2->common.owner != p_tr->pcity1->common.owner)
      {
        pp_tr = &etr;
      } else if (homecity_first
                 && (p_tr->pcity1->common.id == punit->homecity
                     || p_tr->pcity2->common.id == punit->homecity))
      {
        pp_tr = &htr;
      } else {
        pp_tr = &btr;
      }

      /* Update trade route datas */
      p_tr->value = trade_between_cities(p_tr->pcity1, p_tr->pcity2);
      ounit = p_tr->punit;
      p_tr->punit = punit;
      p_tr->move_cost = calculate_trade_move_cost(p_tr);
      p_tr->move_turns = COST_TO_TURNS(p_tr);
      p_tr->punit = ounit; /* Reverse */

      /* Test if this trade route is better */
      if (best_value) {
        if (!(*pp_tr) || p_tr->value > (*pp_tr)->value
            || (p_tr->value == (*pp_tr)->value
                && (p_tr->move_turns < (*pp_tr)->move_turns
                    || (p_tr->move_turns == (*pp_tr)->move_turns
                        && p_tr->move_cost > (*pp_tr)->move_cost)))) {
          *pp_tr = p_tr;
        }
      } else {
        if (!(*pp_tr) || p_tr->move_turns < (*pp_tr)->move_turns
            || (p_tr->move_turns == (*pp_tr)->move_turns
                && (p_tr->move_cost > (*pp_tr)->move_cost
                    || (p_tr->move_cost == (*pp_tr)->move_cost
                        && p_tr->value > (*pp_tr)->value)))) {
          *pp_tr = p_tr;
        }
      }
    } trade_route_list_iterate_end;
  } city_list_iterate_end;

  if (htr) {
    return htr;
  } else if (!btr && etr) {
    return etr;
  } /* else */
  return btr;
}

/****************************************************************************
  Remove all trade routes, called in game_free().
****************************************************************************/
void game_remove_all_trade_routes(void)
{
  cities_iterate(pcity) {
    trade_route_list_iterate(pcity->common.trade_routes, p_tr) {
      game_trade_route_remove(p_tr);
    } trade_route_list_iterate_end;
  } cities_iterate_end;

  if (tp_calculations) {
    tp_calc_list_iterate(pcalc) {
      trade_planning_calculation_destroy(pcalc, FALSE);
    } tp_calc_list_iterate_end;
    tp_calc_list_free(tp_calculations);
    tp_calculations = NULL;
  }
}

/****************************************************************************
  Check if we use a pointer to a destroyed city in a trade planning calculation.
****************************************************************************/
void check_removed_city(const city_t *pcity)
{
  unsigned int i;

  tp_calc_list_iterate(pcalc) {
    for (i = 0; i < pcalc->size; i++) {
      if (pcalc->tcities[i].pcity == pcity) {
        trade_planning_calculation_destroy(pcalc, FALSE);
        break;
      }
    }
  } tp_calc_list_iterate_end;
}
