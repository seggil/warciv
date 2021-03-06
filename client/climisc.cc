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
#  include "../config.hh"
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "city.hh"
#include "diptreaty.hh"
#include "wc_intl.hh"
#include "game.hh"
#include "hash.hh"
#include "log.hh"
#include "map.hh"
#include "packets.hh"
#include "shared.hh"
#include "spaceship.hh"
#include "support.hh"

#include "include/chatline_g.hh"
#include "include/citydlg_g.hh"
#include "include/cityrep_g.hh"
#include "civclient.hh"
#include "climap.hh"
#include "clinet.hh"
#include "control.hh"
#include "include/dialogs_g.hh"
#include "mapctrl_common.hh"
#include "include/mapview_g.hh"
#include "messagewin_common.hh"
#include "multiselect.hh"
#include "packhand.hh"
#include "include/pages_g.hh"
#include "plrdlg_common.hh"
#include "repodlgs_common.hh"
#include "tilespec.hh"
#include "trade.hh"

#include "climisc.hh"


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

struct map_link {
  enum tag_link_types type;
  int id, turns;
};

#define SPECLIST_TAG map_link
#define SPECLIST_TYPE struct map_link
#include "speclist.hh"
#define map_link_list_iterate(pml) \
  TYPED_LIST_ITERATE(struct map_link, link_marks, pml)
#define map_link_list_iterate_end LIST_ITERATE_END

static struct map_link_list *link_marks = NULL;

static void draw_link_mark(struct map_link *pml);

tile_t *ltile = NULL;
int ltilex, ltiley;

struct voteinfo_list *voteinfo_queue = NULL;
static int voteinfo_queue_current_index = 0;

