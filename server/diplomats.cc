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

#include <stdio.h>

#include "wc_intl.hh"
#include "log.hh"
#include "rand.hh"

#include "events.hh"
#include "game.hh"
#include "government.hh"
#include "player.hh"

#include "citytools.hh"
#include "cityturn.hh"
#include "diplhand.hh"
#include "gamelog.hh"
#include "gotohand.hh"
#include "plrhand.hh"
#include "unithand.hh"
#include "unittools.hh"

#include "diplomats.hh"


/****************************************************************************/

static void diplomat_charge_movement(unit_t *pdiplomat,
                                     tile_t *ptile);
static bool diplomat_success_vs_defender(unit_t *patt, unit_t *pdef,
                                         tile_t *pdefender_tile);
static bool diplomat_infiltrate_tile(player_t *pplayer, player_t *cplayer,
                                     unit_t *pdiplomat, tile_t *ptile);
static void diplomat_escape(player_t *pplayer, unit_t *pdiplomat,
                            const city_t *pcity);
static void maybe_cause_incident(enum diplomat_actions action, player_t *offender,
                                 unit_t *victim_unit, city_t *victim_city);

/******************************************************************************
  Poison a city's water supply.

  - Only a Spy can poison a city's water supply.
  - Only allowed against players you are at war with.

  - Check for infiltration success.  Our poisoner may not survive this.
  - Only cities of size greater than one may be poisoned.
  - If successful, reduces population by one point.

  - The poisoner may be captured and executed, or escape to its home town.
****************************************************************************/
void spy_poison(player_t *pplayer, unit_t *pdiplomat,
                city_t *pcity)
{
  player_t *cplayer;

  /* Fetch target city's player.  Sanity checks. */
  if (!pcity)
    return;
  cplayer = city_owner(pcity);
  if (!cplayer || !pplayers_at_war(pplayer, cplayer))
    return;

  freelog (LOG_DEBUG, "poison: unit: %d", pdiplomat->id);

  /* If not a Spy, can't poison. */
  if (!unit_flag (pdiplomat, F_SPY))
    return;

  /* Check if the Diplomat/Spy succeeds against defending Diplomats/Spies. */
  if (!diplomat_infiltrate_tile(pplayer, cplayer, pdiplomat,
                                pcity->common.tile)) {
    return;
  }

  freelog (LOG_DEBUG, "poison: infiltrated");

  /* If city is too small, can't poison. */
  if (pcity->common.pop_size < 2) {
    notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                     _("Game: Your %s could not poison the water"
                       " supply in %s."),
                     unit_name(pdiplomat->type), pcity->common.name);
    freelog (LOG_DEBUG, "poison: target city too small");
    return;
  }

  freelog (LOG_DEBUG, "poison: succeeded");

  /* Poison people! */
  city_reduce_size(pcity, 1);

  /* Notify everybody involved. */
  notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_POISON,
                   _("Game: Your %s poisoned the water supply of %s."),
                   unit_name(pdiplomat->type), pcity->common.name);
  notify_player_ex(cplayer, pcity->common.tile, E_ENEMY_DIPLOMAT_POISON,
                   _("Game: %s is suspected of poisoning the water supply"
                     " of %s."), pplayer->name, pcity->common.name);

  /* Update clients. */
  city_refresh (pcity);
  send_city_info(NULL, pcity);

  /* this may cause a diplomatic incident */
  maybe_cause_incident(SPY_POISON, pplayer, NULL, pcity);

  /* Now lets see if the spy survives. */
  diplomat_escape(pplayer, pdiplomat, pcity);
}

/******************************************************************************
  Investigate a city.

  - Either a Diplomat or Spy can investigate a city.
  - Allowed against all players.

  - It costs some minimal movement to investigate a city.

  - Diplomats die after investigation.
  - Spies always survive.  There is no risk.
****************************************************************************/
void diplomat_investigate(player_t *pplayer, unit_t *pdiplomat,
                          city_t *pcity)
{
  player_t *cplayer;
  bool first_packet;
  struct packet_unit_short_info unit_packet;
  struct packet_city_info city_packet;

  /* Fetch target city's player.  Sanity checks. */
  if (!pcity)
    return;
  cplayer = city_owner (pcity);
  if ((cplayer == pplayer) || !cplayer)
    return;

  freelog (LOG_DEBUG, "investigate: unit: %d", pdiplomat->id);

  /* Do It... */
  update_dumb_city(pplayer, pcity);
  /* Special case for a diplomat/spy investigating a city:
     The investigator needs to know the supported and present
     units of a city, whether or not they are fogged. So, we
     send a list of them all before sending the city info.
     As this is a special case we bypass send_unit_info. */
  first_packet = TRUE;
  unit_list_iterate(pcity->common.units_supported, punit) {
    package_short_unit(punit, &unit_packet,
                       UNIT_INFO_CITY_SUPPORTED, pcity->common.id, first_packet);
    lsend_packet_unit_short_info(pplayer->connections, &unit_packet);
    first_packet = FALSE;
  } unit_list_iterate_end;
  unit_list_iterate((pcity->common.tile)->units, punit) {
    package_short_unit(punit, &unit_packet,
                       UNIT_INFO_CITY_PRESENT, pcity->common.id, first_packet);
    lsend_packet_unit_short_info(pplayer->connections, &unit_packet);
    first_packet = FALSE;
  } unit_list_iterate_end;
  /* Send city info to investigator's player.
     As this is a special case we bypass send_city_info. */
  package_city(pcity, &city_packet, TRUE);
  lsend_packet_city_info(pplayer->connections, &city_packet);

  /* Charge a nominal amount of movement for this. */
  (pdiplomat->moves_left)--;
  if (pdiplomat->moves_left < 0) {
    pdiplomat->moves_left = 0;
  }

  /* this may cause a diplomatic incident */
  maybe_cause_incident(DIPLOMAT_INVESTIGATE, pplayer, NULL, pcity);

  /* Spies always survive. Diplomats never do. */
  if (!unit_flag (pdiplomat, F_SPY)) {
    wipe_unit(pdiplomat);
  } else {
    send_unit_info (pplayer, pdiplomat);
  }
}

/******************************************************************************
  Get list of improvements from city (for purposes of sabotage).

  - Only a Spy can get a a city's sabotage list.

  - Always successful; returns list.

  - Spies always survive.

  Only send back to the originating connection, if there is one. (?)
****************************************************************************/
void spy_get_sabotage_list(player_t *pplayer, unit_t *pdiplomat,
                           city_t *pcity)
{
  struct packet_city_sabotage_list packet;
  char *p;

  /* Send city improvements info to player. */
  p = packet.improvements;

  impr_type_iterate(i) {
    *p++=city_got_building(pcity,i)?'1':'0';
  } impr_type_iterate_end;

  *p='\0';
  packet.diplomat_id = pdiplomat->id;
  packet.city_id = pcity->common.id;
  lsend_packet_city_sabotage_list(player_reply_dest(pplayer), &packet);

  /* this may cause a diplomatic incident */
  maybe_cause_incident(SPY_GET_SABOTAGE_LIST, pplayer, NULL, pcity);
}

