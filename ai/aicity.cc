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
#include <string.h>

#include "city.hh"
#include "combat.hh"
#include "effects.hh"
#include "events.hh"
#include "wc_intl.hh"
#include "game.hh"
#include "government.hh"
#include "log.hh"
#include "map.hh"
#include "packets.hh"
#include "player.hh"
#include "rand.hh"
#include "shared.hh"
#include "support.hh"
#include "unit.hh"

#include "cityhand.hh"
#include "citytools.hh"
#include "cityturn.hh"
#include "gotohand.hh"
#include "plrhand.hh"
#include "settlers.hh"
#include "unithand.hh"
#include "unittools.hh"

#include "advdomestic.hh"
#include "advmilitary.hh"
#include "aidata.hh"
#include "aihand.hh"
#include "ailog.hh"
#include "aitools.hh"
#include "aiunit.hh"

#include "aicity.hh"

/* Iterate over cities within a certain range around a given city
 * (city_here) that exist within a given city list. */
#define city_range_iterate(city_here, list, range, city)            \
{                                                                   \
  Continent_id continent = map_get_continent(city_here->common.tile); \
  city_list_iterate(list, city) {                                   \
    if ((range == EFFECT_RANGE_CITY && city == city_here)                    \
        || (range == EFFECT_RANGE_LOCAL && city == city_here)                \
        || (range == EFFECT_RANGE_CONTINENT                                  \
            && map_get_continent(city->common.tile) == continent)   \
        || (range == EFFECT_RANGE_PLAYER)) {
#define city_range_iterate_end \
  } } city_list_iterate_end; }

#define CITY_EMERGENCY(pcity)                        \
 (pcity->common.shield_surplus < 0 || city_unhappy(pcity)   \
  || pcity->common.food_stock + pcity->common.food_surplus < 0)
#define LOG_BUY LOG_DEBUG

static void resolve_city_emergency(player_t *pplayer, struct city_s *pcity);
static void ai_sell_obsolete_buildings(struct city_s *pcity);

/**************************************************************************
  This calculates the usefulness of pcity to us. Note that you can pass
  another player's ai_data structure here for evaluation by different
  priorities.
**************************************************************************/
int ai_eval_calc_city(struct city_s *pcity, struct ai_data *ai)
{
  int i = (pcity->common.food_surplus * ai->food_priority
           + pcity->common.shield_surplus * ai->shield_priority
           + pcity->common.luxury_total * ai->luxury_priority
           + pcity->common.tax_total * ai->gold_priority
           + pcity->common.science_total * ai->science_priority
           + pcity->common.people_happy[4] * ai->happy_priority
           - pcity->common.people_unhappy[4] * ai->unhappy_priority
           - pcity->common.people_angry[4] * ai->angry_priority
           - pcity->common.pollution * ai->pollution_priority);

  if (pcity->common.food_surplus < 0 || pcity->common.shield_surplus < 0) {
    /* The city is unmaintainable, it can't be good */
    i = MIN(i, 0);
  }

  return i;
}

/**************************************************************************
  Calculates city want from some input values.
**************************************************************************/
static inline int city_want(player_t *pplayer, struct city_s *acity,
                            struct ai_data *ai)
{
  int want = 0, food, trade, shields, lux, sci, tax;

  get_food_trade_shields(acity, &food, &trade, &shields);
  trade -= city_corruption(acity, trade);
  shields -= city_waste(acity, shields);
  get_tax_income(pplayer, trade, &sci, &lux, &tax);
  sci += (acity->common.specialists[SP_SCIENTIST]
          * game.ruleset_game.specialist_bonus[SP_SCIENTIST]);
  lux += (acity->common.specialists[SP_ELVIS]
          * game.ruleset_game.specialist_bonus[SP_ELVIS]);
  tax += (acity->common.specialists[SP_TAXMAN]
          * game.ruleset_game.specialist_bonus[SP_TAXMAN]);

  built_impr_iterate(acity, i) {
    tax -= improvement_upkeep(acity, i);
  } built_impr_iterate_end;

  want += food * ai->food_priority;
  if (shields != 0) {
    want += ((shields * get_city_shield_bonus(acity)) / 100)
            * ai->shield_priority;
    want -= city_pollution(acity, shields) * ai->pollution_priority;
  }
  if (lux > 0) {
    want += ((lux * get_city_luxury_bonus(acity)) / 100)
            * ai->luxury_priority;
  }
  if (sci > 0) {
    want += ((sci * get_city_science_bonus(acity)) / 100)
            * ai->science_priority;
  }
  if (tax > 0) {
    tax *= get_city_tax_bonus(acity) / 100;
  }
  want += tax * ai->gold_priority;

  return want;
}

/**************************************************************************
  Calculates want for some buildings by actually adding the building and
  measuring the effect.
**************************************************************************/
static int base_want(player_t *pplayer, struct city_s *pcity,
                     Impr_Type_id id)
{
  struct ai_data *ai = ai_data_get(pplayer);
  int final_want = 0;
  struct city_s *capital = find_palace(pplayer);

  if (ai->impr_calc[id] == AI_IMPR_ESTIMATE) {
    return 0; /* Nothing to calculate here. */
  }

  /* Add the improvement */
  city_add_improvement(pcity, id);
  if (is_wonder(id)) {
    game.info.global_wonders[id] = pcity->common.id;
  }

  /* Stir, then compare notes */
  city_range_iterate(pcity, pplayer->cities, ai->impr_range[id], acity) {
    final_want += city_want(pplayer, acity, ai) - acity->u.server.ai.worth;
  } city_range_iterate_end;

  /* Restore */
  city_remove_improvement(pcity, id);
  if (is_wonder(id)) {
    game.info.global_wonders[id] = 0;
  }

  /* Ensure that we didn't inadvertantly move our palace */
  if (find_palace(pplayer) != capital) {
    city_add_improvement(capital, get_building_for_effect(EFFECT_TYPE_CAPITAL_CITY));
  }

  return final_want;
}

