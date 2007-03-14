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

/********************************************************************** 
This module contains various general - mostly highlevel - functions
used throughout the client.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "city.h"
#include "diptreaty.h"
#include "fcintl.h"
#include "game.h"
#include "hash.h"
#include "log.h"
#include "map.h"
#include "packets.h"
#include "shared.h"
#include "spaceship.h"
#include "support.h"

#include "chatline_g.h"
#include "citydlg_g.h"
#include "cityrep_g.h"
#include "civclient.h"
#include "climap.h"
#include "clinet.h"
#include "control.h"
#include "dialogs_g.h"
#include "mapctrl_common.h"
#include "mapview_g.h"
#include "messagewin_common.h"
#include "multiselect.h"//*pepeto* for PINT_TO_PTR & PPTR_TO_INT
#include "myai.h"//*pepeto*
#include "packhand.h"
#include "plrdlg_common.h"
#include "repodlgs_common.h"
#include "tilespec.h"

#include "climisc.h"


#define SPECLIST_TAG city_name
#define SPECLIST_TYPE char
#include "speclist.h"
#define city_name_list_iterate(alist, pitem)\
  TYPED_LIST_ITERATE(char, alist, pitem)
#define city_name_list_iterate_end  LIST_ITERATE_END

static struct city_name_list *an_city_name_formats = NULL;
static struct hash_table *an_continent_counter_table = NULL;
static struct hash_table *an_city_name_table = NULL;
static struct hash_table *an_city_autoname_data_table = NULL;
static int an_global_city_number_counter = 0;

struct autoname_data {
  char original_name[MAX_LEN_NAME];
  int city_id;
  int continent_id;
  int format_index;
  int global_city_number;
  int continent_city_number;
};

static struct hash_table *mark_table = NULL;

struct tile *ltile = NULL;
int ltilex, ltiley;

/**************************************************************************
  ...
**************************************************************************/
void city_autonaming_add_used_name (const char *city_name)
{
  char *cn = mystrdup (city_name);
  
  freelog (LOG_DEBUG, "city_autonaming_add_used_name \"%s\"", city_name);

  if (!hash_insert (an_city_name_table, cn, NULL))
    free (cn);
}
/**************************************************************************
  ...
**************************************************************************/
void city_autonaming_remove_used_name (const char *city_name)
{
  void *key = NULL;
  freelog (LOG_DEBUG, "city_autonaming_remove_used_name \"%s\"", city_name);
  hash_delete_entry_full (an_city_name_table, city_name, &key);
  if (key)
    free (key);
}

/**************************************************************************
  ...
**************************************************************************/
void city_clear_worklist (struct city *pcity)
{
  struct worklist empty_worklist;
  init_worklist (&empty_worklist);
  city_set_worklist (pcity, &empty_worklist);
}

/**************************************************************************
  ...
**************************************************************************/
void clear_worklists_in_selected_cities (void)
{
  char buf[128];
  if (!tiles_hilited_cities)
    return;

  connection_do_buffer (&aconnection);
  city_list_iterate (game.player_ptr->cities, pcity) {
    if (!is_city_hilited (pcity))
      continue;
    city_clear_worklist (pcity);
    my_snprintf (buf, sizeof(buf), _("Cleared worklist in %s."), pcity->name);
    append_output_window (buf);    
  } city_list_iterate_end;
  connection_do_unbuffer (&aconnection); 
} 

/**************************************************************************
...
**************************************************************************/
void client_remove_player(int plrno)
{
  game_remove_player(get_player(plrno));
  game_renumber_players(plrno);
}

/**************************************************************************
...
**************************************************************************/
void client_remove_unit(struct unit *punit)
{
  struct city *pcity;
  struct tile *ptile = punit->tile;
  int hc = punit->homecity;
  struct unit *ufocus = get_unit_in_focus();

  freelog(LOG_DEBUG, "removing unit %d, %s %s (%d %d) hcity %d",
	  punit->id, get_nation_name(unit_owner(punit)->nation),
	  unit_name(punit->type), TILE_XY(punit->tile), hc);

  my_ai_orders_free(punit);

  if (punit == ufocus) {
    multi_select_wipe_up_unit(punit);
    if(punit == get_unit_in_focus())
    {
      set_unit_focus(NULL);
      game_remove_unit(punit);
      advance_unit_focus();
    }
    else
      game_remove_unit(punit);
    punit = NULL;
  } else {
    /* calculate before punit disappears, use after punit removed: */
    bool update = (ufocus
		   && same_pos(ufocus->tile, punit->tile));

    multi_select_wipe_up_unit(punit);
    game_remove_unit(punit);
    punit = NULL;
    if (update) {
      update_unit_pix_label(get_unit_in_focus());
    }
  }

  pcity = map_get_city(ptile);
  if (pcity) {
    if (can_player_see_units_in_city(game.player_ptr, pcity)) {
      pcity->client.occupied =
	(unit_list_size(&pcity->tile->units) > 0);
    }

    refresh_city_dialog(pcity);
    freelog(LOG_DEBUG, "map city %s, %s, (%d %d)", pcity->name,
	    get_nation_name(city_owner(pcity)->nation),
	    TILE_XY(pcity->tile));
  }

  pcity = player_find_city_by_id(game.player_ptr, hc);
  if (pcity) {
    refresh_city_dialog(pcity);
    freelog(LOG_DEBUG, "home city %s, %s, (%d %d)", pcity->name,
	    get_nation_name(city_owner(pcity)->nation),
	    TILE_XY(pcity->tile));
  }

  refresh_tile_mapcanvas(ptile, FALSE);
}

/**************************************************************************
...
**************************************************************************/
void client_remove_city(struct city *pcity)
{
  bool effect_update;
  struct tile *ptile = pcity->tile;

  freelog(LOG_DEBUG, "removing city %s, %s, (%d %d)", pcity->name,
	  get_nation_name(city_owner(pcity)->nation), TILE_XY(ptile));

  my_ai_city_free(pcity);//*pepeto*

/* Explicitly remove all improvements, to properly remove any global effects
     and to handle the preservation of "destroyed" effects. */
  effect_update=FALSE;

 built_impr_iterate(pcity, i) {
    effect_update = TRUE;
    city_remove_improvement(pcity, i);
  } built_impr_iterate_end;

  if (effect_update) {
    /* nothing yet */
  }

  popdown_city_dialog(pcity);
  game_remove_city(pcity);
  city_report_dialog_update();
  refresh_tile_mapcanvas(ptile, FALSE);
}

