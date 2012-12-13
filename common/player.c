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
#  include "../config.h"
#endif

#include <assert.h>

#include "city.h"
#include "wc_intl.h"
#include "game.h"
#include "government.h"
#include "hash.h"
#include "idex.h"
#include "improvement.h"
#include "map.h"
#include "mem.h"
#include "rand.h"
#include "shared.h"
#include "support.h"
#include "tech.h"
#include "unit.h"

#include "player.h"

/* Must match enum player_results in player.h */
static const char *player_result_strings[PR_NUM_PLAYER_RESULTS] = {
  N_("none"),
  N_("win"),
  N_("lose"),
  N_("draw")
};

/***************************************************************
  Returns true iff p1 can ally p2. There is only one condition:
  We are not at war with any of p2's allies. Note that for an
  alliance to be made, we need to check this both ways.

  The reason for this is to avoid the dread 'love-love-hate'
  triad, in which p1 is allied to p2 is allied to p3 is at
  war with p1. These lead to strange situations.
***************************************************************/
bool pplayer_can_ally(struct player *p1, struct player *p2)
{
  players_iterate(pplayer) {
    enum diplstate_type ds = pplayer_get_diplstate(p1, pplayer)->type;
    if (pplayer != p1
        && pplayer != p2
        && pplayers_allied(p2, pplayer)
        && ds == DS_WAR /* do not count 'never met' as war here */
        && pplayer->is_alive) {
      return FALSE;
    }
  } players_iterate_end;
  return TRUE;
}

/***************************************************************
  Check if pplayer has an embassy with pplayer2. We always have
  an embassy with ourselves.
***************************************************************/
bool player_has_embassy(struct player *pplayer, struct player *pplayer2)
{
  return (TEST_BIT(pplayer->embassy, pplayer2->player_no)
          || (pplayer == pplayer2)
          || (get_player_bonus(pplayer, EFT_HAVE_EMBASSIES) > 0
              && !is_barbarian(pplayer2)));
}

/****************************************************************
...
*****************************************************************/
bool player_owns_city(struct player *pplayer, city_t *pcity)
{
  if (!pcity || !pplayer)
    return FALSE;                       /* better safe than sorry */
  return (pcity->common.owner==pplayer->player_no);
}

