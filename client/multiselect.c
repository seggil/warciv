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
# include "../config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

#include "city.h"
#include "game.h"
#include "player.h"
#include "unit.h"

#include "civclient.h"
#include "climisc.h"
#include "clinet.h"
#include "control.h"
#include "goto.h"
#include "multiselect.h"

#include "include/chatline_g.h"
#include "include/mapview_g.h"
#include "include/menu_g.h"

/**********************************************************************
  Filters...
***********************************************************************/

/**********************************************************************
  Change the value of the filter to/of a given value. Should be the only
  accessor function. It is dangerous to modify a filter manually.
  Returns TRUE if other bits than the value are changed.
***********************************************************************/
bool filter_change(filter *pfilter, enum filter_value value)
{
  bool ret = FALSE;

  switch (value) {
  case FILTER_ALL:
  case FILTER_OFF:
    *pfilter = *pfilter & value ? *pfilter & ~value : value;
    ret = TRUE;
    break;
  default:
    if (*pfilter & value) {
      *pfilter &= ~value;
    } else {
      if (*pfilter & FILTER_ALL) {
        *pfilter &= ~FILTER_ALL;
        ret = TRUE;
      } else if (*pfilter & FILTER_OFF) {
        *pfilter &= ~FILTER_OFF;
        ret = TRUE;
      }
      *pfilter |= value;
    }
    break;
  }
  if (*pfilter == 0) {
    *pfilter = FILTER_OFF;
    ret = TRUE;
  }

  return ret;
}

/**********************************************************************
  Returns TRUE iff the unit satisfy all inclusive conditions and
  doesn't satisfy any of the exclusive ones.
***********************************************************************/
bool unit_satisfies_filter(struct unit *punit, filter inclusive_filter,
                           filter exclusive_filter)
{
  if (!punit || inclusive_filter & FILTER_OFF
      || exclusive_filter & FILTER_ALL) {
    return FALSE;
  }
  if (!(inclusive_filter & FILTER_ALL)
      && ((inclusive_filter & FILTER_NEW && !punit->is_new)
          || (inclusive_filter & FILTER_FORTIFIED
              && punit->activity != ACTIVITY_FORTIFIED)
          || (inclusive_filter & FILTER_SENTRIED
              && punit->activity != ACTIVITY_SENTRY)
          || (inclusive_filter & FILTER_AUTO
              && !punit->ai.control && !punit->air_patrol_tile && !punit->ptr)
          || (inclusive_filter & FILTER_VETERAN && punit->veteran == 0)
          || (inclusive_filter & FILTER_IDLE
              && punit->activity != ACTIVITY_IDLE)
          || (inclusive_filter & FILTER_ABLE_TO_MOVE
              && punit->moves_left <= 0)
          || (inclusive_filter & FILTER_FULL_MOVES
              && punit->moves_left != unit_move_rate(punit))
          || (inclusive_filter & FILTER_FULL_HP
              && punit->hp != unit_type(punit)->hp)
          || (inclusive_filter & FILTER_MILITARY
              && !is_military_unit(punit)))) {
    return FALSE;
  }
  if (!(exclusive_filter & FILTER_OFF)
      && ((exclusive_filter & FILTER_NEW && punit->is_new)
          || (exclusive_filter & FILTER_FORTIFIED
              && punit->activity == ACTIVITY_FORTIFIED)
          || (exclusive_filter & FILTER_SENTRIED
              && punit->activity == ACTIVITY_SENTRY)
          || (exclusive_filter & FILTER_AUTO
              && (punit->ai.control || punit->air_patrol_tile || punit->ptr))
          || (exclusive_filter & FILTER_VETERAN && punit->veteran > 0)
          || (exclusive_filter & FILTER_IDLE
              && punit->activity == ACTIVITY_IDLE)
          || (exclusive_filter & FILTER_ABLE_TO_MOVE
              && punit->moves_left > 0)
          || (exclusive_filter & FILTER_FULL_MOVES
              && punit->moves_left == unit_move_rate(punit))
          || (exclusive_filter & FILTER_FULL_HP
              && punit->hp == unit_type(punit)->hp)
          || (exclusive_filter & FILTER_MILITARY
              && is_military_unit(punit)))) {
    return FALSE;
  }
  return TRUE;
}



/**********************************************************************
  Unit multi-selection
***********************************************************************/
#define msassert(multi) assert(multi_select_is_initialized      \
                               && (multi) >= 0                  \
                               && (multi) < MULTI_SELECT_NUM)

/* 0 is the current focus, 1-9 are the battle groups */
static struct multi_select multi_selection[MULTI_SELECT_NUM];
static bool multi_select_is_initialized = FALSE;

filter multi_select_inclusive_filter;
filter multi_select_exclusive_filter;
enum place_value multi_select_place;
enum utype_value multi_select_utype;

bool multi_select_count_all;
bool multi_select_blink_all;
bool multi_select_blink;
bool multi_select_map_selection;
bool multi_select_spread_airport_cities;
bool multi_select_spread_allied_cities;
bool multi_select_double_click;

/**********************************************************************
  ...
***********************************************************************/
bool is_unit_in_multi_select(int multi, struct unit *punit)
{
  msassert(multi);

  unit_list_iterate(multi_selection[multi].ulist, lpunit) {
    if (lpunit == punit) {
      return TRUE;
    }
  } unit_list_iterate_end;
  return FALSE;
}

/**********************************************************************
  Request IDLE action to the selected units.
***********************************************************************/
void multi_select_active_all(int multi)
{
  msassert(multi);

  unit_list_iterate(multi_selection[multi].ulist, punit) {
    request_active_unit(punit);
  } unit_list_iterate_end;

  if (multi == 0) {
    update_unit_info_label(get_unit_in_focus());
  }
}

/**********************************************************************
  ...
***********************************************************************/
void multi_select_add_unit(struct unit *punit)
{
  if (punit->owner != get_player_idx()
      || is_unit_in_multi_select(0, punit)) {
    /* No foreigner units, no units duplications! */
    return;
  }

  if (!multi_select_size(0)) {
    set_unit_focus(punit);
  } else {
    unit_list_append(multi_selection[0].ulist, punit);

    /* Maybe replace the current unit in focus. */
    if (!unit_satisfies_filter(get_unit_in_focus(),
                               multi_select_inclusive_filter,
                               multi_select_exclusive_filter)
        && unit_satisfies_filter(punit, multi_select_inclusive_filter,
                                 multi_select_exclusive_filter)) {
      set_unit_focus(punit);
    }
  }

  /* Blink */
  refresh_tile_mapcanvas(punit->tile, MUT_NORMAL);
}