/******************************************************************************
  Establish an embassy.

  - Either a Diplomat or Spy can establish an embassy.

  - Barbarians always execute ambassadors.
  - Otherwise, the embassy is created.
  - It costs some minimal movement to establish an embassy.

  - Diplomats are consumed in creation of embassy.
  - Spies always survive.
****************************************************************************/
void diplomat_embassy(player_t *pplayer, unit_t *pdiplomat,
                      city_t *pcity)
{
  player_t *cplayer;

  /* Fetch target city's player.  Sanity checks. */
  if (!pcity)
    return;
  cplayer = city_owner (pcity);
  if ((cplayer == pplayer) || !cplayer)
    return;

  freelog (LOG_DEBUG, "embassy: unit: %d", pdiplomat->id);

  /* Check for Barbarian response. */
  if (is_barbarian (cplayer)) {
    notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                     _("Game: Your %s was executed in %s by primitive %s."),
                     unit_name(pdiplomat->type),
                     pcity->common.name, get_nation_name_plural(cplayer->nation));
    wipe_unit(pdiplomat);
    return;
  }

  freelog (LOG_DEBUG, "embassy: succeeded");

  establish_embassy(pplayer, cplayer);

  /* Notify everybody involved. */
  notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_EMBASSY,
                   _("Game: You have established an embassy in %s."),
                   pcity->common.name);
  notify_player_ex(cplayer, pcity->common.tile, E_ENEMY_DIPLOMAT_EMBASSY,
                   _("Game: The %s have established an embassy in %s."),
                   get_nation_name_plural(pplayer->nation), pcity->common.name);
  gamelog(GAMELOG_EMBASSY, pplayer, pcity);

  /* Charge a nominal amount of movement for this. */
  (pdiplomat->moves_left)--;
  if (pdiplomat->moves_left < 0) {
    pdiplomat->moves_left = 0;
  }

  /* this may cause a diplomatic incident */
  maybe_cause_incident(DIPLOMAT_EMBASSY, pplayer, NULL, pcity);

  /* Spies always survive. Diplomats never do. */
  if (!unit_flag (pdiplomat, F_SPY)) {
    wipe_unit(pdiplomat);
  } else {
    send_unit_info (pplayer, pdiplomat);
  }
}

/******************************************************************************
  Sabotage an enemy unit.

  - Only a Spy can sabotage an enemy unit.
  - Only allowed against players you are at war with.

  - Can't sabotage a unit if:
    - It has only one hit point left.
    - It's not the only unit on the square
      (this is handled outside this function).
  - If successful, reduces hit points by half of those remaining.

  - The saboteur may be captured and executed, or escape to its home town.
****************************************************************************/
void spy_sabotage_unit(player_t *pplayer, unit_t *pdiplomat,
                       unit_t *pvictim)
{
  player_t *uplayer;

  /* Fetch target unit's player.  Sanity checks. */
  if (!pvictim)
    return;
  uplayer = unit_owner(pvictim);
  if (!uplayer || pplayers_allied(pplayer, uplayer))
    return;

  freelog (LOG_DEBUG, "sabotage-unit: unit: %d", pdiplomat->id);

  /* If not a Spy, can't sabotage unit. */
  if (!unit_flag (pdiplomat, F_SPY))
    return;

  /* If unit has too few hp, can't sabotage. */
  if (pvictim->hp < 2) {
    notify_player_ex(pplayer, pvictim->tile, E_MY_DIPLOMAT_FAILED,
                     _("Game: Your %s could not sabotage %s's %s."),
                     unit_name(pdiplomat->type),
                     unit_owner(pvictim)->name, unit_name(pvictim->type));
    freelog (LOG_DEBUG, "sabotage-unit: unit has too few hit points");
    return;
  }

  /* Check if the Diplomat/Spy succeeds against defending Diplomats/Spies. */
  if (!diplomat_infiltrate_tile(pplayer, uplayer, pdiplomat,
                                pvictim->tile)) {
    return;
  }

  freelog (LOG_DEBUG, "sabotage-unit: succeeded");

  /* Sabotage the unit by removing half its remaining hit points. */
  pvictim->hp /= 2;
  send_unit_info(NULL, pvictim);

  /* Notify everybody involved. */
  notify_player_ex(pplayer, pvictim->tile, E_MY_DIPLOMAT_SABOTAGE,
                   _("Game: Your %s succeeded in sabotaging %s's %s."),
                   unit_name(pdiplomat->type),
                   unit_owner(pvictim)->name, unit_name(pvictim->type));
  notify_player_ex(uplayer, pvictim->tile,
                   E_ENEMY_DIPLOMAT_SABOTAGE,
                   _("Game: Your %s was sabotaged by %s!"),
                   unit_name(pvictim->type), pplayer->name);

  /* this may cause a diplomatic incident */
  maybe_cause_incident(SPY_SABOTAGE_UNIT, pplayer, pvictim, NULL);

  /* Now lets see if the spy survives. */
  diplomat_escape(pplayer, pdiplomat, NULL);
}

