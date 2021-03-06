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
#include "../config.hh"
#endif

#include "game.hh"
#include "log.hh"
#include "mem.hh"
#include "player.hh"

#include "diptreaty.hh"

/**************************************************************************
  Returns TRUE iff pplayer could do diplomancy in the game at all.
  These values are set by player in stdinhand.c.
**************************************************************************/
bool diplomacy_possible(player_t *pplayer, player_t *aplayer)
{
  if (game.info.diplomacy == 4
      || (game.ext_info.maxallies
          && !pplayers_allied(pplayer, aplayer)
          && (player_allies_count(pplayer) >= game.ext_info.maxallies
              || player_allies_count(aplayer) >= game.ext_info.maxallies))) {
    return FALSE;
  }

  return  (game.info.diplomacy == 0      /* Unlimited diplomacy */
           || (game.info.diplomacy == 1  /* Human diplomacy only */
               && !pplayer->ai.control
               && !aplayer->ai.control)
           || (game.info.diplomacy == 2  /* AI diplomacy only */
               && pplayer->ai.control
               && aplayer->ai.control)
           || (game.info.diplomacy == 3  /* Team diplomacy only */
               && players_on_same_team(pplayer, aplayer)));
}

/**************************************************************************
  Returns TRUE iff pplayer could do diplomatic meetings with aplayer.
**************************************************************************/
bool could_meet_with_player(player_t *pplayer, player_t *aplayer)
{
  return (pplayer->is_alive
          && aplayer->is_alive
          && pplayer != aplayer
          && diplomacy_possible(pplayer,aplayer)
          && (player_has_embassy(aplayer, pplayer)
              || player_has_embassy(pplayer, aplayer)
              || pplayer->diplstates[aplayer->player_no].contact_turns_left > 0
              || aplayer->diplstates[pplayer->player_no].contact_turns_left > 0)
          && (pplayer->is_connected || pplayer->ai.control));
}

/**************************************************************************
  Returns TRUE iff pplayer could do diplomatic meetings with aplayer.
**************************************************************************/
bool could_intel_with_player(player_t *pplayer, player_t *aplayer)
{
  return (pplayer->is_alive
          && aplayer->is_alive
          && pplayer != aplayer
          && (pplayer->diplstates[aplayer->player_no].contact_turns_left > 0
              || aplayer->diplstates[pplayer->player_no].contact_turns_left > 0
              || player_has_embassy(pplayer, aplayer)));
}

/****************************************************************
...
*****************************************************************/
struct Treaty *treaty_new(player_t *plr0, player_t *plr1)
{
  struct Treaty *ptreaty = (struct Treaty *)wc_malloc(sizeof(struct Treaty));

  ptreaty->plr0 = plr0;
  ptreaty->plr1 = plr1;
  ptreaty->accept0 = FALSE;
  ptreaty->accept1 = FALSE;
  ptreaty->clauses = clause_list_new();

  return ptreaty;
}

/****************************************************************
  ...
*****************************************************************/
void treaty_free(struct Treaty *ptreaty)
{
  clause_list_iterate(ptreaty->clauses, pclause) {
    free(pclause);
  } clause_list_iterate_end;
  clause_list_free(ptreaty->clauses);
  free(ptreaty);
}

/****************************************************************
...
*****************************************************************/
bool remove_clause(struct Treaty *ptreaty, player_t *pfrom,
                  enum clause_type type, int val)
{
  clause_list_iterate(ptreaty->clauses, pclause) {
    if (pclause->type == type && pclause->from == pfrom
        && pclause->value == val) {
      clause_list_unlink(ptreaty->clauses, pclause);
      free(pclause);

      ptreaty->accept0 = FALSE;
      ptreaty->accept1 = FALSE;

      return TRUE;
    }
  } clause_list_iterate_end;

  return FALSE;
}


/****************************************************************
...
*****************************************************************/
bool add_clause(struct Treaty *ptreaty, player_t *pfrom,
                enum clause_type type, int val)
{
  struct Clause *pclause;
  enum diplstate_type ds =
                     pplayer_get_diplstate(ptreaty->plr0, ptreaty->plr1)->type;

  if ( type >= CLAUSE_LAST) {
    freelog(LOG_ERROR, "Illegal clause type encountered.");
    return FALSE;
  }

  if (type == CLAUSE_ADVANCE && !tech_exists(val)) {
    freelog(LOG_ERROR, "Illegal tech value %i in clause.", val);
    return FALSE;
  }

  if (!game.ext_info.goldtrading && type == CLAUSE_GOLD) {
    return FALSE;
  }
  if (!game.ext_info.techtrading && type == CLAUSE_ADVANCE) {
    return FALSE;
  }
  if (!game.ext_info.citytrading && type == CLAUSE_CITY) {
    return FALSE;
  }

  if (is_pact_clause(type)
      && ((ds == DIPLSTATE_PEACE && type == CLAUSE_PEACE)
          || (ds == DIPLSTATE_ALLIANCE && type == CLAUSE_ALLIANCE)
          || (ds == DIPLSTATE_CEASEFIRE && type == CLAUSE_CEASEFIRE))) {
    /* we already have this diplomatic state */
    freelog(LOG_ERROR, "Illegal treaty suggested between %s and %s - they "
                       "already have this treaty level.", ptreaty->plr0->name,
                       ptreaty->plr1->name);
    return FALSE;
  }

  clause_list_iterate(ptreaty->clauses, pclause) {
    if (pclause->type == type
       && pclause->from == pfrom
       && pclause->value == val) {
      /* same clause already there */
      return FALSE;
    }
    if(is_pact_clause(type) &&
       is_pact_clause(pclause->type)) {
      /* pact clause already there */
      ptreaty->accept0 = FALSE;
      ptreaty->accept1 = FALSE;
      pclause->type=type;
      return TRUE;
    }
    if (type == CLAUSE_GOLD && pclause->type==CLAUSE_GOLD &&
        pclause->from == pfrom) {
      /* gold clause there, different value */
      ptreaty->accept0 = FALSE;
      ptreaty->accept1 = FALSE;
      pclause->value=val;
      return TRUE;
    }
  } clause_list_iterate_end;

  pclause = (struct Clause *)wc_malloc(sizeof(struct Clause));

  pclause->type = type;
  pclause->from = pfrom;
  pclause->value = val;

  clause_list_append(ptreaty->clauses, pclause);

  ptreaty->accept0 = FALSE;
  ptreaty->accept1 = FALSE;

  return TRUE;
}