/**********************************************************************
  Add if the unit is not in focus, else remove.
***********************************************************************/
void multi_select_add_or_remove_unit(struct unit *punit)
{
  if (is_unit_in_multi_select(0, punit)) {
    multi_select_remove_unit(punit);
  } else {
    multi_select_add_unit(punit);
  }
}

/**********************************************************************
  ...
***********************************************************************/
void multi_select_add_units(struct unit_list *ulist)
{
  unit_list_iterate(ulist, punit) {
    multi_select_add_unit(punit);
  } unit_list_iterate_end;
}

/**********************************************************************
  ...
***********************************************************************/
void multi_select_blink_update(void)
{
  unit_list_iterate(multi_selection[0].ulist, punit) {
    if (punit->focus_status != FOCUS_DONE) {
      refresh_tile_mapcanvas(punit->tile, MUT_NORMAL);
    }
  } unit_list_iterate_end;
}

/**********************************************************************
  Add a battle group in an other.
***********************************************************************/
void multi_select_cat(int dest, int src)
{
  char buf[256];

  msassert(src);
  msassert(dest);

  if (multi_select_size(src) == 0) {
    return;
  } else if (multi_select_size(dest) == 0) {
    multi_select_copy(dest, src);
    return;
  }

  unit_list_iterate(multi_selection[src].ulist, punit) {
    if (!is_unit_in_multi_select(dest, punit)) {
      unit_list_append(multi_selection[dest].ulist, punit);
    }
  } unit_list_iterate_end;

  update_unit_info_label(get_unit_in_focus());
  my_snprintf(buf, sizeof(buf),
              _("Warclient: Multi-selection %d added."), src);
  append_output_window(buf);
}

/**********************************************************************
  ...
***********************************************************************/
void multi_select_clear(int multi)
{
  msassert(multi);

  if (multi_select_size(multi) == 0) {
    return;
  }

  if (multi == 0) {
    unit_list_iterate(multi_selection[multi].ulist, punit) {
      unit_list_unlink(multi_selection[multi].ulist, punit);
      refresh_tile_mapcanvas(punit->tile, MUT_NORMAL);
    } unit_list_iterate_end;
  }
  unit_list_unlink_all(multi_selection[multi].ulist); /* to be sure about it */
  multi_selection[multi].punit_focus = NULL;

  if (multi == 0) {
    advance_unit_focus();
  } else {
    char buf[256];
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Multi-selection %d cleared."), multi);
    append_output_window(buf);
  }
}

/**********************************************************************
  ...
***********************************************************************/
void multi_select_clear_all(void)
{
  int i;

  for (i = 0; i < MULTI_SELECT_NUM; i++) {
    unit_list_free(multi_selection[i].ulist);
    multi_selection[i].punit_focus = NULL;
  }
  multi_select_is_initialized = FALSE;
}

/**********************************************************************
  ...
***********************************************************************/
void multi_select_copy(int dest, int src)
{
  char buf[256];

  msassert(src);
  msassert(dest);

  if (multi_select_size(src) == 0) {
    return;
  }

  if (dest == 0) {
    unit_list_iterate(multi_selection[dest].ulist, punit) {
      unit_list_unlink(multi_selection[dest].ulist, punit);
      refresh_tile_mapcanvas(punit->tile, MUT_NORMAL);
    } unit_list_iterate_end;
  }

  unit_list_unlink_all(multi_selection[dest].ulist);
  unit_list_iterate(multi_selection[src].ulist, punit) {
    if (!dest
        || unit_satisfies_filter(punit, multi_select_inclusive_filter,
                                 multi_select_exclusive_filter)) {
      unit_list_append(multi_selection[dest].ulist, punit);
    }
  } unit_list_iterate_end;
  multi_selection[dest].punit_focus = multi_selection[src].punit_focus;

  if (dest == 0) {
    if (multi_selection[dest].punit_focus != get_unit_in_focus()) {
      set_unit_focus(multi_selection[0].punit_focus);
    } else {
      update_unit_info_label(get_unit_in_focus());
    }
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Multi-selection %d selected."), src);
  } else {
    int size = multi_select_size(dest);
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Multi-selection %d: %d %s."), dest, size,
                PL_("unit", "units", size));
  }
  append_output_window(buf);
}

/**********************************************************************
  ...
***********************************************************************/
const struct unit_list *multi_select_get_units_focus(void)
{
  return multi_selection[0].ulist;
}

/**********************************************************************
  ...
***********************************************************************/
void multi_select_init_all(void)
{
  int i;

  for (i = 0; i < MULTI_SELECT_NUM; i++) {
    multi_selection[i].ulist = unit_list_new();
    multi_selection[i].punit_focus = NULL;
  }
  multi_select_is_initialized = TRUE;
}

/**********************************************************************
  ...
***********************************************************************/
void multi_select_remove_unit(struct unit *punit)
{
  unit_list_unlink(multi_selection[0].ulist, punit);
  refresh_tile_mapcanvas(punit->tile, MUT_NORMAL);

  if (punit == get_unit_in_focus()) {
    struct unit *pnuf = NULL;

    unit_list_iterate(multi_selection[0].ulist, msunit) {
      if (unit_satisfies_filter(msunit, multi_select_inclusive_filter,
                                multi_select_exclusive_filter)) {
        set_unit_focus(msunit);
        return;
      } else if (!pnuf) {
        pnuf = msunit;
      }
    } unit_list_iterate_end;

    if (pnuf) {
      set_unit_focus(pnuf);
    } else {
      advance_unit_focus();
    }
  }
}

/**********************************************************************
  Return the number of selected units which statisfy the filters.
***********************************************************************/
int multi_select_satisfies_filter(int multi)
{
  int count = 0;

  msassert(multi);

  unit_list_iterate(multi_selection[multi].ulist, punit) {
    if (punit->focus_status != FOCUS_DONE
        && unit_satisfies_filter(punit, multi_select_inclusive_filter,
                                 multi_select_exclusive_filter)) {
      count++;
    }
  } unit_list_iterate_end;

  return count;
}

/**********************************************************************
  ...
***********************************************************************/
Unit_Type_id multi_select_unit_type(int multi)
{
  msassert(multi);
  assert(multi_selection[multi].punit_focus != NULL);

  return multi_selection[multi].punit_focus->type;
}

