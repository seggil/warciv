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

#include <stdio.h>
#include <stdlib.h>

#include "diptreaty.h"
#include "events.h"
#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "packets.h"
#include "player.h"
#include "unit.h"

#include "citytools.h"
#include "cityturn.h"
#include "gamelog.h"
#include "maphand.h"
#include "plrhand.h"
#include "settlers.h"
#include "unittools.h"

#include "advdiplomacy.h"

#include "diplhand.h"

#define SPECLIST_TAG treaty
#define SPECLIST_TYPE struct Treaty
#include "speclist.h"

#define treaty_list_iterate(list, p) \
    TYPED_LIST_ITERATE(struct Treaty, list, p)
#define treaty_list_iterate_end  LIST_ITERATE_END

static struct treaty_list *treaties = NULL;

/**************************************************************************
...
**************************************************************************/
void diplhand_init(void)
{
  if (treaties != NULL) {
    diplhand_free();
  }
  treaties = treaty_list_new();
}

/**************************************************************************
  Free all the resources allocated by diplhand.
**************************************************************************/
void diplhand_free(void)
{
}

/**************************************************************************
  Free all the treaties currently in treaty list.
**************************************************************************/
void free_treaties(void)
{
  if (!treaties) {
    return;
  }

  /* Free memory allocated for treaties */
  treaty_list_iterate(treaties, pt) {
    treaty_free(pt);
  } treaty_list_iterate_end;

  treaty_list_free(treaties);
  treaties = NULL;
}

/**************************************************************************
...
**************************************************************************/
struct Treaty *find_treaty(struct player *plr0, struct player *plr1)
{
  treaty_list_iterate(treaties, ptreaty) {
    if ((ptreaty->plr0 == plr0 && ptreaty->plr1 == plr1) ||
	(ptreaty->plr0 == plr1 && ptreaty->plr1 == plr0)) {
      return ptreaty;
    }
  } treaty_list_iterate_end;

  return NULL;
}