/**************************************************************************
Change all cities building X to building Y, if possible.  X and Y
could be improvements or units. X and Y are compound ids.
**************************************************************************/
void client_change_all(cid x, cid y)
{
  int fr_id = cid_id(x), to_id = cid_id(y);
  bool fr_is_unit = cid_is_unit(x), to_is_unit = cid_is_unit(y);
  char buf[512];
  int last_request_id = 0;

  my_snprintf(buf, sizeof(buf),
	      _("Game: Changing production of every %s into %s."),
	      fr_is_unit ? get_unit_type(fr_id)->name :
	      get_improvement_name(fr_id),
	      to_is_unit ? get_unit_type(to_id)->
	      name : get_improvement_name(to_id));
  append_output_window(buf);

  connection_do_buffer(&aconnection);
  city_list_iterate (game.player_ptr->cities, pcity) {
    if (((fr_is_unit &&
	  (pcity->is_building_unit) &&
	  (pcity->currently_building == fr_id)) ||
	 (!fr_is_unit &&
	  !(pcity->is_building_unit) &&
	  (pcity->currently_building == fr_id))) &&
	((to_is_unit &&
	  can_build_unit (pcity, to_id)) ||
	 (!to_is_unit &&
	  can_build_improvement (pcity, to_id))))
      {
	last_request_id = city_change_production(pcity, to_is_unit, to_id);
      }
  } city_list_iterate_end;

  connection_do_unbuffer(&aconnection);
  reports_freeze_till(last_request_id);
}

/***************************************************************************
  Return a string indicating one nation's embassy status with another
***************************************************************************/
const char *get_embassy_status(struct player *me, struct player *them)
{
  if (me == them
      || !them->is_alive
      || !me->is_alive) {
    return "-";
  }
  if (player_has_embassy(me, them)) {
    if (player_has_embassy(them, me)) {
      return Q_("?embassy:Both");
    } else {
      return Q_("?embassy:Yes");
    }
  } else if (player_has_embassy(them, me)) {
    return Q_("?embassy:With Us");
  } else if (me->diplstates[them->player_no].contact_turns_left > 0
             || them->diplstates[me->player_no].contact_turns_left > 0) {
    return Q_("?embassy:Contact");
  } else {
    return Q_("?embassy:No Contact");
  }
}

/***************************************************************************
  Return a string indicating one nation's shaed vision status with another
***************************************************************************/
const char *get_vision_status(struct player *me, struct player *them)
{
  if (gives_shared_vision(me, them)) {
    if (gives_shared_vision(them, me)) {
      return Q_("?vision:Both");
    } else {
      return Q_("?vision:To Them");
    }
  } else if (gives_shared_vision(them, me)) {
    return Q_("?vision:To Us");
  } else {
    return "";
  }
}

/**************************************************************************
Copy a string that describes the given clause into the return buffer.
**************************************************************************/
void client_diplomacy_clause_string(char *buf, int bufsiz,
				    struct Clause *pclause)
{
  struct city *pcity;

  switch(pclause->type) {
  case CLAUSE_ADVANCE:
    my_snprintf(buf, bufsiz, _("The %s give %s"),
		get_nation_name_plural(pclause->from->nation),
		get_tech_name(game.player_ptr, pclause->value));
    break;
  case CLAUSE_CITY:
    pcity = find_city_by_id(pclause->value);
    if (pcity) {
      my_snprintf(buf, bufsiz, _("The %s give %s"),
                  get_nation_name_plural(pclause->from->nation),
		  pcity->name);
    } else {
      my_snprintf(buf, bufsiz,_("The %s give unknown city."),
                  get_nation_name_plural(pclause->from->nation));
    }
    break;
  case CLAUSE_GOLD:
    my_snprintf(buf, bufsiz, _("The %s give %d gold"),
		get_nation_name_plural(pclause->from->nation),
		pclause->value);
    break;
  case CLAUSE_MAP:
    my_snprintf(buf, bufsiz, _("The %s give their worldmap"),
		get_nation_name_plural(pclause->from->nation));
    break;
  case CLAUSE_SEAMAP:
    my_snprintf(buf, bufsiz, _("The %s give their seamap"),
		get_nation_name_plural(pclause->from->nation));
    break;
  case CLAUSE_CEASEFIRE:
    my_snprintf(buf, bufsiz, _("The parties agree on a cease-fire"));
    break;
  case CLAUSE_PEACE:
    my_snprintf(buf, bufsiz, _("The parties agree on a peace"));
    break;
  case CLAUSE_ALLIANCE:
    my_snprintf(buf, bufsiz, _("The parties create an alliance"));
    break;
  case CLAUSE_VISION:
    my_snprintf(buf, bufsiz, _("The %s gives shared vision"),
		get_nation_name_plural(pclause->from->nation));
    break;
  case CLAUSE_EMBASSY:
    my_snprintf(buf, bufsiz, _("The %s gives an embassy"),
                get_nation_name_plural(pclause->from->nation));
    break;
  default:
    assert(FALSE);
    if (bufsiz > 0) {
      *buf = '\0';
    }
    break;
  }
}

/**************************************************************************
Return the sprite index for the research indicator.
**************************************************************************/
int client_research_sprite(void)
{
  return (NUM_TILES_PROGRESS *
	  game.player_ptr->research.bulbs_researched) /
      (total_bulbs_required(game.player_ptr) + 1);
}

/**************************************************************************
Return the sprite index for the global-warming indicator.
**************************************************************************/
int client_warming_sprite(void)
{
  int index;
  if ((game.globalwarming <= 0) &&
      (game.heating < (NUM_TILES_PROGRESS / 2))) {
    index = MAX(0, game.heating);
  } else {
    index = MIN(NUM_TILES_PROGRESS,
		(MAX(0, 4 + game.globalwarming) / 5) +
		((NUM_TILES_PROGRESS / 2) - 1));
  }
  return index;
}