/******************************************************************************
  Bribe an enemy unit.

  - Either a Diplomat or Spy can bribe an other players unit.

  - Can't bribe a unit if:
    - Owner runs an unbribable government (e.g., democracy).
    - Player doesn't have enough gold.
    - It's not the only unit on the square
      (this is handled outside this function).
    - You are allied with the unit owner.
  - Otherwise, the unit will be bribed.

  - A successful briber will try to move onto the victim's square.
****************************************************************************/
void diplomat_bribe(player_t *pplayer, unit_t *pdiplomat,
                    tile_t *ptile)
{
  player_t *uplayer = NULL;
  int diplomat_id;
  bool vet = FALSE;
  unit_t *gained_unit = NULL;
  int total_cost = 0;

  /* do some checks for all units on tile first */
  unit_list_iterate_safe(ptile->units, pvictim) {
    /* Fetch target unit's player.  Sanity checks. */
    if (!pvictim) {
      return;
    }

    uplayer = unit_owner(pvictim);

    /* We might make it allowable in peace with a liss of reputaion */
    if (!uplayer || pplayers_allied(pplayer, uplayer)) {
      return;
    }

    /* Calculate bribe cost. */
    total_cost += unit_bribe_cost(pvictim);

    /* Check for unit from a bribable government. */
    if (government_has_flag(get_gov_pplayer(unit_owner(pvictim)),
                            G_UNBRIBABLE)) {
      notify_player_ex(pplayer, pdiplomat->tile, E_MY_DIPLOMAT_FAILED,
                       _("Game: You can't bribe a unit from this nation."));
      freelog(LOG_DEBUG, "bribe-unit: unit's government is unbribable");
      return;
    }

    if (unit_flag(pvictim, F_UNBRIBABLE)) {
      notify_player_ex(pplayer, pdiplomat->tile, E_MY_DIPLOMAT_FAILED,
                       _("Game: You cannot bribe %s!"),
                       unit_name(pvictim->type));
      return;
    }
  } unit_list_iterate_safe_end;

  /* If player doesn't have enough gold, can't bribe. */
  if (pplayer->economic.gold < total_cost) {
    notify_player_ex(pplayer, pdiplomat->tile,
                     E_MY_DIPLOMAT_FAILED,
                     _("Game: You don't have enough gold to"
                       " bribe unit(s). Cost was %d gold."), total_cost);
    freelog(LOG_DEBUG, "bribe-unit: not enough gold");
    return;
  }

  /* This costs! */
  pplayer->economic.gold -= total_cost;

  /* Check if the Diplomat/Spy succeeds with his/her task. */
  if (myrand(100) >= game.server.diplbribechance) {
    notify_player_ex(pplayer, ptile, E_MY_DIPLOMAT_FAILED,
                     _("Game: Your %s was caught in the attempt"
                       " of bribing enemy unit(s) along with %d gold!"),
                     unit_name(pdiplomat->type), total_cost);
    notify_player_ex(uplayer, ptile, E_ENEMY_DIPLOMAT_FAILED,
                     _("Game: You caught %s %s attempting"
                       " to bribe unit(s)! %d gold recovered."),
                     get_nation_name(pplayer->nation),
                     unit_name(pdiplomat->type), total_cost);

    uplayer->economic.gold += total_cost; /* defender gets the gold */

    wipe_unit(pdiplomat);

    /* Update the players gold in the client */
    send_player_info(pplayer, pplayer);
    send_player_info(uplayer, uplayer);

    return;
  }

  freelog(LOG_DEBUG, "bribe-unit: succeeded");


  unit_list_iterate_safe(ptile->units, pvictim) {
    /* This may cause a diplomatic incident */
    maybe_cause_incident(DIPLOMAT_BRIBE, pplayer, pvictim, NULL);

    /* Convert the unit to your cause. Fog is lifted in the create
     * algorithm. */
    gained_unit = create_unit_full(pplayer, pvictim->tile,
                                   pvictim->type, pvictim->veteran,
                                   pdiplomat->homecity,
                                   pvictim->moves_left,
                                   pvictim->hp, NULL);

    /* Copy some more unit fields */
    gained_unit->fuel = pvictim->fuel;
    gained_unit->paradropped = pvictim->paradropped;

    /* Inform owner about less than full fuel */
    send_unit_info(pplayer, gained_unit);

    notify_player_ex(uplayer, pvictim->tile, E_ENEMY_DIPLOMAT_BRIBE,
                     _("Game: Your %s was bribed by %s."),
                     unit_name(pvictim->type), pplayer->name);

    /* Be sure to wipe the converted unit! */
    wipe_unit(pvictim);
  } unit_list_iterate_safe_end;

  /* Check if the unit gained veteran level */
  vet = maybe_make_veteran(pdiplomat);

  /* Notify everybody involved. */
  if (vet) {
    notify_player_ex(pplayer, ptile, E_MY_DIPLOMAT_BRIBE,
                     _("Game: Your %s succeeded in bribing unit(s)"
                       " and became more experienced. Cost was %d gold."),
                     unit_name(pdiplomat->type), total_cost);
  } else {
    notify_player_ex(pplayer, ptile, E_MY_DIPLOMAT_BRIBE,
                     _("Game: Your %s succeeded in bribing unit(s)."
                       "Cost was %d gold."), unit_name(pdiplomat->type),
                     total_cost);
  }

  /* Now, try to move the briber onto the victim's square. */
  diplomat_id = pdiplomat->id;
  if (!handle_unit_move_request(pdiplomat, ptile, FALSE, FALSE)) {
    pdiplomat->moves_left = 0;
  }
  if (player_find_unit_by_id(pplayer, diplomat_id)) {
    send_unit_info(pplayer, pdiplomat);
  }

  /* Update clients. */
  send_player_info(pplayer, NULL);
}