/**********************************************************************
  Set one unit in a battle group.
***********************************************************************/
void multi_select_set_unit(int multi, struct unit *punit)
{
  msassert(multi);

  if (multi == 0) {
    unit_list_iterate(multi_selection[multi].ulist, punit) {
      unit_list_unlink(multi_selection[multi].ulist, punit);
      refresh_tile_mapcanvas(punit->tile, MUT_NORMAL);
    } unit_list_iterate_end;
  }

  unit_list_unlink_all(multi_selection[multi].ulist);
  multi_selection[multi].punit_focus = punit;

  if (punit) {
    unit_list_append(multi_selection[multi].ulist, punit);
  }
}

/**********************************************************************
  ...
***********************************************************************/
void multi_select_set_unit_focus(int multi, struct unit *punit)
{
  msassert(multi);

  assert(is_unit_in_multi_select(multi, punit));

  multi_selection[multi].punit_focus = punit;
}

/**********************************************************************
  ...
***********************************************************************/
int multi_select_size(int multi)
{
  msassert(multi);

  return unit_list_size(multi_selection[multi].ulist);
}

/**************************************************************************
  Spread function
**************************************************************************/
struct scity {
  int tdv, rdv;
  int tav, rav;
  struct city *pcity;
  struct unit_list *ulist;
};

#define SPECLIST_TAG scity
#define SPECLIST_TYPE struct scity
#include "speclist.h"
#define scity_list_iterate(alist,pitem) \
  TYPED_LIST_ITERATE(struct scity, alist, pitem)
#define scity_list_iterate_end LIST_ITERATE_END

struct scity *find_weakest_city(struct scity_list *psclist);
struct unit *find_best_unit(struct unit_list *pulist);

/**************************************************************************
  ...
**************************************************************************/
struct scity *find_weakest_city(struct scity_list *psclist)
{
  struct scity *wscity = NULL;

  scity_list_iterate(psclist, pscity) {
    if (unit_list_size(pscity->ulist) == 0) {
      continue;
    }
    if (!wscity
        || pscity->rdv < wscity->rdv
        || (pscity->rdv == wscity->rdv
            && (pscity->tdv < wscity->tdv
                || (pscity->tdv == wscity->tdv
                    && (pscity->rav < wscity->rav
                        || (pscity->rav == wscity->rav
                            && pscity->tav < wscity->rav)))))) {
      wscity = pscity;
    }
  } scity_list_iterate_end;
  return wscity;
}

/**************************************************************************
  ...
**************************************************************************/
struct unit *find_best_unit(struct unit_list *pulist)
{
  struct unit *bunit = NULL;

  unit_list_iterate(pulist, punit) {
    if (!bunit
        || unit_type(punit)->defense_strength
           > unit_type(bunit)->defense_strength
        || (unit_type(punit)->defense_strength
            == unit_type(bunit)->defense_strength
            && unit_type(punit)->attack_strength
               > unit_type(bunit)->attack_strength))
      bunit = punit;
  } unit_list_iterate_end;
  return bunit;
}

/**************************************************************************
  ...
**************************************************************************/
void multi_select_spread(void)
{
  if (multi_select_satisfies_filter(0) == 0) {
    return;
  }

  struct scity_list *sclist;
  struct scity *pscity = NULL;
  Continent_id cid = get_unit_in_focus()->tile->continent;

  /* get datas */
  sclist = scity_list_new();
  players_iterate(pplayer) {
    if ((!multi_select_spread_allied_cities && pplayer != get_player_ptr())
        || !pplayers_allied(get_player_ptr(), pplayer)) {
      continue;
    }

    city_list_iterate(pplayer->cities, pcity) {
      if (pcity->tile->continent != cid
          || (multi_select_spread_airport_cities
              && get_city_bonus(pcity, EFT_AIRLIFT) == 0)) {
        continue;
      }
      pscity = fc_malloc(sizeof(struct scity));
      pscity->tdv = pscity->rdv = pscity->tav = pscity->rav = 0;
      pscity->pcity = pcity;
      pscity->ulist = unit_list_new();
      scity_list_append(sclist, pscity);
    } city_list_iterate_end;
  } players_iterate_end;

  if (scity_list_size(sclist) == 0) {
    goto clean_up;
  }
  connection_do_buffer(&aconnection);
  multi_select_iterate(FALSE, punit) {
    struct unit_type *type = unit_type(punit);
    struct scity *last = NULL;
    bool multi = FALSE;

    scity_list_iterate(sclist, pscity) {
      if (calculate_move_cost(punit, pscity->pcity->tile) > punit->moves_left) {
        continue;
      }
      unit_list_append(pscity->ulist, punit);
      pscity->tdv += type->defense_strength;
      pscity->tav += type->attack_strength;

      if (last) {
        multi = TRUE;
      }
      last = pscity;
    } scity_list_iterate_end;
    if (!multi && last) {
      /* send if the unit can go to one only city */
      send_goto_unit(punit, last->pcity->tile);
      unit_list_unlink(pscity->ulist, punit);
      last->tdv -= type->defense_strength;
      last->tav -= type->attack_strength;
      last->rdv += type->defense_strength;
      last->rav += type->attack_strength;
    }
  } multi_select_iterate_end;

  /* Execute */
  while ((pscity = find_weakest_city(sclist))) {
    struct unit *punit = find_best_unit(pscity->ulist);

    if (punit) {
      struct unit_type *type = unit_type(punit);
      send_goto_unit(punit, pscity->pcity->tile);
      pscity->rdv += type->defense_strength;
      pscity->rav += type->attack_strength;
      scity_list_iterate(sclist, pscity) {
        if (!unit_list_find(pscity->ulist, punit->id)) {
          continue;
        }
        unit_list_unlink(pscity->ulist, punit);
        pscity->tdv -= type->defense_strength;
        pscity->tav -= type->attack_strength;
      } scity_list_iterate_end;
    }
  }
  connection_do_unbuffer(&aconnection);

  /* Free datas */
clean_up:
  scity_list_iterate(sclist, pscity) {
    unit_list_free(pscity->ulist);
    free(pscity);
  } scity_list_iterate_end;
  scity_list_free(sclist);
}