/**************************************************************************
Return the sprite index for the global-cooling indicator.
**************************************************************************/
int client_cooling_sprite(void)
{
  int index;
  if ((game.nuclearwinter <= 0) &&
      (game.cooling < (NUM_TILES_PROGRESS / 2))) {
    index = MAX(0, game.cooling);
  } else {
    index = MIN(NUM_TILES_PROGRESS,
		(MAX(0, 4 + game.nuclearwinter) / 5) +
		((NUM_TILES_PROGRESS / 2) - 1));
  }
  return index;
}

/**************************************************************************
Find something sensible to display. This is used to overwrite the
intro gfx.
**************************************************************************/
void center_on_something(void)
{
  struct city *pcity;
  struct unit *punit;

  if (!can_client_change_view()) {
    return;
  }

  can_slide = FALSE;
  if ((punit = get_unit_in_focus())) {
    center_tile_mapcanvas(punit->tile);
  } else if ((pcity = find_palace(game.player_ptr))) {
    /* Else focus on the capital. */
    center_tile_mapcanvas(pcity->tile);
  } else if (city_list_size(&game.player_ptr->cities) > 0) {
    /* Just focus on any city. */
    pcity = city_list_get(&game.player_ptr->cities, 0);
    assert(pcity != NULL);
    center_tile_mapcanvas(pcity->tile);
  } else if (unit_list_size(&game.player_ptr->units) > 0) {
    /* Just focus on any unit. */
    punit = unit_list_get(&game.player_ptr->units, 0);
    assert(punit != NULL);
    center_tile_mapcanvas(punit->tile);
  } else {
    struct tile *ctile = native_pos_to_tile(map.xsize / 2, map.ysize / 2);

    /* Just any known tile will do; search near the middle first. */
    /* Iterate outward from the center tile.  We have to give a radius that
     * is guaranteed to be larger than the map will be.  Although this is
     * a misuse of map.xsize and map.ysize (which are native dimensions),
     * it should give a sufficiently large radius. */
    iterate_outward(native_pos_to_tile(map.xsize / 2, map.ysize / 2),
		    map.xsize + map.ysize, ptile) {
      if (tile_get_known(ptile) != TILE_UNKNOWN) {
	ctile = ptile;
	break;
      }
    } iterate_outward_end;

    center_tile_mapcanvas(ctile);
  }
  can_slide = TRUE;
}

/**************************************************************************
...
**************************************************************************/
cid cid_encode(bool is_unit, int id)
{
  return id + (is_unit ? B_LAST : 0);
}

/**************************************************************************
...
**************************************************************************/
cid cid_encode_from_city(struct city * pcity)
{
  return cid_encode(pcity->is_building_unit, pcity->currently_building);
}

/**************************************************************************
...
**************************************************************************/
void cid_decode(cid cid, bool *is_unit, int *id)
{
  *is_unit = cid_is_unit(cid);
  *id = cid_id(cid);
}

/**************************************************************************
...
**************************************************************************/
bool cid_is_unit(cid cid)
{
  return (cid >= B_LAST);
}

/**************************************************************************
...
**************************************************************************/
int cid_id(cid cid)
{
  return (cid >= B_LAST) ? (cid - B_LAST) : cid;
}

/**************************************************************************
...
**************************************************************************/
wid wid_encode(bool is_unit, bool is_worklist, int id)
{
  assert(!is_unit || !is_worklist);

  if (is_unit)
    return id + B_LAST;
  if (is_worklist)
    return id + B_LAST + U_LAST;
  return id;
}

/**************************************************************************
...
**************************************************************************/
bool wid_is_unit(wid wid)
{
  assert(wid != WORKLIST_END);

  return (wid >= B_LAST && wid < B_LAST + U_LAST);
}

/**************************************************************************
...
**************************************************************************/
bool wid_is_worklist(wid wid)
{
  assert(wid != WORKLIST_END);

  return (wid >= B_LAST + U_LAST);
}

/**************************************************************************
...
**************************************************************************/
int wid_id(wid wid)
{
  assert(wid != WORKLIST_END);

  if (wid >= B_LAST + U_LAST)
    return wid - (B_LAST + U_LAST);
  if (wid >= B_LAST)
    return wid - B_LAST;
  return wid;
}

/****************************************************************
...
*****************************************************************/
bool city_can_build_impr_or_unit(struct city *pcity, cid cid)
{
  if (cid_is_unit(cid))
    return can_build_unit(pcity, cid_id(cid));
  else
    return can_build_improvement(pcity, cid_id(cid));
}

/****************************************************************
...
*****************************************************************/
bool city_can_sell_impr(struct city *pcity, cid cid)
{
  int id;
  id = cid_id(cid);
  
  if (cid_is_unit(cid))
    return FALSE;
  else
    return !pcity->did_sell && city_got_building(pcity, id)
      && !is_wonder(id);
}

/****************************************************************
...
*****************************************************************/
bool city_unit_supported(struct city *pcity, cid cid)
{
  if (cid_is_unit(cid)) {
    int unit_type = cid_id(cid);

    unit_list_iterate(pcity->units_supported, punit) {
      if (punit->type == unit_type)
	return TRUE;
    }
    unit_list_iterate_end;
  }
  return FALSE;
}

/****************************************************************
...
*****************************************************************/
bool city_unit_present(struct city *pcity, cid cid)
{
  if (cid_is_unit(cid)) {
    int unit_type = cid_id(cid);

    unit_list_iterate(pcity->tile->units, punit) {
      if (punit->type == unit_type)
	return TRUE;
    }
    unit_list_iterate_end;
  }
  return FALSE;
}

/****************************************************************************
  A TestCityFunc to tell whether the item is a building and is present.
****************************************************************************/
bool city_building_present(struct city *pcity, cid cid)
{
  if (!cid_is_unit(cid)) {
    int impr_type = cid_id(cid);

    return city_got_building(pcity, impr_type);
  }
  return FALSE;
}

/**************************************************************************
 Helper for name_and_sort_items.
**************************************************************************/
static int my_cmp(const void *p1, const void *p2)
{
  const struct item *i1 = (const struct item *) p1;
  const struct item *i2 = (const struct item *) p2;

  if (i1->section == i2->section)
    return mystrcasecmp(i1->descr, i2->descr);
  return (i1->section - i2->section);
}

