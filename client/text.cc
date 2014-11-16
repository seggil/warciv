/**********************************************************************
 Freeciv - Copyright (C) 2002 - The Freeciv Poject
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
#include <stdarg.h>
#include <string.h>

#include "wc_intl.hh"
#include "log.hh"
#include "support.hh"

#include "city.hh"
#include "map.hh"
#include "combat.hh"
#include "government.hh"

#include "civclient.hh"
#include "climisc.hh"
#include "clinet.hh"
#include "control.hh"
#include "goto.hh"
#include "multiselect.hh"
#include "text.hh"

/*
 * An individual add(_line) string has to fit into GROW_TMP_SIZE. One buffer
 * of GROW_TMP_SIZE size will be allocated for the entire client.
 */
#define GROW_TMP_SIZE   (1024)

/*
 * Initial size of the buffer for each function.  It may later be
 * grown as needed.
 */
#define START_SIZE      10

static void real_add_line(char **buffer, size_t * buffer_size,
                          const char *format, ...)
wc__attribute((__format__(__printf__, 3, 4)));
static void real_add(char **buffer, size_t * buffer_size,
                     const char *format, ...)
wc__attribute((__format__(__printf__, 3, 4)));

#define add_line(...) real_add_line(&out,&out_size, __VA_ARGS__)
#define add(...) real_add(&out,&out_size, __VA_ARGS__)

#define INIT                    \
  static char *out = NULL;      \
  static size_t out_size = 0;   \
  if (!out) {                   \
    out_size = START_SIZE;      \
    out = (char*)wc_malloc(out_size);  \
  }                             \
  out[0] = '\0';

#define RETURN  return out;

/****************************************************************************
  AI names are prefixed with "AI, ".
****************************************************************************/
static inline const char *get_name_prefix(player_t *plr)
{
  return plr->ai.control ? "AI, " : "";
}

/****************************************************************************

****************************************************************************/
static inline const char *get_proper_username(player_t *plr)
{
  return plr->ai.control ? plr->name : plr->username;
}

/****************************************************************************
  Formats the parameters and appends them. Grows the buffer if
  required.
****************************************************************************/
static void grow_printf(char **buffer, size_t *buffer_size,
                        const char *format, va_list ap)
{
  size_t new_len;
  static char buf[GROW_TMP_SIZE];

  if (my_vsnprintf(buf, sizeof(buf), format, ap) == -1) {
    die("Formatted string bigger than %lu", (unsigned long)sizeof(buf));
  }

  new_len = strlen(*buffer) + strlen(buf) + 1;

  if (new_len > *buffer_size) {
    /* It's important that we grow the buffer geometrically, otherwise the
     * overhead adds up quickly. */
    size_t new_size = MAX(new_len, *buffer_size * 2);

    freelog(LOG_VERBOSE, "expand from %lu to %lu to add '%s'",
            (unsigned long)*buffer_size, (unsigned long)new_size, buf);

    *buffer_size = new_size;
    *buffer = (char*)wc_realloc(*buffer, *buffer_size);
  }
  mystrlcat(*buffer, buf, *buffer_size);
}

/****************************************************************************
  Add a full line of text to the buffer.
****************************************************************************/
static void real_add_line(char **buffer, size_t * buffer_size,
                          const char *format, ...)
{
  va_list args;

  if ((*buffer)[0] != '\0') {
    real_add(buffer, buffer_size, "%s", "\n");
  }

  va_start(args, format);
  grow_printf(buffer, buffer_size, format, args);
  va_end(args);
}

/****************************************************************************
  Add the text to the buffer.
****************************************************************************/
static void real_add(char **buffer, size_t * buffer_size, const char *format,
                     ...)
{
  va_list args;

  va_start(args, format);
  grow_printf(buffer, buffer_size, format, args);
  va_end(args);
}