/**********************************************************************
  Remove all reference to one unit (called in client_remove_unit).
***********************************************************************/
void multi_select_wipe_up_unit(struct unit *punit)
{
  bool need_update_menus = FALSE;
  int i;

  multi_select_remove_unit(punit);
  for (i = 1; i <= 9; i++) {
    unit_list_unlink(multi_selection[i].ulist, punit);
    if (!unit_list_size(multi_selection[i].ulist)) {
      multi_select_clear(i);
      need_update_menus = TRUE;
    } else if (multi_selection[i].punit_focus == punit) {
      multi_selection[i].punit_focus =
          unit_list_get(multi_selection[i].ulist, 0);
    }
  }

  if (need_update_menus) {
    update_menus();
  }
}

/**********************************************************************
  Use the selection mode and the filters to select units.
***********************************************************************/
void multi_select_select(void)
{
  struct unit *punit_focus = get_unit_in_focus(), *punf = NULL;
  struct unit_list *ulist;

  if (!punit_focus) {
    return;
  }

  if (multi_select_size(0) > 1) {
    multi_select_set_unit(0, punit_focus);
  }
  if (multi_select_place == PLACE_SINGLE_UNIT) {
    return;
  }
  if (multi_select_place == PLACE_IN_TRANSPORTER
      || multi_select_place == PLACE_ON_TILE) {
    ulist = punit_focus->tile->units;
  } else {
    ulist = get_player_ptr()->units;
  }

  unit_list_iterate(ulist, punit) {
    if (punit->owner != get_player_idx()
        || (multi_select_place == PLACE_IN_TRANSPORTER
            && punit->transported_by != punit_focus->transported_by)
        || (multi_select_place == PLACE_ON_CONTINENT
            && punit->tile->continent != punit_focus->tile->continent)
        || (multi_select_utype == UTYPE_SAME_MOVE_TYPE
            && unit_type(punit)->move_type !=
            unit_type(punit_focus)->move_type)
        || (multi_select_utype == UTYPE_SAME_TYPE
            && punit->type != punit_focus->type)) {
      continue;
    }
    multi_select_add_unit(punit);
    if (unit_satisfies_filter(punit, multi_select_inclusive_filter,
                              multi_select_exclusive_filter)) {
      punf = punit;
    }
  } unit_list_iterate_end;

  if (!unit_satisfies_filter(punit_focus, multi_select_inclusive_filter,
                             multi_select_exclusive_filter)
      && punf) {
    punit_focus = punf;
    set_unit_focus(punf);
  }
  update_unit_info_label(punit_focus);
  update_menus();
}



/**********************************************************************
  Delayed goto.
***********************************************************************/
#define dgassert(dg) assert(delayed_goto_is_initialized \
                            && (dg) >= 0                \
                            && (dg) < DELAYED_GOTO_NUM)
#define auto_timers_num 5

filter delayed_goto_inclusive_filter;
filter delayed_goto_exclusive_filter;
enum place_value delayed_goto_place;
enum utype_value delayed_goto_utype;

enum delayed_goto_type delayed_goto_state;
int delayed_goto_unit_limit;    /* 0 = unlimited */
int delayed_goto_need_tile_for = -1;

/* 0 is the current delayed queue, 1-3 are the extra queues */
struct delayed_goto delayed_goto_list[DELAYED_GOTO_NUM];
static bool delayed_goto_is_initialized = FALSE;
static const int auto_timer_values[auto_timers_num] = { 50, 80, 90, 95, -5 };
static const enum automatic_execution auto_timer_event[auto_timers_num] = {
  AUTO_50_TIMEOUT, AUTO_80_TIMEOUT, AUTO_90_TIMEOUT,
  AUTO_95_TIMEOUT, AUTO_5_SECONDS
};
static struct auto_timer auto_timers[auto_timers_num];

/**********************************************************************
  Add a delayed goto structure in a queue.
***********************************************************************/
void delayed_goto_add_unit(int dg, int id, int type, struct tile *ptile)
{
  dgassert(dg);

  struct delayed_goto_data *dgd
    = fc_malloc(sizeof(struct delayed_goto_data));

  dgd->id = id;
  dgd->type = type;
  dgd->ptile = ptile;
  delayed_goto_data_list_append(delayed_goto_list[dg].dglist, dgd);
  delayed_goto_list[dg].pplayer = get_tile_player(ptile);
}

/**********************************************************************
  Change the automatic_execution filter to/of the given value.
  Returns TRUE if other bits than the value are changed.
***********************************************************************/
bool delayed_goto_auto_filter_change(filter *pfilter,
                                     enum automatic_execution value)
{
  bool ret = FALSE;

  switch (value) {
  case AUTO_OFF:
    *pfilter = AUTO_OFF;
    return TRUE;
  default:
    if (*pfilter & value) {
      *pfilter &= ~value;
    } else {
      if (*pfilter & AUTO_OFF) {
        *pfilter = 0;
        ret = TRUE;
      }
      *pfilter |= value;
    }
    break;
  }
  if (*pfilter == 0) {
    *pfilter = AUTO_OFF;
    ret = TRUE;
  }

  return ret;
}

/**********************************************************************
  Init the timers.
***********************************************************************/
void delayed_goto_auto_timers_init(void)
{
  int i;

  for (i = 0; i < auto_timers_num; i++) {
    if (game.info.timeout > 0) {
      if (auto_timer_values[i] >= 0) {
        auto_timers[i].seconds = (game.info.timeout
                                  * (100 - auto_timer_values[i])) / 100;
      } else {
        auto_timers[i].seconds = -auto_timer_values[i];
      }
    } else {
      auto_timers[i].seconds = -1;
    }
    auto_timers[i].npassed = TRUE;
  }
}

/**********************************************************************
  Update the timers and check if one event should be created.
***********************************************************************/
void delayed_goto_auto_timers_update(void)
{
  int i;

  for (i = 0; i < auto_timers_num; i++) {
    if (auto_timers[i].npassed
        && game.info.seconds_to_turndone <= auto_timers[i].seconds) {
      delayed_goto_event(auto_timer_event[i], NULL);
      auto_timers[i].npassed = FALSE;
    }
  }
}