/***************************************************************
In the server you must use server_player_init
***************************************************************/
void player_init(struct player *_player)
{
  int i;

  _player->player_no=_player-game.players;

  sz_strlcpy(_player->name, ANON_PLAYER_NAME);
  sz_strlcpy(_player->username, ANON_USER_NAME);
  _player->is_male = TRUE;
  _player->government = game.ruleset_control.default_government;
  _player->target_government = game.ruleset_control.default_government;
  _player->nation = NO_NATION_SELECTED;
  _player->team = TEAM_NONE;
  _player->is_started = FALSE;
  _player->revolution_finishes = -1;
  _player->capital = FALSE;
  _player->units = unit_list_new();
  _player->cities = city_list_new();
  _player->connections = conn_list_new();
  _player->current_conn = NULL;
  _player->is_connected = FALSE;
  _player->was_created = FALSE;
  _player->is_civil_war_split = FALSE;
  _player->is_alive=TRUE;
  _player->is_dying = FALSE;
  _player->embassy=0;
  _player->reputation=GAME_DEFAULT_REPUTATION;
  for(i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    _player->diplstates[i].type = DS_NO_CONTACT;
    _player->diplstates[i].has_reason_to_cancel = 0;
    _player->diplstates[i].contact_turns_left = 0;
  }
  _player->ignore_diplomacy = FALSE;
  _player->city_style=0;            /* should be first basic style */
  _player->ai.control=FALSE;
  _player->ai.tech_goal = A_UNSET;
  _player->ai.handicap = 0;
  _player->ai.skill_level = 0;
  _player->ai.fuzzy = 0;
  _player->ai.expand = 100;
  _player->ai.barbarian_type = NOT_A_BARBARIAN;
  _player->future_tech=0;
  _player->economic.tax=PLAYER_DEFAULT_TAX_RATE;
  _player->economic.science=PLAYER_DEFAULT_SCIENCE_RATE;
  _player->economic.luxury=PLAYER_DEFAULT_LUXURY_RATE;
  _player->research.changed_from = -1;
  player_limit_to_government_rates(_player);
  spaceship_init(&_player->spaceship);

  _player->gives_shared_vision = 0;
  _player->really_gives_vision = 0;

  /* Initialise list of improvements with Player-wide equiv_range */
  improvement_status_init(_player->improvements, ARRAY_SIZE(_player->improvements));

  /* Initialise list of improvements with Island-wide equiv_range */
  _player->island_improv = NULL;

  if (map.num_continents > 0 && game.ruleset_control.num_impr_types > 0) {
    _player->island_improv = wc_malloc((map.num_continents + 1)
        * game.ruleset_control.num_impr_types * sizeof(Impr_Status));
    for (i = 1; i <= map.num_continents; i++) {
      improvement_status_init(&_player->island_improv[i
                                  * game.ruleset_control.num_impr_types],
                              game.ruleset_control.num_impr_types);
    }
  }

  _player->attribute_block.data = NULL;
  _player->attribute_block.length = 0;
  _player->attribute_block_buffer.data = NULL;
  _player->attribute_block_buffer.length = 0;

  _player->debug = FALSE;

  _player->result = PR_NONE;
  _player->rank = RANK_NONE;
  _player->team_rank = RANK_NONE;

  _player->wcdb.player_id = 0;
  _player->wcdb.rated_user_id = 0;
  _player->wcdb.rated_user_name[0] = '\0';
  _player->wcdb.rating = 0;
  _player->wcdb.rating_deviation = 0;
  _player->wcdb.last_rating_timestamp = 0;
  _player->wcdb.new_rating = 0;
  _player->wcdb.new_rating_deviation = 0;
  player_free_turns_played(_player);

  _player->score.units_built = 0;
  _player->score.units_killed = 0;
  _player->score.units_lost = 0;
}

/***************************************************************
...
***************************************************************/
void player_set_unit_focus_status(struct player *pplayer)
{
  if (!pplayer) {
    return;
  }
  unit_list_iterate(pplayer->units, punit)
    punit->focus_status = FOCUS_AVAIL;
  unit_list_iterate_end;
}

/***************************************************************
...
***************************************************************/
struct player *find_player_by_name(const char *name)
{
  players_iterate(pplayer) {
    if (mystrcasecmp(name, pplayer->name) == 0) {
      return pplayer;
    }
  } players_iterate_end;

  return NULL;
}

/***************************************************************
  Find player by name, allowing unambigous prefix (ie abbreviation).
  Returns NULL if could not match, or if ambiguous or other
  problem, and fills *result with characterisation of match/non-match
  (see shared.[ch])
***************************************************************/
static const char *pname_accessor(int i) {
  return game.players[i].name;
}

/***************************************************************
Find player by its name prefix
***************************************************************/
struct player *find_player_by_name_prefix(const char *name,
                                          enum m_pre_result *result)
{
  int ind;

  *result = match_prefix(pname_accessor, game.info.nplayers, MAX_LEN_NAME - 1,
                         mystrncasecmp, name, &ind);

  if (*result < M_PRE_AMBIGUOUS) {
    return get_player(ind);
  } else {
    return NULL;
  }
}

/***************************************************************
Find player by its user name (not player/leader name)
***************************************************************/
struct player *find_player_by_user(const char *name)
{
  players_iterate(pplayer) {
    if (mystrcasecmp(name, pplayer->username) == 0) {
      return pplayer;
    }
  } players_iterate_end;

  return NULL;
}