/***************************************************************
  Return a (static) string with a tile's food/prod/trade
***************************************************************/
static const char *map_get_tile_fpt_text(const tile_t *ptile)
{
  static char s[64];
  char food[16];
  char shields[16];
  char trade[16];
  int x, before_penalty;

  struct government *gov = client_is_global_observer()
                               ? NULL : get_gov_pplayer(get_player_ptr());

  x = get_food_tile(ptile);
  before_penalty = gov ? gov->food_before_penalty : 0;

  if (before_penalty > 0 && x > before_penalty) {
    my_snprintf(food, sizeof(food), "%d(-1)", x);
  } else {
    my_snprintf(food, sizeof(food), "%d", x);
  }

  x = get_shields_tile(ptile);
  before_penalty = gov ? gov->shields_before_penalty : 0;

  if (before_penalty > 0 && x > before_penalty) {
    my_snprintf(shields, sizeof(shields), "%d(-1)", x);
  } else {
    my_snprintf(shields, sizeof(shields), "%d", x);
  }

  x = get_trade_tile(ptile);
  before_penalty = gov ? gov->trade_before_penalty : 0;

  if (before_penalty > 0 && x > before_penalty) {
    my_snprintf(trade, sizeof(trade), "%d(-1)", x);
  } else {
    my_snprintf(trade, sizeof(trade), "%d", x);
  }

  my_snprintf(s, sizeof(s), "%s/%s/%s", food, shields, trade);
  return s;
}

/****************************************************************************
  Build the nation info text. e.g. "Polish, team first).
****************************************************************************/
static const char *get_player_nation_name(player_t *pplayer)
{
  INIT;

  add("%s", get_nation_name(pplayer->nation));

  if (pplayer->team != TEAM_NONE) {
    add(_(", team %s"), get_team_name(pplayer->team));
  }

  RETURN;
}