/**************************************************************************
  ...
**************************************************************************/
void city_autonaming_add_used_name(const char *city_name)
{
  char *cn = mystrdup(city_name);

  freelog(LOG_DEBUG, "city_autonaming_add_used_name \"%s\"", city_name);

  if (!hash_insert(an_city_name_table, cn, NULL)) {
    free(cn);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void city_autonaming_remove_used_name(const char *city_name)
{
  void *key = NULL;
  freelog(LOG_DEBUG, "city_autonaming_remove_used_name \"%s\"", city_name);
  hash_delete_entry_full(an_city_name_table, city_name, &key);
  if (key) {
    free(key);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void city_clear_worklist(city_t *pcity)
{
  struct worklist empty_worklist;
  init_worklist(&empty_worklist);
  city_set_worklist(pcity, &empty_worklist);
}

/**************************************************************************
  ...
**************************************************************************/
void clear_worklists_in_selected_cities(void)
{
  char buf[128];
  if (!tiles_hilited_cities) {
    return;
  }

  connection_do_buffer(&aconnection);
  city_list_iterate(get_player_ptr()->cities, pcity) {
    if (!is_city_hilited(pcity)) {
      continue;
    }
    city_clear_worklist(pcity);
    my_snprintf(buf, sizeof(buf), _("Cleared worklist in %s."),
                pcity->common.name);
    append_output_window(buf);
  } city_list_iterate_end;
  connection_do_unbuffer(&aconnection);
}

/**************************************************************************
  Advance the worlist while the city can't build
**************************************************************************/
void city_worklist_check(city_t *pcity, struct worklist *pwl)
{
  while (pwl->wlefs[0] != WEF_END
         && (pwl->wlefs[0] == WEF_UNIT
             ? !can_build_unit_direct(pcity, pwl->wlids[0])
             : !can_build_improvement_direct(pcity, pwl->wlids[0]))) {
    worklist_advance(pwl);
  }
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
void client_remove_unit(unit_t *punit)
{
  city_t *pcity;
  tile_t *ptile = punit->tile;
  int hc = punit->homecity;
  unit_t *ufocus = get_unit_in_focus();

  freelog(LOG_DEBUG, "removing unit %d, %s %s (%d %d) hcity %d",
          punit->id, get_nation_name(unit_owner(punit)->nation),
          unit_name(punit->type), TILE_XY(punit->tile), hc);

  trade_free_unit(punit);
  refresh_city_dialog_maps(punit->tile);

  if (punit == ufocus) {
    multi_select_wipe_up_unit(punit);
    if (punit == get_unit_in_focus()) {
      set_unit_focus(NULL);
      game_remove_unit(punit);
      advance_unit_focus();
      if (!get_unit_in_focus()) {
        /* It was the last unit */
        delayed_goto_event(AUTO_NO_UNIT_SELECTED, NULL);
      }
    } else {
      game_remove_unit(punit);
    }
    punit = NULL;
  } else {
    /* calculate before punit disappears, use after punit removed: */
    bool update = (ufocus && same_pos(ufocus->tile, punit->tile));

    multi_select_wipe_up_unit(punit);
    game_remove_unit(punit);
    punit = NULL;
    if (update) {
      update_unit_pix_label(get_unit_in_focus());
    }
  }

  pcity = map_get_city(ptile);
  if (pcity) {
    if (client_is_global_observer()
        || can_player_see_units_in_city(get_player_ptr(), pcity)) {
      pcity->u.client.occupied = (unit_list_size(pcity->common.tile->units) > 0);
    }

    refresh_city_dialog(pcity, UPDATE_PRESENT_UNITS);
    freelog(LOG_DEBUG, "map city %s, %s, (%d %d)", pcity->common.name,
            get_nation_name(city_owner(pcity)->nation),
            TILE_XY(pcity->common.tile));
  }

  pcity = player_find_city_by_id(get_player_ptr(), hc);
  if (pcity) {
    refresh_city_dialog(pcity, UPDATE_SUPPORTED_UNITS);
    freelog(LOG_DEBUG, "home city %s, %s, (%d %d)", pcity->common.name,
            get_nation_name(city_owner(pcity)->nation),
            TILE_XY(pcity->common.tile));
  }

  refresh_tile_mapcanvas(ptile, MAP_UPDATE_NORMAL);
}

/**************************************************************************
...
**************************************************************************/
void client_remove_city(city_t *pcity)
{
  bool effect_update;
  tile_t *ptile = pcity->common.tile;

  freelog(LOG_DEBUG, "removing city %s, %s, (%d %d)", pcity->common.name,
          get_nation_name(city_owner(pcity)->nation), TILE_XY(ptile));

  trade_remove_city(pcity);

    /* Explicitly remove all improvements, to properly remove any global effects
     and to handle the preservation of "destroyed" effects. */
  effect_update = FALSE;

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
  refresh_tile_mapcanvas(ptile, MAP_UPDATE_NORMAL);
}

/**************************************************************************
Change all cities building X to building Y, if possible.  X and Y
could be improvements or units. X and Y are compound ids.
**************************************************************************/
void client_change_all(city_cid x, city_cid y)
{
  int fr_id = city_cid_id(x);
  int to_id = city_cid_id(y);
  bool fr_is_unit = city_cid_is_unit(x);
  bool to_is_unit = city_cid_is_unit(y);
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
  city_list_iterate(get_player_ptr()->cities, pcity) {
    if (((fr_is_unit &&
          (pcity->common.is_building_unit) &&
          (pcity->common.currently_building == fr_id)) ||
         (!fr_is_unit &&
          !(pcity->common.is_building_unit) &&
          (pcity->common.currently_building == fr_id))) &&
        ((to_is_unit &&
          can_build_unit(pcity, to_id)) ||
         (!to_is_unit && can_build_improvement(pcity, to_id)))) {
      last_request_id = city_change_production(pcity, to_is_unit, to_id);
    }
  } city_list_iterate_end;

  connection_do_unbuffer(&aconnection);
  reports_freeze_till(last_request_id);
}

/***************************************************************************
  Return a string indicating one nation's embassy status with another
***************************************************************************/
const char *get_embassy_status(player_t *me, player_t *them)
{
  if (me == them || !them->is_alive || !me->is_alive) {
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
const char *get_vision_status(player_t *me, player_t *them)
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
  city_t *pcity;

  switch (pclause->type) {
  case CLAUSE_ADVANCE:
    my_snprintf(buf, bufsiz, _("The %s give %s"),
                get_nation_name_plural(pclause->from->nation),
                get_tech_name(get_player_ptr(), pclause->value));
    break;
  case CLAUSE_CITY:
    pcity = find_city_by_id(pclause->value);
    if (pcity) {
      my_snprintf(buf, bufsiz, _("The %s give %s"),
                  get_nation_name_plural(pclause->from->nation),
                  pcity->common.name);
    } else {
      my_snprintf(buf, bufsiz, _("The %s give unknown city."),
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

/**********************************************************************
  Return a text info about a tile with link mark.
  Do not call this function both in a list of format arguments because
  it returns a static buffer.
***********************************************************************/
char *get_tile_info(tile_t *ptile)
{
  static char buf[256];
  city_t *pcity;

  if (!ptile) {
    my_snprintf(buf, sizeof(buf), _("(unknown tile)"));
  } else if ((pcity = map_get_city(ptile))) {
    insert_city_link(buf, sizeof(buf), pcity);
  } else {
    insert_tile_link(buf, sizeof(buf), ptile);
  }
  return buf;
}

/**************************************************************************
Return the sprite index for the research indicator.
**************************************************************************/
int client_research_sprite(void)
{
  return (NUM_TILES_PROGRESS *
          get_player_ptr()->research.bulbs_researched) /
      (total_bulbs_required(get_player_ptr()) + 1);
}

/**************************************************************************
Return the sprite index for the global-warming indicator.
**************************************************************************/
int client_warming_sprite(void)
{
  int index;
  if ((game.info.globalwarming <= 0) &&
      (game.info.heating < (NUM_TILES_PROGRESS / 2))) {
    index = MAX(0, game.info.heating);
  } else {
    index = MIN(NUM_TILES_PROGRESS,
                (MAX(0, 4 + game.info.globalwarming) / 5) +
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
  if ((game.info.nuclearwinter <= 0) &&
      (game.info.cooling < (NUM_TILES_PROGRESS / 2))) {
    index = MAX(0, game.info.cooling);
  } else {
    index = MIN(NUM_TILES_PROGRESS,
                (MAX(0, 4 + game.info.nuclearwinter) / 5) +
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
  player_t *pplayer;
  city_t *pcity;
  unit_t *punit;

  if (!can_client_change_view()) {
    return;
  }

  pplayer = get_player_ptr();
  can_slide = FALSE;
  if ((punit = get_unit_in_focus())) {
    center_tile_mapcanvas(punit->tile);
  } else if (pplayer && (pcity = find_palace(pplayer))) {
    /* Else focus on the capital. */
    center_tile_mapcanvas(pcity->common.tile);
  } else if (pplayer && city_list_size(pplayer->cities) > 0) {
    /* Just focus on any city. */
    pcity = city_list_get(pplayer->cities, 0);
    assert(pcity != NULL);
    center_tile_mapcanvas(pcity->common.tile);
  } else if (pplayer && unit_list_size(pplayer->units) > 0) {
    /* Just focus on any unit. */
    punit = unit_list_get(pplayer->units, 0);
    assert(punit != NULL);
    center_tile_mapcanvas(punit->tile);
  } else {
    tile_t *ctile = native_pos_to_tile(map.info.xsize / 2,
                                       map.info.ysize / 2);

    /* Just any known tile will do; search near the middle first. */
    /* Iterate outward from the center tile.  We have to give a radius that
     * is guaranteed to be larger than the map will be.  Although this is
     * a misuse of map.xsize and map.ysize (which are native dimensions),
     * it should give a sufficiently large radius. */
    iterate_outward(native_pos_to_tile(map.info.xsize / 2, map.info.ysize / 2),
                    map.info.xsize + map.info.ysize, ptile) {
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
city_cid city_cid_encode(bool is_unit, int id)
{
  return id + (is_unit ? B_LAST : 0);
}

/**************************************************************************
...
**************************************************************************/
city_cid city_cid_encode_from_city(city_t * pcity)
{
  return city_cid_encode(pcity->common.is_building_unit,
                         pcity->common.currently_building);
}

/**************************************************************************
...
**************************************************************************/
void city_cid_decode(city_cid cid, bool * is_unit, int *id)
{
  *is_unit = city_cid_is_unit(cid);
  *id = city_cid_id(cid);
}

/**************************************************************************
...
**************************************************************************/
bool city_cid_is_unit(city_cid cid)
{
  return (cid >= B_LAST);
}

/**************************************************************************
...
**************************************************************************/
int city_cid_id(city_cid cid)
{
  if (cid >= B_LAST)
    return cid - B_LAST;
  else
    return cid;
}

/**************************************************************************
...
**************************************************************************/
worklist_id worklist_id_encode(bool is_unit, bool is_worklist, int id)
{
  assert(!is_unit || !is_worklist);

  if (is_unit) {
    return id + B_LAST;
  }
  if (is_worklist) {
    return id + B_LAST + U_LAST;
  }
  return id;
}

/**************************************************************************
...
**************************************************************************/
bool worklist_id_is_unit(worklist_id wid)
{
  assert(wid != WORKLIST_END);

  return (wid >= B_LAST && wid < B_LAST + U_LAST);
}

/**************************************************************************
...
**************************************************************************/
bool worklist_id_is_worklist(worklist_id wid)
{
  assert(wid != WORKLIST_END);

  return (wid >= B_LAST + U_LAST);
}

/**************************************************************************
...
**************************************************************************/
int worklist_id_id(worklist_id wid)
{
  assert(wid != WORKLIST_END);

  if (wid >= B_LAST + U_LAST) {
    return wid - (B_LAST + U_LAST);
  }
  if (wid >= B_LAST) {
    return wid - B_LAST;
  }
  return wid;
}

/****************************************************************
...
*****************************************************************/
bool city_can_build_impr_or_unit(city_t * pcity, city_cid cid)
{
  if (city_cid_is_unit(cid)) {
    return can_build_unit(pcity, city_cid_id(cid));
  } else {
    return can_build_improvement(pcity, city_cid_id(cid));
  }
}

/****************************************************************
...
*****************************************************************/
bool city_can_sell_impr(city_t * pcity, city_cid cid)
{
  int id;
  id = city_cid_id(cid);

  if (city_cid_is_unit(cid)) {
    return FALSE;
  }

  return !pcity->common.did_sell && city_got_building(pcity, id)
    && !is_wonder(id);
}

/****************************************************************
...
*****************************************************************/
bool city_unit_supported(city_t * pcity, city_cid cid)
{
  if (city_cid_is_unit(cid)) {
    int unit_type = city_cid_id(cid);

    unit_list_iterate(pcity->common.units_supported, punit) {
      if (punit->type == unit_type) {
        return TRUE;
      }
    } unit_list_iterate_end;
  }
  return FALSE;
}

/****************************************************************
...
*****************************************************************/
bool city_unit_present(city_t * pcity, city_cid cid)
{
  if (city_cid_is_unit(cid)) {
    int unit_type = city_cid_id(cid);

    unit_list_iterate(pcity->common.tile->units, punit) {
      if (punit->type == unit_type) {
        return TRUE;
      }
    } unit_list_iterate_end;
  }
  return FALSE;
}

/****************************************************************************
  A TestCityFunc to tell whether the item is a building and is present.
****************************************************************************/
bool city_building_present(city_t * pcity, city_cid cid)
{
  if (!city_cid_is_unit(cid)) {
    int impr_type = city_cid_id(cid);

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

  if (i1->section == i2->section) {
    return mystrcasecmp(i1->descr, i2->descr);
  }
  return (i1->section - i2->section);
}

/**************************************************************************
 Takes an array of city compound ids (city_cids). It will fill out an
 array of struct items and also sort it.

 section 0: normal buildings
 section 1: Capitalization
 section 2: F_NONMIL units
 section 3: other units
 section 4: wonders
**************************************************************************/
void name_and_sort_items(city_cid *pcids, int num_cids, struct item *items,
                         bool show_cost, city_t *pcity)
{
  int i;

  for (i = 0; i < num_cids; i++) {
    bool is_unit = city_cid_is_unit(pcids[i]);
    int id = city_cid_id(pcids[i]), cost;
    struct item *pitem = &items[i];
    const char *name;

    pitem->cid_ = pcids[i];

    if (is_unit) {
      name = get_unit_name(id);
      cost = unit_build_shield_cost(id);
      pitem->section = unit_type_flag(id, F_NONMIL) ? 2 : 3;
    } else {
      name = get_impr_name_ex(pcity, id);
      if (building_has_effect(id, EFFECT_TYPE_PROD_TO_GOLD)) {
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
        my_snprintf(pitem->descr, sizeof(pitem->descr), "%s (%d)", name,
                    cost);
      }
    } else {
      mystrlcpy(pitem->descr, name, sizeof(pitem->descr));
    }
  }

  qsort(items, num_cids, sizeof(struct item), my_cmp);
}

/**************************************************************************
...
**************************************************************************/
int collect_city_cids1(city_cid *dest_cids, city_t **selected_cities,
                       int num_selected_cities, bool append_units,
                       bool append_wonders, bool change_prod,
                       bool(*test_func) (city_t *, city_cid))
{
  city_cid first = append_units ? B_LAST : 0;
  city_cid last = (append_units
              ? game.ruleset_control.num_unit_types + B_LAST
              : game.ruleset_control.num_impr_types);
  city_cid cid;
  int items_used = 0;

  for (cid = first; cid < last; cid++) {
    bool append = FALSE;
    int id = city_cid_id(cid);

    if (!append_units && (append_wonders != is_wonder(id))) {
      continue;
    }

    if (!change_prod) {
      city_list_iterate(get_player_ptr()->cities, pcity) {
        append |= test_func(pcity, cid);
      } city_list_iterate_end;
    } else {
      int i;

      for (i = 0; i < num_selected_cities; i++) {
        append |= test_func(selected_cities[i], cid);
      }
    }

    if (!append) {
      continue;
    }

    dest_cids[items_used] = cid;
    items_used++;
  }
  return items_used;
}

/**************************************************************************
 Collect the city_cids of all targets (improvements and units) which are
 currently built in a city.
**************************************************************************/
int collect_city_cids2(city_cid *dest_city_cids)
{
  bool mapping[B_LAST + U_LAST];
  int city_cids_used = 0;
  city_cid cid;

  memset(mapping, 0, sizeof(mapping));
  city_list_iterate(get_player_ptr()->cities, pcity) {
    mapping[city_cid_encode_from_city(pcity)] = TRUE;
  } city_list_iterate_end;

  for (cid = 0; cid < ARRAY_SIZE(mapping); cid++) {
    if (mapping[cid]) {
      dest_city_cids[city_cids_used] = cid;
      city_cids_used++;
    }
  }
  return city_cids_used;
}

/**************************************************************************
 Collect the city_cids of all targets (improvements and units) which can
 be build in a city.
**************************************************************************/
int collect_city_cids3(city_cid *dest_cids)
{
  int cids_used = 0;

  impr_type_iterate(id) {
    if (can_player_build_improvement(get_player_ptr(), id)) {
      dest_cids[cids_used] = city_cid_encode(FALSE, id);
      cids_used++;
    }
  } impr_type_iterate_end;

  unit_type_iterate(id) {
    if (can_player_build_unit(get_player_ptr(), id)) {
      dest_cids[cids_used] = city_cid_encode(TRUE, id);
      cids_used++;
    }
  } unit_type_iterate_end;

  return cids_used;
}

/**************************************************************************
 Collect the city_cids of all targets which can be build by this city or
 in general.
**************************************************************************/
int collect_city_cids4(city_cid *dest_cids,
                       city_t *pcity,
                       bool advanced_tech)
{
  player_t *pplayer = get_player_ptr();
  int cids_used = 0;

  impr_type_iterate(id) {
    bool can_build;
    bool can_eventually_build;

    if (pplayer) {
      can_build = can_player_build_improvement(pplayer, id);
      can_eventually_build =
          can_player_eventually_build_improvement(pplayer, id);
    } else {
      can_build = FALSE;
      players_iterate(pplayer) {
        if (can_player_build_improvement(pplayer, id)) {
          can_build = TRUE;
          break;
        }
      } players_iterate_end;

      can_eventually_build = FALSE;
      players_iterate(pplayer) {
        if (can_player_eventually_build_improvement(pplayer, id)) {
          can_eventually_build = TRUE;
          break;
        }
      } players_iterate_end;
    }

    /* If there's a city, can the city build the improvement? */
    if (pcity) {
      can_build = can_build && can_build_improvement(pcity, id);
      can_eventually_build = can_eventually_build &&
          can_eventually_build_improvement(pcity, id);
    }

    if ((advanced_tech && can_eventually_build) ||
        (!advanced_tech && can_build)) {
      dest_cids[cids_used] = city_cid_encode(FALSE, id);
      cids_used++;
    }
  } impr_type_iterate_end;

  unit_type_iterate(id) {
    bool can_build;
    bool can_eventually_build;

    if (pplayer) {
      can_build = can_player_build_unit(pplayer, id);
      can_eventually_build = can_player_eventually_build_unit(pplayer, id);
    } else {
      can_build = FALSE;
      players_iterate(pplayer) {
        if (can_player_build_unit(pplayer, id)) {
          can_build = TRUE;
          break;
        }
      } players_iterate_end;

      can_eventually_build = FALSE;
      players_iterate(pplayer) {
        if (can_player_eventually_build_unit(pplayer, id)) {
          can_eventually_build = TRUE;
          break;
        }
      } players_iterate_end;
    }

    /* If there's a city, can the city build the unit? */
    if (pcity) {
      can_build = can_build && can_build_unit(pcity, id);
      can_eventually_build = can_eventually_build &&
          can_eventually_build_unit(pcity, id);
    }

    if ((advanced_tech && can_eventually_build) ||
        (!advanced_tech && can_build)) {
      dest_cids[cids_used] = city_cid_encode(TRUE, id);
      cids_used++;
    }
  } unit_type_iterate_end;

  return cids_used;
}

/**************************************************************************
 Collect the city_cids of all improvements which are built in the given city.
**************************************************************************/
int collect_city_cids5(city_cid *dest_cids, city_t *pcity)
{
  int cids_used = 0;

  assert(pcity != NULL);

  built_impr_iterate(pcity, id) {
    dest_cids[cids_used] = city_cid_encode(FALSE, id);
    cids_used++;
  } built_impr_iterate_end;

  return cids_used;
}

/**************************************************************************
...
**************************************************************************/
int num_supported_units_in_city(city_t *pcity)
{
  struct unit_list *plist;

  if (pcity->common.owner != get_player_idx()) {
    plist = pcity->u.client.info_units_supported;
  } else {
    plist = pcity->common.units_supported;
  }

  return unit_list_size(plist);
}

/**************************************************************************
...
**************************************************************************/
int num_present_units_in_city(city_t *pcity)
{
  struct unit_list *plist;

  if (pcity->common.owner != get_player_idx()) {
    plist = pcity->u.client.info_units_present;
  } else {
    plist = pcity->common.tile->units;
  }

  return unit_list_size(plist);
}

/**************************************************************************
  Handles a chat message.
**************************************************************************/
void handle_event(char *message, tile_t *ptile,
                  enum event_type event, int conn_id)
{
  int where = MSG_TO_OUTPUT_WIN;        /* where to display the message */

  if (event >= E_LAST) {
    /* Server may have added a new event; leave as MW_OUTPUT */
    freelog(LOG_NORMAL, "Unknown event type %d!", event);
  } else if (event >= 0) {
    where = messages_where[event];
  }

  if (BOOL_VAL(where & MSG_TO_OUTPUT_WIN)
      || get_client_state() != CLIENT_GAME_RUNNING_STATE) {
    /* When the game isn't running, the messages dialog isn't present and
     * we want to send all messages to the chatline.  There shouldn't be
     * any problem with server spam in pregame anyway. */
    append_output_window_full(message, conn_id);
  }
  if (BOOL_VAL(where & MSG_TO_MESSAGES_WIN)) {
    add_notify_window(message, ptile, event);
  }
  if (BOOL_VAL(where & MSG_TO_POPUP_WIN) && get_player_ptr()
      && (!get_player_ptr()->ai.control || ai_popup_windows)) {
    popup_notify_goto_dialog(_("Popup Request"), message, ptile);
  }

  play_sound_for_event(event);
}

/**************************************************************************
  Creates a struct packet_generic_message packet and injects it via
  handle_chat_msg.
**************************************************************************/
void create_event(tile_t *ptile, enum event_type event,
                  const char *format, ...)
{
  va_list ap;
  char message[MAX_LEN_MSG];

  va_start(ap, format);
  my_vsnprintf(message, sizeof(message), format, ap);
  va_end(ap);

  handle_event(message, ptile, event, -1);
}

/**************************************************************************
  Writes the supplied string into the file wargame.log.
**************************************************************************/
void write_chatline_content(const char *txt)
{
  char logname[256];
  time_t now;
  struct tm *nowtm;
  FILE *fp;

  now = time(NULL);
  nowtm = localtime(&now);
  strftime(logname, sizeof(logname), "wargame-%y%m%d-%H%M%S.log", nowtm);
  fp = fopen(logname, "w");

  append_output_window(_("Exporting output window to wargame.log ..."));
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
  On the client side, 'pplayer' is not used at all.
*************************************************************************/
enum known_type map_get_known(const tile_t *ptile,
                              player_t *pplayer)
{
  return tile_get_known(ptile);
}

/****************************************************************************
  Convert an integer into string.
  The string will be prefix with spaces.
  string is _[\s]* for n == 0 others [A-Za-z]*[\s]*
****************************************************************************/
static const char *int_to_string(int n, int min_size)
{
  unsigned int size;
  unsigned int i, j;
  int n2;
  double reste;
  static char buf[16];
  static char alphab[52] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z' };

  /* Calcul the length of string produce by "n" */
  reste = (double)n;
  if ( n == 0) {
    size = 1;
  }
  else {
    reste = reste / 52.0;
    for (size = 1; reste > 1.0 ; size++) {
      reste = reste / 52.0;
    }
  }
  assert(size < sizeof(buf));

  if ( min_size > size ) {
    for (i = 0; i < min_size - size; i++) {
      buf[i] = ' ';   /* Complete with spaces */
    }
  } else {
    i = 0;
  }

  if ( n == 0 ) {   // for "n" equal zero, special case
    buf[i++]='_';
    buf[i] = '\0';
  }
  else {
    n2 = n-1;
    for ( j = 0; j < size; j++) {
      buf[i++] = alphab[n2%52];
      n2 = n2/52;
    }
  }

   return buf;
}

/**************************************************************************
  ...
**************************************************************************/
static int an_make_city_name(const char *format, char *buf, int buflen,
                             struct autoname_data *ad)
{
  const char *in = format;
  char *out = buf;
  //char int_buf[32];
  int rem = buflen, len, fw = 0;

  int *pcontinent_counter;

  assert(buflen > 0);

  freelog(LOG_DEBUG, "%s an_make_city_name format=\"%s\" ad=%p", __FUNCTION__,
          format, ad);

  if (!format) {
    mystrlcpy(buf, ad->original_name, buflen);
    return strlen(ad->original_name) + 1;
  }

  pcontinent_counter = (int*)hash_lookup_data(an_continent_counter_table,
                                              INT_TO_PTR(ad->continent_id));
  if (!pcontinent_counter) {
    pcontinent_counter = (int*)wc_malloc(sizeof(int));
    *pcontinent_counter = 0;
    hash_insert(an_continent_counter_table, INT_TO_PTR(ad->continent_id),
                pcontinent_counter);
  }

  freelog(LOG_DEBUG, "%s   *pcontinent_counter = %d", __FUNCTION__,
          *pcontinent_counter);

  if (ad->global_city_number <= 0) {
    ad->global_city_number = ++an_global_city_number_counter;
    freelog(LOG_DEBUG, "%s   assigned new global_city_number (%d)", __FUNCTION__,
            ad->global_city_number);
  }
  if (ad->continent_city_number <= 0) {
    ad->continent_city_number = ++(*pcontinent_counter);
    freelog(LOG_DEBUG, "%s   assigned new continent_city_number (%d)",
            __FUNCTION__, ad->continent_city_number);
  }

  while (rem > 1 && *in != '\0' && *in != ';') {
    if (*in == '%') {
      in++;

      fw = 0;
      while (my_isdigit(*in)) {
        fw = (*in++ - '0') + 10 * fw;
      }

      switch (*in) {
      case 'c':                /* continent id */
        len = my_snprintf(out, rem, "%0*d", fw, ad->continent_id);
        break;

      case 'C':
        len = my_snprintf(out, rem, "%s", int_to_string(ad->continent_id, fw));
        break;

      case 'g':                /* global city counter */
        len = my_snprintf(out, rem, "%0*d", fw, ad->global_city_number);
        break;

      case 'G':
        len = my_snprintf(out, rem, "%s",
                          int_to_string(ad->global_city_number, fw));
        break;

      case 'n':                /* per continent city counter */
        len = my_snprintf(out, rem, "%0*d", fw, ad->continent_city_number);
        break;

      case 'N':
        len = my_snprintf(out, rem, "%s",
                          int_to_string(ad->continent_city_number, fw));
        break;

      case '%':                /* a single percent sign */
        *out = '%';
        len = 1;
        break;

      default:                 /* everything else */
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
      if (*in == '\0') {
        break;
      }
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
static int an_generate_city_name(char *buf, int buflen,
                                 city_t *pcity, char *err, int errlen)
{
  struct autoname_data *ad;
  int num_formats, len = -1, start_format_index;
  char last_generated_name[MAX_LEN_NAME];
  const char *format = NULL;
  bool name_exists = FALSE;

  assert(city_name_formats != NULL);

  freelog(LOG_DEBUG, "agcn an_generate_city_name pcity->common.id=%d \"%s\"",
          pcity->common.id, pcity->common.name);

  num_formats = string_vector_size(city_name_formats) + 1;

  if (num_formats <= 1) {
    my_snprintf(err, errlen, _("There are no city name formats defined. "
                               "Try adding some to Local Options."));
    return 0;
  }

  ad = static_cast<autoname_data*>(
           hash_lookup_data(an_city_autoname_data_table, INT_TO_PTR(pcity->common.id)));
  if (!ad) {
    ad = static_cast<autoname_data*>(wc_malloc(sizeof(struct autoname_data)));
    freelog(LOG_DEBUG, "agcn   new ad %p", ad);
    //printf("%s   new ad %p\n", __FILE__, ad);
    sz_strlcpy(ad->original_name, pcity->common.name);
    ad->city_id = pcity->common.id;
    ad->continent_id = pcity->common.tile->continent;
    ad->format_index = 0;
    ad->global_city_number = 0;
    ad->continent_city_number = 0;
    hash_insert(an_city_autoname_data_table, INT_TO_PTR(pcity->common.id), ad);
  } else {
    ad->format_index++;
    ad->format_index %= num_formats;
  }

  start_format_index = ad->format_index;
  last_generated_name[0] = 0;

  for (;;) {
    format = string_vector_get(city_name_formats, ad->format_index);
    freelog(LOG_DEBUG, "%s   trying format \"%s\" [%d]", __FILE__, format,
            ad->format_index);

    len = an_make_city_name(format, buf, buflen, ad);
    if (len <= 0) {
      break;
    }

    if (!strcmp(buf, last_generated_name)) {
      break;
    }

    sz_strlcpy(last_generated_name, buf);

    name_exists = hash_key_exists(an_city_name_table, buf)
        || !strcmp(pcity->common.name, buf);

    if (!name_exists) {
      break;
    }

    ad->format_index++;
    ad->format_index %= num_formats;

    if (ad->format_index == start_format_index) {
      break;
    }
  }

  if (name_exists) {
    my_snprintf(err, errlen, _("A new name was not generated from "
                               "the format \"%s\"."), format);
    return 0;
  }

  if (len <= 0) {
    my_snprintf(err, errlen, _("Failed to generate name from "
                               "format \"%s\"."), format);
    return 0;
  }

  return len;
}

/**************************************************************************
  ...
**************************************************************************/
void city_autonaming_init(void)
{
  freelog(LOG_DEBUG, "cai city_autonaming_init");
  if (!an_city_name_table) {
    an_city_name_table = hash_new(hash_fval_string2, hash_fcmp_string);
  }
  if (!an_continent_counter_table) {
    an_continent_counter_table = hash_new(hash_fval_int, hash_fcmp_int);
  }
  if (!an_city_autoname_data_table) {
    an_city_autoname_data_table = hash_new(hash_fval_int, hash_fcmp_int);
  }
  an_global_city_number_counter = 0;
}

/**************************************************************************
  ...
**************************************************************************/
void city_autonaming_free(void)
{
  freelog(LOG_DEBUG, "caf city_autonaming_free");

  /* NB we can't iterate over the hash entries and free the
     keys at the same time */
  while (hash_num_entries(an_city_name_table) > 0) {
    char *name = (char *) hash_key_by_number(an_city_name_table, 0);
    hash_delete_entry(an_city_name_table, name);
    freelog(LOG_DEBUG, "caf   freeing \"%s\"", name);
    free(name);
  }
  hash_free(an_city_name_table);
  an_city_name_table = NULL;

  hash_kv_iterate(an_continent_counter_table, void *, key, int *, pcc) {
    int id = PTR_TO_INT(key);
    freelog(LOG_DEBUG, "caf   freeing counter for continent %d (%d)", id,
            *pcc);
    free(pcc);
  } hash_kv_iterate_end;
  freelog(LOG_DEBUG, "caf   deleting all in an_continent_counter_table");
  hash_delete_all_entries(an_continent_counter_table);
  hash_free(an_continent_counter_table);
  an_continent_counter_table = NULL;

  hash_kv_iterate(an_city_autoname_data_table, void *, key,
                  struct autoname_data *, ad) {
    int id = PTR_TO_INT(key);
    freelog(LOG_DEBUG, "caf   freeing ad %p (for city id=%d)", ad, id);
    free(ad);
  } hash_kv_iterate_end;
  freelog(LOG_DEBUG, "caf   deleting all in an_city_autoname_data_table");
  hash_delete_all_entries(an_city_autoname_data_table);
  hash_free(an_city_autoname_data_table);
  an_city_autoname_data_table = NULL;

  an_global_city_number_counter = 0;
}

/**************************************************************************
  ...
**************************************************************************/
void normalize_names_in_selected_cities(void)
{
  char buf[512], err[256];

  freelog(LOG_DEBUG, "normalize_names_in_selected_cities");

  if (!tiles_hilited_cities) {
    return;
  }

  connection_do_buffer(&aconnection);
  city_list_iterate(get_player_ptr()->cities, pcity) {
    if (!is_city_hilited(pcity)) {
      //printf("not highlighted city=%s\n", &pcity->common.name[0]);
      continue;
    }

    if (an_generate_city_name(buf, sizeof(buf), pcity,
                              err, sizeof(err)) > 0) {
      //printf("highlighted city=%s\n", &pcity->common.name[0]);
      city_autonaming_add_used_name(buf);
      city_rename(pcity, buf);
    } else {
      my_snprintf(buf, sizeof(buf),
                  _("Warclient: Could not auto-rename city %s! %s"),
                  pcity->common.name, err);
      append_output_window(buf);
    }
  } city_list_iterate_end;
  connection_do_unbuffer(&aconnection);
}

/**************************************************************************
  Find city nearest to given unit and optionally return squared city
  distance Parameter sq_dist may be NULL. Returns NULL only if no city is
  known. Favors punit owner's cities over other cities if equally distant.
**************************************************************************/
city_t *get_nearest_city(unit_t *punit, int *sq_dist)
{
  city_t *pcity_near;
  int pcity_near_dist;

  if ((pcity_near = map_get_city(punit->tile))) {
    pcity_near_dist = 0;
  } else {
    pcity_near = NULL;
    pcity_near_dist = -1;
    players_iterate(pplayer) {
      city_list_iterate(pplayer->cities, pcity_current) {
        int dist = sq_map_distance(pcity_current->common.tile,
                                   punit->tile);
        if (pcity_near_dist == -1 || dist < pcity_near_dist
            || (dist == pcity_near_dist
                && punit->owner == pcity_current->common.owner)) {
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
void cityrep_buy(city_t *pcity)
{
  int value = city_buy_cost(pcity);

  if (get_current_construction_bonus(pcity, EFFECT_TYPE_PROD_TO_GOLD) > 0) {
    char buf[512];

    assert(!pcity->common.is_building_unit);
    my_snprintf(buf, sizeof(buf),
                _("Game: You don't buy %s in %s!"),
                improvement_types[pcity->common.currently_building].name,
                pcity->common.name);
    append_output_window(buf);
    return;
  }

  if (get_player_ptr()->economic.gold >= value) {
    city_buy_production(pcity);
  } else {
    char buf[512];
    const char *name;

    if (pcity->common.is_building_unit) {
      name = get_unit_type(pcity->common.currently_building)->name;
    } else {
      name = get_impr_name_ex(pcity, pcity->common.currently_building);
    }

    my_snprintf(buf, sizeof(buf),
                _("Game: %s costs %d gold and you only have %d gold."),
                name, value, get_player_ptr()->economic.gold);
    append_output_window(buf);
  }
}

void common_taxrates_callback(int i)
{
  player_t *pplayer;
  int lux_end, sci_end, tax, lux, sci;
  int delta = 10;

  if (!can_client_issue_orders()) {
    return;
  }

  pplayer = get_player_ptr();
  lux_end = pplayer->economic.luxury;
  sci_end = lux_end + pplayer->economic.science;

  lux = pplayer->economic.luxury;
  sci = pplayer->economic.science;
  tax = pplayer->economic.tax;

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
int buy_production_in_selected_cities(void)
{
  if (!tiles_hilited_cities) {
    return 0;
  }

  connection_do_buffer(&aconnection);
  city_list_iterate(get_player_ptr()->cities, pcity) {

    if (get_current_construction_bonus(pcity, EFFECT_TYPE_PROD_TO_GOLD) > 0) {
      continue;
    }

    if (is_city_hilited(pcity)) {
      city_buy_production(pcity);
    }

  } city_list_iterate_end;
  connection_do_unbuffer(&aconnection);

  return 1;
}

/**********************************************************************
  Init.
***********************************************************************/
void link_marks_init(void)
{
  if (!link_marks) {
    link_marks = map_link_list_new();
  }
}

/**********************************************************************
  Find a link mark in the list.
***********************************************************************/
static struct map_link *find_link_mark(enum tag_link_types type, int id)
{
  map_link_list_iterate(pml) {
    if (pml->type == type && pml->id == id) {
      return pml;
    }
  } map_link_list_iterate_end;

  return NULL;
}

/**********************************************************************
  Create a new link mark.
***********************************************************************/
static struct map_link *map_link_new(enum tag_link_types type, int id)
{
  struct map_link *pml = static_cast<map_link*>(wc_malloc(sizeof(struct map_link)));

  pml->type = type;
  pml->id = id;
  map_link_list_append(link_marks, pml);

  return pml;
}

/**********************************************************************
  Remove a link mark.
***********************************************************************/
static void map_link_remove(struct map_link *pml)
{
  map_link_list_unlink(link_marks, pml);
  free(pml);
}

/**********************************************************************
  Clear all visible links.
***********************************************************************/
void clear_all_link_marks(void)
{
  map_link_list_iterate(pml) {
    map_link_remove(pml);
  } map_link_list_iterate_end;

  update_map_canvas_visible(MAP_UPDATE_NORMAL);
}

/**********************************************************************
  Decrease the link marks turn counter.
***********************************************************************/
void decrease_link_mark_turn_counters(void)
{
  map_link_list_iterate(pml) {
    pml->turns--;
    if (pml->turns <= 0) {
      map_link_remove(pml);
    }
  } map_link_list_iterate_end;
}

/**********************************************************************
  Returns the location of the pointed mark.
***********************************************************************/
static tile_t *get_link_mark_tile(struct map_link *pml)
{
  switch (pml->type) {
  case LINK_LOCATION:
    return index_to_tile(pml->id);
  case LINK_CITY:
  case LINK_CITY_ID:
  case LINK_CITY_ID_AND_NAME:
    {
      city_t *pcity = find_city_by_id(pml->id);
      return pcity ? pcity->common.tile : NULL;
    }
  case LINK_UNIT:
    {
      unit_t *punit = find_unit_by_id(pml->id);
      return punit ? punit->tile : NULL;
    }
  }
  return NULL;
}

/**********************************************************************
  Returns the color of the pointed mark.
***********************************************************************/
static enum color_std get_link_mark_color(struct map_link *pml)
{
  switch (pml->type) {
  case LINK_LOCATION:
    return COLOR_STD_RED;
  case LINK_CITY:
  case LINK_CITY_ID:
  case LINK_CITY_ID_AND_NAME:
    return COLOR_STD_GREEN;
  case LINK_UNIT:
    return COLOR_STD_RACE1;     // Cyan
  }
  return COLOR_STD_BLACK;
}

/**********************************************************************
  Add a visible link.
***********************************************************************/
void add_link_mark(enum tag_link_types type, int id)
{
  struct map_link *pml = find_link_mark(type, id);
  tile_t *ptile;

  if (!pml) {
    pml = map_link_new(type, id);
  }
  pml->turns = 2;

  ptile = get_link_mark_tile(pml);
  if (ptile && tile_visible_mapcanvas(ptile)) {
    refresh_tile_mapcanvas(ptile, MAP_UPDATE_NORMAL);
  }
}

/**********************************************************************
  Add a visible link for 1 turn.
***********************************************************************/
void restore_link_mark(enum tag_link_types type, int id)
{
  if (find_link_mark(type, id)) {
    return;
  }

  struct map_link *pml;
  tile_t *ptile;

  pml = map_link_new(type, id);
  pml->turns = 1;

  ptile = get_link_mark_tile(pml);
  if (ptile && tile_visible_mapcanvas(ptile)) {
    refresh_tile_mapcanvas(ptile, MAP_UPDATE_NORMAL);
  }
}

/**********************************************************************
  Print a link mark.
***********************************************************************/
static void draw_link_mark(struct map_link *pml)
{
  int xd = UNIT_TILE_WIDTH / 20, yd = UNIT_TILE_HEIGHT / 20;
  int xlen = UNIT_TILE_WIDTH / 3, ylen = UNIT_TILE_HEIGHT / 3;
  int canvas_x, canvas_y, x0, x1, y0, y1;
  tile_t *ptile = get_link_mark_tile(pml);
  enum color_std color = get_link_mark_color(pml);

  if (!ptile || !tile_visible_mapcanvas(ptile)) {
    return;
  }
  tile_to_canvas_pos(&canvas_x, &canvas_y, ptile);

  x0 = canvas_x + xd;
  x1 = canvas_x + NORMAL_TILE_WIDTH - xd;
  y0 = canvas_y + yd;
  y1 = canvas_y + NORMAL_TILE_HEIGHT - yd;

  canvas_put_line(mapview_canvas.store, color, LINE_TILE_FRAME,
                  x0, y0, xlen, 0);
  canvas_put_line(mapview_canvas.store, color, LINE_TILE_FRAME,
                  x0, y0, 0, ylen);

  canvas_put_line(mapview_canvas.store, color, LINE_TILE_FRAME,
                  x1, y0, -xlen, 0);
  canvas_put_line(mapview_canvas.store, color, LINE_TILE_FRAME,
                  x1, y0, 0, ylen);

  canvas_put_line(mapview_canvas.store, color, LINE_TILE_FRAME,
                  x0, y1, xlen, 0);
  canvas_put_line(mapview_canvas.store, color, LINE_TILE_FRAME,
                  x0, y1, 0, -ylen);

  canvas_put_line(mapview_canvas.store, color, LINE_TILE_FRAME,
                  x1, y1, -xlen, 0);
  canvas_put_line(mapview_canvas.store, color, LINE_TILE_FRAME,
                  x1, y1, 0, -ylen);
}

/**********************************************************************
  Draw all link marks.
***********************************************************************/
void draw_all_link_marks(void)
{
  map_link_list_iterate(pml) {
    draw_link_mark(pml);
  } map_link_list_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void set_default_user_tech_goal(void)
{
  Tech_Type_id goal;

  string_vector_iterate(default_user_tech_goal, tech) {
    goal = find_tech_by_name(tech);
    if (A_LAST == goal) {
      goal = find_tech_by_name_orig(tech);
    }

    if (A_LAST != goal) {
      force_tech_goal(goal);
      return;
    }
  } string_vector_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void force_tech_goal(Tech_Type_id goal)
{
  if (!tech_exists(goal)) {
    return;
  }

  Tech_Type_id next = get_next_tech(get_player_ptr(), goal);
  if (next == A_UNSET) {
    return;
  }

  if (get_player_ptr()->ai.tech_goal != goal) {
    dsend_packet_player_tech_goal(&aconnection, goal);
  }
  dsend_packet_player_research(&aconnection, next);
}

/**************************************************************************
  ...
**************************************************************************/
void voteinfo_queue_delayed_remove(int vote_no)
{
  struct voteinfo *vi;

  if (voteinfo_queue == NULL) {
    freelog(LOG_ERROR, "voteinfo_queue_delayed_remove called before "
            "votinfo_queue_init!");
    return;
  }

  vi = voteinfo_queue_find(vote_no);
  if (vi == NULL) {
    return;
  }
  vi->remove_time = time(NULL);
}

/**************************************************************************
  ...
**************************************************************************/
void voteinfo_queue_check_removed(void)
{
  time_t now;
  struct voteinfo_list *removed;

  if (voteinfo_queue == NULL) {
    return;
  }

  now = time(NULL);
  removed = voteinfo_list_new();
  voteinfo_list_iterate(voteinfo_queue, vi) {
    if (vi != NULL && vi->remove_time > 0 && now - vi->remove_time > 2) {
      voteinfo_list_append(removed, vi);
    }
  } voteinfo_list_iterate_end;

  voteinfo_list_iterate(removed, vi) {
    voteinfo_queue_remove(vi->vote_no);
  } voteinfo_list_iterate_end;

  if (voteinfo_list_size(removed) > 0) {
    voteinfo_gui_update();
  }

  voteinfo_list_free(removed);
}

/**************************************************************************
  ...
**************************************************************************/
void voteinfo_queue_remove(int vote_no)
{
  struct voteinfo *vi;

  if (voteinfo_queue == NULL) {
    freelog(LOG_ERROR, "voteinfo_queue_prepare_remove called before "
            "votinfo_queue_init!");
    return;
  }

  vi = voteinfo_queue_find(vote_no);
  if (vi == NULL) {
    return;
  }

  voteinfo_list_unlink(voteinfo_queue, vi);
  free(vi);
}

/**************************************************************************
  Why not just pass the packet_vote_new all the way here? Modularity and
  dependency redution.
**************************************************************************/
void voteinfo_queue_add(int vote_no,
                        const char *user,
                        const char *desc,
                        int percent_required, int flags, bool is_poll)
{
  struct voteinfo *vi;

  if (voteinfo_queue == NULL) {
    freelog(LOG_ERROR, "voteinfo_queue_add called before "
            "votinfo_queue_init!");
    return;
  }

  vi = static_cast<voteinfo*>(wc_calloc(1, sizeof(struct voteinfo)));
  vi->vote_no = vote_no;
  sz_strlcpy(vi->user, user);
  sz_strlcpy(vi->desc, desc);
  vi->percent_required = percent_required;
  vi->flags = flags;
  vi->is_poll = is_poll;

  if (show_new_vote_in_front) {
    voteinfo_list_prepend(voteinfo_queue, vi);
    voteinfo_queue_current_index = 0;
  } else {
    voteinfo_list_append(voteinfo_queue, vi);
  }
}

/**************************************************************************
  ...
**************************************************************************/
struct voteinfo *voteinfo_queue_find(int vote_no)
{
  if (voteinfo_queue == NULL) {
    freelog(LOG_ERROR, "voteinfo_queue_find called before "
            "votinfo_queue_init!");
    return NULL;
  }

  voteinfo_list_iterate(voteinfo_queue, vi) {
    if (vi->vote_no == vote_no) {
      return vi;
    }
  } voteinfo_list_iterate_end;
  return NULL;
}

/**************************************************************************
  ...
**************************************************************************/
void voteinfo_queue_init(void)
{
  if (voteinfo_queue != NULL) {
    voteinfo_queue_free();
  }
  voteinfo_queue = voteinfo_list_new();
  voteinfo_queue_current_index = 0;
}

/**************************************************************************
  ...
**************************************************************************/
void voteinfo_queue_free(void)
{
  if (voteinfo_queue == NULL) {
    return;
  }

  voteinfo_list_iterate(voteinfo_queue, vi) {
    if (vi != NULL) {
      free(vi);
    }
  } voteinfo_list_iterate_end;

  voteinfo_list_unlink_all(voteinfo_queue);
  voteinfo_list_free(voteinfo_queue);
  voteinfo_queue = NULL;
  voteinfo_queue_current_index = 0;
}

/**************************************************************************
  ...
**************************************************************************/
struct voteinfo *voteinfo_queue_get_current(int *pindex)
{
  struct voteinfo *vi;
  int size;

  if (voteinfo_queue == NULL) {
    return NULL;
  }

  size = voteinfo_list_size(voteinfo_queue);

  if (size <= 0) {
    return NULL;
  }

  if (!(0 <= voteinfo_queue_current_index
        && voteinfo_queue_current_index < size)) {
    voteinfo_queue_next();
  }

  vi = voteinfo_list_get(voteinfo_queue, voteinfo_queue_current_index);

  if (vi != NULL && pindex != NULL) {
    *pindex = voteinfo_queue_current_index;
  }

  return vi;
}

/**************************************************************************
  Obviously only to be used if the server has the voteinfo capability,
  which is the case when this function is called via the voteinfo gui.
**************************************************************************/
void voteinfo_do_vote(int vote_no, enum client_vote_type vote)
{
  struct voteinfo *vi;
  struct packet_vote_submit packet;

  vi = voteinfo_queue_find(vote_no);
  if (vi == NULL) {
    return;
  }

  packet.vote_no = vi->vote_no;

  switch (vote) {
  case CLIENT_VOTE_YES:
    packet.value = 1;
    break;
  case CLIENT_VOTE_NO:
    packet.value = -1;
    break;
  case CLIENT_VOTE_ABSTAIN:
    packet.value = 0;
    break;
  default:
    return;
    break;
  }

  send_packet_vote_submit(&aconnection, &packet);
  vi->client_vote = vote;
}

/**************************************************************************
  ...
**************************************************************************/
void voteinfo_queue_next(void)
{
  int size;

  if (voteinfo_queue == NULL) {
    return;
  }

  size = voteinfo_list_size(voteinfo_queue);

  voteinfo_queue_current_index++;
  if (voteinfo_queue_current_index >= size) {
    voteinfo_queue_current_index = 0;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void toggle_traderoute_drawing_in_selected_cities(void)
{
  player_t *me = get_player_ptr();

  if (!me || !tiles_hilited_cities) {
    return;
  }

  if (!draw_city_traderoutes) {
    city_list_iterate(me->cities, pcity) {
      pcity->u.client.traderoute_drawing_disabled = TRUE;
    } city_list_iterate_end;
    draw_city_traderoutes = TRUE;
  }

  city_list_iterate(me->cities, pcity) {
    if (is_city_hilited(pcity)) {
      pcity->u.client.traderoute_drawing_disabled ^= 1;
    }
  } city_list_iterate_end;

  update_map_canvas_visible(MAP_UPDATE_NORMAL);
}

/**********************************************************************
  Rally point interface...
  Is the server has the extglobalinfo capability, send infos to it.
  Else, do all locally.
  If ptile = NULL, remove it.
***********************************************************************/
void set_rally_point_for_selected_cities(tile_t *ptile)
{
  if (!can_client_issue_orders()) {
    return;
  }

  if (!tiles_hilited_cities) {
    append_output_window(_("Warclient: You must select cities "
                           "to set a rally points."));
    return;
  }

  char buf[1024] = "\0", message[1024], city_name[256];
  bool first = TRUE;

  city_list_iterate(get_player_ptr()->cities, pcity) {
    if (!is_city_hilited(pcity) || pcity->common.rally_point == ptile) {
      continue;
    }

    sz_strlcpy(city_name, get_tile_info(pcity->common.tile));
    if (ptile) {
      cat_snprintf(buf, sizeof(buf), "%s%s", first ? "" : ", ", city_name);
    } else {
      cat_snprintf(buf, sizeof(buf), "%s%s %s",
                   first ? "" : ", ", city_name,
                   get_tile_info(pcity->common.rally_point));
    }
    first = FALSE;

    if (server_has_extglobalinfo) {
      if (ptile) {
        dsend_packet_city_set_rally_point(&aconnection, pcity->common.id,
                                          TILE_XY(ptile));
      } else {
        dsend_packet_city_clear_rally_point(&aconnection, pcity->common.id);
      }
    } else {
      pcity->common.rally_point = ptile;
    }
  } city_list_iterate_end;

  if (!first) {
    if (ptile) {
      my_snprintf(message, sizeof(message),
                  _("Warclient: Set rally point %s for: %s."),
                  get_tile_info(ptile), buf);
      append_output_window(message);
    } else {
      my_snprintf(message, sizeof(message),
                  _("Warclient: Remove rally points for: %s."), buf);
      append_output_window(message);
    }
  }
}

/**********************************************************************
  Execute all air patrol orders.
***********************************************************************/
void execute_air_patrol_orders(void)
{
  if (server_has_extglobalinfo || !can_client_issue_orders()) {
    /* Should be done by the server */
    return;
  }

  connection_do_buffer(&aconnection);
  unit_list_iterate(get_player_ptr()->units, punit) {
    if (punit->air_patrol_tile) {
      send_goto_unit(punit, punit->air_patrol_tile);
    }
  } unit_list_iterate_end;
  connection_do_unbuffer(&aconnection);
}

/**********************************************************************
  ...
***********************************************************************/
void do_unit_air_patrol(unit_t *punit, tile_t *ptile)
{
  char buf[1024];

  if (!can_unit_do_air_patrol(punit)) {
    append_output_window(_("Warclient: This unit cannot do air patrol."));
    return;
  }

  /* Check what to do */
  if (ptile == punit->air_patrol_tile) {
    if (ptile == NULL) {
      /* Was not patrolling */
      return;
    }
    /* Stop patrol when the tile is the same */
    ptile = NULL;
  }

  /* Print message */
  if (ptile) {
    if (punit->air_patrol_tile) {
      char tile_info[32];

      /* Note, it is a static buffer, we cannot use one both. */
      sz_strlcpy(tile_info, get_tile_info(ptile));
      my_snprintf(buf, sizeof(buf),
                  _("Warclient: %s %d patrolling %s "
                    "instead of %s."),
                  unit_name(punit->type), punit->id,
                  tile_info, get_tile_info(punit->air_patrol_tile));
    } else {
      my_snprintf(buf, sizeof(buf), _("Warclient: %s %d patrolling %s."),
                  unit_name(punit->type), punit->id, get_tile_info(ptile));
    }
  } else if (punit->air_patrol_tile) {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: %s %d stopped patrolling %s."),
                unit_name(punit->type), punit->id,
                get_tile_info(punit->air_patrol_tile));
  }
  append_output_window(buf);

  /* Do the change */
  if (server_has_extglobalinfo) {
    if (ptile) {
      dsend_packet_unit_air_patrol(&aconnection, punit->id, TILE_XY(ptile));
    } else {
      dsend_packet_unit_air_patrol_stop(&aconnection, punit->id);
    }
  } else {
    punit->air_patrol_tile = ptile;
  }
}