/****************************************************************************
  Checks if a unit can be seen by pplayer at (x,y).
  A player can see a unit if he:
  (a) can see the tile AND
  (b) can see the unit at the tile (i.e. unit not invisible at this tile) AND
  (c) the unit is outside a city OR in an allied city AND
  (d) the unit isn't in a transporter, or we are allied AND
  (e) the unit isn't in a transporter, or we can see the transporter
****************************************************************************/
bool can_player_see_unit_at(struct player *pplayer, struct unit *punit,
                            tile_t *ptile)
{
  city_t *pcity;

  /* If the player can't even see the tile... */
  if (map_get_known(ptile, pplayer) != TILE_KNOWN) {
    return FALSE;
  }

  /* Don't show non-allied units that are in transports.  This is logical
   * because allied transports can also contain our units.  Shared vision
   * isn't taken into account. */
  if (punit->transported_by != -1 && unit_owner(punit) != pplayer
      && !pplayers_allied(pplayer, unit_owner(punit))) {
    return FALSE;
  }

  /* Units in cities may be hidden. */
  pcity = map_get_city(ptile);
  if (pcity && !can_player_see_units_in_city(pplayer, pcity)) {
    return FALSE;
  }

  /* Allied or non-hiding units are always seen. */
  if (pplayers_allied(unit_owner(punit), pplayer)
      || !is_hiding_unit(punit)) {
    return TRUE;
  }

  /* Hiding units may only be seen by adjacent allied units or cities. */
  /* FIXME: shouldn't a check for shared vision be done here? */
  adjc_iterate(ptile, ptile1) {
    city_t *pcity = map_get_city(ptile1);
    if (pcity && pplayers_allied(city_owner(pcity), pplayer)) {
      return TRUE;
    }
    unit_list_iterate(ptile1->units, punit2) {
      if (pplayers_allied(unit_owner(punit2), pplayer)) {
        return TRUE;
      }
    } unit_list_iterate_end;
  } adjc_iterate_end;

  return FALSE;
}


/****************************************************************************
  Checks if a unit can be seen by pplayer at its current location.

  See can_player_see_unit_at.
****************************************************************************/
bool can_player_see_unit(struct player *pplayer, struct unit *punit)
{
  return can_player_see_unit_at(pplayer, punit, punit->tile);
}

/****************************************************************************
  Return TRUE iff the player can see units in the city.  Either they
  can see all units or none.

  If the player can see units in the city, then the server sends the
  unit info for units in the city to the client.  The client uses the
  tile's unitlist to determine whether to show the city occupied flag.  Of
  course the units will be visible to the player as well, if he clicks on
  them.

  If the player can't see units in the city, then the server doesn't send
  the unit info for these units.  The client therefore uses the "occupied"
  flag sent in the short city packet to determine whether to show the city
  occupied flag.

  Note that can_player_see_city_internals => can_player_see_units_in_city.
  Otherwise the player would not know anything about the city's units at
  all, since the full city packet has no "occupied" flag.
****************************************************************************/
bool can_player_see_units_in_city(struct player *pplayer,
                                  city_t *pcity)
{
  return (can_player_see_city_internals(pplayer, pcity)
          || pplayers_allied(pplayer, city_owner(pcity)));
}

/****************************************************************************
  Return TRUE iff the player can see the city's internals.  This means the
  full city packet is sent to the client, who should then be able to popup
  a dialog for it.
****************************************************************************/
bool can_player_see_city_internals(struct player *pplayer,
                                   city_t *pcity)
{
  return (pplayer == city_owner(pcity));
}

/***************************************************************
 If the specified player owns the city with the specified id,
 return pointer to the city struct.  Else return NULL.
 Now always uses fast idex_lookup_city.
***************************************************************/
city_t *player_find_city_by_id(const struct player *pplayer,
                                    int city_id)
{
  city_t *pcity = idex_lookup_city(city_id);

  if (pplayer == NULL) {
    return NULL;
  }

  if(pcity && (pcity->common.owner==pplayer->player_no)) {
    return pcity;
  } else {
    return NULL;
  }
}