/****************************************************************************
  Text to popup on a middle-click in the mapview.
****************************************************************************/
const char *popup_info_text(tile_t *ptile)
{
  const char *activity_text;
  city_t *pcity = ptile->city;
  unit_t *punit = find_visible_unit(ptile);
  unit_t *pfocus_unit = get_unit_in_focus();
  const char *diplo_nation_plural_adjectives[DIPLSTATE_LAST] =
    {Q_("?nation:Neutral"), Q_("?nation:Hostile"),
     "" /* unused, DS_CEASEFIRE*/,
     Q_("?nation:Peaceful"), Q_("?nation:Friendly"),
    Q_("?nation:Mysterious"), Q_("?nation:Friendly(team)")
  };
  const char *diplo_city_adjectives[DIPLSTATE_LAST] =
    {Q_("?city:Neutral"), Q_("?city:Hostile"),
     "" /*unused, DS_CEASEFIRE */,
     Q_("?city:Peaceful"), Q_("?city:Friendly"), Q_("?city:Mysterious"),
     Q_("?city:Friendly(team)")
    };
  INIT;

  add_line(_("Location: (%d, %d) [%d]"),
           ptile->x, ptile->y, ptile->continent);
  add_line(_("Native coordinates: (%d, %d)"), ptile->nat_x, ptile->nat_y);

  add_line(_("Terrain: %s"),  map_get_tile_info_text(ptile));
  add_line(_("Food/Prod/Trade: %s"), map_get_tile_fpt_text(ptile));
  if (tile_has_special(ptile, S_HUT)) {
    add_line(_("Minor Tribe Village"));
  }
  if (game.ruleset_control.borders > 0 && !pcity) {
    player_t *owner = map_get_owner(ptile);

    if (client_is_global_observer() && owner) {
      /* TRANS: "Territory of Username (Polish)" */
      add_line(_("Territory of %s%s (%s)"),
               get_name_prefix(owner), get_proper_username(owner),
               get_nation_name_plural(owner->nation));
    } else if (!client_is_global_observer() && owner == get_player_ptr()) {
      add_line(_("Our Territory"));
    } else if (!client_is_global_observer() && owner) {
      struct player_diplstate *ds =
          &get_player_ptr()->diplstates[owner->player_no];
      if (ds[owner->player_no].type == DIPLSTATE_CEASEFIRE) {
        int turns = ds[owner->player_no].turns_left;

        /* TRANS: "Territory of Username (Polish) (5 turn ceasefire)" */
        add_line(PL_("Territory of %s%s (%s) (%d turn ceasefire)",
                     "Territory of %s%s (%s) (%d turn ceasefire)",
                     turns),
                 get_name_prefix(owner),
                 get_proper_username(owner),
                 get_player_nation_name(owner), turns);
      } else {
        /* TRANS: "Territory of friendly Username (Polish)".  See the
         * ?nation adjectives. */
        add_line(_("Territory of %s %s%s (%s)"),
                 diplo_nation_plural_adjectives[ds[owner->player_no].type],
                 get_name_prefix(owner),
                 get_proper_username(owner),
                 get_player_nation_name(owner));
      }
    } else {
      add_line(_("Unclaimed territory"));
    }
  }
  if (pcity) {
    /* Look at city owner, not tile owner (the two should be the same, if
     * borders are in use). */
    player_t *owner = city_owner(pcity);

    if (client_is_global_observer()) {
      /* TRANS: "City: Warsaw | Username (Polish)" */
      add_line(_("City: %s | %s%s (%s)"),
               pcity->common.name,
               get_name_prefix(owner),
               get_proper_username(owner),
               get_player_nation_name(owner));
    } else if (owner == get_player_ptr()) {
      /* TRANS: "City: Warsaw (Polish)" */
      add_line(_("City: %s (%s)"), pcity->common.name, get_player_nation_name(owner));
    } else {
      struct player_diplstate *ds =
          &get_player_ptr()->diplstates[owner->player_no];
      if (ds->type == DIPLSTATE_CEASEFIRE) {
        int turns = ds[owner->player_no].turns_left;

        /* TRANS:  "City: Warsaw | Username (5 turn ceasefire, Polish)" */
        add_line(PL_("City: %s | %s%s (%d turn ceasefire, %s)",
                     "City: %s | %s%s (%d turn ceasefire, %s)",
                                       turns),
                 pcity->common.name,
                 get_name_prefix(owner),
                 get_proper_username(owner),
                 turns, get_player_nation_name(owner));
      } else {
        /* TRANS: "City: Warsaw | Username (friendly, Polish)" */
        add_line(_("City: %s | %s%s (%s, %s)"), pcity->common.name,
                 get_name_prefix(owner),
                 get_proper_username(owner),
                 diplo_city_adjectives[ds->type],
                 get_player_nation_name(owner));
      }
    }
    if (city_got_citywalls(pcity)) {
      /* TRANS: previous lines gave other information about the city. */
      add("%s", _(" with City Walls"));
    }

    if (pfocus_unit) {
      city_t *hcity = find_city_by_id(pfocus_unit->homecity);

      if (unit_flag(pfocus_unit, F_TRADE_ROUTE)
          && can_cities_trade(hcity, pcity)
          && can_establish_trade_route(hcity, pcity)) {
        /* TRANS: "Trade from Warsaw: 5" */
        add_line(_("Trade from %s: %d"),
                 hcity->common.name, trade_between_cities(hcity, pcity));
      }
    }
  }
  if (get_tile_infrastructure_set(ptile)) {
    add_line(_("Infrastructure: %s"),
             map_get_infrastructure_text(ptile->special));
  }
  activity_text = concat_tile_activity_text(ptile);
  if (strlen(activity_text) > 0) {
    add_line(_("Activity: %s"), activity_text);
  }
  if (punit && !pcity) {
    player_t *owner = unit_owner(punit);
    struct unit_type *ptype = unit_type(punit);
    char vet[1024] = "";

    if (client_is_global_observer()) {
      city_t *pcity;
      char tmp[64] = {0};

      pcity = find_city_by_id(punit->homecity);
      if (pcity) {
        my_snprintf(tmp, sizeof(tmp), "/%s", pcity->common.name);
      }
      /* TRANS: "Unit: Musketeers | Username (Polish/Warsaw)" */
      add_line(_("Unit: %s | %s%s (%s%s)"), ptype->name,
               get_name_prefix(owner),
               get_proper_username(owner),
               get_player_nation_name(owner), tmp);
    } else if (owner == get_player_ptr()) {
      city_t *pcity;
      char tmp[64] = {0};

      pcity = find_city_by_id(punit->homecity);
      if (pcity) {
        my_snprintf(tmp, sizeof(tmp), "/%s", pcity->common.name);
      }
      /* TRANS: "Unit: Musketeers (Polish/Warsaw)" */
      add_line(_("Unit: %s (%s%s)"), ptype->name,
               get_player_nation_name(owner), tmp);
    } else {
      struct player_diplstate *ds =
          &get_player_ptr()->diplstates[owner->player_no];
      if (ds->type == DIPLSTATE_CEASEFIRE) {
        int turns = ds[owner->player_no].turns_left;

        /* TRANS:  "Unit: Musketeers | Username (5 turn ceasefire, Polish)" */
        add_line(PL_("Unit: %s | %s%s (%d turn ceasefire, %s)",
                     "Unit: %s | %s%s (%d turn ceasefire, %s)",
                                       turns),
                 ptype->name,
                 get_name_prefix(owner),
                 get_proper_username(owner),
                 turns, get_player_nation_name(owner));
      } else {
        /* TRANS: "Unit: Musketeers | Username (friendly, Polish)" */
        add_line(_("Unit: %s | %s%s (%s, %s)"), ptype->name,
                 get_name_prefix(owner),
                 get_proper_username(owner),
                 diplo_city_adjectives[ds->type],
                 get_player_nation_name(owner));
      }
    }

    if (pfocus_unit) {
      int att_chance = WC_INFINITY, def_chance = WC_INFINITY;
      bool found = FALSE;

      unit_list_iterate(ptile->units, tile_unit) {
        if (tile_unit->owner != pfocus_unit->owner) {
          int att = unit_win_chance(pfocus_unit, tile_unit) * 100;
          int def = (1.0 - unit_win_chance(tile_unit, pfocus_unit)) * 100;

          found = TRUE;

          /* Presumably the best attacker and defender will be used. */
          att_chance = MIN(att, att_chance);
          def_chance = MIN(def, def_chance);
        }
      } unit_list_iterate_end;

      if (found) {
        /* TRANS: "Chance to win: A:95% D:46%" */
        add_line(_("Chance to win: A:%d%% D:%d%%"), att_chance, def_chance);
      }
    }

    my_snprintf(vet, sizeof(vet), " (%s)",
                _(unit_type(punit)->veteran[punit->veteran].name));

    /* TRANS: A is attack power, D is defense power, FP is firepower,
     * HP is hitpoints (current and max). */
    add_line(_("A:%d D:%d FP:%d HP:%d/%d%s"),
             ptype->attack_strength,
             ptype->defense_strength, ptype->firepower, punit->hp,
             ptype->hp, vet);
    if (owner == get_player_ptr() && unit_list_size(ptile->units) >= 2) {
      /* TRANS: "5 more" units on this tile */
      add(_("  (%d more)"), unit_list_size(ptile->units) - 1);
    }
  }
  RETURN;
}