/****************************************************************************
  Try to steal a technology from an enemy city.
  If "technology" is game.ruleset_control.num_tech_types, steal a random technology.
  Otherwise, steal the technology whose ID is "technology".
  (Note: Only Spies can select what to steal.)

  - Either a Diplomat or Spy can steal a technology.

  - Check for infiltration success.  Our thief may not survive this.
  - Check for basic success.  Again, our thief may not survive this.
  - If a technology has already been stolen from this city at any time:
    - Diplomats will be caught and executed.
    - Spies will have an increasing chance of being caught and executed.
  - Determine target, given arguments and constraints.
  - Steal technology!

  - The thief may be captured and executed, or escape to its home town.

  FIXME: It should give a loss of reputation to steal from a player you are
  not at war with
****************************************************************************/
void diplomat_get_tech(player_t *pplayer, unit_t *pdiplomat,
                       city_t  *pcity, int technology)
{
  player_t *cplayer;
  int count, /*which,*/ target;

  /* Fetch target civilization's player.  Sanity checks. */
  if (!pcity)
    return;
  cplayer = city_owner (pcity);
  if (!cplayer || (cplayer == pplayer) || pplayers_allied(pplayer, cplayer))
    return;

  freelog (LOG_DEBUG, "steal-tech: unit: %d", pdiplomat->id);

  /* If not a Spy, do something random. */
  if (!unit_flag(pdiplomat, F_SPY)
      || get_invention(pplayer, technology) == TECH_KNOWN
      || get_invention (cplayer, technology) != TECH_KNOWN
      || !tech_is_available(pplayer, technology)) {
    technology = game.ruleset_control.num_tech_types;
  }

  /* Check if the Diplomat/Spy succeeds against defending Diplomats/Spies. */
  if (!diplomat_infiltrate_tile(pplayer, cplayer, pdiplomat,
                                pcity->common.tile)) {
    return;
  }

  freelog (LOG_DEBUG, "steal-tech: infiltrated");

  /* Check if the Diplomat/Spy succeeds with his/her task. */
  /* (Twice as difficult if target is specified.) */
  /* (If already stolen from, impossible for Diplomats and harder for Spies.) */
  if ((pcity->u.server.steal > 0) && (!unit_flag (pdiplomat, F_SPY))) {
    /* Already stolen from: Diplomat always fails! */
    count = 1;
    freelog (LOG_DEBUG, "steal-tech: difficulty: impossible");
  } else {
    /* Determine difficulty. */
    count = 1;
    if (technology < game.ruleset_control.num_tech_types) count++;
    count += pcity->u.server.steal;
    freelog (LOG_DEBUG, "steal-tech: difficulty: %d", count);
    /* Determine success or failure. */
    while (count > 0) {
      if (myrand (100) >= game.server.diplchance) {
        break;
      }
      count--;
    }
  }
  if (count > 0) {
    notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                     _("Game: Your %s was caught in the attempt of"
                       " stealing technology from %s."),
                     unit_name(pdiplomat->type), pcity->common.name);
    notify_player_ex(cplayer, pcity->common.tile, E_ENEMY_DIPLOMAT_FAILED,
                     _("Game: %s's %s failed to steal technology from %s."),
                     pplayer->name, unit_name(pdiplomat->type), pcity->common.name);
    /* this may cause a diplomatic incident */
    maybe_cause_incident(DIPLOMAT_STEAL, pplayer, NULL, pcity);
    wipe_unit(pdiplomat);
    return;
  }

  freelog (LOG_DEBUG, "steal-tech: succeeded");

  /* Examine the civilization for technologies to steal. */
  count = 0;
  tech_type_iterate(index) {
    if (get_invention(pplayer, index) != TECH_KNOWN
        && get_invention(cplayer, index) == TECH_KNOWN
        && tech_is_available(pplayer, index)) {
      count++;
    }
  } tech_type_iterate_end;

  freelog (LOG_DEBUG, "steal-tech: count of technologies: %d", count);

  /* Determine the target (-1 is future tech). */
  if (count == 0) {
    /*
     * Either only future-tech or nothing to steal:
     * If nothing to steal, say so, deduct movement cost and return.
     */
    if (cplayer->future_tech > pplayer->future_tech) {
      target = -1;
      freelog (LOG_DEBUG, "steal-tech: targeted future-tech: %d", target);
    } else {
      notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                       _("Game: No new technology found in %s."),
                       pcity->common.name);
      diplomat_charge_movement (pdiplomat, pcity->common.tile);
      send_unit_info (pplayer, pdiplomat);
      freelog (LOG_DEBUG, "steal-tech: nothing to steal");
      return;
    }
  } else if (technology >= game.ruleset_control.num_tech_types) {
    /* Pick first available tech to steal. */
    target = -1;
    tech_type_iterate(index) {
      if (get_invention(pplayer, index) != TECH_KNOWN
          && get_invention(cplayer, index) == TECH_KNOWN
          && tech_is_available(pplayer, index)) {
          target = index;
          break;
      }
    } tech_type_iterate_end;
    /* This player cannot steal a tech that the target player doesn't own */
    if (target == -1 && cplayer->future_tech <= pplayer->future_tech) {
      freelog(LOG_DEBUG, "steal-tech: random: couldn't find any tech to steal");
      return;
    }
    freelog(LOG_DEBUG, "steal-tech: random: targeted technology: %d (%s)",
            target, get_tech_name(pplayer, target));
  } else {
    /*
     * Told which technology to steal:
     * If not available, say so, deduct movement cost and return.
     */
    if ((get_invention (pplayer, technology) != TECH_KNOWN) &&
        (get_invention (cplayer, technology) == TECH_KNOWN)) {
      target = technology;
      freelog(LOG_DEBUG, "steal-tech: specified target technology: %d (%s)",
              target, get_tech_name(pplayer, target));
    } else {
      notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                       _("Game: Your %s could not find the %s technology"
                         " to steal in %s."),
                       unit_name(pdiplomat->type),
                       get_tech_name(pplayer, technology), pcity->common.name);
      diplomat_charge_movement (pdiplomat, pcity->common.tile);
      send_unit_info (pplayer, pdiplomat);
      freelog(LOG_DEBUG, "steal-tech: target technology not found: %d (%s)",
              technology, get_tech_name(pplayer, technology));
      return;
    }
  }

  /* Now, the fun stuff!  Steal some technology! */
  if (target < 0) {
    /* Steal a future-tech. */

    /* Do it. */
    found_new_future_tech(pplayer);

    /* Report it. */
    notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_THEFT,
                     _("Game: Your %s stole Future Tech. %d from %s."),
                     unit_name(pdiplomat->type),
                     pplayer->future_tech, cplayer->name);
    notify_player_ex(cplayer, pcity->common.tile, E_ENEMY_DIPLOMAT_THEFT,
                     _("Game: Future Tech. %d stolen by %s %s from %s."),
                     pplayer->future_tech, get_nation_name(pplayer->nation),
                     unit_name(pdiplomat->type), pcity->common.name);
    freelog (LOG_DEBUG, "steal-tech: stole future-tech %d",
             pplayer->future_tech);
  } else {
    /* Steal a technology. */

    /* Do it. */
    do_conquer_cost (pplayer);
    found_new_tech (pplayer, target, FALSE, TRUE, A_NONE);
    /* Report it. */
    notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_THEFT,
                     _("Game: Your %s stole %s from %s."),
                     unit_name(pdiplomat->type),
                     get_tech_name(pplayer, target), cplayer->name);
    notify_player_ex(cplayer, pcity->common.tile, E_ENEMY_DIPLOMAT_THEFT,
                     _("Game: %s's %s stole %s from %s."),
                     pplayer->name, unit_name(pdiplomat->type),
                     get_tech_name(cplayer, target), pcity->common.name);
    notify_embassies(pplayer, cplayer,
                     _("Game: The %s have stolen %s from the %s."),
                     get_nation_name_plural(pplayer->nation),
                     get_tech_name(cplayer, target),
                     get_nation_name_plural(cplayer->nation));
    freelog(LOG_DEBUG, "steal-tech: stole %s",
            get_tech_name(cplayer, target));
  }

  gamelog(GAMELOG_TECH, pplayer, cplayer, target, "steal");

  /* Update stealing player's science progress and research fields */
  send_player_info(pplayer, pplayer);

  /* Record the theft. */
  (pcity->u.server.steal)++;

  /* this may cause a diplomatic incident */
  maybe_cause_incident(DIPLOMAT_STEAL, pplayer, NULL, pcity);

  /* Check if a spy survives her mission. Diplomats never do. */
  diplomat_escape(pplayer, pdiplomat, pcity);
}