/**********************************************************************
  Add a delayed goto queue to an other.
***********************************************************************/
void delayed_goto_cat(int dest, int src)
{
  char buf[256];

  dgassert(src);
  dgassert(dest);

  if (delayed_goto_size(src) == 0) {
    return;
  }

  if (delayed_goto_size(dest) == 0) {
    delayed_goto_copy(dest, src);
    return;
  }

  delayed_goto_data_list_iterate(delayed_goto_list[src].dglist, dgd) {
    delayed_goto_add_unit(dest, dgd->id, dgd->type, dgd->ptile);
  } delayed_goto_data_list_iterate_end;

  delayed_goto_list[dest].automatic_execution
    |= delayed_goto_list[src].automatic_execution;
  if (delayed_goto_list[dest].automatic_execution & AUTO_OFF) {
    delayed_goto_list[dest].automatic_execution = AUTO_OFF;
  }

  if (!delayed_goto_list[dest].pplayer) {
    delayed_goto_list[dest].pplayer = delayed_goto_list[src].pplayer;
  }

  my_snprintf(buf, sizeof(buf),
              _("Warclient: Adding %d delayed orders to queue."),
              delayed_goto_size(src));
  append_output_window(buf);
}

/**********************************************************************
  Clear an queue.
***********************************************************************/
void delayed_goto_clear(int dg)
{
  char buf[256];

  dgassert(dg);

  if (delayed_goto_size(dg) == 0) {
    return;
  }

  delayed_goto_data_list_iterate(delayed_goto_list[dg].dglist, dgd) {
    free(dgd);
  } delayed_goto_data_list_iterate_end;
  delayed_goto_data_list_unlink_all(delayed_goto_list[dg].dglist);
  delayed_goto_list[dg].pplayer = NULL;

  if (dg != 0) {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Delayed goto selection %d cleared."), dg);
  } else {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Delayed orders queue cleared."));
  }
  append_output_window(buf);
  update_delayed_goto_menu(dg);
}

/**********************************************************************
  Free all...
***********************************************************************/
void delayed_goto_clear_all(void)
{
  int i;

  for (i = 0; i < DELAYED_GOTO_NUM; i++) {
    delayed_goto_data_list_iterate(delayed_goto_list[i].dglist, dgd) {
      free(dgd);
    } delayed_goto_data_list_iterate_end;
    delayed_goto_data_list_free(delayed_goto_list[i].dglist);
    delayed_goto_list[i].pplayer = NULL;
  }
  delayed_goto_is_initialized = FALSE;
}

/**********************************************************************
  Copy a queue to an other.
***********************************************************************/
void delayed_goto_copy(int dest, int src)
{
  char buf[256];

  dgassert(src);
  dgassert(dest);

  if (delayed_goto_size(src) == 0) {
    return;
  }

  delayed_goto_clear(dest);
  delayed_goto_data_list_iterate(delayed_goto_list[src].dglist, dgd) {
    delayed_goto_add_unit(dest, dgd->id, dgd->type, dgd->ptile);
  } delayed_goto_data_list_iterate_end;
  delayed_goto_list[dest].pplayer = delayed_goto_list[src].pplayer;
  update_delayed_goto_menu(dest);
  if (dest) {
    my_snprintf(buf, sizeof(buf),
        _("Warclient: Delayed goto selection %d: %d delayed order(s)."),
        dest, delayed_goto_size(dest));
  } else {
    my_snprintf(buf, sizeof(buf),
        _("Warclient: Set %d delayed order(s) to queue."),
        delayed_goto_size(src));
  }
  append_output_window(buf);
}

/**********************************************************************
  Receive an event. pplayer can be NULL.
***********************************************************************/
void delayed_goto_event(enum automatic_execution event, struct player *pplayer)
{
  int i;

  freelog(LOG_DEBUG, "delayed_goto_event: receive signal %d", event);
  for (i = 0; i < DELAYED_GOTO_NUM; i++) {
    if (delayed_goto_list[i].automatic_execution & event) {
      if (pplayer) {
        request_player_execute_delayed_goto(pplayer, i);
      } else {
        request_execute_delayed_goto(NULL, i);
      }
    }
  }
}

/**********************************************************************
  ...
***********************************************************************/
const char *delayed_goto_get_auto_name(enum automatic_execution value)
{
  switch (value) {
  case AUTO_NEW_YEAR:
    return N_("Start of turn");
  case AUTO_PRESS_TURN_DONE:
    return N_("Turn done pressed");
  case AUTO_NO_UNIT_SELECTED:
    return N_("No unit in focus");
  case AUTO_50_TIMEOUT:
    return N_("50% of timeout");
  case AUTO_80_TIMEOUT:
    return N_("80% of timeout");
  case AUTO_90_TIMEOUT:
    return N_("90% of timeout");
  case AUTO_95_TIMEOUT:
    return N_("95% of timeout");
  case AUTO_5_SECONDS:
    return N_("5 seconds before turn done");
  case AUTO_WAR_DIPLSTATE:
    return N_("Targetted player declares war");
  case AUTO_OFF:
    return N_("Off");
  }
  /* Don't set as default case to be warned if we forgot to add a value */
  return NULL;
}

/**********************************************************************
  Init all...
***********************************************************************/
void delayed_goto_init_all(void)
{
  int i;

  for (i = 0; i < DELAYED_GOTO_NUM; i++) {
    delayed_goto_list[i].dglist = delayed_goto_data_list_new();
    delayed_goto_list[i].pplayer = NULL;
    delayed_goto_list[i].automatic_execution = AUTO_WAR_DIPLSTATE;
  }
  delayed_goto_is_initialized = TRUE;
}

/**********************************************************************
  Copy a queue to an other and clear the source.
***********************************************************************/
void delayed_goto_move(int dest, int src)
{
  dgassert(src);
  dgassert(dest);

  delayed_goto_copy(dest, src);
  delayed_goto_clear(src);
}

/**********************************************************************
  Return the size of the queue.
***********************************************************************/
int delayed_goto_size(int dg)
{
  dgassert(dg);

  return delayed_goto_data_list_size(delayed_goto_list[dg].dglist);
}

/**********************************************************************
  Try to evaluate the target player for the given tile.
***********************************************************************/
struct player *get_tile_player(struct tile *ptile)
{
  int count[game.info.nplayers], best = -1;

  if (!ptile) {
    return NULL;
  }

  if (ptile->city) {
    return city_owner(ptile->city);
  }

  memset(count, 0, sizeof(count));

  unit_list_iterate(ptile->units, punit) {
    switch (pplayer_get_diplstate(get_player_ptr(), unit_owner(punit))->type) {
    case DS_NEUTRAL:
    case DS_WAR:
    case DS_CEASEFIRE:
    case DS_PEACE:
    case DS_NO_CONTACT:
      count[punit->owner]++;
      if (best < 0 || count[punit->owner] > count[best]) {
        best = punit->owner;
      }
      break;
    default:
      return NULL;
    }
  } unit_list_iterate_end;