/**************************************************************************
 Takes an array of compound ids (cids). It will fill out an array of
 struct items and also sort it.

 section 0: normal buildings
 section 1: Capitalization
 section 2: F_NONMIL units
 section 3: other units
 section 4: wonders
**************************************************************************/
void name_and_sort_items(int *pcids, int num_cids, struct item *items,
			 bool show_cost, struct city *pcity)
{
  int i;

  for (i = 0; i < num_cids; i++) {
    bool is_unit = cid_is_unit(pcids[i]);
    int id = cid_id(pcids[i]), cost;
    struct item *pitem = &items[i];
    const char *name;

    pitem->cid = pcids[i];

    if (is_unit) {
      name = get_unit_name(id);
      cost = unit_build_shield_cost(id);
      pitem->section = unit_type_flag(id, F_NONMIL) ? 2 : 3;
    } else {
      name = get_impr_name_ex(pcity, id);
      if (building_has_effect(id, EFT_PROD_TO_GOLD)) {
	cost = -1;
	pitem->section = 1;
      } else {
	cost = impr_build_shield_cost(id);
	if (is_wonder(id)) {
      	  pitem->section = 4;
        } else {
	  pitem->section = 0;
	}
      }
    }

    if (show_cost) {
      if (cost < 0) {
	my_snprintf(pitem->descr, sizeof(pitem->descr), "%s (XX)", name);
      } else {
	my_snprintf(pitem->descr, sizeof(pitem->descr), "%s (%d)", name, cost);
      }
    } else {
      (void) mystrlcpy(pitem->descr, name, sizeof(pitem->descr));
    }
  }

  qsort(items, num_cids, sizeof(struct item), my_cmp);
}

/**************************************************************************
...
**************************************************************************/
int collect_cids1(cid * dest_cids, struct city **selected_cities,
		  int num_selected_cities, bool append_units,
		  bool append_wonders, bool change_prod,
		  bool (*test_func) (struct city *, int))
{
  cid first = append_units ? B_LAST : 0;
  cid last = (append_units
	      ? game.num_unit_types + B_LAST
	      : game.num_impr_types);
  cid cid;
  int items_used = 0;

  for (cid = first; cid < last; cid++) {
    bool append = FALSE;
    int id = cid_id(cid);

    if (!append_units && (append_wonders != is_wonder(id)))
      continue;

    if (!change_prod) {
      city_list_iterate(game.player_ptr->cities, pcity) {
	append |= test_func(pcity, cid);
      }
      city_list_iterate_end;
    } else {
      int i;

      for (i = 0; i < num_selected_cities; i++) {
	append |= test_func(selected_cities[i], cid);
      }
    }

    if (!append)
      continue;

    dest_cids[items_used] = cid;
    items_used++;
  }
  return items_used;
}

/**************************************************************************
 Collect the cids of all targets (improvements and units) which are
 currently built in a city.
**************************************************************************/
int collect_cids2(cid * dest_cids)
{
  bool mapping[B_LAST + U_LAST];
  int cids_used = 0;
  cid cid;

  memset(mapping, 0, sizeof(mapping));
  city_list_iterate(game.player_ptr->cities, pcity) {
    mapping[cid_encode_from_city(pcity)] = TRUE;
  }
  city_list_iterate_end;

  for (cid = 0; cid < ARRAY_SIZE(mapping); cid++) {
    if (mapping[cid]) {
      dest_cids[cids_used] = cid;
      cids_used++;
    }
  }
  return cids_used;
}

/**************************************************************************
 Collect the cids of all targets (improvements and units) which can
 be build in a city.
**************************************************************************/
int collect_cids3(cid * dest_cids)
{
  int cids_used = 0;

  impr_type_iterate(id) {
    if (can_player_build_improvement(game.player_ptr, id)) {
      dest_cids[cids_used] = cid_encode(FALSE, id);
      cids_used++;
    }
  } impr_type_iterate_end;

  unit_type_iterate(id) {
    if (can_player_build_unit(game.player_ptr, id)) {
      dest_cids[cids_used] = cid_encode(TRUE, id);
      cids_used++;
    }
  } unit_type_iterate_end

  return cids_used;
}

/**************************************************************************
 Collect the cids of all targets which can be build by this city or
 in general.
**************************************************************************/
int collect_cids4(cid * dest_cids, struct city *pcity, bool advanced_tech)
{
  int cids_used = 0;

  impr_type_iterate(id) {
    bool can_build = can_player_build_improvement(game.player_ptr, id);
    bool can_eventually_build =
	can_player_eventually_build_improvement(game.player_ptr, id);

    /* If there's a city, can the city build the improvement? */
    if (pcity) {
      can_build = can_build && can_build_improvement(pcity, id);
      can_eventually_build = can_eventually_build &&
	  can_eventually_build_improvement(pcity, id);
    }

    if ((advanced_tech && can_eventually_build) ||
	(!advanced_tech && can_build)) {
      dest_cids[cids_used] = cid_encode(FALSE, id);
      cids_used++;
    }
  } impr_type_iterate_end;

  unit_type_iterate(id) {
    bool can_build = can_player_build_unit(game.player_ptr, id);
    bool can_eventually_build =
	can_player_eventually_build_unit(game.player_ptr, id);

    /* If there's a city, can the city build the unit? */
    if (pcity) {
      can_build = can_build && can_build_unit(pcity, id);
      can_eventually_build = can_eventually_build &&
	  can_eventually_build_unit(pcity, id);
    }

    if ((advanced_tech && can_eventually_build) ||
	(!advanced_tech && can_build)) {
      dest_cids[cids_used] = cid_encode(TRUE, id);
      cids_used++;
    }
  } unit_type_iterate_end;

  return cids_used;
}

/**************************************************************************
 Collect the cids of all improvements which are built in the given city.
**************************************************************************/
int collect_cids5(cid * dest_cids, struct city *pcity)
{
  int cids_used = 0;

  assert(pcity != NULL);

  built_impr_iterate(pcity, id) {
    dest_cids[cids_used] = cid_encode(FALSE, id);
    cids_used++;
  } built_impr_iterate_end;

  return cids_used;
}