/**************************************************************************
pplayer clicked the accept button. If he accepted the treaty we check the
clauses. If both players have now accepted the treaty we execute the agreed
clauses.
**************************************************************************/
void handle_diplomacy_accept_treaty_req(struct player *pplayer,
					int counterpart)
{
  struct Treaty *ptreaty;
  struct player *pother;
  bool *player_accept, *other_accept;

  if (!pplayer
      || !is_valid_player_id(counterpart)
      || pplayer->player_no == counterpart) {
    return;
  }

  pother = get_player(counterpart);
  ptreaty = find_treaty(pplayer, pother);

  if (!ptreaty) {
    return;
  }

  if (ptreaty->plr0 == pplayer) {
    player_accept = &ptreaty->accept0;
    other_accept = &ptreaty->accept1;
  } else {
    player_accept = &ptreaty->accept1;
    other_accept = &ptreaty->accept0;
  }

  if (!*player_accept) {	/* Tries to accept. */

    /* Check that player who accepts can keep what (s)he promises. */

    clause_list_iterate(ptreaty->clauses, pclause) {
      struct city *pcity = NULL;

      if (pclause->from == pplayer) {
	switch(pclause->type) {
	case CLAUSE_EMBASSY:
          if (player_has_embassy(pother, pplayer)) {
            freelog(LOG_ERROR, "%s tried to give embassy to %s, who already "
                    "has an embassy", pplayer->name, pother->name);
            return;
          }
          break;
	case CLAUSE_ADVANCE:
          if (!tech_is_available(pother, pclause->value)) {
	    /* It is impossible to give a technology to a civilization that
	     * can never possess it (the client should enforce this). */
	    freelog(LOG_ERROR, "Treaty: The %s can't have tech %s",
                    get_nation_name_plural(pother->nation),
		    get_tech_name(pplayer, pclause->value));
	    notify_player(pplayer,
                          _("Game: The %s can't accept %s."),
                          get_nation_name_plural(pother->nation),
			  get_tech_name(pplayer, pclause->value));
	    return;
          }
	  if (get_invention(pplayer, pclause->value) != TECH_KNOWN) {
	    freelog(LOG_ERROR,
                    "The %s don't know tech %s, but try to give it to the %s.",
		    get_nation_name_plural(pplayer->nation),
		    get_tech_name(pplayer, pclause->value),
		    get_nation_name_plural(pother->nation));
	    notify_player(pplayer,
			  _("Game: You don't have tech %s, you can't accept treaty."),
			  get_tech_name(pplayer, pclause->value));
	    return;
	  }
	  break;
	case CLAUSE_CITY:
	  pcity = find_city_by_id(pclause->value);
	  if (!pcity) { /* Can't find out cityname any more. */
	    notify_player(pplayer,
			  _("City you are trying to give no longer exists, "
			    "you can't accept treaty."));
	    return;
	  }
	  if (pcity->owner != pplayer->player_no) {
	    notify_player(pplayer,
			  _("You are not owner of %s, you can't accept treaty."),
			  pcity->name);
	    return;
	  }
	  if (is_capital(pcity)) {
	    notify_player(pplayer,
			  _("Game: Your capital (%s) is requested, "
			    "you can't accept treaty."),
			  pcity->name);
	    return;
	  }
	  break;
	case CLAUSE_ALLIANCE:
	  if(game.ext_info.maxallies) {
	    if(player_allies_count(pplayer) >= game.ext_info.maxallies) {
	      notify_player(pplayer, _("Game: You have already %d/%d alliances."),
		player_allies_count(pplayer), game.ext_info.maxallies);
	      return;
            }
	    if(player_allies_count(pother) >= game.ext_info.maxallies) {
	      notify_player(pplayer, _("Game: %s has already %d/%d alliances."),
		pother->name, player_allies_count(pother), game.ext_info.maxallies);
	      return;
            }
	  } 
          if (!pplayer_can_ally(pplayer, pother)) {
	    notify_player(pplayer,
			  _("Game: You are at war with one of %s's "
			    "allies - an alliance with %s is impossible."),
			  pother->name, pother->name);
            return;
          }
          if (!pplayer_can_ally(pother, pplayer)) {
	    notify_player(pplayer,
			  _("Game: %s is at war with one of your allies "
			    "- an alliance with %s is impossible."),
			  pother->name, pother->name);
            return;
          }
          break;
	case CLAUSE_GOLD:
	  if (pplayer->economic.gold < pclause->value) {
	    notify_player(pplayer,
			  _("Game: You don't have enough gold, "
			    "you can't accept treaty."));
	    return;
	  }
	  break;
	default:
	  ; /* nothing */
	}
      }
    } clause_list_iterate_end;
  }

  *player_accept = ! *player_accept;

  dlsend_packet_diplomacy_accept_treaty(pplayer->connections,
					pother->player_no, *player_accept,
					*other_accept);
  dlsend_packet_diplomacy_accept_treaty(pother->connections,
					pplayer->player_no, *other_accept,
					*player_accept);

  if (ptreaty->accept0 && ptreaty->accept1) {
    int nclauses = clause_list_size(ptreaty->clauses);

    dlsend_packet_diplomacy_cancel_meeting(pplayer->connections,
					   pother->player_no,
					   pplayer->player_no);
    dlsend_packet_diplomacy_cancel_meeting(pother->connections,
					   pplayer->player_no,
 					   pplayer->player_no);

    notify_player(pplayer,
		  PL_("Game: A treaty containing %d clause was agreed upon.",
		      "Game: A treaty containing %d clauses was agreed upon.",
		      nclauses),
		  nclauses);
    notify_player(pother,
		  PL_("Game: A treaty containing %d clause was agreed upon.",
		      "Game: A treaty containing %d clauses was agreed upon.",
		      nclauses),
		  nclauses);

    /* Check that one who accepted treaty earlier still have everything
       (s)he promised to give. */

    clause_list_iterate(ptreaty->clauses, pclause) {
      struct city *pcity;
      if (pclause->from == pother) {
	switch (pclause->type) {
	case CLAUSE_CITY:
	  pcity = find_city_by_id(pclause->value);
	  if (!pcity) { /* Can't find out cityname any more. */
	    notify_player(pplayer,
			  _("Game: One of the cities %s is giving away is destroyed! "
			    "Treaty canceled!"),
			  get_nation_name_plural(pother->nation));
	    notify_player(pother,
			  _("Game: One of the cities %s is giving away is destroyed! "
			    "Treaty canceled!"),
			  get_nation_name_plural(pother->nation));
	    goto cleanup;
	  }
	  if (pcity->owner != pother->player_no) {
	    notify_player(pplayer,
			  _("Game: The %s no longer control %s! "
			    "Treaty canceled!"),
			  get_nation_name_plural(pother->nation),
			  pcity->name);
	    notify_player(pother,
			  _("Game: The %s no longer control %s! "
			    "Treaty canceled!"),
			  get_nation_name_plural(pother->nation),
			  pcity->name);
	    goto cleanup;
	  }
	  if (is_capital(pcity)) {
	    notify_player(pother,
			  _("Game: Your capital (%s) is requested, "
			    "you can't accept treaty."), pcity->name);
	    goto cleanup;
	  }

	  break;
	case CLAUSE_ALLIANCE:
          /* We need to recheck this way since things might have
           * changed. */
	  if(game.ext_info.maxallies && player_allies_count(pother) >= game.ext_info.maxallies) {
	    notify_player(pother, _("Game: You have already %d/%d alliances."),
	      player_allies_count(pother), game.ext_info.maxallies);
	    notify_player(pplayer, _("Game: %s has already %d/%d alliances."),
	      pother->name, player_allies_count(pother), game.ext_info.maxallies);
	    goto cleanup;
	  } 
          if (!pplayer_can_ally(pother, pplayer)) {
	    notify_player(pplayer,
			  _("Game: %s is at war with one of your "
			    "allies - an alliance with %s is impossible."),
			  pother->name, pother->name);
	    notify_player(pother,
			  _("Game: You are at war with one of %s's "
			    "allies - an alliance with %s is impossible."),
			  pplayer->name, pplayer->name);
	    goto cleanup;
          }
          break;
	case CLAUSE_GOLD:
	  if (pother->economic.gold < pclause->value) {
	    notify_player(pplayer,
			  _("Game: The %s don't have the promised amount "
			    "of gold! Treaty canceled!"),
			  get_nation_name_plural(pother->nation));
	    notify_player(pother,
			  _("Game: The %s don't have the promised amount "
			    "of gold! Treaty canceled!"),
			  get_nation_name_plural(pother->nation));
	    goto cleanup;
	  }
	  break;
	default:
	  ; /* nothing */
	}
      }
    } clause_list_iterate_end;

    if (pplayer->ai.control) {
      ai_treaty_accepted(pplayer, pother, ptreaty);
    }
    if (pother->ai.control) {
      ai_treaty_accepted(pother, pplayer, ptreaty);
    }

    clause_list_iterate(ptreaty->clauses, pclause) {
      struct player *pgiver = pclause->from;
      struct player *pdest = (pplayer == pgiver) ? pother : pplayer;
      enum diplstate_type old_diplstate = 
        pgiver->diplstates[pdest->player_no].type;

      switch (pclause->type) {
      case CLAUSE_EMBASSY:
        establish_embassy(pdest, pgiver); /* sic */
        notify_player_ex(pgiver, NULL, E_TREATY_SHARED_VISION,
                         _("Game: You gave an embassy to %s."),
                         pdest->name);
        notify_player_ex(pdest, NULL, E_TREATY_SHARED_VISION,
                         _("Game: %s allowed you to create an embassy!"),
                         pgiver->name);
        gamelog(GAMELOG_TREATY, GL_EMBASSY, pgiver, pdest);
        break;
      case CLAUSE_ADVANCE:
        /* It is possible that two players open the diplomacy dialog
         * and try to give us the same tech at the same time. This
         * should be handled discreetly instead of giving a core dump. */
        if (get_invention(pdest, pclause->value) == TECH_KNOWN) {
	  freelog(LOG_VERBOSE,
                  "The %s already know tech %s, that %s want to give them.",
		  get_nation_name_plural(pdest->nation),
		  get_tech_name(pplayer, pclause->value),
		  get_nation_name_plural(pgiver->nation));
          break;
        }
	notify_player_ex(pdest, NULL, E_TECH_GAIN,
			 _("Game: You are taught the knowledge of %s."),
			 get_tech_name(pdest, pclause->value));

	notify_embassies(pdest, pgiver,
			 _("Game: The %s have acquired %s from the %s."),
			 get_nation_name_plural(pdest->nation),
			 get_tech_name(pdest, pclause->value),
			 get_nation_name_plural(pgiver->nation));

        gamelog(GAMELOG_TECH, pdest, pgiver, pclause->value, "acquire");
        gamelog(GAMELOG_TREATY, GL_TECH, pgiver, pdest);

	do_dipl_cost(pdest,pclause->value);

	found_new_tech(pdest, pclause->value, FALSE, TRUE, A_NONE);
	break;
      case CLAUSE_GOLD:
	notify_player(pdest, _("Game: You get %d gold."), pclause->value);
	pgiver->economic.gold -= pclause->value;
	pdest->economic.gold += pclause->value;
        gamelog(GAMELOG_TREATY, GL_GOLD, pgiver, pdest);
	break;
      case CLAUSE_MAP:
	give_map_from_player_to_player(pgiver, pdest);
	notify_player(pdest, _("Game: You receive %s's worldmap."),
		      pgiver->name);

        check_city_workers(pdest); /* See CLAUSE_VISION */
        gamelog(GAMELOG_TREATY, GL_MAP, pgiver, pdest);
	break;
      case CLAUSE_SEAMAP:
	give_seamap_from_player_to_player(pgiver, pdest);
	notify_player(pdest, _("Game: You receive %s's seamap."),
		      pgiver->name);

        check_city_workers(pdest); /* See CLAUSE_VISION */
        gamelog(GAMELOG_TREATY, GL_SEAMAP, pgiver, pdest);
	break;
      case CLAUSE_CITY:
	{
	  struct city *pcity = find_city_by_id(pclause->value);

	  if (!pcity) {
	    freelog(LOG_NORMAL,
		    "Treaty city id %d not found - skipping clause.",
		    pclause->value);
	    break;
	  }

	  notify_player_ex(pdest, pcity->tile, E_CITY_TRANSFER,
			   _("Game: You receive city of %s from %s."),
			   pcity->name, pgiver->name);

	  notify_player_ex(pgiver, pcity->tile, E_CITY_LOST,
			   _("Game: You give city of %s to %s."),
			   pcity->name, pdest->name);

          gamelog(GAMELOG_LOSECITY, pgiver, pdest, pcity, "acquired");
          gamelog(GAMELOG_TREATY, GL_CITY, pgiver, pdest, pcity);
	  transfer_city(pdest, pcity, -1, TRUE, TRUE, FALSE);
	  break;
	}
      case CLAUSE_CEASEFIRE:
	pgiver->diplstates[pdest->player_no].type=DS_CEASEFIRE;
	pgiver->diplstates[pdest->player_no].turns_left=16;
	pdest->diplstates[pgiver->player_no].type=DS_CEASEFIRE;
	pdest->diplstates[pgiver->player_no].turns_left=16;
	notify_player_ex(pgiver, NULL, E_TREATY_CEASEFIRE,
			 _("Game: You agree on a cease-fire with %s."),
			 pdest->name);
	notify_player_ex(pdest, NULL, E_TREATY_CEASEFIRE,
			 _("Game: You agree on a cease-fire with %s."),
			 pgiver->name);
        gamelog(GAMELOG_TREATY, GL_CEASEFIRE, pgiver, pdest);
	if (old_diplstate == DS_ALLIANCE) {
	  update_players_after_alliance_breakup(pgiver, pdest);
	}
	check_city_workers(pplayer);
	check_city_workers(pother);
	break;
      case CLAUSE_PEACE:
	pgiver->diplstates[pdest->player_no].type=DS_PEACE;
	pdest->diplstates[pgiver->player_no].type=DS_PEACE;
	notify_player_ex(pgiver, NULL, E_TREATY_PEACE,
			 _("Game: You agree on a peace treaty with %s."),
			 pdest->name);
	notify_player_ex(pdest, NULL, E_TREATY_PEACE,
			 _("Game: You agree on a peace treaty with %s."),
			 pgiver->name);
        gamelog(GAMELOG_TREATY, GL_PEACE, pgiver, pdest);
	if (old_diplstate == DS_ALLIANCE) {
	  update_players_after_alliance_breakup(pgiver, pdest);
	}
	check_city_workers(pplayer);
	check_city_workers(pother);
	break;
      case CLAUSE_ALLIANCE:
	pgiver->diplstates[pdest->player_no].type=DS_ALLIANCE;
	pdest->diplstates[pgiver->player_no].type=DS_ALLIANCE;
	notify_player_ex(pgiver, NULL, E_TREATY_ALLIANCE,
			 _("Game: You agree on an alliance with %s."),
			 pdest->name);
	notify_player_ex(pdest, NULL, E_TREATY_ALLIANCE,
			 _("Game: You agree on an alliance with %s."),
			 pgiver->name);

        gamelog(GAMELOG_TREATY, GL_ALLIANCE, pgiver, pdest);
	check_city_workers(pplayer);
	check_city_workers(pother);
	if(game.ext_info.maxallies && player_allies_count(pgiver) >= game.ext_info.maxallies)
	  cancel_diplomacy(pgiver);
	if(game.ext_info.maxallies && player_allies_count(pdest) >= game.ext_info.maxallies)
	  cancel_diplomacy(pdest);
	break;
      case CLAUSE_VISION:
	give_shared_vision(pgiver, pdest);
	notify_player_ex(pgiver, NULL, E_TREATY_SHARED_VISION,
			 _("Game: You give shared vision to %s."),
			 pdest->name);
	notify_player_ex(pdest, NULL, E_TREATY_SHARED_VISION,
			 _("Game: %s gives you shared vision."),
			 pgiver->name);

        /* Yes, shared vision may let us to _know_ tiles
         * within radius of our own city. */
 	check_city_workers(pdest);
        gamelog(GAMELOG_TREATY, GL_VISION, pgiver, pdest);
	break;
      case CLAUSE_LAST:
      case CLAUSE_UNUSED:
        freelog(LOG_ERROR, "Received bad clause type");
        break;
      }

    } clause_list_iterate_end;
  cleanup:
    treaty_list_unlink(treaties, ptreaty);
    treaty_free(ptreaty);
    send_player_info(pplayer, NULL);
    send_player_info(pother, NULL);
  }
}