/**************************************************************************
  Calculate effects. A few base variables:
    c - number of cities we have in current range
    u - units we have of currently affected type
    v - the want for the improvement we are considering

  This function contains a whole lot of WAGs. We ignore cond_* for now,
  thinking that one day we may fulfill the cond_s anyway. In general, we
  first add bonus for city improvements, then for wonders.

  IDEA: Calculate per-continent aggregates of various data, and use this
  for wonders below for better wonder placements.
**************************************************************************/
static void adjust_building_want_by_effects(struct city_s *pcity,
                                            Impr_Type_id id)
{
  player_t *pplayer = city_owner(pcity);
  struct impr_type *pimpr = get_improvement_type(id);
  int v = 0;
  int cities[EFFECT_RANGE_LAST];
  int nplayers = game.info.nplayers
                 - game.server.nbarbarians
                 - team_count_members_alive(pplayer->team);
  struct ai_data *ai = ai_data_get(pplayer);
  tile_t *ptile = pcity->common.tile;
  bool capital = is_capital(pcity);
  struct government *gov = get_gov_pplayer(pplayer);

  /* Base want is calculated above using a more direct approach. */
  v += base_want(pplayer, pcity, id);
  if (v != 0) {
    CITY_LOG(LOG_DEBUG, pcity, "%s base_want is %d (range=%d)",
             get_improvement_name(id), v, ai->impr_range[id]);
  }

  /* Find number of cities per range.  */
  cities[EFFECT_RANGE_PLAYER] = city_list_size(pplayer->cities);
  cities[EFFECT_RANGE_WORLD] = cities[EFFECT_RANGE_PLAYER]; /* kludge. */

  cities[EFFECT_RANGE_CONTINENT] = ai->stats.cities[ptile->continent];

  cities[EFFECT_RANGE_CITY] = 1;
  cities[EFFECT_RANGE_LOCAL] = 0;

  /* Calculate desire value. */
  effect_type_vector_iterate(get_building_effect_types(id), ptype) {
    effect_list_iterate(get_building_effects(id, *ptype), peff) {
      if (is_effect_useful(TARGET_BUILDING, pplayer, pcity, id,
                           NULL, id, peff)) {
        int amount = peff->value, c = cities[peff->range];
        struct city_s *palace = find_palace(pplayer);

        switch (*ptype) {
          case EFFECT_TYPE_PROD_TO_GOLD:
            /* Since coinage contains some entirely spurious ruleset values,
             * we need to return here with some spurious want. */
            pcity->u.server.ai.building_want[id] = TRADE_WEIGHTING;
            return;
          /* These have already been evaluated in base_want() */
          case EFFECT_TYPE_CAPITAL_CITY:
          case EFFECT_TYPE_UPKEEP_FREE:
          case EFFECT_TYPE_POLLU_POP_PCT:
          case EFFECT_TYPE_POLLU_PROD_PCT:
          case EFFECT_TYPE_TRADE_PER_TILE:
          case EFFECT_TYPE_TRADE_INC_TILE:
          case EFFECT_TYPE_FOOD_INC_TILE:
          case EFFECT_TYPE_TRADE_ADD_TILE:
          case EFFECT_TYPE_PROD_INC_TILE:
          case EFFECT_TYPE_PROD_PER_TILE:
          case EFFECT_TYPE_PROD_ADD_TILE:
          case EFFECT_TYPE_FOOD_PER_TILE:
          case EFFECT_TYPE_FOOD_ADD_TILE:
          case EFFECT_TYPE_PROD_BONUS:
          case EFFECT_TYPE_TAX_BONUS:
          case EFFECT_TYPE_SCIENCE_BONUS:
          case EFFECT_TYPE_LUXURY_BONUS:
          case EFFECT_TYPE_CORRUPT_PCT:
          case EFFECT_TYPE_WASTE_PCT:
            break;

          /* WAG evaluated effects */
          case EFFECT_TYPE_INCITE_DIST_PCT:
            if (palace) {
              v += real_map_distance(pcity->common.tile, palace->common.tile);
            }
            break;
          case EFFECT_TYPE_MAKE_HAPPY:
            /* TODO */
            break;
          case EFFECT_TYPE_UNIT_RECOVER:
            /* TODO */
            break;
          case EFFECT_TYPE_NO_UNHAPPY:
            v += (pcity->common.specialists[SP_ELVIS] + pcity->common.people_unhappy[4]) * 20;
            break;
          case EFFECT_TYPE_FORCE_CONTENT:
            if (!government_has_flag(gov, G_NO_UNHAPPY_CITIZENS)) {
              v += (pcity->common.people_unhappy[4] + pcity->common.specialists[SP_ELVIS]) * 20;
              v += 5 * c;
            }
            break;
          case EFFECT_TYPE_MAKE_CONTENT_MIL_PER:
          case EFFECT_TYPE_MAKE_CONTENT:
            if (!government_has_flag(gov, G_NO_UNHAPPY_CITIZENS)) {
              v += MIN(pcity->common.people_unhappy[4] + pcity->common.specialists[SP_ELVIS],
                       amount) * 20;
              v += MIN(amount, 5) * c;
            }
            break;
          case EFFECT_TYPE_MAKE_CONTENT_MIL:
            if (!government_has_flag(gov, G_NO_UNHAPPY_CITIZENS)) {
              v += pcity->common.people_unhappy[4] * amount
                * MAX(unit_list_size(pcity->common.units_supported)
                    - gov->free_happy, 0) * 2;
              v += c * MAX(amount + 2 - gov->free_happy, 1);
            }
            break;
          case EFFECT_TYPE_TECH_PARASITE:
            v += (total_bulbs_required(pplayer) * (100 - game.info.freecost)
                * (nplayers - amount)) / (nplayers * amount * 100);
            break;
          case EFFECT_TYPE_GROWTH_FOOD:
            v += c * 4 + (amount / 7) * pcity->common.food_surplus;
            break;
          case EFFECT_TYPE_AIRLIFT:
            /* FIXME: We need some smart algorithm here. The below is
             * totally braindead. */
            v += c + MIN(ai->stats.units.land, 13);
            break;
          case EFFECT_TYPE_ANY_GOVERNMENT:
            if (!can_change_to_government(pplayer, ai->goal.govt.idx)) {
              v += MIN(MIN(ai->goal.govt.val, 65),
                  num_unknown_techs_for_goal(pplayer, ai->goal.govt.req) * 10);
            }
            break;
          case EFFECT_TYPE_ENABLE_NUKE:
            /* Treat nuke as a Cruise Missile upgrade */
            v += 20 + ai->stats.units.missiles * 5;
            break;
          case EFFECT_TYPE_ENABLE_SPACE:
            if (game.info.spacerace) {
              v += 5;
              if (ai->diplomacy.production_leader == pplayer) {
                v += 100;
              }
            }
            break;
          case EFFECT_TYPE_GIVE_IMM_TECH:
            if (!ai_wants_no_science(pplayer)) {
              v += amount * (game.info.researchcost + 1);
            }
            break;
          case EFFECT_TYPE_HAVE_EMBASSIES:
            v += 5 * nplayers;
            break;
          case EFFECT_TYPE_REVEAL_CITIES:
          case EFFECT_TYPE_NO_ANARCHY:
            break;  /* Useless for AI */
          case EFFECT_TYPE_NO_SINK_DEEP:
            v += 15 + ai->stats.units.triremes * 5;
            break;
          case EFFECT_TYPE_NUKE_PROOF:
            if (ai->threats.nuclear) {
              v += pcity->common.pop_size * unit_list_size(ptile->units) * (capital + 1);
            }
            break;
          case EFFECT_TYPE_REVEAL_MAP:
            if (!ai->explore.land_done || !ai->explore.sea_done) {
              v += 10;
            }
            break;
          case EFFECT_TYPE_SIZE_UNLIMIT:
            amount = 20; /* really big city */
            /* there not being a break here is deliberate, mind you */
          case EFFECT_TYPE_SIZE_ADJ:
            if (!city_can_grow_to(pcity, pcity->common.pop_size + 1)) {
              v += pcity->common.food_surplus * ai->food_priority * amount;
              if (pcity->common.pop_size == game.ruleset_control.aqueduct_size) {
                v += 30 * pcity->common.food_surplus;
              }
            }
            v += c * amount * 4 / game.ruleset_control.aqueduct_size;
            break;
          case EFFECT_TYPE_SS_STRUCTURAL:
          case EFFECT_TYPE_SS_COMPONENT:
          case EFFECT_TYPE_SS_MODULE:
            if (game.info.spacerace
                /* If someone has started building spaceship already or
                 * we have chance to win a spacerace */
                && (ai->diplomacy.spacerace_leader
                    || ai->diplomacy.production_leader == pplayer)) {
              v += 95;
            }
            break;
          case EFFECT_TYPE_SPY_RESISTANT:
            /* Uhm, problem: City Wall has -50% here!! */
            break;
          case EFFECT_TYPE_SEA_MOVE:
            v += ai->stats.units.sea * 8 * amount;
            break;
          case EFFECT_TYPE_UNIT_NO_LOSE_POP:
            v += unit_list_size(ptile->units) * 2;
            break;
          case EFFECT_TYPE_LAND_REGEN:
            v += 15 * c + ai->stats.units.land * 3;
            break;
          case EFFECT_TYPE_SEA_REGEN:
            v += 15 * c + ai->stats.units.sea * 3;
            break;
          case EFFECT_TYPE_AIR_REGEN:
            v += 15 * c + ai->stats.units.air * 3;
            break;
          case EFFECT_TYPE_LAND_VET_COMBAT:
            v += 2 * c + ai->stats.units.land * 2;
            break;
          case EFFECT_TYPE_LAND_VETERAN:
            v += 5 * c + ai->stats.units.land;
            break;
          case EFFECT_TYPE_SEA_VETERAN:
            v += 5 * c + ai->stats.units.sea;
            break;
          case EFFECT_TYPE_AIR_VETERAN:
            v += 5 * c + ai->stats.units.air;
            break;
          case EFFECT_TYPE_UPGRADE_UNIT:
            v += ai->stats.units.upgradeable;
            if (amount == 1) {
              v *= 2;
            } else if (amount == 2) {
              v *= 3;
            } else {
              v *= 4;
            }
            break;
          case EFFECT_TYPE_SEA_DEFEND:
            if (ai_handicap(pplayer, H_DEFENSIVE)) {
              v += amount / 10; /* make AI slow */
            }
            if (is_ocean(map_get_terrain(pcity->common.tile))) {
              v += ai->threats.ocean[-map_get_continent(pcity->common.tile)]
                   ? amount/5 : amount/20;
            } else {
              adjc_iterate(pcity->common.tile, tile2) {
                if (is_ocean(map_get_terrain(tile2))) {
                  if (ai->threats.ocean[-map_get_continent(tile2)]) {
                    v += amount/5;
                    break;
                  }
                }
              } adjc_iterate_end;
            }
            v += (amount/20 + ai->threats.invasions - 1) * c; /* for wonder */
            if (capital && ai->threats.invasions) {
              v += amount; /* defend capital! */
            }
            break;
          case EFFECT_TYPE_AIR_DEFEND:
            if (ai_handicap(pplayer, H_DEFENSIVE)) {
              v += amount / 15; /* make AI slow */
            }
            v += (ai->threats.air && ai->threats.continent[ptile->continent])
              ? amount/10 * 5 + amount/10 * c : c;
            break;
          case EFFECT_TYPE_MISSILE_DEFEND:
            if (ai->threats.missile
                && (ai->threats.continent[ptile->continent] || capital)) {
              v += amount/10 * 5 + (amount/10 - 1) * c;
            }
            break;
          case EFFECT_TYPE_LAND_DEFEND:
            if (ai_handicap(pplayer, H_DEFENSIVE)) {
              v += amount / 10; /* make AI slow */
            }
            if (ai->threats.continent[ptile->continent]
                || capital
                || (ai->threats.invasions
                  && is_water_adjacent_to_tile(pcity->common.tile))) {
              v += amount / (!ai->threats.igwall ? (15 - capital * 5) : 15);
            }
            v += (1 + ai->threats.invasions + !ai->threats.igwall) * c;
            break;
          case EFFECT_TYPE_NO_INCITE:
            if (!government_has_flag(gov, G_UNBRIBABLE)) {
              v += MAX((game.server.diplchance * 2
                        - game.server.incite_cost.total_factor) / 2
                  - game.server.incite_cost.improvement_factor * 5
                  - game.server.incite_cost.unit_factor * 5, 0);
            }
            break;
          case EFFECT_TYPE_REGEN_REPUTATION:
            v += (GAME_MAX_REPUTATION - pplayer->reputation) * 50 /
                    GAME_MAX_REPUTATION +
                  amount * 4;
            break;
          case EFFECT_TYPE_GAIN_AI_LOVE:
            players_iterate(aplayer) {
              if (aplayer->ai.control) {
                if (ai_handicap(pplayer, H_DEFENSIVE)) {
                  v += amount / 10;
                } else {
                  v += amount / 20;
                }
              }
            } players_iterate_end;
            break;
          case EFFECT_TYPE_LAST:
            freelog(LOG_ERROR, "Bad effect type.");
            break;
        }
      }
    } effect_list_iterate_end;
  } effect_type_vector_iterate_end;

  /* Reduce want if building gets obsoleted soon */
  if (tech_exists(pimpr->obsolete_by)) {
    v -= v / MAX(1, num_unknown_techs_for_goal(pplayer, pimpr->obsolete_by));
   }

  /* Adjust by building cost */
  v -= pimpr->build_cost / (pcity->common.shield_surplus * 10 + 1);

  /* Set */
  pcity->u.server.ai.building_want[id] = v;
}