/****************************************************************************
  Creates the activity progress text for the given tile.

  This should only be used inside popup_info_text and should eventually be
  made static.
****************************************************************************/
const char *concat_tile_activity_text(tile_t *ptile)
{
  int activity_total[ACTIVITY_LAST];
  int activity_units[ACTIVITY_LAST];
  int num_activities = 0;
  int remains, turns, i;
  INIT;

  memset(activity_total, 0, sizeof(activity_total));
  memset(activity_units, 0, sizeof(activity_units));

  unit_list_iterate(ptile->units, punit) {
    activity_total[punit->activity] += punit->activity_count;
    activity_total[punit->activity] += get_activity_rate_this_turn(punit);
    activity_units[punit->activity] += get_activity_rate(punit);
  } unit_list_iterate_end;

  for (i = 0; i < ACTIVITY_LAST; i++) {
    if (is_build_or_clean_activity((unit_activity)i) && activity_units[i] > 0) {
      if (num_activities > 0) {
        add("/");
      }
      remains = map_activity_time((unit_activity)i, ptile) - activity_total[i];
      if (remains > 0) {
        turns = 1 + (remains + activity_units[i] - 1) / activity_units[i];
      } else {
        /* activity will be finished this turn */
        turns = 1;
      }
      add("%s(%d)", get_activity_text((unit_activity)i), turns);
      num_activities++;
    }
  }

  RETURN;
}

#define FAR_CITY_SQUARE_DIST (2*(6*6))

/****************************************************************************
  Returns the text describing the city and its distance.
****************************************************************************/
const char *get_nearest_city_text(city_t *pcity, int sq_dist)
{
  INIT;

  /* just to be sure */
  if (!pcity) {
    sq_dist = -1;
  }

  add((sq_dist >= FAR_CITY_SQUARE_DIST) ? _("far from %s")
      : (sq_dist > 0) ? _("near %s")
      : (sq_dist == 0) ? _("in %s")
      : "%s", pcity ? pcity->common.name : "");

  RETURN;
}