/****************************************************************************
  Create an embassy. pplayer gets an embassy with aplayer.
****************************************************************************/
void establish_embassy(struct player *pplayer, struct player *aplayer)
{
  /* Establish the embassy. */
  pplayer->embassy |= (1 << aplayer->player_no);
  send_player_info(pplayer, pplayer);
  send_player_info(pplayer, aplayer);  /* update player dialog with embassy */
  send_player_info(aplayer, pplayer);  /* INFO_EMBASSY level info */
}

/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_remove_clause_req(struct player *pplayer,
					int counterpart, int giver,
					enum clause_type type, int value)
{
  struct Treaty *ptreaty;
  struct player *pgiver, *pother;

  if (!pplayer
      || !is_valid_player_id(counterpart)
      || pplayer->player_no == counterpart
      || !is_valid_player_id(giver)) {
    return;
  }

  pother = get_player(counterpart);
  pgiver = get_player(giver);

  if (pgiver != pplayer && pgiver != pother) {
    return;
  }
  
  ptreaty = find_treaty(pplayer, pother);

  if (ptreaty && remove_clause(ptreaty, pgiver, type, value)) {
    dlsend_packet_diplomacy_remove_clause(pplayer->connections,
					  pother->player_no, giver, type,
					  value);
    dlsend_packet_diplomacy_remove_clause(pother->connections,
					  pplayer->player_no, giver, type,
					  value);
    if (pplayer->ai.control) {
      ai_treaty_evaluate(pplayer, pother, ptreaty);
    }
    if (pother->ai.control) {
      ai_treaty_evaluate(pother, pplayer, ptreaty);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_create_clause_req(struct player *pplayer,
					int counterpart, int giver,
					enum clause_type type, int value)
{
  struct Treaty *ptreaty;
  struct player *pgiver, *pother;

  if (!pplayer
      || !is_valid_player_id(counterpart)
      || pplayer->player_no == counterpart
      || !is_valid_player_id(giver)) {
    return;
  }

  pother = get_player(counterpart);
  pgiver = get_player(giver);

  if (pgiver != pplayer && pgiver != pother) {
    return;
  }

  ptreaty = find_treaty(pplayer, pother);

  if (ptreaty && add_clause(ptreaty, pgiver, type, value)) {
    /* 
     * If we are trading cities, then it is possible that the
     * dest is unaware of it's existence.  We have 2 choices,
     * forbid it, or lighten that area.  If we assume that
     * the giver knows what they are doing, then 2. is the
     * most powerful option - I'll choose that for now.
     *                           - Kris Bubendorfer
     */
    if (type == CLAUSE_CITY) {
      struct city *pcity = find_city_by_id(value);

      if (pcity && !map_is_known_and_seen(pcity->tile, pother))
	give_citymap_from_player_to_player(pcity, pplayer, pother);
    }

    dlsend_packet_diplomacy_create_clause(pplayer->connections,
					  pother->player_no, giver, type,
					  value);
    dlsend_packet_diplomacy_create_clause(pother->connections,
					  pplayer->player_no, giver, type,
					  value);
    if (pplayer->ai.control) {
      ai_treaty_evaluate(pplayer, pother, ptreaty);
    }
    if (pother->ai.control) {
      ai_treaty_evaluate(pother, pplayer, ptreaty);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
static void really_diplomacy_cancel_meeting(struct player *pplayer,
					    struct player *pother)
{
  struct Treaty *ptreaty = find_treaty(pplayer, pother);

  if (ptreaty) {
    dlsend_packet_diplomacy_cancel_meeting(pother->connections,
					   pplayer->player_no,
					   pplayer->player_no);
    notify_player(pother, _("Game: %s canceled the meeting!"), 
		  pplayer->name);
    /* Need to send to pplayer too, for multi-connects: */
    dlsend_packet_diplomacy_cancel_meeting(pplayer->connections,
					   pother->player_no,
					   pplayer->player_no);
    notify_player(pplayer, _("Game: Meeting with %s canceled."), 
		  pother->name);
    treaty_list_unlink(treaties, ptreaty);
    treaty_free(ptreaty);
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_cancel_meeting_req(struct player *pplayer,
					 int counterpart)
{
  if (!pplayer
      || !is_valid_player_id(counterpart)
      || pplayer->player_no == counterpart) {
    return;
  }

  really_diplomacy_cancel_meeting(pplayer, get_player(counterpart));
}

/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_init_meeting_req(struct player *pplayer,
				       int counterpart)
{
  struct player *pother;

  if (!pplayer
      || !is_valid_player_id(counterpart)
      || pplayer->player_no == counterpart) {
    return;
  }

  pother = get_player(counterpart);

  if (find_treaty(pplayer, pother)) {
    return;
  }

  if (is_barbarian(pplayer) || is_barbarian(pother)) {
    notify_player(pplayer, _("Your diplomatic envoy was decapitated!"));
    return;
  }

  if (could_meet_with_player(pplayer, pother)) {
    struct Treaty *ptreaty = treaty_new(pplayer, pother);

    treaty_list_prepend(treaties, ptreaty);

    dlsend_packet_diplomacy_init_meeting(pplayer->connections,
					 pother->player_no,
					 pplayer->player_no);
    dlsend_packet_diplomacy_init_meeting(pother->connections,
					 pplayer->player_no,
					 pplayer->player_no);
  }
}

/**************************************************************************
  Send information on any on-going diplomatic meetings for connection's
  player.  (For re-connection in multi-connect case.)
**************************************************************************/
void send_diplomatic_meetings(struct connection *dest)
{
  struct player *pplayer = dest->player;
  bool player_accept, other_accept;

  if (!pplayer) {
    return;
  }

  conn_list_do_buffer(pplayer->connections);
  players_iterate(pother) {
    struct Treaty *ptreaty;

    if (pother == pplayer) {
      continue;
    }
    
    ptreaty = find_treaty(pplayer, pother);

    if (!ptreaty) {
      continue;
    }

    dsend_packet_diplomacy_init_meeting(dest, pother->player_no,
                                        ptreaty->plr0->player_no);

    clause_list_iterate(ptreaty->clauses, pclause) {
      dsend_packet_diplomacy_create_clause(dest, pother->player_no,
                                           pclause->from->player_no,
                                           pclause->type,
                                           pclause->value);
    } clause_list_iterate_end;

    if (ptreaty->plr0 == pplayer) {
      player_accept = ptreaty->accept0;
      other_accept = ptreaty->accept1;
    } else {
      player_accept = ptreaty->accept1;
      other_accept = ptreaty->accept0;
    }

    dlsend_packet_diplomacy_accept_treaty(pplayer->connections,
                                          pother->player_no,
                                          player_accept,
                                          other_accept);
  } players_iterate_end;
  conn_list_do_unbuffer(pplayer->connections);
}

/**************************************************************************
...
**************************************************************************/
void cancel_all_meetings(struct player *pplayer)
{
  players_iterate(pplayer2) {
    if (find_treaty(pplayer, pplayer2)) {
      really_diplomacy_cancel_meeting(pplayer, pplayer2);
    }
  } players_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void cancel_diplomacy(struct player *pplayer)
{
  notify_player(pplayer, _("Game: You have got %d/%d alliances. "
			   "Cancelling all other diplomacy..."),
  	        player_allies_count(pplayer), game.ext_info.maxallies);

  players_iterate(pplayer2) {
    if (pplayers_allied(pplayer, pplayer2)) {
      continue;
    }

    if (find_treaty(pplayer, pplayer2)) {
      really_diplomacy_cancel_meeting(pplayer, pplayer2);
    }

    bool message = FALSE;

    if (gives_shared_vision(pplayer, pplayer2)) {
      remove_shared_vision(pplayer, pplayer2);
      message = TRUE;
    }
    if (gives_shared_vision(pplayer2, pplayer)) {
      remove_shared_vision(pplayer2, pplayer);
      message = TRUE;
    }
    if (pplayer_get_diplstate(pplayer, pplayer2)->type != DS_NO_CONTACT
        && pplayer_get_diplstate(pplayer, pplayer2)->type != DS_WAR) {
      pplayer->diplstates[pplayer2->player_no].type = DS_WAR;
      pplayer2->diplstates[pplayer->player_no].type = DS_WAR;
      message = TRUE;
    }
    if (message) {
      notify_player(pplayer2, _("Game: %s has got %d/%d alliances. "
				"All diplomacy cancelled."),
      	pplayer->name, player_allies_count(pplayer), game.ext_info.maxallies);
      send_player_info(pplayer2, NULL);
    }
  } players_iterate_end;
  send_player_info(pplayer, NULL);
}