/**************************************************************************
  Prime pcity->u.server.ai.building_want[]
**************************************************************************/
void ai_manage_buildings(player_t *pplayer)
/* TODO:  RECALC_SPEED should be configurable to ai difficulty. -kauf  */
#define RECALC_SPEED 5
{
  struct ai_data *ai = ai_data_get(pplayer);

  /* First find current worth of cities and cache this. */
  city_list_iterate(pplayer->cities, acity) {
    acity->u.server.ai.worth = city_want(pplayer, acity, ai);
  } city_list_iterate_end;

  impr_type_iterate(id) {
    if (!can_player_build_improvement(pplayer, id)
        || improvement_obsolete(pplayer, id)) {
      continue;
    }
    city_list_iterate(pplayer->cities, pcity) {
      if (pplayer->ai.control
          && pcity->u.server.ai.next_recalc > game.info.turn) {
        continue; /* do not recalc yet */
      } else {
        pcity->u.server.ai.building_want[id] = 0; /* do recalc */
      }
      if (city_got_building(pcity, id)
          || pcity->common.shield_surplus == 0
          || !can_build_improvement(pcity, id)
          || improvement_redundant(pplayer, pcity, id, FALSE)) {
        continue; /* Don't build redundant buildings */
      }
      adjust_building_want_by_effects(pcity, id);
      CITY_LOG(LOG_DEBUG, pcity, "want to build %s with %d",
               get_improvement_name(id), pcity->u.server.ai.building_want[id]);
    } city_list_iterate_end;
  } impr_type_iterate_end;

  /* Reset recalc counter */
  city_list_iterate(pplayer->cities, pcity) {
    if (pcity->u.server.ai.next_recalc <= game.info.turn) {
      /* This will spread recalcs out so that no one turn end is
       * much longer than others */
      pcity->u.server.ai.next_recalc = game.info.turn + myrand(RECALC_SPEED)
                                     + RECALC_SPEED;
    }
  } city_list_iterate_end;
}