/**************************************************************************
  Incite a city to disaffect.

  - Either a Diplomat or Spy can incite a city to disaffect.

  - Can't incite a city to disaffect if:
    - Owner runs an unbribable government (e.g., democracy).
    - Player doesn't have enough gold.
    - You are allied with the city owner.
  - Check for infiltration success.  Our provocateur may not survive this.
  - Check for basic success.  Again, our provocateur may not survive this.
  - Otherwise, the city will disaffect:
    - Player gets the city.
    - Player gets certain of the city's present/supported units.
    - Player gets a technology advance, if any were unknown.

  - The provocateur may be captured and executed, or escape to its home town.
**************************************************************************/
void diplomat_incite(player_t *pplayer, unit_t *pdiplomat,
                     city_t *pcity)
{
  player_t *cplayer;
  int revolt_cost;

  /* Fetch target civilization's player.  Sanity checks. */
  if (!pcity) {
    return;
  }

  cplayer = city_owner(pcity);
  if (!cplayer || pplayers_allied(cplayer, pplayer)) {
    return;
  }

  freelog(LOG_DEBUG, "incite: unit: %d", pdiplomat->id);

  /* Check for city from a bribable government. */
  if (government_has_flag(get_gov_pcity(pcity), G_UNBRIBABLE)) {
    notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                     _("Game: You can't subvert a city from this nation."));
    freelog(LOG_DEBUG, "incite: city's government is unbribable");
    return;
  }

  /* See if the city is subvertable. */
  if (get_city_bonus(pcity, EFFECT_TYPE_NO_INCITE) > 0) {
    notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                     _("Game: You can't subvert this city."));
    freelog(LOG_DEBUG, "incite: city is protected");
    return;
  }

  /* Get incite cost. */
  revolt_cost = city_incite_cost(pplayer, pcity);

  /* If player doesn't have enough gold, can't incite a revolt. */
  if (pplayer->economic.gold < revolt_cost) {
    notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                     _("Game: You don't have enough gold to"
                       " subvert %s. Cost was %d gold."), pcity->common.name,
                     revolt_cost);
    freelog(LOG_DEBUG, "incite: not enough gold");
    return;
  }

  /* Check if the Diplomat/Spy succeeds against defending
   * Diplomats/Spies. */
  if (!diplomat_infiltrate_tile(pplayer, cplayer, pdiplomat,
                                pcity->common.tile)) {
    return;
  }

  freelog(LOG_DEBUG, "incite: infiltrated");

  /* Check if the Diplomat/Spy succeeds with his/her task. */
  if (myrand(100) >= game.server.diplincitechance) {
    notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                     _("Game: Your %s was caught in the attempt"
                       " of inciting a revolt along with %d gold!"),
                     unit_name(pdiplomat->type), revolt_cost);
    notify_player_ex(cplayer, pcity->common.tile, E_ENEMY_DIPLOMAT_FAILED,
                     _("Game: You caught %s %s attempting"
                       " to incite a revolt in %s! %d gold recovered."),
                     get_nation_name(pplayer->nation),
                     unit_name(pdiplomat->type), pcity->common.name,
                     revolt_cost);
    pplayer->economic.gold -= revolt_cost;
    cplayer->economic.gold += revolt_cost;  /* defender gets the gold */
    wipe_unit(pdiplomat);
    /* Update the players gold in the client */
    send_player_info(pplayer, pplayer);
    send_player_info(cplayer, cplayer);
    return;
  }

  freelog(LOG_DEBUG, "incite: succeeded");

  /* Subvert the city to your cause... */

  /* City loses some population. */
  if (pcity->common.pop_size > 1) {
    city_reduce_size(pcity, 1);
  }

  /* This costs! */
  pplayer->economic.gold -= revolt_cost;

  /* Notify everybody involved. */
  gamelog(GAMELOG_LOSECITY, cplayer, pplayer, pcity, "incited to revolt");

  notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_INCITE,
                   _("Game: Revolt incited in %s, you now rule the city! "
                     "Cost was %d gold."), pcity->common.name, revolt_cost);
  notify_player_ex(cplayer, pcity->common.tile, E_ENEMY_DIPLOMAT_INCITE,
                   _("Game: %s has revolted, %s influence suspected."),
                   pcity->common.name, get_nation_name(pplayer->nation));

  pcity->common.shield_stock = 0;
  nullify_prechange_production(pcity);

  /* You get a technology advance, too! */
  get_a_tech(pplayer, cplayer);

  /* this may cause a diplomatic incident */
  maybe_cause_incident(DIPLOMAT_INCITE, pplayer, NULL, pcity);

  /* Transfer city and units supported by this city (that
     are within one square of the city) to the new owner.
     Remember that pcity is destroyed as part of the transfer,
     Which is why we do this last */
  transfer_city(pplayer, pcity, 1, TRUE, TRUE, FALSE);

  /* Check if a spy survives her mission. Diplomats never do.
   * _After_ transferring the city, or the city area is first fogged
   * when the diplomat is removed, and then unfogged when the city
   * is transferred. */
  diplomat_escape(pplayer, pdiplomat, pcity);

  /* Update the players gold in the client */
  send_player_info(pplayer, pplayer);
}