/****************************************************************************
  Returns the unit description.
****************************************************************************/
const char *unit_description(unit_t *punit)
{
  int pcity_near_dist;
  city_t *pcity =
      player_find_city_by_id(get_player_ptr(), punit->homecity);
  city_t *pcity_near = get_nearest_city(punit, &pcity_near_dist);
  struct unit_type *ptype = unit_type(punit);
  INIT;

  add("%s", ptype->name);

  if (ptype->veteran[punit->veteran].name[0] != '\0') {
    add(" (%s)", _(ptype->veteran[punit->veteran].name));
  }
  add("\n");
  add_line("%s", unit_activity_text(punit));

  if (pcity) {
    /* TRANS: "from Warsaw" */
    add_line(_("from %s"), pcity->common.name);
  } else {
    add("\n");
  }

  add_line("%s", get_nearest_city_text(pcity_near, pcity_near_dist));

  RETURN;
}

/****************************************************************************
  Returns the text to display in the science dialog.
****************************************************************************/
const char *science_dialog_text(void)
{
  int turns_to_advance;
  player_t *plr = get_player_ptr();
  int ours = player_get_expected_bulbs(plr), theirs = -1;
  INIT;

  /* Sum up science */
  players_iterate(pplayer) {
    enum diplstate_type ds = pplayer_get_diplstate(plr, pplayer)->type;

    if (plr != pplayer && ds == DIPLSTATE_TEAM) {
      if (theirs == -1) {
        theirs = 0;
      }
      theirs += pplayer->research.bulbs_last_turn;
    }
  } players_iterate_end;

  if (ours <= 0) {
    turns_to_advance = 0;
  } else {
    turns_to_advance = (total_bulbs_required(plr) + ours - 1) / ours;
  }
  if (ours > 0) {
    add(PL_("Progress: %d turn/advance, %d pts/turn",
            "Progress: %d turns/advance, %d pts/turn",
            turns_to_advance), turns_to_advance, ours);
  } else {
    add(_("Progress: Never"));
  }
  if (theirs >= 0) {
    add(_(" (%d pts/turn by team)"), theirs);
  }
  RETURN;
}

/****************************************************************************
  Return the text for the label on the info panel.  (This is traditionally
  shown to the left of the mapview.)
****************************************************************************/
const char *get_info_label_text(void)
{
  INIT;

  player_t *pplayer = get_player_ptr();

  if (pplayer) {
    add_line(_("Population: %s"),
               population_to_text(civ_population(pplayer)));
  }
  add_line(_("Year: %s - T%d"), textyear(game.info.year), game.info.turn);
  if (pplayer) {
    add_line(_("Gold: %d"), pplayer->economic.gold);
    add_line(_("Tax: %d Lux: %d Sci: %d"), pplayer->economic.tax,
             pplayer->economic.luxury, pplayer->economic.science);
  }
  RETURN;
}

/****************************************************************************
  Return the title text for the unit info shown in the info panel.

  FIXME: this should be renamed.
****************************************************************************/
const char *get_unit_info_label_text1(unit_t *punit)
{
  INIT;

  if (punit) {
    struct unit_type *ptype = unit_type(punit);

    int i = multi_select_count_all ? multi_select_size(0) -1
                                   : multi_select_satisfies_filter(0) - 1;
    if (i <= 0) {
      add("%s", ptype->name);
    } else {
      add("%s (+%d %s)", ptype->name, i, PL_("unit", "units", i));
    }
  }
  RETURN;
}

/****************************************************************************
  Return the text body for the unit info shown in the info panel.

  FIXME: this should be renamed.
****************************************************************************/
const char *get_unit_info_label_text2(unit_t *punit)
{
  INIT;

  /* This text should always have the same number of lines.  Otherwise the
   * GUI widgets may be confused and try to resize themselves. */
  if (punit) {
    city_t *pcity =
        player_find_city_by_id(get_player_ptr(), punit->homecity);
    int infrastructure = get_tile_infrastructure_set(punit->tile);

    add_line("%s", unit_activity_text(punit));
    if (hover_unit == punit->id) {
      add_line(_("Turns to target: %d"), get_goto_turns());
    } else {
      add_line(" ");
    }

    add_line("%s", map_get_tile_info_text(punit->tile));
    if (infrastructure) {
      add_line("%s", map_get_infrastructure_text((tile_special_type)infrastructure));
    } else {
      add_line(" ");
    }
    if (pcity) {
      add_line("%s", pcity->common.name);
    } else {
      add_line(" ");
    }
  } else {
    add("\n\n\n\n");
  }
  RETURN;
}

