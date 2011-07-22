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

#ifndef WC_COMMON_TRADE_ROUTE_H
#define WC_COMMON_TRADE_ROUTE_H

#include "map.h"        /* struct tile_list */

enum trade_route_status {
  TR_NONE = 0,
  TR_PLANNED = 1 << 0,
  TR_IN_ROUTE = 1 << 1,
  TR_PL_AND_IR = TR_PLANNED | TR_IN_ROUTE,
  TR_ESTABLISHED = 1 << 2
};

/* the pcity1 owner is the trade route owner */
struct trade_route {
  struct city *pcity1, *pcity2;
  struct unit *punit;
  enum trade_route_status status;
  int value;

  /* Are not usually updated */
  int move_cost, move_turns;
};

#define SPECLIST_TAG trade_route
#define SPECLIST_TYPE struct trade_route
#include "speclist.h"

#define trade_route_list_iterate(traderoutelist, ptr) \
  TYPED_LIST_ITERATE(struct trade_route, traderoutelist, ptr)
#define trade_route_list_iterate_end LIST_ITERATE_END

/* Iterate all established trade routes for a city */
#define established_trade_routes_iterate(pcity, ptr)   \
  trade_route_list_iterate(pcity->trade_routes, ptr) { \
    if (ptr->status != TR_ESTABLISHED) {               \
      continue;                                        \
    }
#define established_trade_routes_iterate_end           \
  } trade_route_list_iterate_end

/* Get the other city member of the trade route */
#define OTHER_CITY(ptr, pcity) ((ptr)->pcity1 == pcity ? (ptr)->pcity2 \
                                                       : (ptr)->pcity1)

#define COST_TO_TURNS(ptr)                                              \
  ((ptr)->punit ? ((ptr)->move_cost > (ptr)->punit->moves_left          \
                   ? (unit_move_rate((ptr)->punit) + (ptr)->move_cost   \
                      - (ptr)->punit->moves_left - 1)                   \
                      / unit_move_rate((ptr)->punit)                    \
                   : ((ptr)->punit->moves_left == 0 ? 1 : 0))           \
   : ((ptr)->move_cost + 1) / SINGLE_MOVE)

struct trade_route *game_trade_route_add(struct city *pcity1,
                                         struct city *pcity2);
void game_trade_route_remove(struct trade_route *ptr);
void game_remove_all_trade_routes(void);
void check_removed_city(const struct city *pcity);

struct trade_route *game_trade_route_find(const struct city *pcity1,
                                          const struct city *pcity2);

int calculate_trade_move_cost(struct trade_route *ptr);
int calculate_trade_move_turns(struct trade_route *ptr);
struct unit_order *make_unit_orders(struct trade_route *ptr, int *length);

/* Trade planning calculation */
#ifdef HAVE_UCONTEXT_H
#define ASYNC_TRADE_PLANNING    1
#else
#ifdef WIN32_NATIVE
#define ASYNC_TRADE_PLANNING    1
#endif /* WIN32_NATIVE */
#endif /* HAVE_UCONTEXT_H */

struct city_list;
struct player;
struct trade_planning_calculation;      /* Opaque type */

int trade_planning_precalculation(const struct tile_list *ptlist,
                                  size_t size, int *free_slots);
struct trade_planning_calculation *trade_planning_calculation_new(
    struct player *pplayer, const struct city_list *pclist,
#ifndef ASYNC_TRADE_PLANNING
    int time_limit,
#endif
    void (*destroy_callback)(const struct trade_planning_calculation *, void *),
    void (*apply_callback)(const struct trade_planning_calculation *, void *),
    void *data);
void trade_planning_calculation_destroy(
    struct trade_planning_calculation *pcalc, bool apply);
bool trade_planning_calculation_resume(
     struct trade_planning_calculation *pcalc);

struct trade_route_list *trade_planning_calculation_get_trade_routes(
    const struct trade_planning_calculation *pcalc,
    char *buf, size_t buf_len);
void trade_planning_calculation_get_advancement(
    const struct trade_planning_calculation *pcalc,
    int *trade_routes_num, int *max_trade_routes_num, int *total_moves);

struct trade_route *get_next_trade_route_to_establish(struct unit *punit,
                                                      bool best_value,
                                                      bool allow_free_other,
                                                      bool internal_first,
                                                      bool homecity_first);

#endif /* WC_COMMON_TRADE_ROUTE_H */