/**************************************************************************
 Collect the wids of all possible targets of the given city.
**************************************************************************/
int collect_wids1(wid * dest_wids, struct city *pcity, bool wl_first, 
                  bool advanced_tech)
{
  cid cids[U_LAST + B_LAST];
  int item, cids_used, wids_used = 0;
  struct item items[U_LAST + B_LAST];

  /* Fill in the global worklists now?                      */
  /* perhaps judicious use of goto would be good here? -mck */
  if (wl_first && game.player_ptr->worklists[0].is_valid && pcity) {
    int i;
    for (i = 0; i < MAX_NUM_WORKLISTS; i++) {
      if (game.player_ptr->worklists[i].is_valid) {
	dest_wids[wids_used] = wid_encode(FALSE, TRUE, i);
	wids_used++;
      }
    }
  }

  /* Fill in improvements and units */
  cids_used = collect_cids4(cids, pcity, advanced_tech);
  name_and_sort_items(cids, cids_used, items, FALSE, pcity);

  for (item = 0; item < cids_used; item++) {
    cid cid = items[item].cid;
    dest_wids[wids_used] = wid_encode(cid_is_unit(cid), FALSE, cid_id(cid));
    wids_used++;
  }

  /* we didn't fill in the global worklists above */
  if (!wl_first && game.player_ptr->worklists[0].is_valid && pcity) {
    int i;
    for (i = 0; i < MAX_NUM_WORKLISTS; i++) {
      if (game.player_ptr->worklists[i].is_valid) {
        dest_wids[wids_used] = wid_encode(FALSE, TRUE, i);
        wids_used++;
      }
    }
  }

  return wids_used;
}

/**************************************************************************
...
**************************************************************************/
int num_supported_units_in_city(struct city *pcity)
{
  struct unit_list *plist;

  if (pcity->owner != game.player_idx) {
    plist = &pcity->info_units_supported;
  } else {
    plist = &pcity->units_supported;
  }

  return unit_list_size(plist);
}

/**************************************************************************
...
**************************************************************************/
int num_present_units_in_city(struct city *pcity)
{
  struct unit_list *plist;

  if (pcity->owner != game.player_idx) {
    plist = &pcity->info_units_present;
  } else {
    plist = &pcity->tile->units;
  }

  return unit_list_size(plist);
}

/**************************************************************************
  Handles a chat message.
**************************************************************************/
void handle_event(char *message, struct tile *ptile,
		  enum event_type event, int conn_id)
{
  int where = MW_OUTPUT;	/* where to display the message */
  
  if (event >= E_LAST)  {
    /* Server may have added a new event; leave as MW_OUTPUT */
    freelog(LOG_NORMAL, "Unknown event type %d!", event);
  } else if (event >= 0)  {
    where = messages_where[event];
  }

  if (BOOL_VAL(where & MW_OUTPUT)
      || get_client_state() != CLIENT_GAME_RUNNING_STATE) {
    /* When the game isn't running, the messages dialog isn't present and
     * we want to send all messages to the chatline.  There shouldn't be
     * any problem with server spam in pregame anyway. */
    append_output_window_full(message, conn_id);
  }
  if (BOOL_VAL(where & MW_MESSAGES)) {
    add_notify_window(message, ptile, event);
  }
  if (BOOL_VAL(where & MW_POPUP) &&
      (!game.player_ptr->ai.control || ai_popup_windows)) {
    popup_notify_goto_dialog(_("Popup Request"), message, ptile);
  }

  play_sound_for_event(event);
}

/**************************************************************************
  Creates a struct packet_generic_message packet and injects it via
  handle_chat_msg.
**************************************************************************/
void create_event(struct tile *ptile, enum event_type event,
		  const char *format, ...)
{
  va_list ap;
  char message[MAX_LEN_MSG];

  va_start(ap, format);
  my_vsnprintf(message, sizeof(message), format, ap);
  va_end(ap);

  handle_event(message, ptile, event, aconnection.id);
}

/**************************************************************************
  Writes the supplied string into the file civgame.log.
**************************************************************************/
void write_chatline_content(const char *txt)
{
  char logname[256];
  time_t now;
  struct tm *nowtm;
  FILE *fp;

  now = time (NULL);
  nowtm = localtime (&now);
  strftime (logname, sizeof (logname), "civgame-%y%m%d-%H%M%S.log", nowtm);
  fp = fopen(logname, "w");

  append_output_window(_("Exporting output window to civgame.log ..."));
  if (fp) {
    fputs(txt, fp);
    fclose(fp);
    append_output_window(_("Export complete."));
  } else {
    append_output_window(_("Export failed, couldn't write to file."));
  }
}

/**************************************************************************
  Freeze all reports and other GUI elements.
**************************************************************************/
void reports_freeze(void)
{
  freelog(LOG_DEBUG, "reports_freeze");

  meswin_freeze();
  plrdlg_freeze();
  report_dialogs_freeze();
  output_window_freeze();
}

/**************************************************************************
  Freeze all reports and other GUI elements until the given request
  was executed.
**************************************************************************/
void reports_freeze_till(int request_id)
{
  if (request_id != 0) {
    reports_freeze();
    set_reports_thaw_request(request_id);
  }
}

/**************************************************************************
  Thaw all reports and other GUI elements.
**************************************************************************/
void reports_thaw(void)
{
  freelog(LOG_DEBUG, "reports_thaw");

  meswin_thaw();
  plrdlg_thaw();
  report_dialogs_thaw();
  output_window_thaw();
}

/**************************************************************************
  Thaw all reports and other GUI elements unconditionally.
**************************************************************************/
void reports_force_thaw(void)
{
  meswin_force_thaw();
  plrdlg_force_thaw();
  report_dialogs_force_thaw();
  output_window_force_thaw();
}

/*************************************************************************
...
*************************************************************************/
enum known_type map_get_known(const struct tile *ptile,
			      struct player *pplayer)
{
  assert(pplayer == game.player_ptr);
  return tile_get_known(ptile);
}