/***************************************************************************
  This function computes distances between cities for purpose of building
  crowds of water-consuming Caravans or smoggish Freights which want to add
  their brick to the wonder being built in pcity.

  At the function entry point, our warmap is intact.  We need to do two
  things: (1) establish "downtown" for THIS city (which is an estimate of
  how much help we can expect when building a wonder) and
  (2) establish distance to pcity for ALL cities on our continent.

  If there are more than one wondercity, things will get a bit random.
****************************************************************************/
static void establish_city_distances(player_t *pplayer,
                                     struct city_s *pcity)
{
  int distance;
  Continent_id wonder_continent;
  Unit_Type_id freight = best_role_unit(pcity, F_HELP_WONDER);
  int moverate = (freight == U_LAST) ? SINGLE_MOVE
                                     : get_unit_type(freight)->move_rate;

  if (!pcity->common.is_building_unit && is_wonder(pcity->common.currently_building)) {
    wonder_continent = map_get_continent(pcity->common.tile);
  } else {
    wonder_continent = 0;
  }

  pcity->u.server.ai.downtown = 0;
  city_list_iterate(pplayer->cities, othercity) {
    distance = WARMAP_COST(othercity->common.tile);
    if (wonder_continent != 0
        && map_get_continent(othercity->common.tile) == wonder_continent) {
      othercity->u.server.ai.distance_to_wonder_city = distance;
    }

    /* How many people near enough would help us? */
    distance += moverate - 1; distance /= moverate;
    pcity->u.server.ai.downtown += MAX(0, 5 - distance);
  } city_list_iterate_end;
}