/**************************************************************************
  Sabotage enemy city's improvement or production.
  If "improvement" is B_LAST, sabotage a random improvement or production.
  Else, if "improvement" is -1, sabotage current production.
  Otherwise, sabotage the city improvement whose ID is "improvement".
  (Note: Only Spies can select what to sabotage.)

  - Either a Diplomat or Spy can sabotage an enemy city.
  - The players must be at war

  - Check for infiltration success.  Our saboteur may not survive this.
  - Check for basic success.  Again, our saboteur may not survive this.
  - Determine target, given arguments and constraints.
  - If specified, city walls and anything in a capital are 50% likely to fail.
  - Do sabotage!

  - The saboteur may be captured and executed, or escape to its home town.
**************************************************************************/
void diplomat_sabotage(player_t *pplayer, unit_t *pdiplomat,
                       city_t *pcity, Impr_Type_id improvement)
{
  player_t *cplayer;
  int count, which, target;
  const char *prod;
  /* Twice as difficult if target is specified. */
  int success_prob = (improvement >= B_LAST ? game.server.diplchance
                      : game.server.diplchance / 2);

  /* Fetch target city's player.  Sanity checks. */
  if (!pcity)
    return;
  cplayer = city_owner (pcity);
  if (!cplayer || !pplayers_at_war(pplayer, cplayer))
    return;

  freelog (LOG_DEBUG, "sabotage: unit: %d", pdiplomat->id);

  /* If not a Spy, do something random. */
  if (!unit_flag (pdiplomat, F_SPY))
    improvement = B_LAST;

  /* Check if the Diplomat/Spy succeeds against defending Diplomats/Spies. */
  if (!diplomat_infiltrate_tile(pplayer, cplayer, pdiplomat,
                                pcity->common.tile)) {
    return;
  }

  freelog (LOG_DEBUG, "sabotage: infiltrated");

  /* Check if the Diplomat/Spy succeeds with his/her task. */
  if (myrand (100) >= success_prob) {
    notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                     _("Game: Your %s was caught in the attempt"
                       " of industrial sabotage!"),
                     unit_name(pdiplomat->type));
    notify_player_ex(cplayer, pcity->common.tile, E_ENEMY_DIPLOMAT_SABOTAGE,
                     _("Game: You caught %s %s attempting"
                       " sabotage in %s!"),
                     get_nation_name(pplayer->nation),
                     unit_name(pdiplomat->type), pcity->common.name);
    wipe_unit(pdiplomat);
    return;
  }

  freelog (LOG_DEBUG, "sabotage: succeeded");

  /* Examine the city for improvements to sabotage. */
  count = 0;
  built_impr_iterate(pcity, index) {
    if (get_improvement_type(index)->sabotage > 0) {
      count++;
    }
  } built_impr_iterate_end;

  freelog (LOG_DEBUG, "sabotage: count of improvements: %d", count);

  /* Determine the target (-1 is production). */
  if (improvement < 0) {
    /* If told to sabotage production, do so. */
    target = -1;
    freelog (LOG_DEBUG, "sabotage: specified target production: %d", target);
  } else if (improvement >= B_LAST) {
    /*
     * Pick random:
     * 50/50 chance to pick production or some improvement.
     * Won't pick something that doesn't exit.
     * If nothing to do, say so, deduct movement cost and return.
     */
    if (count == 0 && pcity->common.shield_stock == 0) {
      notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                       _("Game: Your %s could not find anything to"
                         " sabotage in %s."), unit_name(pdiplomat->type),
                       pcity->common.name);
      diplomat_charge_movement (pdiplomat, pcity->common.tile);
      send_unit_info (pplayer, pdiplomat);
      freelog (LOG_DEBUG, "sabotage: random: nothing to do");
      return;
    }
    if (count == 0 || myrand (2) == 1) {
      target = -1;
      freelog (LOG_DEBUG, "sabotage: random: targeted production: %d", target);
    } else {
      target = -1;
      which = myrand (count);

      built_impr_iterate(pcity, index) {
        if (get_improvement_type(index)->sabotage > 0) {
          if (which > 0) {
            which--;
          } else {
            target = index;
            break;
          }
        }
      } built_impr_iterate_end;

      freelog (LOG_DEBUG, "sabotage: random: targeted improvement: %d (%s)",
               target, get_improvement_name (target));
    }
  } else {
    /*
     * Told which improvement to pick:
     * If try for wonder or palace, complain, deduct movement cost and return.
     * If not available, say so, deduct movement cost and return.
     */
    if (city_got_building (pcity, improvement)) {
      if (get_improvement_type(improvement)->sabotage > 0) {
        target = improvement;
        freelog (LOG_DEBUG, "sabotage: specified target improvement: %d (%s)",
               target, get_improvement_name (target));
      } else {
        notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                         _("Game: You cannot sabotage a %s!"),
                         improvement_types[improvement].name);
        diplomat_charge_movement (pdiplomat, pcity->common.tile);
        send_unit_info (pplayer, pdiplomat);
        freelog (LOG_DEBUG, "sabotage: disallowed target improvement: %d (%s)",
               improvement, get_improvement_name (improvement));
        return;
      }
    } else {
      notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                       _("Game: Your %s could not find the %s to"
                         " sabotage in %s."), unit_name(pdiplomat->type),
                       get_improvement_name(improvement), pcity->common.name);
      diplomat_charge_movement (pdiplomat, pcity->common.tile);
      send_unit_info (pplayer, pdiplomat);
      freelog (LOG_DEBUG, "sabotage: target improvement not found: %d (%s)",
               improvement, get_improvement_name (improvement));
      return;
    }
  }

  /* Now, the fun stuff!  Do the sabotage! */
  if (target < 0) {
    /* Sabotage current production. */

    /* Do it. */
    pcity->common.shield_stock = 0;
    nullify_prechange_production(pcity); /* Make it impossible to recover */

    /* Report it. */
    if (pcity->common.is_building_unit)
      prod = unit_name (pcity->common.currently_building);
    else
      prod = get_improvement_name (pcity->common.currently_building);
    notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_SABOTAGE,
                     _("Game: Your %s succeeded in destroying"
                       " the production of %s in %s."),
                     unit_name(pdiplomat->type), prod, pcity->common.name);
    notify_player_ex(cplayer, pcity->common.tile, E_ENEMY_DIPLOMAT_SABOTAGE,
                     _("Game: The production of %s was destroyed in %s,"
                       " %s are suspected."), prod, pcity->common.name,
                     get_nation_name_plural(pplayer->nation));
    freelog (LOG_DEBUG, "sabotage: sabotaged production");
  } else {
    int vulnerability;

    /* Sabotage a city improvement. */

    /*
     * One last chance to get caught:
     * If target was specified, and it is in the capital or are
     * City Walls, then there is a 50% chance of getting caught.
     */
    vulnerability = get_improvement_type(target)->sabotage;

    vulnerability -= (vulnerability
                      * get_city_bonus(pcity, EFFECT_TYPE_SPY_RESISTANT) / 100);
    if (myrand(100) >= vulnerability) {
      /* Caught! */
      notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_FAILED,
                       _("Game: Your %s was caught in the attempt"
                         " of sabotage!"), unit_name(pdiplomat->type));
      notify_player_ex(cplayer, pcity->common.tile,
                       E_ENEMY_DIPLOMAT_FAILED,
                       _("Game: You caught %s %s attempting"
                         " to sabotage the %s in %s!"),
                       get_nation_name(pplayer->nation),
                       unit_name(pdiplomat->type),
                       get_improvement_name(target), pcity->common.name);
      wipe_unit(pdiplomat);
      freelog (LOG_DEBUG, "sabotage: caught in capital or on city walls");
      return;
    }

    /* Report it. */
    notify_player_ex(pplayer, pcity->common.tile, E_MY_DIPLOMAT_SABOTAGE,
                     _("Game: Your %s destroyed the %s in %s."),
                     unit_name(pdiplomat->type),
                     get_improvement_name(target), pcity->common.name);
    notify_player_ex(cplayer, pcity->common.tile, E_ENEMY_DIPLOMAT_SABOTAGE,
                     _("Game: The %s destroyed the %s in %s."),
                     get_nation_name_plural(pplayer->nation),
                     get_improvement_name(target), pcity->common.name);
    freelog (LOG_DEBUG, "sabotage: sabotaged improvement: %d (%s)",
               target, get_improvement_name (target));

    /* Do it. */
    building_lost(pcity, target);
  }

  /* Update clients. */
  send_city_info(NULL, pcity);

  /* this may cause a diplomatic incident */
  maybe_cause_incident(DIPLOMAT_SABOTAGE, pplayer, NULL, pcity);

  /* Check if a spy survives her mission. Diplomats never do. */
  diplomat_escape(pplayer, pdiplomat, pcity);
}

/**************************************************************************
  This subtracts the destination movement cost from a diplomat/spy.
**************************************************************************/
static void diplomat_charge_movement (unit_t *pdiplomat, tile_t *ptile)
{
  pdiplomat->moves_left -=
    map_move_cost (pdiplomat, ptile);
  if (pdiplomat->moves_left < 0) {
    pdiplomat->moves_left = 0;
  }
}

