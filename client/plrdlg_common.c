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
#include <config.h>
#endif

#include <assert.h>

#include "fcintl.h"
#include "support.h"

#include "city.h"
#include "connection.h"
#include "game.h"
#include "government.h"
#include "player.h"
#include "unit.h"

#include "civclient.h"
#include "climisc.h"
#include "text.h"

#include "plrdlg_g.h"

#include "plrdlg_common.h"

static int frozen_level = 0;

/******************************************************************
 Turn off updating of player dialog
*******************************************************************/
void plrdlg_freeze(void)
{
  frozen_level++;
}

/******************************************************************
 Turn on updating of player dialog
*******************************************************************/
void plrdlg_thaw(void)
{
  frozen_level--;
  assert(frozen_level >= 0);
  if (frozen_level == 0) {
    update_players_dialog();
  }
}

/******************************************************************
 Turn on updating of player dialog
*******************************************************************/
void plrdlg_force_thaw(void)
{
  frozen_level = 1;
  plrdlg_thaw();
}

/******************************************************************
 ...
*******************************************************************/
bool is_plrdlg_frozen(void)
{
  return frozen_level > 0;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_name(struct player *player)
{
  return player->name;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_username(struct player *player)
{
  return player->username;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_nation(struct player *player)
{
  return get_nation_name(player->nation);
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_government(struct player *player)
{
  if (government_exists(player->government)) {
    return get_government_name(player->government);
  }
  return "-";
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_team(struct player *player)
{
  if (player->team != TEAM_NONE) {
    return get_team_name(player->team);
  } else {
    return "";
  }
}

/******************************************************************
 ...
*******************************************************************/
static bool col_ai(struct player *plr)
{
  return plr->ai.control;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_embassy(struct player *player)
{
  return client_is_global_observer()
	     ? "-" : get_embassy_status(get_player_ptr(), player);
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_diplstate(struct player *player)
{
  static char buf[100];
  const struct player_diplstate *pds;

  if (client_is_global_observer() || player == get_player_ptr()) {
    return "-";
  } else {
    pds = pplayer_get_diplstate(get_player_ptr(), player);
    if (pds->type == DS_CEASEFIRE) {
      my_snprintf(buf, sizeof(buf), "%s (%d)",
		  diplstate_text(pds->type), pds->turns_left);
      return buf;
    } else {
      return diplstate_text(pds->type);
    }
  }
}

/******************************************************************
  Return a string displaying the AI's love (or not) for you...
*******************************************************************/
static const char *col_love(struct player *player)
{
  if (client_is_global_observer()
      || player == get_player_ptr()
      || !player->ai.control) {
    return "-";
  } else {
    return love_text(player->ai.love[get_player_idx()]);
  }
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_vision(struct player *player)
{
  return client_is_global_observer()
	     ? "-" : get_vision_status(get_player_ptr(), player);
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_cities(struct player *player)
{
  static char buf[100];

  my_snprintf(buf, sizeof(buf), "%4d", city_list_size(player->cities));

  return buf;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_units(struct player *player)
{
  static char buf[100];

  my_snprintf(buf, sizeof(buf), "%4d", unit_list_size(player->units));

  return buf;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_population(struct player *player)
{
  return population_to_text(civ_population(player));
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_gold(struct player *player)
{
  static char buf[100];

  my_snprintf(buf, sizeof(buf), "%6d", player->economic.gold);

  return buf;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_income(struct player *player)
{
  static char buf[100];

  my_snprintf(buf, sizeof(buf), "%+4d", player_get_expected_income(player));

  return buf;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_research(struct player *player)
{
  static char buf[100];

  if (player->research.researching != A_NOINFO) {
    my_snprintf(buf, sizeof(buf), "%s(%d/%d)",
		get_tech_name(player, player->research.researching),
		player->research.bulbs_researched,
		player->research.researching_cost);
  } else {
    my_snprintf(buf, sizeof(buf), _("(Unknown)"));
  }

  return buf;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_bulbs(struct player *player)
{
  static char buf[100];

  my_snprintf(buf, sizeof(buf), "%+4d", player_get_expected_bulbs(player));

  return buf;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_science_goal(struct player *player)
{
  static char buf[100];

  if (player->ai.tech_goal != A_UNSET) {
    int steps = num_unknown_techs_for_goal(player, player->ai.tech_goal);

    my_snprintf(buf, sizeof(buf), "%s (%d %s)",
		get_tech_name(player, player->ai.tech_goal),
		steps, PL_("step", "steps", steps));
  } else {
    sz_strlcpy(buf, "-");
  }

  return buf;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_techs(struct player *player)
{
  static char buf[100];

  if (player->research.techs_researched > 0) {
    /* We always know A_NONE, so remove one tech in the counter. */
    my_snprintf(buf, sizeof(buf), "%3d", player->research.techs_researched - 1);
  } else {
    sz_strlcpy(buf, "-");
  }

  return buf;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_production(struct player *player)
{
  static char buf[100];
  int prod = 0;

  city_list_iterate(player->cities, pcity) {
    prod += pcity->shield_surplus;
  } city_list_iterate_end;

  my_snprintf(buf, sizeof(buf), "%4d", prod);

  return buf;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_economics(struct player *player)
{
  static char buf[100];
  int eco = 0;

  city_list_iterate(player->cities, pcity) {
    eco += pcity->trade_prod;
  } city_list_iterate_end;

  my_snprintf(buf, sizeof(buf), "%4d", eco);

  return buf;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_rates(struct player *player)
{
  static char buf[100];

  if (player->economic.tax + player->economic.luxury
      + player->economic.science == 100) {
    my_snprintf(buf, sizeof(buf), "T:%d L:%d S:%d",
		player->economic.tax, player->economic.luxury,
		player->economic.science);
  } else {
    sz_strlcpy(buf, "-");
  }

  return buf;
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_reputation(struct player *player)
{
  return reputation_text(player->reputation);
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_state(struct player *plr)
{
  if (plr->is_alive) {
    if (plr->is_connected) {
      if (plr->turn_done) {
	return _("done");
      } else {
	return _("moving");
      }
    } else {
      return "";
    }
  } else {
    return _("R.I.P");
  }
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_host(struct player *player)
{
  return player_addr_hack(player);
}

/******************************************************************
 ...
*******************************************************************/
static const char *col_idle(struct player *plr)
{
  int idle;
  static char buf[100];

  if (plr->nturns_idle > 3) {
    idle = plr->nturns_idle - 1;
  } else {
    idle = 0;
  }
  my_snprintf(buf, sizeof(buf), "%d", idle);
  return buf;
}

/******************************************************************
 ...
*******************************************************************/
struct player_dlg_column player_dlg_columns[] = {
  {TRUE, COL_TEXT, CF_COMMON, N_("?Player:Name"), col_name, NULL, "name"},
  {FALSE, COL_TEXT, CF_COMMON, N_("Username"), col_username, NULL, "username"},
  {TRUE, COL_FLAG, CF_COMMON, N_("Flag"), NULL, NULL, "flag"},
  {TRUE, COL_TEXT, CF_COMMON, N_("Nation"), col_nation, NULL, "nation"},
  {FALSE, COL_TEXT, CF_COMMON, N_("Government"), col_government, NULL, "government"},
  {TRUE, COL_COLOR, CF_COMMON, N_("Border"), NULL, NULL, "border"},
  {TRUE, COL_TEXT, CF_COMMON, N_("Team"), col_team, NULL, "team"},
  {TRUE, COL_BOOLEAN, CF_COMMON, N_("AI"), NULL, col_ai, "ai"},
  {TRUE, COL_TEXT, CF_PLAYER, N_("Attitude"), col_love, NULL, "attitude"},
  {TRUE, COL_TEXT, CF_PLAYER, N_("Embassy"), col_embassy, NULL, "embassy"},
  {TRUE, COL_TEXT, CF_PLAYER, N_("Dipl.State"), col_diplstate, NULL, "diplstate"},
  {TRUE, COL_TEXT, CF_PLAYER, N_("Vision"), col_vision, NULL, "vision"},
  {FALSE, COL_TEXT, CF_COMMON, N_("Cities"), col_cities, NULL, "cities"},
  {FALSE, COL_TEXT, CF_COMMON, N_("Units"), col_units, NULL, "units"},
  {FALSE, COL_TEXT, CF_COMMON, N_("Population"), col_population, NULL, "population"},
  {FALSE, COL_TEXT, CF_COMMON, N_("Gold"), col_gold, NULL, "gold"},
  {FALSE, COL_TEXT, CF_GLOBAL_OBSERVER, N_("Income"), col_income, NULL, "income"},
  {FALSE, COL_TEXT, CF_COMMON, N_("Research"), col_research, NULL, "research"},
  {FALSE, COL_TEXT, CF_GLOBAL_OBSERVER, N_("Bulbs"), col_bulbs, NULL, "bulbs"},
  {FALSE, COL_TEXT, CF_COMMON, N_("Science goal"), col_science_goal, NULL, "goal"},
  {FALSE, COL_TEXT, CF_COMMON, N_("Techs"), col_techs, NULL, "techs"},
  {TRUE, COL_TEXT, CF_GLOBAL_OBSERVER, N_("Production"), col_production, NULL, "production"},
  {TRUE, COL_TEXT, CF_GLOBAL_OBSERVER, N_("Economics"), col_economics, NULL, "economics"},
  {FALSE, COL_TEXT, CF_COMMON, N_("Rates"), col_rates, NULL, "rates"},
  {TRUE, COL_TEXT, CF_COMMON, N_("Reputation"), col_reputation, NULL, "reputation"},
  {TRUE, COL_TEXT, CF_COMMON, N_("State"), col_state, NULL, "state"},
  {FALSE, COL_TEXT, CF_COMMON, N_("?Player_dlg:Host"), col_host, NULL, "host"},
  {FALSE, COL_RIGHT_TEXT, CF_COMMON, N_("?Player_dlg:Idle"), col_idle, NULL, "idle"},
  {FALSE, COL_RIGHT_TEXT, CF_COMMON, N_("Ping"), get_ping_time_text, NULL, "ping"}
};

const int num_player_dlg_columns = ARRAY_SIZE(player_dlg_columns);

/******************************************************************
 ...
*******************************************************************/
int player_dlg_default_sort_column(void)
{
  return 3;
}

/****************************************************************************
  Translate all titles
****************************************************************************/
void init_player_dlg_common()
{
  int i;

  for (i = 0; i < num_player_dlg_columns; i++) {
    player_dlg_columns[i].title = Q_(player_dlg_columns[i].title);
  }
}

/**************************************************************************
  The only place where this is used is the player dialog.
  Eventually this should go the way of the dodo with everything here
  moved into col_host above, but some of the older clients (+win32) still
  use this function directly.

  This code in this function is only really needed so that the host is
  kept as a blank address if no one is controlling a player, but there are
  observers.
**************************************************************************/
const char *player_addr_hack(struct player *pplayer)
{ 
  conn_list_iterate(pplayer->connections, pconn) {
    if (!pconn->observer) {
      return pconn->addr;
    }
  } conn_list_iterate_end;

  return blank_addr_str;
}   

/****************************************************************************
  Return TRUE if this column can be visible.
****************************************************************************/
bool column_can_be_visible(struct player_dlg_column *pcol)
{
  if (client_is_global_observer()) {
    return pcol->flag != CF_PLAYER;
  } else {
    return pcol->flag != CF_GLOBAL_OBSERVER;
  }
}
