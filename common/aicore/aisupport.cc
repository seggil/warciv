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
#  include "../../config.hh"
#endif

#include "../city.hh"
#include "../game.hh"
#include "../map.hh"
#include "../player.hh"
#include "shared.hh"
#include "../spaceship.hh"
#include "support.hh"
#include "../tech.hh"
#include "../traderoute.hh"

#include "aisupport.hh"

/**********************************************************************
  Find who is leading the space race. Returns NULL if nobody is.
***********************************************************************/
player_t *player_leading_spacerace(void)
{
  player_t *best = NULL;
  int best_arrival = WC_INFINITY;
  enum spaceship_state best_state = SSHIP_NONE;

  if (game.info.spacerace == FALSE) {
    return NULL;
  }

  players_iterate(pplayer) {
    struct player_spaceship *ship = &pplayer->spaceship;
    int arrival = (int) ship->travel_time + ship->launch_year;

    if (!pplayer->is_alive || is_barbarian(pplayer)
        || ship->state == SSHIP_NONE) {
      continue;
    }

    if (ship->state != SSHIP_LAUNCHED
        && ship->state > best_state) {
      best_state = ship->state;
      best = pplayer;
    } else if (ship->state == SSHIP_LAUNCHED
               && arrival < best_arrival) {
      best_state = ship->state;
      best_arrival = arrival;
      best = pplayer;
    }
  } players_iterate_end;

  return best;
}

/**********************************************************************
  Calculate average distances to other players. We calculate the
  average distance from all of our cities to the closest enemy city.
***********************************************************************/
int player_distance_to_player(player_t *pplayer, player_t *target)
{
  int cities = 0;
  int dists = 0;

  if (pplayer == target
      || !target->is_alive
      || !pplayer->is_alive
      || city_list_size(pplayer->cities) == 0
      || city_list_size(target->cities) == 0) {
    return 1;
  }

  /* For all our cities, find the closest distance to an enemy city. */
  city_list_iterate(pplayer->cities, pcity) {
    int min_dist = WC_INFINITY;

    city_list_iterate(target->cities, c2) {
      int dist = real_map_distance(c2->common.tile, pcity->common.tile);

      if (min_dist > dist) {
        min_dist = dist;
      }
    } city_list_iterate_end;
    dists += min_dist;
    cities++;
  } city_list_iterate_end;

  return MAX(dists / cities, 1);
}

/**********************************************************************
  Rough calculation of the worth of pcity in gold.
***********************************************************************/
int city_gold_worth(city_t *pcity)
{
  int worth;

  worth = pcity->common.pop_size * 150; /* reasonable base cost */
  unit_list_iterate(pcity->common.units_supported, punit) {
    if (same_pos(punit->tile, pcity->common.tile)) {
      Unit_Type_id id = unit_type(punit)->obsoleted_by;

      if (id >= 0 && can_build_unit_direct(pcity, id)) {
        worth += unit_disband_shields(punit->type) / 2; /* obsolete */
      } else {
        worth += unit_disband_shields(punit->type); /* good stuff */
      }
    }
  } unit_list_iterate_end;
  built_impr_iterate(pcity, impr) {
    if (improvement_types[impr].is_wonder && !wonder_obsolete(impr)) {
      worth += impr_sell_gold(impr);
   } else {
      worth += impr_sell_gold(impr) / 4;
    }
  } built_impr_iterate_end;
  if (city_unhappy(pcity)) {
    worth *= 0.75;
  }
  return worth;
}