/****************************************************************************
  Get a tooltip text for the info panel research indicator.  See
  client_research_sprite().
****************************************************************************/
const char *get_bulb_tooltip(void)
{
  INIT;

  player_t *pplayer = get_player_ptr();

  add(_("Shows your progress in researching "
        "the current technology.\n%s: %d/%d."),
      get_tech_name(pplayer, pplayer->research.researching),
      pplayer->research.bulbs_researched, total_bulbs_required(pplayer));
  RETURN;
}

/****************************************************************************
  Get a tooltip text for the info panel global warning indicator.  See also
  client_warming_sprite().
****************************************************************************/
const char *get_global_warming_tooltip(void)
{
  INIT;

  add(_("Shows the progress of global warming:\n%d."),
      client_warming_sprite());
  RETURN;
}

/****************************************************************************
  Get a tooltip text for the info panel nuclear winter indicator.  See also
  client_cooling_sprite().
****************************************************************************/
const char *get_nuclear_winter_tooltip(void)
{
  INIT;

  add(_("Shows the progress of nuclear winter:\n%d."),
      client_cooling_sprite());
  RETURN;
}

/****************************************************************************
  Get a tooltip text for the info panel government indicator.  See also
  get_government(...)->sprite.
****************************************************************************/
const char *get_government_tooltip(void)
{
  INIT;

  add(_("Shows your current government:\n%s."),
      get_government_name(get_player_ptr()->government));
  RETURN;
}

/****************************************************************************
  Returns a description of the given spaceship.  If there is no spaceship
  (pship is NULL) then text with dummy values is returned.
****************************************************************************/
const char *get_spaceship_descr(struct player_spaceship *pship)
{
  struct player_spaceship ship;
  INIT;

  if (!pship) {
    pship = &ship;
    memset(&ship, 0, sizeof(ship));
  }

  /* TRANS: spaceship text; should have constant width. */
  add_line(_("Population:      %5d"), pship->population);

  /* TRANS: spaceship text; should have constant width. */
  add_line(_("Support:         %5d %%"),
           (int) (pship->support_rate * 100.0));

  /* TRANS: spaceship text; should have constant width. */
  add_line(_("Energy:          %5d %%"), (int) (pship->energy_rate * 100.0));

  /* TRANS: spaceship text; should have constant width. */
  add_line(PL_("Mass:            %5d ton",
               "Mass:            %5d tons", pship->mass), pship->mass);

  if (pship->propulsion > 0) {
    /* TRANS: spaceship text; should have constant width. */
    add_line(_("Travel time:     %5.1f years"),
             (float) (0.1 * ((int) (pship->travel_time * 10.0))));
  } else {
    /* TRANS: spaceship text; should have constant width. */
    add_line("%s", _("Travel time:        N/A     "));
  }

  /* TRANS: spaceship text; should have constant width. */
  add_line(_("Success prob.:   %5d %%"),
           (int) (pship->success_rate * 100.0));

  /* TRANS: spaceship text; should have constant width. */
  add_line(_("Year of arrival: %8s"),
           (pship->state ==
            SSHIP_LAUNCHED) ? textyear((int) (pship->launch_year +
                                              (int) pship->
                                              travel_time)) : "-   ");

  RETURN;
}

/****************************************************************************
  Get the text showing the timeout.  This is generally disaplyed on the info
  panel.
****************************************************************************/
const char *get_timeout_label_text(void)
{
  INIT;

  if (game.info.timeout <= 0) {
    add("%s", Q_("?timeout:off"));
  } else {
    add("%s", format_duration(game.info.seconds_to_turndone));
  }

  RETURN;
}