  return best >= 0 ? get_player(best) : NULL;
}

/**********************************************************************
  ...
***********************************************************************/
static bool unit_can_do_delayed_action(struct unit *punit,
                                       enum delayed_goto_type dgtype)
{
  switch (dgtype) {
  case DGT_NORMAL:
    return TRUE;
  case DGT_NUKE:
    return unit_flag(punit, F_NUCLEAR);
  case DGT_PARADROP:
    return unit_flag(punit, F_PARATROOPERS);
  case DGT_AIRLIFT:
  case DGT_BREAK:
    return FALSE;
  }
  freelog(LOG_ERROR, "Unkown delayed goto type varient (%d).", dgtype);
  return FALSE;
}

/**********************************************************************
  Add units to delayed goto queue.
***********************************************************************/
void add_unit_to_delayed_goto(struct tile *ptile)
{
  struct unit *punit_focus = get_unit_in_focus();
  struct unit_list *ulist;
  int count = 0;
  char buf[256];

  if (!punit_focus || hover_state != HOVER_DELAYED_GOTO) {
    return;
  }

  if (delayed_goto_place == PLACE_SINGLE_UNIT) {
    multi_select_iterate(FALSE, punit) {
      if (unit_can_do_delayed_action(punit, delayed_goto_state)) {
        delayed_goto_add_unit(0, punit->id, delayed_goto_state, ptile);
        count++;
      }
    } multi_select_iterate_end;
  } else {
    if (delayed_goto_place == PLACE_IN_TRANSPORTER
        || delayed_goto_place == PLACE_ON_TILE) {
      ulist = punit_focus->tile->units;
    } else {
      ulist = get_player_ptr()->units;
    }

    unit_list_iterate(ulist, punit) {
      if (punit->owner != get_player_idx()
          || (delayed_goto_place == PLACE_IN_TRANSPORTER
              && punit->transported_by != punit_focus->transported_by)
          || (delayed_goto_place == PLACE_ON_CONTINENT
              && punit->tile->continent != punit_focus->tile->continent)
          || (delayed_goto_utype == UTYPE_SAME_MOVE_TYPE
              && unit_type(punit)->move_type !=
              unit_type(punit_focus)->move_type)
          || (delayed_goto_utype == UTYPE_SAME_TYPE
              && punit->type != punit_focus->type)
          || !unit_satisfies_filter(punit, delayed_goto_inclusive_filter,
                                    delayed_goto_exclusive_filter)
          || !unit_can_do_delayed_action(punit, delayed_goto_state)) {
        continue;
      }
      delayed_goto_add_unit(0, punit->id, delayed_goto_state, ptile);
      count++;
    } unit_list_iterate_end;
  }

  if (count == 0) {
    append_output_window(_("Warclient: No units selected for "
                           "delayed goto."));
    return;
  }

  switch (delayed_goto_state) {
  case DGT_NORMAL:
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Adding %d %s goto %s to queue."), count,
                PL_("unit", "units", count), get_tile_info(ptile));
    break;
  case DGT_NUKE:
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Adding %d %s NUCLEAR DETONATION at %s to queue."),
                count, PL_("unit", "units", count), get_tile_info(ptile));
    break;
  case DGT_PARADROP:
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Adding %d %s paradrop to %s to queue."),
                count, PL_("unit", "units", count), get_tile_info(ptile));
    break;
  default:
    return;
  }

  append_output_window(buf);
  update_delayed_goto_menu(0);
}

/**********************************************************************
  Maybe execute a delayed goto queue after a
  player cancel diplomacy event.
***********************************************************************/
void request_player_execute_delayed_goto(struct player *pplayer, int dg)
{
  dgassert(dg);

  if (delayed_goto_list[dg].pplayer == pplayer) {
    request_execute_delayed_goto(NULL, dg);
    update_delayed_goto_menu(dg);
  }
}

/**********************************************************************
  Execute a delated queue.
***********************************************************************/
void request_unit_execute_delayed_goto(int dg)
{
  dgassert(dg);

  if (delayed_goto_size(dg) == 0) {
    return;
  }
  /* Check if there is a unkown tile, then ask the user to pick it first */
  delayed_goto_data_list_iterate(delayed_goto_list[dg].dglist, dgd) {
    if (!dgd->ptile && dgd->type != DGT_BREAK) {
      hover_state = HOVER_DELAYED_GOTO;
      delayed_goto_need_tile_for = dg;
      update_hover_cursor();
      return;
    }
  } delayed_goto_data_list_iterate_end;
  request_execute_delayed_goto(NULL, dg);
}

/**********************************************************************
  Really execute it...
***********************************************************************/
void request_execute_delayed_goto(struct tile *ptile, int dg)
{
  char buf[256];
  int counter = 0;

  dgassert(dg);

  if (delayed_goto_size(dg) == 0) {
    return;
  }

  if (dg != 0) {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Executing delayed goto selection %d."), dg);
  } else {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Executing delayed goto."));
  }
  append_output_window(buf);

  connection_do_buffer(&aconnection);
  delayed_goto_data_list_iterate(delayed_goto_list[dg].dglist, dgd) {
    if (dgd->type == DGT_BREAK) {
      delayed_goto_data_list_unlink(delayed_goto_list[dg].dglist, dgd);
      free(dgd);
      break;
    }

    if (!dgd->ptile) {
      /* No selected target: use the default one */
      if (ptile) {
        dgd->ptile = ptile;
      } else {
        continue;
      }
    }

    if (delayed_goto_unit_limit && (++counter > delayed_goto_unit_limit)) {
      /* Check the unit limit */
      break;
    }

    if (dgd->type == DGT_AIRLIFT) {
      airlift_queue_need_city_for = dgd->id;
      do_airlift(dgd->ptile);
    } else {
      struct unit *punit =
          player_find_unit_by_id(get_player_ptr(), dgd->id);

      if (!punit) {
        /* Unfortunately, it seems we already lost this unit :( */
        delayed_goto_data_list_unlink(delayed_goto_list[dg].dglist, dgd);
        free(dgd);
        continue;
      }

      switch (dgd->type) {
      case DGT_NORMAL:
        /* Normal move */
        send_goto_unit(punit, dgd->ptile);
        punit->is_new = FALSE;
        break;
      case DGT_NUKE:
        if (unit_flag(punit, F_NUCLEAR)) {
          /* Move nuke */
          send_goto_unit(punit, dgd->ptile);
          if (calculate_move_cost(punit, dgd->ptile) > punit->moves_left) {
            /* Cannot reach */
            request_new_unit_activity(punit, ACTIVITY_IDLE);
            /* FIXME: Should we stop the execution of the whole queue? */
          } else {
            /* Boom! */
            do_unit_nuke(punit);
          }
          punit->is_new = FALSE;
        }
        break;
      case DGT_PARADROP:
        if (unit_flag(punit, F_PARATROOPERS)) {
          /* Paradrop */
          do_unit_paradrop_to(punit, dgd->ptile);
          punit->is_new = FALSE;
        }
        break;
      default:
        freelog(LOG_ERROR, "Unkown delayed goto type varient (%d).", dgd->type);
        break;
      }
    }
    delayed_goto_data_list_unlink(delayed_goto_list[dg].dglist, dgd);
    free(dgd);
  } delayed_goto_data_list_iterate_end;
  connection_do_unbuffer(&aconnection);

  /* Count orders left */
  counter = delayed_goto_size(dg);
  if (counter == 0) {
    delayed_goto_clear(dg);
  } else {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: %d delayed goto(s) remain in the queue. "
                  "Press Y to continue."), counter);
    append_output_window(buf);
  }
  update_delayed_goto_menu(dg);
}