/**************************************************************************
  ...
**************************************************************************/
static int an_make_city_name (const char *format, char *buf, int buflen,
                              struct autoname_data *ad)
{
  const char *in = format;
  char *out = buf, tmp[32];
  int rem = buflen, len, fw = 0;
  
  int *pcontinent_counter;

  assert (buflen > 0);

  freelog (LOG_DEBUG, "amcn an_make_city_name format=\"%s\" ad=%p", format, ad);

  if (!*format) {
    mystrlcpy (buf, ad->original_name, buflen);
    return strlen (ad->original_name) + 1;
  }

  pcontinent_counter = hash_lookup_data (an_continent_counter_table,
                                         PINT_TO_PTR (ad->continent_id));
  if (!pcontinent_counter) {
    pcontinent_counter = fc_malloc (sizeof (int));
    *pcontinent_counter = 0;
    hash_insert (an_continent_counter_table, PINT_TO_PTR (ad->continent_id),
                 pcontinent_counter);
  }

  freelog (LOG_DEBUG, "amcn   *pcontinent_counter = %d", *pcontinent_counter);

  if (ad->global_city_number <= 0) {
    ad->global_city_number = ++an_global_city_number_counter;
    freelog (LOG_DEBUG, "amcn   assigned new global_city_number (%d)", ad->global_city_number);
  }
  if (ad->continent_city_number <= 0) {
    ad->continent_city_number = ++(*pcontinent_counter);
    freelog (LOG_DEBUG, "amcn   assigned new continent_city_number (%d)", ad->continent_city_number);
  }

  while (rem > 1 && *in != '\0' && *in != ';') {
    if (*in == '%') {
      in++;
      
      fw = 0;
      while (my_isdigit (*in)) {
        fw = (*in++ - '0') + 10 * fw;
      }
      if (fw <= 0)
        fw = 1;
      my_snprintf (tmp, sizeof (tmp), "%%0%dd", fw);
      
      switch (*in) {
      case 'c': /* continent id */
        len = my_snprintf (out, rem, tmp, ad->continent_id);
        break;
        
      case 'g': /* global city counter */
        len = my_snprintf (out, rem, tmp, ad->global_city_number);
        break;
        
      case 'n': /* per continent city counter */
        len = my_snprintf (out, rem, tmp, ad->continent_city_number);
        break;
        
      case '%': /* a single percent sign */
        *out = '%';
        len = 1;
        break;
        
      default: /* everything else */
        if (rem > 2) {
          *out = '%';
          out[1] = *in;
          len = 2;
        } else {
          len = 0;
        }
        break;
      }
      
      out += len;
      rem -= len;
      if (*in == '\0')
        break;
      in++;
    } else {
      *out++ = *in++;
      rem--;
    }
  }
  
  if (rem == buflen) {
    buf[0] = 0;
    return 0;
  }

  if (rem > 0) {
    *out++ = '\0';
    rem--;
  }

  return out - buf;
}

/**************************************************************************
  ...
**************************************************************************/
static int an_generate_city_name (char *buf, int buflen,
                                  struct city *pcity,
                                  char *err, int errlen)
{
  struct autoname_data *ad;
  int num_formats, len = -1, start_format_index;
  char last_generated_name[MAX_LEN_NAME];
  const char *format = NULL;
  bool name_exists = FALSE;

  assert (an_city_name_formats != NULL);

  freelog (LOG_DEBUG, "agcn an_generate_city_name pcity->id=%d \"%s\"", pcity->id, pcity->name);

  num_formats = city_name_list_size (an_city_name_formats);

  if (num_formats <= 1) {
    my_snprintf (err, errlen, _("There are no city name formats defined. "
                 "Try adding some to Local Options."));
    return 0;
  }

  ad = hash_lookup_data (an_city_autoname_data_table, PINT_TO_PTR (pcity->id));
  if (!ad) {
    ad = fc_malloc (sizeof (struct autoname_data));
    freelog (LOG_DEBUG, "agcn   new ad %p", ad);
    sz_strlcpy (ad->original_name, pcity->name);
    ad->city_id = pcity->id;
    ad->continent_id = pcity->tile->continent;
    ad->format_index = 1;
    ad->global_city_number = 0;
    ad->continent_city_number = 0;
    hash_insert (an_city_autoname_data_table, PINT_TO_PTR (pcity->id), ad);
  } else {
    ad->format_index++;
    ad->format_index %= num_formats;
  }

  start_format_index = ad->format_index;
  last_generated_name[0] = 0;

  for (;;) {
    format = city_name_list_get (an_city_name_formats, ad->format_index);
    freelog (LOG_DEBUG, "agcn   trying format \"%s\" [%d]", format, ad->format_index);
    
    len = an_make_city_name (format, buf, buflen, ad);
    if (len <= 0)
      break;
    
    if (!strcmp (buf, last_generated_name))
      break;
    
    sz_strlcpy (last_generated_name, buf);

    name_exists = hash_key_exists (an_city_name_table, buf)
      || !strcmp (pcity->name, buf);
    
    if (!name_exists)
      break;
    
    ad->format_index++;
    ad->format_index %= num_formats;
    
    if (ad->format_index == start_format_index)
      break;
  }

  if (name_exists) {
    my_snprintf (err, errlen, _("A new name was not generated from "
                                "the format \"%s\"."), format);
    return 0;
  }

  if (len <= 0) {
    my_snprintf (err, errlen, _("Failed to generate name from "
                                "format \"%s\"."), format);
    return 0;
  }
  
  return len;
}
/**************************************************************************
  ...
**************************************************************************/
static void an_parse_city_name_formats (void)
{
  char *p, *q, buf[1024];
  
  freelog (LOG_DEBUG, "apcnf an_parse_city_name_formats");

  assert (an_city_name_formats != NULL);
  
  city_name_list_iterate (*an_city_name_formats, fmt) {
    free (fmt);
  } city_name_list_iterate_end;
  city_name_list_unlink_all (an_city_name_formats);
  
  sz_strlcpy (buf, city_name_formats);

  freelog (LOG_DEBUG, "apcnf   adding special first format");
  city_name_list_append (an_city_name_formats, mystrdup (""));

  freelog (LOG_DEBUG, "apcnf   parsing \"%s\"", city_name_formats);
  
  for (p = buf; p && *p; p = q) {
    if ((q = strchr (p, ';'))) {
      *q++ = 0;
    }
    remove_leading_trailing_spaces (p);
    if (!*p && !q) {
      break;
    }
    if (*p) {
      freelog (LOG_DEBUG, "apcnf   adding format to list \"%s\"", p);
      city_name_list_append (an_city_name_formats, mystrdup (p));
    }
  }

  freelog (LOG_DEBUG, "apcnf   parsed %d formats", city_name_list_size (an_city_name_formats));
}
/**************************************************************************
  ...
**************************************************************************/
void city_autonaming_init (void)
{
  freelog (LOG_DEBUG, "cai city_autonaming_init");
  if (!an_city_name_table)
    an_city_name_table = hash_new (hash_fval_string2, hash_fcmp_string);
  if (!an_continent_counter_table)
    an_continent_counter_table = hash_new (hash_fval_int, hash_fcmp_int);
  if (!an_city_autoname_data_table)
    an_city_autoname_data_table = hash_new (hash_fval_int, hash_fcmp_int);
  
  if (!an_city_name_formats) {
    an_city_name_formats = fc_malloc (sizeof (struct city_name_list));
    city_name_list_init (an_city_name_formats);    
    an_global_city_number_counter = 0;
  }
}
  