/****************************************************************************
  Format a duration, in seconds, so it comes up in minutes or hours if
  that would be more meaningful.

  (7 characters, maximum.  Enough for, e.g., "99h 59m".)
****************************************************************************/
const char *format_duration(int duration)
{
  INIT;

  if (duration < 0) {
    duration = 0;
  }
  if (duration < 60) {
    add(Q_("?seconds:%02ds"), duration);
  } else if (duration < 3600) { /* < 60 minutes */
    add(Q_("?mins/secs:%02dm %02ds"), duration / 60, duration % 60);
  } else if (duration < 360000) {       /* < 100 hours */
    add(Q_("?hrs/mns:%02dh %02dm"), duration / 3600, (duration / 60) % 60);
  } else if (duration < 8640000) {      /* < 100 days */
    add(Q_("?dys/hrs:%02dd %02dh"), duration / 86400,
        (duration / 3600) % 24);
  } else {
    add("%s", Q_("?duration:overflow"));
  }

  RETURN;
}

/****************************************************************************
  Return text giving the ping time for the player.  This is generally used
  used in the playerdlg.  This should only be used in playerdlg_common.c.
****************************************************************************/
const char *get_ping_time_text(player_t *pplayer)
{
  connection_t *pconn = NULL;
  double ping_time_in_ms;
  INIT;

  connection_list_iterate(game.est_connections, oconn) {
    if (oconn->player == pplayer
        && !oconn->observer
        /* Certainly not needed, but safer */
        && 0 == strcmp(oconn->username, pplayer->username)) {
      pconn = oconn;
    }
  } connection_list_iterate_end;

  if (pconn && pconn->ping_time != -1) {
    ping_time_in_ms = 1000 * pconn->ping_time;

    add(_("%6d.%02d ms"), (int) ping_time_in_ms,
        ((int) (ping_time_in_ms * 100.0)) % 100);
  }

  RETURN;
}

/****************************************************************************
  Get the title for a "report".  This may include the city, economy,
  military, trade, player, etc., reports.  Some clients may generate the
  text themselves to get a better GUI layout.
****************************************************************************/
const char *get_report_title(const char *report_name)
{
  INIT;

  player_t *pplayer = get_player_ptr();

  add_line("%s", report_name);

  /* TRANS: "Republic of the Polish" */
  add_line(_("%s of the %s"),
           get_government_name(pplayer->government),
           get_nation_name_plural(pplayer->nation));

  add_line("%s %s: %s",
           get_ruler_title(pplayer->government,
                           pplayer->is_male,
                           pplayer->nation),
           pplayer->name, textyear(game.info.year));
  RETURN;
}

/****************************************************************************
  Get the text describing buildings that affect happiness.
****************************************************************************/
const char *get_happiness_buildings(const city_t *pcity)
{
  int faces = 0;
  struct effect_source_vector sources;
  INIT;

  add_line(_("Buildings: "));

  get_city_bonus_sources(&sources, pcity, EFFECT_TYPE_MAKE_CONTENT);
  effect_source_vector_iterate(&sources, src) {
    faces++;
    add(_("%s. "), get_improvement_name(src->building));
  } effect_source_vector_iterate_end;
  effect_source_vector_free(&sources);

  if (faces == 0) {
    add(_("None. "));
  }

  RETURN;
}

/****************************************************************************
  Get the text describing wonders that affect happiness.
****************************************************************************/
const char *get_happiness_wonders(const city_t *pcity)
{
  int faces = 0;
  struct effect_source_vector sources;
  INIT;

  add_line(_("Wonders: "));

  get_city_bonus_sources(&sources, pcity, EFFECT_TYPE_MAKE_HAPPY);
  effect_source_vector_iterate(&sources, src) {
    faces++;
    add(_("%s. "), get_improvement_name(src->building));
  } effect_source_vector_iterate_end;
  effect_source_vector_free(&sources);

  get_city_bonus_sources(&sources, pcity, EFFECT_TYPE_FORCE_CONTENT);
  effect_source_vector_iterate(&sources, src) {
    faces++;
    add(_("%s. "), get_improvement_name(src->building));
  } effect_source_vector_iterate_end;
  effect_source_vector_free(&sources);

  get_city_bonus_sources(&sources, pcity, EFFECT_TYPE_NO_UNHAPPY);
  effect_source_vector_iterate(&sources, src) {
    faces++;
    add(_("%s. "), get_improvement_name(src->building));
  } effect_source_vector_iterate_end;
  effect_source_vector_free(&sources);

  if (faces == 0) {
    add(_("None. "));
  }

  RETURN;
}