/**************************************************************************
  Choose a build for the barbarian player.

  TODO: Move this into advmilitary.c
  TODO: It will be called for each city but doesn't depend on the city,
  maybe cache it?  Although barbarians don't normally have many cities,
  so can be a bigger bother to cache it.
**************************************************************************/
static void ai_barbarian_choose_build(player_t *pplayer,
                                      struct ai_choice *choice)
{
  Unit_Type_id bestunit = -1;
  int i, bestattack = 0;

  /* Choose the best unit among the basic ones */
  for(i = 0; i < num_role_units(L_BARBARIAN_BUILD); i++) {
    Unit_Type_id iunit = get_role_unit(L_BARBARIAN_BUILD, i);

    if (get_unit_type(iunit)->attack_strength > bestattack) {
      bestunit = iunit;
      bestattack = get_unit_type(iunit)->attack_strength;
    }
  }

  /* Choose among those made available through other civ's research */
  for(i = 0; i < num_role_units(L_BARBARIAN_BUILD_TECH); i++) {
    Unit_Type_id iunit = get_role_unit(L_BARBARIAN_BUILD_TECH, i);

    if (game.info.global_advances[get_unit_type(iunit)->tech_requirement] != 0
        && get_unit_type(iunit)->attack_strength > bestattack) {
      bestunit = iunit;
      bestattack = get_unit_type(iunit)->attack_strength;
    }
  }

  /* If found anything, put it into the choice */
  if (bestunit != -1) {
    choice->choice = bestunit;
    /* FIXME: 101 is the "overriding military emergency" indicator */
    choice->want   = 101;
    choice->type   = CT_ATTACKER;
  } else {
    freelog(LOG_VERBOSE, "Barbarians don't know what to build!");
  }
}