/***************************************************************
 If the specified player owns the unit with the specified id,
 return pointer to the unit struct.  Else return NULL.
 Uses fast idex_lookup_city.
***************************************************************/
struct unit *player_find_unit_by_id(const struct player *pplayer,
                                    int unit_id)
{
  struct unit *punit = idex_lookup_unit(unit_id);

  if (punit != NULL && pplayer != NULL
      && unit_owner(punit) == pplayer) {
    return punit;
  }
  return NULL;
}

/*************************************************************************
Return 1 if x,y is inside any of the player's city radii.
**************************************************************************/
bool player_in_city_radius(struct player *pplayer, tile_t *ptile)
{
  city_t *pcity;
  map_city_radius_iterate(ptile, ptile1) {
    pcity = map_get_city(ptile1);
    if (pcity && (pcity->common.owner == pplayer->player_no))
      return TRUE;
  } map_city_radius_iterate_end;
  return FALSE;
}

/**************************************************************************
 Returns the number of techs the player has researched which has this
 flag. Needs to be optimized later (e.g. int tech_flags[TF_LAST] in
 struct player)
**************************************************************************/
int num_known_tech_with_flag(struct player *pplayer, enum tech_flag_id flag)
{
  return pplayer->research.num_known_tech_with_flag[flag];
}

/**************************************************************************
  Return the expected net income of the player this turn.  This includes
  tax revenue and upkeep, but not one-time purchases or found gold.

  This function depends on pcity->tax_total being set for all cities, so
  make sure the player's cities have been refreshed.
**************************************************************************/
int player_get_expected_income(struct player *pplayer)
{
  int income = 0;

  /* City income/expenses. */
  city_list_iterate(pplayer->cities, pcity) {
    /* Gold suplus accounts for imcome plus building and unit upkeep. */
    income += city_gold_surplus(pcity, pcity->common.tax_total);

    /* Capitalization income. */
    if (get_current_construction_bonus(pcity, EFT_PROD_TO_GOLD) > 0) {
      income += pcity->common.shield_stock + pcity->common.shield_surplus;
    }
  } city_list_iterate_end;

  return income;
}

/**************************************************************************
  Return the expected bulb income of the player this turn.

  This function depends on pcity->science_total being set for all cities, so
  make sure the player's cities have been refreshed.
**************************************************************************/
int player_get_expected_bulbs(struct player *pplayer)
{
  int bulbs = 0;

  city_list_iterate(pplayer->cities, pcity) {
    bulbs += pcity->common.science_total;
  } city_list_iterate_end;

  return bulbs;
}

/**************************************************************************
 Returns TRUE iff the player knows at least one tech which has the
 given flag.
**************************************************************************/
bool player_knows_techs_with_flag(struct player *pplayer,
                                 enum tech_flag_id flag)
{
  return num_known_tech_with_flag(pplayer, flag) > 0;
}

/**************************************************************************
The following limits a player's rates to those that are acceptable for the
present form of government.  If a rate exceeds maxrate for this government,
it adjusts rates automatically adding the extra to the 2nd highest rate,
preferring science to taxes and taxes to luxuries.
(It assumes that for any government maxrate>=50)
**************************************************************************/
void player_limit_to_government_rates(struct player *pplayer)
{
  int maxrate, surplus;

  /* ai players allowed to cheat */
  if (pplayer->ai.control) {
    return;
  }

  maxrate = get_government_max_rate(pplayer->government);
  surplus = 0;
  if (pplayer->economic.luxury > maxrate) {
    surplus += pplayer->economic.luxury - maxrate;
    pplayer->economic.luxury = maxrate;
  }
  if (pplayer->economic.tax > maxrate) {
    surplus += pplayer->economic.tax - maxrate;
    pplayer->economic.tax = maxrate;
  }
  if (pplayer->economic.science > maxrate) {
    surplus += pplayer->economic.science - maxrate;
    pplayer->economic.science = maxrate;
  }

  assert(surplus % 10 == 0);
  while (surplus > 0) {
    if (pplayer->economic.science < maxrate) {
      pplayer->economic.science += 10;
    } else if (pplayer->economic.tax < maxrate) {
      pplayer->economic.tax += 10;
    } else if (pplayer->economic.luxury < maxrate) {
      pplayer->economic.luxury += 10;
    } else {
      die("byebye");
    }
    surplus -= 10;
  }

  return;
}