/**************************************************************************
  ...
**************************************************************************/
void city_autonaming_free (void)
{
  freelog (LOG_DEBUG, "caf city_autonaming_free");

  /* NB we can't iterate over the hash entries and free the
     keys at the same time */
  while (hash_num_entries (an_city_name_table) > 0) {
    char *name = (char *) hash_key_by_number (an_city_name_table, 0);
    hash_delete_entry (an_city_name_table, name);
    freelog (LOG_DEBUG, "caf   freeing \"%s\"", name);
    free (name);
  }
  hash_free (an_city_name_table);
  an_city_name_table = NULL;

  hash_iterate (an_continent_counter_table, void *, key, int *, pcc) {
    int id = PPTR_TO_INT (key);
    freelog (LOG_DEBUG, "caf   freeing counter for continent %d (%d)", id, *pcc);
    free (pcc);
  } hash_iterate_end;
  freelog (LOG_DEBUG, "caf   deleting all in an_continent_counter_table");
  hash_delete_all_entries (an_continent_counter_table);  
  hash_free (an_continent_counter_table);
  an_continent_counter_table = NULL;

  hash_iterate (an_city_autoname_data_table, void *, key,
                struct autoname_data *, ad)
  {
    int id = PPTR_TO_INT (key);
    freelog (LOG_DEBUG, "caf   freeing ad %p (for city id=%d)", ad, id);
    free (ad);
  } hash_iterate_end;
  freelog (LOG_DEBUG, "caf   deleting all in an_city_autoname_data_table");
  hash_delete_all_entries (an_city_autoname_data_table);
  hash_free (an_city_autoname_data_table);
  an_city_autoname_data_table = NULL;

  an_global_city_number_counter = 0;

  city_name_list_iterate (*an_city_name_formats, fmt) {
    free (fmt);
  } city_list_iterate_end;
  city_name_list_unlink_all (an_city_name_formats);
  free (an_city_name_formats);
  an_city_name_formats = NULL;
}
        
/**************************************************************************
  ...
**************************************************************************/
void normalize_names_in_selected_cities (void)
{
  char buf[512], err[256];

  freelog (LOG_DEBUG, "normalize_names_in_selected_cities");
  
  if (!tiles_hilited_cities)
    return;

  an_parse_city_name_formats();

  connection_do_buffer (&aconnection);
  city_list_iterate (game.player_ptr->cities, pcity) {
    if (!is_city_hilited (pcity))
      continue;
    
    if (an_generate_city_name (buf, sizeof (buf), pcity,
                               err, sizeof (err)) > 0)
    {
      city_autonaming_add_used_name (buf);
      city_rename (pcity, buf);
    } else {
      my_snprintf (buf, sizeof (buf),
          _("Warclient: Could not auto-rename city %s! %s"),
          pcity->name, err);
      append_output_window (buf);
    }
  } city_list_iterate_end;
  connection_do_unbuffer (&aconnection);
}

/**************************************************************************
  Find city nearest to given unit and optionally return squared city
  distance Parameter sq_dist may be NULL. Returns NULL only if no city is
  known. Favors punit owner's cities over other cities if equally distant.
**************************************************************************/
struct city *get_nearest_city(struct unit *punit, int *sq_dist)
{
  struct city *pcity_near;
  int pcity_near_dist;

  if ((pcity_near = map_get_city(punit->tile))) {
    pcity_near_dist = 0;
  } else {
    pcity_near = NULL;
    pcity_near_dist = -1;
    players_iterate(pplayer) {
      city_list_iterate(pplayer->cities, pcity_current) {
        int dist = sq_map_distance(pcity_current->tile, punit->tile);
        if (pcity_near_dist == -1 || dist < pcity_near_dist
	    || (dist == pcity_near_dist
		&& punit->owner == pcity_current->owner)) {
          pcity_near = pcity_current;
          pcity_near_dist = dist;
        }
      } city_list_iterate_end;
    } players_iterate_end;
  }

  if (sq_dist) {
    *sq_dist = pcity_near_dist;
  }

  return pcity_near;
}

/**************************************************************************
  Called when the "Buy" button is pressed in the city report for every
  selected city. Checks for coinage and sufficient funds or request the
  purchase if everything is ok.
**************************************************************************/
void cityrep_buy(struct city *pcity)
{
  int value = city_buy_cost(pcity);

  if (get_current_construction_bonus(pcity, EFT_PROD_TO_GOLD) > 0) {
    char buf[512];

    assert(!pcity->is_building_unit);
    my_snprintf(buf, sizeof(buf),
		_("Game: You don't buy %s in %s!"),
		improvement_types[pcity->currently_building].name,
		pcity->name);
    append_output_window(buf);
    return;
  }

  if (game.player_ptr->economic.gold >= value) {
    city_buy_production(pcity);
  } else {
    char buf[512];
    const char *name;

    if (pcity->is_building_unit) {
      name = get_unit_type(pcity->currently_building)->name;
    } else {
      name = get_impr_name_ex(pcity, pcity->currently_building);
    }

    my_snprintf(buf, sizeof(buf),
		_("Game: %s costs %d gold and you only have %d gold."),
		name, value, game.player_ptr->economic.gold);
    append_output_window(buf);
  }
}