/**********************************************************************
  Add a airlift request in the delayed queue.
***********************************************************************/
void schedule_delayed_airlift(struct tile *ptile)
{
  char buf[256];

  my_snprintf(buf, sizeof(buf),
              _("Warclient: Scheduling delayed airlift for %s."),
              get_tile_info(ptile));
  append_output_window(buf);
  delayed_goto_add_unit(0, airlift_queue_need_city_for, DGT_AIRLIFT, ptile);
  update_delayed_goto_menu(0);
  airlift_queue_need_city_for = -1;
}

/**********************************************************************
  Add a pause in the delayed queue.
***********************************************************************/
void add_pause_delayed_goto(void)
{
  append_output_window(_("Warclient: Adding pause in delayed goto."));
  delayed_goto_add_unit(0, 0, DGT_BREAK, NULL);
  update_delayed_goto_menu(0);
}



/**********************************************************************
  Airlift queues.
***********************************************************************/
#define aqassert(aq) assert(airlift_is_initialized              \
                            && (aq) >= 0                        \
                            && (aq) < AIRLIFT_QUEUE_NUM)

int airlift_queue_need_city_for = -1;
/* 0 is the current one 1-6 are the extra ones (4-9 in the menus) */
static struct airlift_queue airlift_queues[AIRLIFT_QUEUE_NUM];
static bool airlift_is_initialized = FALSE;

/**********************************************************************
  Add an airlift queue in an other.
***********************************************************************/
void airlift_queue_cat(int dest, int src)
{
  char buf[256];

  aqassert(src);
  aqassert(dest);

  if (airlift_queue_size(src) == 0) {
    return;
  }
  if (airlift_queue_size(dest) == 0) {
    airlift_queue_copy(dest, src);
    return;
  }

  tile_list_iterate(airlift_queues[src].tlist, ptile) {
    if (!tile_list_search(airlift_queues[dest].tlist, ptile)) {
      tile_list_append(airlift_queues[dest].tlist, ptile);
    }
  } tile_list_iterate_end;

  if (airlift_queues[dest].utype == U_LAST) {
    airlift_queues[dest].utype = airlift_queues[src].utype;
  }
  my_snprintf(buf, sizeof(buf),
              _("Warclient: Adding %d tiles to airlift queue."),
              airlift_queue_size(src));
  append_output_window(buf);
}

/**********************************************************************
  Clear an airlift queue.
***********************************************************************/
void airlift_queue_clear(int aq)
{
  char buf[256];

  aqassert(aq);

  if (airlift_queue_size(aq) == 0) {
    return;
  }
  tile_list_unlink_all(airlift_queues[aq].tlist);

  if (aq != 0) {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Airlift queue %d cleared."),
                aq + DELAYED_GOTO_NUM - 1);
  } else {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Airlift queue cleared."));
  }
  append_output_window(buf);

  update_airlift_menu(aq);
}

/**********************************************************************
  ...
***********************************************************************/
void airlift_queue_clear_all(void)
{
  int i;

  for (i = 0; i < AIRLIFT_QUEUE_NUM; i++) {
    tile_list_free(airlift_queues[i].tlist);
  }
  airlift_is_initialized = FALSE;
}

/**********************************************************************
  Copy an airlift queue to an other.
***********************************************************************/
void airlift_queue_copy(int dest, int src)
{
  char buf[256];

  aqassert(src);
  aqassert(dest);

  if (airlift_queue_size(src) == 0) {
    return;
  }

  airlift_queue_clear(dest);
  tile_list_iterate(airlift_queues[src].tlist, ptile) {
    tile_list_append(airlift_queues[dest].tlist, ptile);
  } tile_list_iterate_end;

  airlift_queues[dest].utype = airlift_queues[src].utype;

  init_menus();

  if (dest != 0) {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Airlift queue %d: %d selected tile(s)."),
                dest, airlift_queue_size(dest));
  } else {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Set airlift queue %d on current "
                  "airlift queue (%d tile(s))."), src,
                airlift_queue_size(src));
  }
  append_output_window(buf);
}

/**********************************************************************
  ...
***********************************************************************/
const char *airlift_queue_get_menu_name(int aq, Unit_Type_id utype)
{
  aqassert(aq);
  assert(utype >= 0 && utype <= U_LAST);

  return airlift_queues[aq].namemenu[utype];
}

/**********************************************************************
  ...
***********************************************************************/
Unit_Type_id airlift_queue_get_unit_type(int aq)
{
  aqassert(aq);

  return airlift_queues[aq].utype;
}

/**********************************************************************
  ...
***********************************************************************/
void airlift_queue_init_all(void)
{
  int i, j;

  for (i = 0; i < AIRLIFT_QUEUE_NUM; i++) {
    airlift_queues[i].tlist = tile_list_new();
    airlift_queues[i].utype = U_LAST;
    for (j = 0; j <= U_LAST; j++) {
      sz_strlcpy(airlift_queues[i].namemenu[j], "\0");
    }
  }
  airlift_is_initialized = TRUE;
}