/**************************************************************************
Locate the city where the players palace is located, (NULL Otherwise)
**************************************************************************/
city_t *find_palace(struct player *pplayer)
{
  city_list_iterate(pplayer->cities, pcity) {
    if (is_capital(pcity)) {
      return pcity;
    }
  } city_list_iterate_end;
  return NULL;
}

/**************************************************************************
...
**************************************************************************/
bool player_knows_improvement_tech(struct player *pplayer,
                                   Impr_Type_id id)
{
  int t;
  if (!improvement_exists(id)) return FALSE;
  t = get_improvement_type(id)->tech_req;
  return (get_invention(pplayer, t) == TECH_KNOWN);
}

/**************************************************************************
...
**************************************************************************/
bool ai_handicap(struct player *pplayer, enum handicap_type htype)
{
  if (!pplayer->ai.control) {
    return TRUE;
  }
  return BOOL_VAL(pplayer->ai.handicap & htype);
}

/**************************************************************************
Return the value normal_decision (a boolean), except if the AI is fuzzy,
then sometimes flip the value.  The intention of this is that instead of
    if (condition) { action }
you can use
    if (ai_fuzzy(pplayer, condition)) { action }
to sometimes flip a decision, to simulate an AI with some confusion,
indecisiveness, forgetfulness etc. In practice its often safer to use
    if (condition && ai_fuzzy(pplayer,1)) { action }
for an action which only makes sense if condition holds, but which a
fuzzy AI can safely "forget".  Note that for a non-fuzzy AI, or for a
human player being helped by the AI (eg, autosettlers), you can ignore
the "ai_fuzzy(pplayer," part, and read the previous example as:
    if (condition && 1) { action }
--dwp
**************************************************************************/
bool ai_fuzzy(struct player *pplayer, bool normal_decision)
{
  if (!pplayer->ai.control || pplayer->ai.fuzzy == 0) return normal_decision;
  if (myrand(1000) >= pplayer->ai.fuzzy) return normal_decision;
  return !normal_decision;
}



/**************************************************************************
  Return a text describing an AI's love for you.  (Oooh, kinky!!)
  These words should be adjectives which can fit in the sentence
  "The x are y towards us"
  "The Babylonians are respectful towards us"
**************************************************************************/
const char *love_text(const int love)
{
  if (love <= - MAX_AI_LOVE * 90 / 100) {
    return Q_("?attitude:Genocidal");
  } else if (love <= - MAX_AI_LOVE * 70 / 100) {
    return Q_("?attitude:Belligerent");
  } else if (love <= - MAX_AI_LOVE * 50 / 100) {
    return Q_("?attitude:Hostile");
  } else if (love <= - MAX_AI_LOVE * 25 / 100) {
    return Q_("?attitude:Uncooperative");
  } else if (love <= - MAX_AI_LOVE * 10 / 100) {
    return Q_("?attitude:Uneasy");
  } else if (love <= MAX_AI_LOVE * 10 / 100) {
    return Q_("?attitude:Neutral");
  } else if (love <= MAX_AI_LOVE * 25 / 100) {
    return Q_("?attitude:Respectful");
  } else if (love <= MAX_AI_LOVE * 50 / 100) {
    return Q_("?attitude:Helpful");
  } else if (love <= MAX_AI_LOVE * 70 / 100) {
    return Q_("?attitude:Enthusiastic");
  } else if (love <= MAX_AI_LOVE * 90 / 100) {
    return Q_("?attitude:Admiring");
  } else {
    assert(love > MAX_AI_LOVE * 90 / 100);
    return Q_("?attitude:Worshipful");
  }
}