/**************************************************************************
  This determines if a diplomat/spy succeeds against some defender,
  who is also a diplomat or spy.
  (Note: This is weird in order to try to conform to Civ2 rules.)

  - Depends entirely upon game.server.diplchance and the defender:
    - Spies are much better.
    - Veterans are somewhat better.

  - Return TRUE if the "attacker" succeeds.
**************************************************************************/
static bool diplomat_success_vs_defender(unit_t *pattacker,
                                         unit_t *pdefender,
                                         tile_t *pdefender_tile)
{
  int att = game.server.dipldefchance; /* I.e. "diplomat defeat chance". */
  int def = 100 - game.server.dipldefchance;

  if (unit_flag(pdefender, F_SUPERSPY)) {
    return TRUE;
  }
  if (unit_flag (pattacker, F_SPY)) {
    att *= 2;
  }
  if (unit_flag (pdefender, F_SPY)) {
    def *= 2;
  }

  att += (att/5.0) * pattacker->veteran;
  def += (def/5.0) * pdefender->veteran;

  if (pdefender_tile->city) {
    def = def * (100 + get_city_bonus(pdefender_tile->city,
                                      EFFECT_TYPE_SPY_RESISTANT)) / 100;
  } else {
    if (tile_has_special(pdefender_tile, S_FORTRESS)
        || tile_has_special(pdefender_tile, S_AIRBASE)) {
      def = (def * 5) / 4;/* +25% */
    }
  }

  return myrand(att) > myrand(def);
}

/**************************************************************************
  This determines if a diplomat/spy succeeds in infiltrating a tile.

  - The infiltrator must go up against each defender.
  - One or the other is eliminated in each contest.

  - Return TRUE if the infiltrator succeeds.
**************************************************************************/
static bool diplomat_infiltrate_tile(player_t *pplayer,
                                     player_t *cplayer,
                                     unit_t *pdiplomat,
                                     tile_t *ptile)
{
  city_t *pcity = ptile->city;

  /* We don't need a _safe iterate since no transporters should be
   * destroyed. */
  unit_list_iterate(ptile->units, punit) {
    if (unit_flag(punit, F_DIPLOMAT) || unit_flag(punit, F_SUPERSPY)) {
      /* A F_SUPERSPY unit may not acutally be a spy, but a superboss which
         we cannot allow puny diplomats from getting the better of. Note that
         diplomat_success_vs_defender(punit) is always TRUE if the attacker
         is F_SUPERSPY. Hence F_SUPERSPY vs F_SUPERSPY in a diplomatic contest
         always kills the attacker. */
      if (diplomat_success_vs_defender(pdiplomat, punit, ptile)
          && !unit_flag(punit, F_SUPERSPY)) {
        /* Defending Spy/Diplomat dies. */

        notify_player_ex(cplayer, ptile, E_MY_DIPLOMAT_FAILED,
                         _("Game: Your %s has been eliminated defending %s"
                           " against a %s."), unit_name(punit->type),
                (pcity ? pcity->common.name : ""), unit_name(pdiplomat->type));
        notify_player_ex(pplayer, ptile, E_ENEMY_DIPLOMAT_FAILED,
                 _("Game: An enemy %s has been eliminated defending %s."),
                unit_name(punit->type), (pcity ? pcity->common.name : ""));

        wipe_unit(punit);
        pdiplomat->moves_left = MAX(0, pdiplomat->moves_left - SINGLE_MOVE);
        send_unit_info(pplayer, pdiplomat);
        return FALSE;
      } else {
        /* Check to see if defending unit became more experienced */
        bool vet = maybe_make_veteran(punit);

        /* Attacking Spy/Diplomat dies. */

        notify_player_ex(pplayer, ptile, E_MY_DIPLOMAT_FAILED,
                         _("Game: Your %s was eliminated"
                           " by a defending %s."),
                         unit_name(pdiplomat->type), unit_name(punit->type));
        if (vet) {
          if (pcity) {
            notify_player_ex(cplayer, ptile,
                             E_ENEMY_DIPLOMAT_FAILED,
                             _("Game: Eliminated %s %s while infiltrating "
                               "%s. The defender became more experienced."),
                             get_nation_name(pplayer->nation),
                             unit_name(pdiplomat->type), pcity->common.name);
          } else {
            notify_player_ex(cplayer, ptile,
                             E_ENEMY_DIPLOMAT_FAILED,
                             _("Game: Eliminated %s %s while infiltrating "
                               "our troops. The defender became more "
                               "experienced."),
                             get_nation_name(pplayer->nation),
                             unit_name(pdiplomat->type));
          }
        } else {
          if (pcity) {
            notify_player_ex(cplayer, ptile,
                             E_ENEMY_DIPLOMAT_FAILED,
                             _("Game: Eliminated %s %s while infiltrating "
                               "%s."), get_nation_name(pplayer->nation),
                             unit_name(pdiplomat->type), pcity->common.name);
          } else {
            notify_player_ex(cplayer, ptile,
                             E_ENEMY_DIPLOMAT_FAILED,
                             _("Game: Eliminated %s %s while infiltrating "
                               "our troops."),
                             get_nation_name(pplayer->nation),
                             unit_name(pdiplomat->type));
          }
        }
        wipe_unit(pdiplomat);
        return FALSE;
      }
    }
  } unit_list_iterate_end;

  return TRUE;
}

/**************************************************************************
  This determines if a diplomat/spy survives and escapes.
  If "pcity" is NULL, assume action was in the field.

  Spies have a game.server.spyreturnchance specified chance of survival
  (better if veteran):
    - Diplomats always die.
    - Escapes to home city.
    - Escapee may become a veteran.
**************************************************************************/
static void diplomat_escape(player_t *pplayer, unit_t *pdiplomat,
                            const city_t *pcity)
{
  tile_t *ptile;
  int escapechance;
  bool vet;
  city_t *spy_city;

  escapechance = game.server.spyreturnchance + pdiplomat->veteran * 5;

  if (pcity) {
    ptile = pcity->common.tile;
  } else {
    ptile = pdiplomat->tile;
  }

  /* find closest city for escape target */
  spy_city = find_closest_owned_city(unit_owner(pdiplomat),
                                     ptile, FALSE, NULL);

  if (spy_city && unit_flag(pdiplomat, F_SPY)
      && (myrand (100) < escapechance
          || unit_flag(pdiplomat, F_SUPERSPY)))
  {
    /* Attacking Spy/Diplomat survives. */

    /* may become a veteran */
    vet = maybe_make_veteran(pdiplomat);
    if (vet) {
      notify_player_ex(pplayer, ptile, E_MY_DIPLOMAT_ESCAPE,
                       _("Game: Your %s has successfully completed"
                         " her mission and returned unharmed to %s"
                         " and has become more experienced."),
                       unit_name(pdiplomat->type), spy_city->common.name);
    } else {
      notify_player_ex(pplayer, ptile, E_MY_DIPLOMAT_ESCAPE,
                       _("Game: Your %s has successfully completed"
                         " her mission and returned unharmed to %s."),
                       unit_name(pdiplomat->type), spy_city->common.name);
    }

    /* being teleported costs all movement */
    if (!teleport_unit_to_city(pdiplomat, spy_city, -1, FALSE)) {
      send_unit_info(pplayer, pdiplomat);
      freelog(LOG_ERROR, "Bug in diplomat_escape: Spy can't teleport.");
    }
    return;

  } else {
    if (pcity) {
      notify_player_ex(pplayer, ptile, E_MY_DIPLOMAT_FAILED,
                         _("Game: Your %s was captured after completing"
                           " her mission in %s."),
                         unit_name(pdiplomat->type), pcity->common.name);
    } else {
      notify_player_ex(pplayer, ptile, E_MY_DIPLOMAT_FAILED,
                         _("Game: Your %s was captured after completing"
                           " her mission."), unit_name(pdiplomat->type));
    }
  }

  wipe_unit(pdiplomat);
}