/**********************************************************************
  Copy an airlift queue to an other and free the source queue.
***********************************************************************/
void airlift_queue_move(int dest, int src)
{
  aqassert(src);
  aqassert(dest);

  airlift_queue_copy(dest, src);
  airlift_queue_clear(src);
}

/**********************************************************************
  ...
***********************************************************************/
void airlift_queue_set_menu_name(int aq, Unit_Type_id utype,
                                 const char *namemenu)
{
  aqassert(aq);
  assert(utype >= 0 && utype <= U_LAST);

  sz_strlcpy(airlift_queues[aq].namemenu[utype], namemenu);
}

/**********************************************************************
  ...
***********************************************************************/
void airlift_queue_set_unit_type(int aq, Unit_Type_id utype)
{
  aqassert(aq);
  assert(utype >= 0 && utype <= U_LAST);

  airlift_queues[aq].utype = utype;
  update_airlift_menu(aq);
}

/**********************************************************************
  Show the cities in the queue.
***********************************************************************/
void airlift_queue_show(int aq)
{
  char buf[1024];
  bool first = TRUE;

  aqassert(aq);

  if (airlift_queue_size(aq) == 0) {
    return;
  }

  sz_strlcpy(buf, _("Warclient: Cities in airlift queue:"));
  if (aq != 0) {
    cat_snprintf(buf, sizeof(buf), " %d:", aq + DELAYED_GOTO_NUM - 1);
  } else {
    cat_snprintf(buf, sizeof(buf), ":");
  }

  tile_list_iterate(airlift_queues[aq].tlist, ptile) {
    cat_snprintf(buf, sizeof(buf), "%s %s",
                 first ? "" : ",", get_tile_info(ptile));
    first = FALSE;
  } tile_list_iterate_end;

  cat_snprintf(buf, sizeof(buf), ".");
  append_output_window(buf);
}

/**********************************************************************
  Returns the size of the airlift queue.
***********************************************************************/
int airlift_queue_size(int aq)
{
  aqassert(aq);

  return tile_list_size(airlift_queues[aq].tlist);
}

/**********************************************************************
  Add a city to the current airlift queue. If multi is FALSE, it will
  remove the tile if the tile is already in the queue.
***********************************************************************/
void add_city_to_auto_airlift_queue(struct tile *ptile, bool multi)
{
  char buf[256] = "\0";

  if (!ptile) {
    return;
  }

  if (!ptile->city) {
    append_output_window(_("Warclient: You need to select a tile "
                           "with city."));
    return;
  }

  if (tile_list_search(airlift_queues[0].tlist, ptile)) {
    if (!multi) {
      tile_list_unlink(airlift_queues[0].tlist, ptile);
      my_snprintf(buf, sizeof(buf),
                  _("Warclient: Removed city %s from the auto "
                    "airlift queue."), get_tile_info(ptile));
    }
  } else {
    tile_list_prepend(airlift_queues[0].tlist, ptile);
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Adding city %s to auto airlift queue."),
                get_tile_info(ptile));
  }

  if (buf[0] != '\0') {
    append_output_window(buf);
  }
  if (!multi) {
    update_airlift_menu(0);
  }
}

/**********************************************************************
  Add all cities with an airport in the current airlift queue.
***********************************************************************/
void request_auto_airlift_source_selection_with_airport(void)
{
  city_list_iterate(get_player_ptr()->cities, pcity) {
    if (get_city_bonus(pcity, EFT_AIRLIFT) > 0) {
      add_city_to_auto_airlift_queue(pcity->tile, TRUE);
    }
  } city_list_iterate_end;

  update_airlift_menu(0);
}

/**********************************************************************
  Do airlift for a given airlift queue.
***********************************************************************/
void do_airlift_for(int aq, struct city *pcity)
{
  aqassert(aq);

  if (airlift_queues[aq].utype == U_LAST) {
    return;
  }

  if (!pcity) {
    /* We need to select a city to airlift to, delay it... */
    airlift_queue_need_city_for = aq;
    request_auto_airlift_destination_selection();
    return;
  }

  tile_list_iterate(airlift_queues[aq].tlist, ptile) {
    if (!ptile->city || ptile->city->owner != get_player_idx()
        || unit_list_size(ptile->units) == 0) {
      continue;
    }
    unit_list_iterate(ptile->units, punit) {
      if (punit->type == airlift_queues[aq].utype) {
        request_unit_airlift(punit, pcity);
        break;
      }
    } unit_list_iterate_end;
  } tile_list_iterate_end;
}

/**********************************************************************
  Do massive airlift...
***********************************************************************/
void do_airlift(struct tile *ptile)
{
  if (!ptile->city) {
    char buf[256];

    my_snprintf(buf, sizeof(buf),
                _("Warclient: You cannot airlift there %s; no city exists."),
                get_tile_info(ptile));
    append_output_window(buf);
    return;
  }

  connection_do_buffer(&aconnection);
  if (airlift_queue_need_city_for >= 0
      && airlift_queue_need_city_for < AIRLIFT_QUEUE_NUM) {
    do_airlift_for(airlift_queue_need_city_for, ptile->city);
  } else if (airlift_queue_size(0) && airlift_queues[0].utype != U_LAST) {
    do_airlift_for(0, ptile->city);
  } else {
    int i;

    for (i = 1; i < AIRLIFT_QUEUE_NUM; i++) {
      do_airlift_for(i, ptile->city);
    }
  }
  airlift_queue_need_city_for = -1;
  connection_do_unbuffer(&aconnection);
}

/***********************************************************************
  Add a city to the specified airlift queue. If multi is FALSE, it will
  remove the tile if the tile is already in the queue.
************************************************************************/
void add_city_to_specific_auto_airlift_queue(int aq, struct city *pcity)
{
  char buf[256] = "\0";

  aqassert(aq);

  if (!pcity
      || !pcity->tile
      || tile_list_search(airlift_queues[aq].tlist, pcity->tile)) {
    return;
  }

  tile_list_prepend(airlift_queues[aq].tlist, pcity->tile);
  if (aq == 0) {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Adding city %s to auto airlift queue."),
                get_tile_info(pcity->tile));
  } else {
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Adding city %s to auto airlift queue %d."),
                get_tile_info(pcity->tile), aq + DELAYED_GOTO_NUM - 1);
  }

  append_output_window(buf);
}