/**************************************************************************
Return a reputation level as a human-readable string
**************************************************************************/
const char *reputation_text(const int rep)
{
  if (rep == -1)
    return "-";
  else if (rep > GAME_MAX_REPUTATION * 0.95)
    return _("Spotless");
  else if (rep > GAME_MAX_REPUTATION * 0.85)
    return _("Excellent");
  else if (rep > GAME_MAX_REPUTATION * 0.75)
    return _("Honorable");
  else if (rep > GAME_MAX_REPUTATION * 0.55)
    return _("Questionable");
  else if (rep > GAME_MAX_REPUTATION * 0.30)
    return _("Dishonorable");
  else if (rep > GAME_MAX_REPUTATION * 0.15)
    return _("Poor");
  else if (rep > GAME_MAX_REPUTATION * 0.07)
    return _("Despicable");
  else
    return _("Atrocious");
}

/**************************************************************************
  Return a diplomatic state as a human-readable string
**************************************************************************/
const char *diplstate_text(const enum diplstate_type type)
{
  static const char *ds_names[DS_LAST] =
  {
    N_("?diplomatic_state:Neutral"),
    N_("?diplomatic_state:War"),
    N_("?diplomatic_state:Cease-fire"),
    N_("?diplomatic_state:Peace"),
    N_("?diplomatic_state:Alliance"),
    N_("?diplomatic_state:Never met"),
    N_("?diplomatic_state:Team")
  };

  if (type < DS_LAST) {
    return Q_(ds_names[type]);
  }
  die("Bad diplstate_type in diplstate_text: %d", type);
  return NULL;
}

/***************************************************************
returns diplomatic state type between two players
***************************************************************/
const struct player_diplstate *pplayer_get_diplstate(const struct player *pplayer,
                                                     const struct player *pplayer2)
{
  return &(pplayer->diplstates[pplayer2->player_no]);
}

/***************************************************************
  Returns true iff players can attack each other.
***************************************************************/
bool pplayers_at_war(const struct player *pplayer,
                     const struct player *pplayer2)
{
  enum diplstate_type ds = pplayer_get_diplstate(pplayer, pplayer2)->type;
  if (pplayer == pplayer2) {
    return FALSE;
  }
  if (is_barbarian(pplayer) || is_barbarian(pplayer2)) {
    return TRUE;
  }
  return ds == DS_WAR || ds == DS_NO_CONTACT;
}

/***************************************************************
  Returns true iff players are allied.
***************************************************************/
bool pplayers_allied(const struct player *pplayer,
                     const struct player *pplayer2)
{
  enum diplstate_type ds = pplayer_get_diplstate(pplayer, pplayer2)->type;
  if (pplayer == pplayer2) {
    return TRUE;
  }
  if (is_barbarian(pplayer) || is_barbarian(pplayer2)) {
    return FALSE;
  }
  return (ds == DS_ALLIANCE || ds == DS_TEAM);
}

/***************************************************************
  Returns true iff players are allied or at peace.
***************************************************************/
bool pplayers_in_peace(const struct player *pplayer,
                       const struct player *pplayer2)
{
  enum diplstate_type ds = pplayer_get_diplstate(pplayer, pplayer2)->type;

  if (pplayer == pplayer2) {
    return TRUE;
  }
  if (is_barbarian(pplayer) || is_barbarian(pplayer2)) {
    return FALSE;
  }
  return (ds == DS_PEACE || ds == DS_ALLIANCE || ds == DS_TEAM);
}