/**************************************************************************
...
**************************************************************************/
static void maybe_cause_incident(enum diplomat_actions action, player_t *offender,
                                 unit_t *victim_unit, city_t *victim_city)
{
  player_t *victim_player = 0;
  tile_t *victim_tile = NULL;

  if (victim_city) {
    victim_tile = victim_city->common.tile;
    victim_player = city_owner(victim_city);
  } else if (victim_unit) {
    victim_tile = victim_unit->tile;
    victim_player = unit_owner(victim_unit);
  } else {
    die("No victim in call to maybe_cause_incident()");
  }

  if (!pplayers_at_war(offender, victim_player) &&
      (myrand(GAME_MAX_REPUTATION) - offender->reputation >
       GAME_MAX_REPUTATION/2 - victim_player->reputation)) {
    enum diplstate_type ds = pplayer_get_diplstate(offender, victim_player)->type;
    int punishment = 0;
    switch (action) {
    case DIPLOMAT_BRIBE:
      notify_player_ex(offender, victim_tile, E_DIPLOMATIC_INCIDENT,
                       _("Game: You have caused an incident while bribing "
                         "%s's %s."),
                       victim_player->name,
                       unit_name(victim_unit->type));
      notify_player_ex(victim_player, victim_tile, E_DIPLOMATIC_INCIDENT,
                       _("Game: %s has caused an incident while bribing "
                         "your %s."),
                       offender->name,
                       unit_name(victim_unit->type));
      break;
    case DIPLOMAT_STEAL:
      notify_player_ex(offender, victim_tile, E_DIPLOMATIC_INCIDENT,
                       _("Game: You have caused an incident while stealing "
                         "tech from %s."),
                       victim_player->name);
      notify_player_ex(victim_player, victim_tile, E_DIPLOMATIC_INCIDENT,
                       _("Game: %s has caused an incident while stealing "
                         "tech from you."),
                       offender->name);
      break;
    case DIPLOMAT_INCITE:
      notify_player_ex(offender, victim_tile, E_DIPLOMATIC_INCIDENT,
                       _("Game: You have caused an incident while inciting a "
                         "revolt in %s."), victim_city->common.name);
      notify_player_ex(victim_player, victim_tile, E_DIPLOMATIC_INCIDENT,
                       _("Game: %s have caused an incident while inciting a "
                         "revolt in %s."), offender->name, victim_city->common.name);
      break;
    case DIPLOMAT_MOVE:
    case DIPLOMAT_EMBASSY:
    case DIPLOMAT_INVESTIGATE:
    case SPY_GET_SABOTAGE_LIST:
      return; /* These are not considered offences */
    case DIPLOMAT_ANY_ACTION:
    case SPY_POISON:
    case SPY_SABOTAGE_UNIT:
    case DIPLOMAT_SABOTAGE:
      /* You can only do these when you are at war, so we should never
         get inside this "if" */
      die("Bug in maybe_cause_incident()");
    }
    switch (ds) {
    case DIPLSTATE_WAR:
    case DIPLSTATE_NO_CONTACT:
      freelog(LOG_VERBOSE,"Trying to cause an incident between players at war");
      punishment = 0;
      break;
    case DIPLSTATE_NEUTRAL:
      punishment = GAME_MAX_REPUTATION/20;
      break;
    case DIPLSTATE_CEASEFIRE:
    case DIPLSTATE_PEACE:
      punishment = GAME_MAX_REPUTATION/10;
      break;
    case DIPLSTATE_ALLIANCE:
    case DIPLSTATE_TEAM:
      punishment = GAME_MAX_REPUTATION/5;
      break;
    case DIPLSTATE_LAST:
      assert(FALSE);
      break;
    }
    offender->reputation = MAX(offender->reputation - punishment, 0);
    victim_player->diplstates[offender->player_no].has_reason_to_cancel = 2;
    /* FIXME: Maybe we should try to cause a revolution is the offender's
       government has a senate */
    send_player_info(offender, NULL);
    send_player_info(victim_player, NULL);
  }

  return;
}


/**************************************************************************
 calculate how expensive it is to bribe the unit
 depends on distance to the capital, and government form
 settlers are half price

 Plus, the damage to the unit reduces the price.
**************************************************************************/
int unit_bribe_cost(unit_t *punit)
{
  struct government *g = get_gov_pplayer(unit_owner(punit));
  int cost;
  city_t *capital;
  int dist;
  int default_hp = unit_type(punit)->hp;
  int shield_cost = unit_build_shield_cost(punit->type);

  if (game.ext_info.experimentalbribingcost) {
    /* Experimental cost. */
    city_t *pcity = map_get_city(punit->tile);

    /* Base cost. */
    cost = 2 * shield_cost + (shield_cost * shield_cost) / 20;
    /* Multiply by veterancy, veteran unit = *1.5 cost. */
    cost = (int)(cost * (1 + (float)punit->veteran / 2));
    if (map_has_special(punit->tile, S_FORTRESS) && !pcity) {
      /* Multiply by fort defence factor. */
      cost *= (terrain_control.fortress_defense_bonus) / 100;
    }
    if ((pcity || punit->activity==ACTIVITY_FORTIFIED)
        && is_ground_unittype(punit->type)) {
      /* Multiply by 1.5 if unit is fortified. */
      cost = (cost * 3) / 2;
    }
    /* Multiply by terrain defence bonus. */
    cost = (int)(cost * (float)
                 get_tile_type(punit->tile->terrain)->defense_bonus) / 10;
  } else {
    /* Standard cost. */
    cost = unit_owner(punit)->economic.gold + 750;
    capital = find_palace(unit_owner(punit));
    if (capital) {
      int tmp = map_distance(capital->common.tile, punit->tile);
      dist = MIN(32, tmp);
    } else {
      dist = 32;
    }
    if (g->fixed_corruption_distance != 0) {
      dist = MIN(g->fixed_corruption_distance, dist);
    }
    cost /= dist + 2;

    cost *= unit_build_shield_cost(punit->type) / 10;

    /* FIXME: This is a weird one - should be replaced */
    if (unit_flag(punit, F_CITIES)) {
      cost /= 2;
    }

    cost = (int) ((float) cost / (float) 2
        + ((float) punit->hp / (float) default_hp)
        * ((float) cost / (float) 2));
  }

  return cost;
}

/**************************************************************************
 return number of diplomats on this square.  AJS 20000130
**************************************************************************/
int count_diplomats_on_tile(tile_t *ptile)
{
  int count = 0;

  unit_list_iterate((ptile)->units, punit)
    if (unit_flag(punit, F_DIPLOMAT))
      count++;
  unit_list_iterate_end;
  return count;
}