void common_taxrates_callback(int i)
{
  int tax_end, lux_end, sci_end, tax, lux, sci;
  int delta = 10;

  if (!can_client_issue_orders()) {
    return;
  }

  lux_end = game.player_ptr->economic.luxury;
  sci_end = lux_end + game.player_ptr->economic.science;
  tax_end = 100;

  lux = game.player_ptr->economic.luxury;
  sci = game.player_ptr->economic.science;
  tax = game.player_ptr->economic.tax;

  i *= 10;
  if (i < lux_end) {
    lux -= delta;
    sci += delta;
  } else if (i < sci_end) {
    sci -= delta;
    tax += delta;
  } else {
    tax -= delta;
    lux += delta;
  }
  dsend_packet_player_rates(&aconnection, tax, lux, sci);
}

/**************************************************************************
  Asks the server to buy production in all hilighted (i.e. selected
  cities).
**************************************************************************/
int buy_production_in_selected_cities (void)
{
  if (!tiles_hilited_cities)
    return 0;

  connection_do_buffer (&aconnection);
  city_list_iterate (game.player_ptr->cities, pcity) {
    
    if (get_current_construction_bonus (pcity, EFT_PROD_TO_GOLD) > 0)
      continue;

    if (is_city_hilited (pcity)) {
      city_buy_production (pcity);
    }
    
  } city_list_iterate_end;
  connection_do_unbuffer (&aconnection);
  
  return 1;
}

/**************************************************************************
...
**************************************************************************/
static void check_init_mark_table(void)
{
  if (mark_table)
    return;
  mark_table = hash_new(hash_fval_keyval, hash_fcmp_keyval);
}

/**************************************************************************
...
**************************************************************************/
void clear_link_marks(void)
{
  check_init_mark_table();
  
  hash_iterate(mark_table, struct tile *, ptile, void *, dummy) {
    assert(ptile);
    ptile->client.mark_ttl = 0;
  } hash_iterate_end;

  hash_delete_all_entries(mark_table);

  if(get_client_state() == CLIENT_GAME_RUNNING_STATE) {
    update_map_canvas_visible();
  }
}

/**************************************************************************
  ...
**************************************************************************/
void decrease_link_mark_ttl(void)
{
  check_init_mark_table();

  hash_iterate(mark_table, struct tile *, ptile, void *, dummy) {
    assert(ptile);
    if (--ptile->client.mark_ttl <= 0)
      hash_delete_entry(mark_table, ptile);
  } hash_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void add_link_mark(struct tile *ptile)
{
  check_init_mark_table();

  assert(ptile);

  hash_insert(mark_table, ptile, NULL);
  draw_link_mark(ptile);

  if(get_client_state() == CLIENT_GAME_RUNNING_STATE) {
    update_map_canvas_visible();
  }
}

/**************************************************************************
  ...
**************************************************************************/
void draw_link_mark(struct tile *ptile)
{
  int canvas_x, canvas_y, xlen, ylen;
  if (!tile_visible_mapcanvas(ptile))
    return;
    
  tile_to_canvas_pos(&canvas_x, &canvas_y, ptile);

#if 0
  canvas_put_sprite_full(mapview_canvas.store, canvas_x, canvas_y,
                         sprites.user.attention);
  
  canvas_x += NORMAL_TILE_WIDTH / 2;
  canvas_y += NORMAL_TILE_HEIGHT / 2;
  canvas_put_rectangle(mapview_canvas.store, COLOR_STD_YELLOW,
                       canvas_x-2, canvas_y-2, 4, 4);
#endif
  
  xlen = NORMAL_TILE_WIDTH / 3;
  ylen = NORMAL_TILE_HEIGHT / 3;
  
  canvas_put_line(mapview_canvas.store, COLOR_STD_RED,
                  LINE_TILE_FRAME,
                  canvas_x,
                  canvas_y,
                  xlen, 0);
  canvas_put_line(mapview_canvas.store, COLOR_STD_RED,
                  LINE_TILE_FRAME,
                  canvas_x,
                  canvas_y,
                  0, ylen);
  
  canvas_put_line(mapview_canvas.store, COLOR_STD_RED,
                  LINE_TILE_FRAME,
                  canvas_x + NORMAL_TILE_WIDTH,
                  canvas_y,
                  -xlen, 0);
  canvas_put_line(mapview_canvas.store, COLOR_STD_RED,
                  LINE_TILE_FRAME,
                  canvas_x + NORMAL_TILE_WIDTH,
                  canvas_y,
                  0, ylen);
  
  canvas_put_line(mapview_canvas.store, COLOR_STD_RED,
                  LINE_TILE_FRAME,
                  canvas_x,
                  canvas_y + NORMAL_TILE_HEIGHT,
                  xlen, 0);
  canvas_put_line(mapview_canvas.store, COLOR_STD_RED,
                  LINE_TILE_FRAME,
                  canvas_x,
                  canvas_y + NORMAL_TILE_HEIGHT,
                  0, -ylen);
  
  canvas_put_line(mapview_canvas.store, COLOR_STD_RED,
                  LINE_TILE_FRAME,
                  canvas_x + NORMAL_TILE_WIDTH,
                  canvas_y + NORMAL_TILE_HEIGHT,
                  -xlen, 0);
  canvas_put_line(mapview_canvas.store, COLOR_STD_RED,
                  LINE_TILE_FRAME,
                  canvas_x + NORMAL_TILE_WIDTH,
                  canvas_y + NORMAL_TILE_HEIGHT,
                  0, -ylen);
}

/**************************************************************************
  ...
**************************************************************************/
void draw_link_marks(void)
{
  check_init_mark_table();
  hash_iterate(mark_table, struct tile *, ptile, void *, dummy) {
    assert(ptile);
    if (ptile->client.mark_ttl <= 0) {
      hash_delete_entry(mark_table, ptile);
      continue;
    }
    draw_link_mark(ptile);
  } hash_iterate_end;
}