/***************************************************************
  Returns true iff players have peace or cease-fire.
***************************************************************/
bool pplayers_non_attack(const struct player *pplayer,
                         const struct player *pplayer2)
{
  enum diplstate_type ds = pplayer_get_diplstate(pplayer, pplayer2)->type;
  if (pplayer == pplayer2) {
    return FALSE;
  }
  if (is_barbarian(pplayer) || is_barbarian(pplayer2)) {
    return FALSE;
  }
  return (ds == DS_PEACE || ds == DS_CEASEFIRE || ds == DS_NEUTRAL);
}

/**************************************************************************
  Return TRUE if players are in the same team
**************************************************************************/
bool players_on_same_team(const struct player *pplayer1,
                          const struct player *pplayer2)
{
  if (pplayer1->team == TEAM_NONE) {
    return FALSE;
  }
  return (pplayer1->team == pplayer2->team);
}

/**************************************************************************
 ...
**************************************************************************/
bool is_barbarian(const struct player *pplayer)
{
  return pplayer && pplayer->ai.barbarian_type != NOT_A_BARBARIAN;
}

/**************************************************************************
 ...
**************************************************************************/
bool players_on_different_teams(const struct player *p1,
                                const struct player *p2)
{
  return p1->team != TEAM_NONE &&
         p2->team != TEAM_NONE &&
         p1->team != p2->team;
}
/**************************************************************************
 ...
**************************************************************************/
bool player_is_on_team(const struct player *pplayer)
{
  return pplayer->team != TEAM_NONE;
}
/**************************************************************************
 ...
**************************************************************************/
int player_allies_count(const struct player *pplayer)
{
  int count = 0;
  enum diplstate_type ds;

  players_iterate(oplayer) {
    if(pplayer == oplayer || !oplayer->is_alive)
      continue;
    ds = pplayer_get_diplstate(pplayer, oplayer)->type;
    if(ds == DS_ALLIANCE || ds == DS_TEAM)
      count++;
  } players_iterate_end;
  return count;
}

/**************************************************************************
  Return TRUE iff the player me gives shared vision to player them.
**************************************************************************/
bool gives_shared_vision(struct player *me, struct player *them)
{
  return TEST_BIT(me->gives_shared_vision, them->player_no);
}

/**************************************************************************
  Return TRUE iff the two diplstates are equal.
**************************************************************************/
bool are_diplstates_equal(const struct player_diplstate *pds1,
                          const struct player_diplstate *pds2)
{
  return (pds1->type == pds2->type && pds1->turns_left == pds2->turns_left
          && pds1->has_reason_to_cancel == pds2->has_reason_to_cancel
          && pds1->contact_turns_left == pds2->contact_turns_left);
}

/***************************************************************************
  Return the number of pplayer2's visible units in pplayer's territory,
  from the point of view of pplayer.  Units that cannot be seen by pplayer
  will not be found (this function doesn't cheat).
***************************************************************************/
int player_in_territory(struct player *pplayer, struct player *pplayer2)
{
  int in_territory = 0;

  /* This algorithm should work at server or client.  It only returns the
   * number of visible units (a unit can potentially hide inside the
   * transport of a different player).
   *
   * Note this may be quite slow.  An even slower alternative is to iterate
   * over the entire map, checking all units inside the player's territory
   * to see if they're owned by the enemy. */
  unit_list_iterate(pplayer2->units, punit) {
    /* Get the owner of the tile/territory. */
    struct player *owner = map_get_owner(punit->tile);

    if (owner == pplayer && can_player_see_unit(pplayer, punit)) {
      /* Found one! */
      in_territory += 1;
    }
  } unit_list_iterate_end;

  return in_territory;
}

/****************************************************************************
  Returns whether this is a valid username.  This is used by the server to
  validate usernames and should be used by the client to avoid invalid
  ones.
****************************************************************************/
bool is_valid_username(const char *name)
{
  return (strlen(name) > 0
          && !my_isdigit(name[0])
          && is_ascii_name(name)
          && mystrcasecmp(name, ANON_USER_NAME) != 0);
}