/**************************************************************************
  Chooses what the city will build.  Is called after the military advisor
  put it's choice into pcity->u.server.ai.choice and "settler advisor" put
  settler want into pcity->common.founder_*.

  Note that AI cheats -- it suffers no penalty for switching from unit to
  improvement, etc.
**************************************************************************/
static void ai_city_choose_build(player_t *pplayer, struct city_s *pcity)
{
  struct ai_choice newchoice;

  init_choice(&newchoice);

  if (ai_handicap(pplayer, H_AWAY)
      && city_built_last_turn(pcity)
      && pcity->u.server.ai.urgency == 0) {
    /* Don't change existing productions unless we have to. */
    return;
  }

  if( is_barbarian(pplayer) ) {
    ai_barbarian_choose_build(pplayer, &(pcity->u.server.ai.choice));
  } else {
    /* FIXME: 101 is the "overriding military emergency" indicator */
    if (pcity->u.server.ai.choice.want <= 100 || pcity->u.server.ai.urgency == 0) {
      domestic_advisor_choose_build(pplayer, pcity, &newchoice);
      copy_if_better_choice(&newchoice, &(pcity->u.server.ai.choice));
    }
  }

  /* Fallbacks */
  if (pcity->u.server.ai.choice.want == 0) {
    /* Fallbacks do happen with techlevel 0, which is now default. -- Per */
    CITY_LOG(LOG_VERBOSE, pcity, "Falling back - didn't want to build soldiers,"
             " settlers, or buildings");
    pcity->u.server.ai.choice.want = 1;
    if (best_role_unit(pcity, F_TRADE_ROUTE) != U_LAST) {
      pcity->u.server.ai.choice.choice = best_role_unit(pcity, F_TRADE_ROUTE);
      pcity->u.server.ai.choice.type = CT_NONMIL;
    } else if (can_build_improvement(pcity,
                                     game.ruleset_control.default_building)) {
      pcity->u.server.ai.choice.choice = game.ruleset_control.default_building;
      pcity->u.server.ai.choice.type = CT_BUILDING;
    } else if (best_role_unit(pcity, F_SETTLERS) != U_LAST) {
      pcity->u.server.ai.choice.choice = best_role_unit(pcity, F_SETTLERS);
      pcity->u.server.ai.choice.type = CT_NONMIL;
    } else {
      CITY_LOG(LOG_VERBOSE, pcity, "Cannot even build a fallback "
               "(caravan/coinage/settlers). Fix the ruleset!");
      pcity->u.server.ai.choice.want = 0;
    }
  }

  if (pcity->u.server.ai.choice.want != 0) {
    ASSERT_REAL_CHOICE_TYPE(pcity->u.server.ai.choice.type);

    CITY_LOG(LOG_DEBUG, pcity, "wants %s with desire %d.",
             (is_unit_choice_type((choice_type)pcity->u.server.ai.choice.type) ?
              unit_name(pcity->u.server.ai.choice.choice) :
              get_improvement_name(pcity->u.server.ai.choice.choice)),
             pcity->u.server.ai.choice.want);

    if (!pcity->common.is_building_unit && is_wonder(pcity->common.currently_building)
        && (is_unit_choice_type((choice_type)pcity->u.server.ai.choice.type)
            || pcity->u.server.ai.choice.choice != pcity->common.currently_building))
      notify_player_ex(NULL, pcity->common.tile, E_WONDER_STOPPED,
                       _("Game: The %s have stopped building The %s in %s."),
                       get_nation_name_plural(pplayer->nation),
                       get_impr_name_ex(pcity, pcity->common.currently_building),
                       pcity->common.name);

    if (pcity->u.server.ai.choice.type == CT_BUILDING
        && is_wonder(pcity->u.server.ai.choice.choice)
        && (pcity->common.is_building_unit
            || pcity->common.currently_building != pcity->u.server.ai.choice.choice)) {
      notify_player_ex(NULL, pcity->common.tile, E_WONDER_STARTED,
                       _("Game: The %s have started building The %s in %s."),
                       get_nation_name_plural(city_owner(pcity)->nation),
                       get_impr_name_ex(pcity, pcity->u.server.ai.choice.choice),
                       pcity->common.name);
      pcity->common.currently_building = pcity->u.server.ai.choice.choice;
      pcity->common.is_building_unit = is_unit_choice_type((choice_type)pcity->u.server.ai.choice.type);

      /* Help other cities to send caravans to us */
      generate_warmap(pcity, NULL);
      establish_city_distances(pplayer, pcity);
    } else {
      pcity->common.currently_building = pcity->u.server.ai.choice.choice;
      pcity->common.is_building_unit   = is_unit_choice_type((choice_type)pcity->u.server.ai.choice.type);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
static void try_to_sell_stuff(player_t *pplayer, struct city_s *pcity)
{
  impr_type_iterate(id) {
    if (can_sell_building(pcity, id)
        && !building_has_effect(id, EFFECT_TYPE_LAND_DEFEND)) {
/* selling walls to buy defenders is counterproductive -- Syela */
      really_handle_city_sell(pplayer, pcity, id);
      break;
    }
  } impr_type_iterate_end;
}

/**************************************************************************
  Increase maxbuycost.  This variable indicates (via ai_gold_reserve) to
  the tax selection code how much money do we need for buying stuff.
**************************************************************************/
static void increase_maxbuycost(player_t *pplayer, int new_value)
{
  pplayer->ai.maxbuycost = MAX(pplayer->ai.maxbuycost, new_value);
}

/**************************************************************************
  Try to upgrade a city's units. limit is the last amount of gold we can
  end up with after the upgrade. military is if we want to upgrade non-
  military or military units.
**************************************************************************/
static void ai_upgrade_units(struct city_s *pcity, int limit, bool military)
{
  player_t *pplayer = city_owner(pcity);
  unit_list_iterate(pcity->common.tile->units, punit) {
    int id = can_upgrade_unittype(pplayer, punit->type);
    if (military && (!is_military_unit(punit) || !is_ground_unit(punit))) {
      /* Only upgrade military units this round */
      continue;
    } else if (!military && is_military_unit(punit)
               && unit_type(punit)->transport_capacity == 0) {
      /* Only civilians or tranports this round */
      continue;
    }
    if (id >= 0) {
      int cost = unit_upgrade_price(pplayer, punit->type, id);
      int real_limit = limit;
      /* Triremes are DANGEROUS!! We'll do anything to upgrade 'em. */
      if (unit_flag(punit, F_TRIREME)) {
        real_limit = pplayer->ai.est_upkeep;
      }
      if (pplayer->economic.gold - cost > real_limit) {
        CITY_LOG(LOG_BUY, pcity, "Upgraded %s to %s for %d (%s)",
                 unit_type(punit)->name, unit_types[id].name, cost,
                 military ? "military" : "civilian");
        handle_unit_upgrade(city_owner(pcity), punit->id);
      } else {
        increase_maxbuycost(pplayer, cost);
      }
    }
  } unit_list_iterate_end;
}

/**************************************************************************
  Buy and upgrade stuff!
**************************************************************************/
static void ai_spend_gold(player_t *pplayer)
{
  struct ai_choice bestchoice;
  int cached_limit = ai_gold_reserve(pplayer);

  /* Disband explorers that are at home but don't serve a purpose.
   * FIXME: This is a hack, and should be removed once we
   * learn how to ferry explorers to new land. */
  city_list_iterate(pplayer->cities, pcity) {
    tile_t *ptile = pcity->common.tile;
    unit_list_iterate_safe(ptile->units, punit) {
      if (unit_has_role(punit->type, L_EXPLORER)
          && pcity->common.id == punit->homecity
          && pcity->u.server.ai.urgency == 0) {
        CITY_LOG(LOG_BUY, pcity, "disbanding %s to increase production",
                 unit_name(punit->type));
        handle_unit_disband(pplayer,punit->id);
      }
    } unit_list_iterate_safe_end;
  } city_list_iterate_end;

  do {
    int limit = cached_limit; /* cached_limit is our gold reserve */
    struct city_s *pcity = NULL;
    bool expensive; /* don't buy when it costs x2 unless we must */
    int buycost;

    /* Find highest wanted item on the buy list */
    init_choice(&bestchoice);
    city_list_iterate(pplayer->cities, acity) {
      if (acity->u.server.ai.choice.want > bestchoice.want && ai_fuzzy(pplayer, TRUE)) {
        bestchoice.choice = acity->u.server.ai.choice.choice;
        bestchoice.want = acity->u.server.ai.choice.want;
        bestchoice.type = acity->u.server.ai.choice.type;
        pcity = acity;
      }
    } city_list_iterate_end;

    /* We found nothing, so we're done */
    if (bestchoice.want == 0) {
      break;
    }

    /* Not dealing with this city a second time */
    pcity->u.server.ai.choice.want = 0;

    ASSERT_REAL_CHOICE_TYPE(bestchoice.type);

    /* Try upgrade units at danger location (high want is usually danger) */
    if (pcity->u.server.ai.urgency > 1) {
      if (bestchoice.type == CT_BUILDING && is_wonder(bestchoice.choice)) {
        CITY_LOG(LOG_BUY, pcity, "Wonder being built in dangerous position!");
      } else {
        /* If we have urgent want, spend more */
        int upgrade_limit = limit;
        if (pcity->u.server.ai.urgency > 1) {
          upgrade_limit = pplayer->ai.est_upkeep;
        }
        /* Upgrade only military units now */
        ai_upgrade_units(pcity, upgrade_limit, TRUE);
      }
    }

    if (pcity->common.anarchy != 0 && bestchoice.type != CT_BUILDING) {
      continue; /* Nothing we can do */
    }

    /* Cost to complete production */
    buycost = city_buy_cost(pcity);

    if (buycost <= 0) {
      continue; /* Already completed */
    }

    if (bestchoice.type != CT_BUILDING
        && unit_type_flag(bestchoice.choice, F_CITIES)) {
      if (get_city_bonus(pcity, EFFECT_TYPE_GROWTH_FOOD) == 0
          && pcity->common.pop_size == 1
          && city_granary_size(pcity->common.pop_size)
             > pcity->common.food_stock + pcity->common.food_surplus) {
        /* Don't buy settlers in size 1 cities unless we grow next turn */
        continue;
      } else if (city_list_size(pplayer->cities) > 6) {
          /* Don't waste precious money buying settlers late game
           * since this raises taxes, and we want science. Adjust this
           * again when our tax algorithm is smarter. */
          continue;
      }
    } else {
      /* We are not a settler. Therefore we increase the cash need we
       * balance our buy desire with to keep cash at hand for emergencies
       * and for upgrades */
      limit *= 2;
    }

    /* It costs x2 to buy something with no shields contributed */
    expensive = (pcity->common.shield_stock == 0)
                || (pplayer->economic.gold - buycost < limit);

    if (bestchoice.type == CT_ATTACKER
        && buycost > unit_build_shield_cost(bestchoice.choice) * 2) {
       /* Too expensive for an offensive unit */
       continue;
    }

    /* FIXME: Here Syela wanted some code to check if
     * pcity was doomed, and we should therefore attempt
     * to sell everything in it of non-military value */

    if (pplayer->economic.gold - pplayer->ai.est_upkeep >= buycost
        && (!expensive
            || (pcity->u.server.ai.grave_danger != 0 && assess_defense(pcity) == 0)
            || (bestchoice.want > 200 && pcity->u.server.ai.urgency > 1))) {
      /* Buy stuff */
      CITY_LOG(LOG_BUY, pcity, "Crash buy of %s for %d (want %d)",
               bestchoice.type != CT_BUILDING ? unit_name(bestchoice.choice)
               : get_improvement_name(bestchoice.choice), buycost,
               bestchoice.want);
      really_handle_city_buy(pplayer, pcity);
    } else if (pcity->u.server.ai.grave_danger != 0
               && bestchoice.type == CT_DEFENDER
               && assess_defense(pcity) == 0) {
      /* We have no gold but MUST have a defender */
      CITY_LOG(LOG_BUY, pcity, "must have %s but can't afford it (%d < %d)!",
               unit_name(bestchoice.choice), pplayer->economic.gold, buycost);
      try_to_sell_stuff(pplayer, pcity);
      if (pplayer->economic.gold - pplayer->ai.est_upkeep >= buycost) {
        CITY_LOG(LOG_BUY, pcity, "now we can afford it (sold something)");
        really_handle_city_buy(pplayer, pcity);
      }
      increase_maxbuycost(pplayer, buycost);
    }
  } while (TRUE);

  /* Civilian upgrades now */
  city_list_iterate(pplayer->cities, pcity) {
    ai_upgrade_units(pcity, cached_limit, FALSE);
  } city_list_iterate_end;

  freelog(LOG_BUY, "%s wants to keep %d in reserve (tax factor %d)",
          pplayer->name, cached_limit, pplayer->ai.maxbuycost);
}

/**************************************************************************
  One of the top level AI functions.  It does (by calling other functions):
  worker allocations,
  build choices,
  extra gold spending.
**************************************************************************/
void ai_manage_cities(player_t *pplayer)
{
  pplayer->ai.maxbuycost = 0;

  city_list_iterate(pplayer->cities, pcity) {
    if (CITY_EMERGENCY(pcity)) {
      auto_arrange_workers(pcity); /* this usually helps */
    }
    if (CITY_EMERGENCY(pcity)) {
      /* Fix critical shortages or unhappiness */
      resolve_city_emergency(pplayer, pcity);
    }
    ai_sell_obsolete_buildings(pcity);
    sync_cities();
  } city_list_iterate_end;

  ai_manage_buildings(pplayer);

  /* Initialize the infrastructure cache, which is used shortly. */
  initialize_infrastructure_cache(pplayer);
  city_list_iterate(pplayer->cities, pcity) {
    /* Note that this function mungs the seamap, but we don't care */
    military_advisor_choose_build(pplayer, pcity, &pcity->u.server.ai.choice);
    /* because establish_city_distances doesn't need the seamap
     * it determines downtown and distance_to_wondercity,
     * which ai_city_choose_build will need */
    establish_city_distances(pplayer, pcity);
    /* Will record its findings in pcity->common.settler_want */
    contemplate_terrain_improvements(pcity);

    if (pcity->u.server.ai.next_founder_want_recalc <= game.info.turn) {
      /* Will record its findings in pcity->common.founder_want */
      contemplate_new_city(pcity);
      /* Avoid recalculating all the time.. */
      pcity->u.server.ai.next_founder_want_recalc =
        game.info.turn + myrand(RECALC_SPEED) + RECALC_SPEED;
    }
  } city_list_iterate_end;

  city_list_iterate(pplayer->cities, pcity) {
    ai_city_choose_build(pplayer, pcity);
  } city_list_iterate_end;

  ai_spend_gold(pplayer);
}

/**************************************************************************
...
**************************************************************************/
static bool building_unwanted(player_t *plr, Impr_Type_id i)
{
  return (ai_wants_no_science(plr)
          && building_has_effect(i, EFFECT_TYPE_SCIENCE_BONUS));
}

/**************************************************************************
  Sell an obsolete building if there are any in the city.
**************************************************************************/
static void ai_sell_obsolete_buildings(struct city_s *pcity)
{
  player_t *pplayer = city_owner(pcity);

  built_impr_iterate(pcity, i) {
    if(!is_wonder(i)
       && !building_has_effect(i, EFFECT_TYPE_LAND_DEFEND)
              /* selling city walls is really, really dumb -- Syela */
       && (is_building_replaced(pcity, i)
           || building_unwanted(city_owner(pcity), i))) {
      do_sell_building(pplayer, pcity, i);
      notify_player_ex(pplayer, pcity->common.tile, E_IMP_SOLD,
                       _("Game: %s is selling %s (not needed) for %d."),
                       pcity->common.name, get_improvement_name(i),
                       impr_sell_gold(i));
      return; /* max 1 building each turn */
    }
  } built_impr_iterate_end;
}

/**************************************************************************
  This function tries desperately to save a city from going under by
  revolt or starvation of food or resources. We do this by taking
  over resources held by nearby cities and disbanding units.

  TODO: Try to move units into friendly cities to reduce unhappiness
  instead of disbanding. Also rather starve city than keep it in
  revolt, as long as we don't lose settlers.

  TODO: Make function that tries to save units by moving them into
  cities that can upkeep them and change homecity rather than just
  disband. This means we'll have to move this function to beginning
  of AI turn sequence (before moving units).

  "I don't care how slow this is; it will very rarely be used." -- Syela

  Syela is wrong. It happens quite too often, mostly due to unhappiness.
  Also, most of the time we are unable to resolve the situation.
**************************************************************************/
static void resolve_city_emergency(player_t *pplayer, struct city_s *pcity)
#define LOG_EMERGENCY LOG_DEBUG
{
  struct city_list *minilist = city_list_new();

  freelog(LOG_EMERGENCY,
          "Emergency in %s (%s, angry%d, unhap%d food%d, prod%d)",
          pcity->common.name, city_unhappy(pcity) ? "unhappy" : "content",
          pcity->common.people_angry[4], pcity->common.people_unhappy[4],
          pcity->common.food_surplus, pcity->common.shield_surplus);

  map_city_radius_iterate(pcity->common.tile, ptile) {
    struct city_s *acity = ptile->worked;
    int city_map_x, city_map_y;
#ifndef NDEBUG
    bool is_valid;
#endif

    if (acity
        && acity != pcity
        && acity->common.owner == pcity->common.owner)
    {
      if (same_pos(acity->common.tile, ptile)) {
        /* can't stop working city center */
        continue;
      }
      freelog(LOG_DEBUG, "%s taking over %s's square in (%d, %d)",
              pcity->common.name, acity->common.name, ptile->x, ptile->y);
#ifdef NDEBUG
      map_to_city_map(&city_map_x, &city_map_y, acity, ptile);
#else
      is_valid = map_to_city_map(&city_map_x, &city_map_y, acity, ptile);
      assert(is_valid);
#endif
      server_remove_worker_city(acity, city_map_x, city_map_y);
      acity->common.specialists[SP_ELVIS]++;
      if (!city_list_find_id(minilist, acity->common.id)) {
        city_list_prepend(minilist, acity);
      }
    }
  } map_city_radius_iterate_end;
  auto_arrange_workers(pcity);

  if (!CITY_EMERGENCY(pcity)) {
    freelog(LOG_EMERGENCY, "Emergency in %s resolved", pcity->common.name);
    goto cleanup;
  }

  unit_list_iterate_safe(pcity->common.units_supported, punit) {
    if (city_unhappy(pcity)
        && punit->unhappiness != 0
        && punit->ai.passenger == 0) {
      UNIT_LOG(LOG_EMERGENCY, punit, "is causing unrest, disbanded");
      handle_unit_disband(pplayer, punit->id);
      city_refresh(pcity);
    }
  } unit_list_iterate_safe_end;

  if (CITY_EMERGENCY(pcity)) {
    freelog(LOG_EMERGENCY, "Emergency in %s remains unresolved",
            pcity->common.name);
  } else {
    freelog(LOG_EMERGENCY,
            "Emergency in %s resolved by disbanding unit(s)", pcity->common.name);
  }

  cleanup:
  city_list_iterate(minilist, acity) {
    /* otherwise food total and stuff was wrong. -- Syela */
    city_refresh(acity);
    auto_arrange_workers(pcity);
  } city_list_iterate_end;

  city_list_free(minilist);

  sync_cities();
}
#undef LOG_EMERGENCY