/****************************************************************************
  ...
****************************************************************************/
const char *result_name_orig(enum player_results res)
{
  assert(0 <= res && res < PR_NUM_PLAYER_RESULTS);
  return player_result_strings[res];
}

/****************************************************************************
  ...
****************************************************************************/
const char *result_name(enum player_results res)
{
  assert(0 <= res && res < PR_NUM_PLAYER_RESULTS);
  return _(player_result_strings[res]);
}

/***************************************************************
  ...
***************************************************************/
const char *name_of_skill_level(int level)
{
  static const char *nm[] = {
    "UNUSED", "away", "novice", "easy",
    "UNKNOWN", "normal", "UNKNOWN", "hard",
    "UNKNOWN", "UNKNOWN", "experimental"
  };
  assert(level > 0 && level <= 10);
  return nm[level];
}

/****************************************************************************
  ...
****************************************************************************/
int player_get_username(const struct player *pplayer,
                        char *outbuf, int maxlen)
{
  int len = 0;

  if (0 == strcmp(pplayer->username, ANON_USER_NAME)
      || pplayer->username[0] == '\0') {
    if (pplayer->ai.control) {
      len = my_snprintf(outbuf, maxlen, "AI (%s)",
                        name_of_skill_level(pplayer->ai.skill_level));
    } else {
      outbuf[0] = '\0';
    }
  } else {
    len = mystrlcpy(outbuf, pplayer->username, maxlen);
  }

  return len;
}

/****************************************************************************
  ...
****************************************************************************/
bool player_get_ignore_diplomacy(const struct player *pplayer)
{
  if (pplayer == NULL) {
    return FALSE;
  }
  return pplayer->ignore_diplomacy;
}

/****************************************************************************
  ...
****************************************************************************/
void player_set_ignore_diplomacy(struct player *pplayer, bool value)
{
  if (pplayer == NULL) {
    return;
  }
  pplayer->ignore_diplomacy = value;
}

/****************************************************************************
  ...
****************************************************************************/
void player_set_turns_played(struct player *_player, const char *username,
                             int turns)
{
  struct hash_table *h;
  struct turns_played_info *tp;

  if (!_player || !username) {
    return;
  }

  h = _player->wcdb.turns_played_table;
  if (!h) {
    return;
  }

  tp = hash_lookup_data(h, username);
  if (!tp) {
    tp = wc_malloc(sizeof(*tp));
    sz_strlcpy(tp->username, username);
    hash_insert(h, tp->username, tp);
  }
  tp->turns = turns;
}

/****************************************************************************
  ...
****************************************************************************/
void player_setup_turns_played(struct player *_player)
{
  struct hash_table *h;

  player_free_turns_played(_player);

  h = hash_new(hash_fval_string2, hash_fcmp_string);
  _player->wcdb.turns_played_table = h;
}

/****************************************************************************
  ...
****************************************************************************/
int player_get_turns_played(const struct player *_player, const char *username)
{
  struct hash_table *h;
  struct turns_played_info *tp;

  if (!_player || !username) {
    return 0;
  }

  h = _player->wcdb.turns_played_table;
  if (!h) {
    return 0;
  }

  tp = hash_lookup_data(h, username);
  return tp ? tp->turns : 0;
}

/****************************************************************************
  ...
****************************************************************************/
void player_free_turns_played(struct player *_player)
{
  struct hash_table *h;
  if (!_player) {
    return;
  }

  h = _player->wcdb.turns_played_table;
  if (h) {
    hash_values_iterate(h, tp) {
      if (tp) {
        free(tp);
      }
    } hash_values_iterate_end;
    hash_free(h);
  }

  _player->wcdb.turns_played_table = NULL;
}
