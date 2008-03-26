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
#include <stdlib.h>
#include <string.h>

#include "city.h"
#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "mem.h"
#include "player.h"
#include "unit.h"
#include "shared.h"
#include "support.h"

#include "chatline_g.h"
#include "civclient.h"
#include "clinet.h"
#include "control.h"
#include "goto.h"
#include "mapview_g.h"
#include "menu_g.h"
#include "multiselect.h"

/********************************************************************** 
  Filters...
***********************************************************************/

/********************************************************************** 
  Change the value of the filter to/of a given value.
***********************************************************************/
void filter_change(filter *pfilter, enum filter_value value)
{
  switch (value) {
  case FILTER_ALL:
    *pfilter = *pfilter & FILTER_ALL ? *pfilter - FILTER_ALL : FILTER_ALL;
    break;
  case FILTER_OFF:
    *pfilter = *pfilter & FILTER_OFF ? *pfilter - FILTER_OFF : FILTER_OFF;
    break;
  default:
    if (*pfilter & value) {
      *pfilter -= value;
    } else {
      if (*pfilter & FILTER_ALL) {
        *pfilter -= FILTER_ALL;
      } else if (*pfilter & FILTER_OFF) {
        *pfilter -= FILTER_OFF;
      }
      *pfilter += value;
    }
    break;
  }
  if (*pfilter == 0) {
    *pfilter = FILTER_OFF;
  }
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
              && !punit->ai.control && !punit->my_ai.control)
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
              && (punit->ai.control || punit->my_ai.control))
          || (exclusive_filter & FILTER_VETERAN
              && punit->veteran > 0)
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
  Automatic Processus
***********************************************************************/
#define ap_timers_num 5

static struct automatic_processus_list *automatic_processus_list;
static const int ap_timer_values[ap_timers_num] = { 50, 80, 90, 95, -5 };
static const enum automatic_value ap_timer_event[ap_timers_num] = {
  AUTO_50_TIMEOUT, AUTO_80_TIMEOUT, AUTO_90_TIMEOUT,
  AUTO_95_TIMEOUT, AUTO_5_SECONDS
};
static struct ap_timer ap_timers[ap_timers_num];
static const char *ap_events[] = {
  N_("New Year"),
  N_("Press 'turn done'"),
  N_("No unit selected"),
  N_("50% timeout"),
  N_("80% timeout"),
  N_("90% timeout"),
  N_("95% timeout"),
  N_("5 seconds before end turn"),
  N_("Unit receives auto orders"),
  N_("Player cancels diplomacy"),
  N_("Off")
};

/********************************************************************** 
  Init the timers.
***********************************************************************/
void ap_timers_init(void)
{
  int i;

  for (i = 0; i < ap_timers_num; i++) {
    if (game.info.timeout > 0) {
      if (ap_timer_values[i] >= 0) {
        ap_timers[i].seconds = (game.info.timeout
                                * (100 - ap_timer_values[i])) / 100;
      } else {
        ap_timers[i].seconds = -ap_timer_values[i];
      }
    } else {
      ap_timers[i].seconds = -1;
    }
    ap_timers[i].npassed = TRUE;
  }
}

/********************************************************************** 
  Update the timers and check if one event should be created.
***********************************************************************/
void ap_timers_update(void)
{
  int i;

  for (i = 0; i < ap_timers_num; i++) {
    if (ap_timers[i].npassed
        && game.info.seconds_to_turndone <= ap_timers[i].seconds) {
      automatic_processus_event(ap_timer_event[i], NULL);
      ap_timers[i].npassed = FALSE;
    }
  }
}

/********************************************************************** 
  Change the automatic_processus filter to/of the given value.
***********************************************************************/
void auto_filter_change(automatic_processus *pap,
                        enum automatic_value value)
{
  int real_value = AV_TO_FV(value);
  static const int real_AUTO_OFF = AV_TO_FV(AUTO_OFF);

  if (!pap) {
    return;
  }
  if (!is_auto_value_allowed(pap, value)) {
    freelog(LOG_ERROR,
            "Received a wrong value for automatic_processus '%s': %d",
            pap->description, value);

    return;
  }

  switch (value) {
  case AUTO_OFF:
    pap->auto_filter = real_AUTO_OFF;
    break;
  default:
    if (pap->auto_filter & real_value) {
      pap->auto_filter &= ~real_value;
    } else {
      if (pap->auto_filter & real_AUTO_OFF) {
        pap->auto_filter = 0;
      }
      pap->auto_filter |= real_value;
    }
    break;
  }
  if (pap->auto_filter == 0) {
    pap->auto_filter = real_AUTO_OFF;
  }
}

/********************************************************************** 
  Adapt the automatic_processus filter from a merged one.
***********************************************************************/
void auto_filter_normalize(automatic_processus *pap)
{
  int v, rv;

  for (v = 0; v < AUTO_OFF; v++) {
    rv = AV_TO_FV(v);
    if (pap->auto_filter & rv && !is_auto_value_allowed(pap, v)) {
      pap->auto_filter &= ~rv;
    }
  }
  rv = AV_TO_FV(AUTO_OFF);
  if (pap->auto_filter & rv) {
    pap->auto_filter &= ~rv;
  }
  if (pap->auto_filter == 0) {
    pap->auto_filter = rv;
  }
}

/********************************************************************** 
  Returns TRUE iff there is a callback connected to the given event type.
***********************************************************************/
bool is_auto_value_allowed(automatic_processus *pap,
                           enum automatic_value value)
{
  return pap ? pap->callback[value] != NULL : FALSE;
}

/********************************************************************** 
  Accessor function for event names.
***********************************************************************/
const char *ap_event_name(enum automatic_value event)
{
  assert(event >= 0 && event < AUTO_VALUES_NUM);
  return ap_events[event];
}

/********************************************************************** 
  Receive an new event.
***********************************************************************/
void automatic_processus_event(enum automatic_value event, void *data)
{
  int real_event = AV_TO_FV(event);

  freelog(LOG_DEBUG, "automatic_processus_event: receive signal ' %s '",
          ap_event_name(event));
  automatic_processus_iterate(pap) {
    if (pap->auto_filter & real_event) {
      assert(is_auto_value_allowed(pap, event));
      pap->callback[event](data, pap->data);
    }
  } automatic_processus_iterate_end;
}

/********************************************************************** 
  ...
***********************************************************************/
void automatic_processus_init(void)
{
  automatic_processus_list = automatic_processus_list_new();
}

/**************************************************************************
  - default_auto_filter is the default filter
    (value is an AV_TO_FV(enum automatic_value))
  - page is the dialog page where this appears (or PAGE_NUM).
  - path is the menu filter access. If there is no path, pass "" to the
    function
  - data is an constant int pass to arg2 to callback functions
  - ... is all callback functions to connect (ended by -1). For exemple:
    AP_CONNECT(AUTO_NEW_YEAR, my_callback), -1
**************************************************************************/
automatic_processus *real_automatic_processus_new(const char *file,
                                                  const int line,
                                                  enum peppage page,
                                                  filter default_auto_filter,
                                                  const char *menu,
                                                  const char *description,
                                                  int data, ...)
{
  automatic_processus *pap = fc_malloc(sizeof(automatic_processus));
  filter auto_values_allowed = 0;
  int i;
  va_list callbacks;

  /* init */
  pap->page = page;
  pap->data = data;
  sz_strlcpy(pap->menu, menu);
  sz_strlcpy(pap->description, description);
  memset(pap->callback, 0, AUTO_VALUES_NUM * sizeof(ap_callback));

  /* Be sure this will never happen */
  pap->callback[AUTO_OFF] = (ap_callback) abort;

  va_start(callbacks, data);
  for (i = va_arg(callbacks, int); i >= 0 && i < AUTO_OFF;
       i = va_arg(callbacks, int)) {
    if (auto_values_allowed & AV_TO_FV(i)) {
      freelog(LOG_ERROR,
              "automatic_processus_new: multiple callbacks for "
              "signal '%s' from %s line %d",
              ap_event_name(i), file, line);
    }
    auto_values_allowed |= AV_TO_FV(i);
    pap->callback[i] = va_arg(callbacks, ap_callback);
#ifdef DEBUG
    const char *callback_name = va_arg(callbacks, const char *);
    freelog(LOG_DEBUG, "connecting signal '%s' to '%s' callback",
            ap_event_name(i), callback_name);
#endif /* DEBUG */
  }
  va_end(callbacks);

  /* test for default_auto_filter */
  if (auto_values_allowed != (default_auto_filter | auto_values_allowed)
      && default_auto_filter != AV_TO_FV(AUTO_OFF)) {
    freelog(LOG_ERROR, "automatic_processus_new: bad default "
            "auto filter from %s line %d",
            file, line);
    pap->default_auto_filter = AV_TO_FV(AUTO_OFF);
  } else {
    pap->default_auto_filter = default_auto_filter;
    pap->auto_filter = pap->default_auto_filter;
  }

  /* append on the list */
  automatic_processus_list_append(automatic_processus_list, pap);
  return pap;
}

/********************************************************************** 
  ...
***********************************************************************/
void automatic_processus_remove(automatic_processus *pap)
{
  automatic_processus_list_unlink(automatic_processus_list, pap);
  free(pap);
}

/********************************************************************** 
  ...
***********************************************************************/
const struct automatic_processus_list *get_automatic_processus(void)
{
  return automatic_processus_list;
}

/********************************************************************** 
  ...
***********************************************************************/
automatic_processus *find_automatic_processus_by_name(const char *name)
{
  automatic_processus_iterate(pap) {
    if ((pap->menu[0] && !strcmp(pap->menu, name))
        || (pap->description[0] && !strcmp(pap->description, name))) {
      return pap;
    }
  } automatic_processus_iterate_end;

  return NULL;
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
      my_snprintf(buf, sizeof(buf),
                  _("Warclient: Multi-selection %d selected."), src);
    }
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
const struct multi_select *multi_select_get(int multi)
{
  msassert(multi);

  return &multi_selection[multi];
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
  ...
***********************************************************************/
void multi_select_set(int multi, const struct multi_select *pms)
{
  assert(pms != NULL);
  msassert(multi);

  if (unit_list_size(pms->ulist) == 0) {
    return;
  }

  if (multi == 0) {
    unit_list_iterate(multi_selection[multi].ulist, punit) {
      unit_list_unlink(multi_selection[multi].ulist, punit);
      refresh_tile_mapcanvas(punit->tile, MUT_NORMAL);
    } unit_list_iterate_end;
  }

  unit_list_unlink_all(multi_selection[multi].ulist);
  unit_list_iterate(pms->ulist, punit) {
    unit_list_append(multi_selection[multi].ulist, punit);
  } unit_list_iterate_end;
  multi_selection[multi].punit_focus = pms->punit_focus;

  if (multi == 0) {
    if (multi_selection[0].punit_focus != get_unit_in_focus()) {
      set_unit_focus(multi_selection[0].punit_focus);
    } else {
      update_unit_info_label(get_unit_in_focus());
    }
  }
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

filter delayed_goto_inclusive_filter;
filter delayed_goto_exclusive_filter;
enum place_value delayed_goto_place;
enum utype_value delayed_goto_utype;

int delayed_para_or_nuke = 0;   /* 0 normal, 1 nuke/para, 2 airlift, 3 break */
int unit_limit;                 /* 0 = unlimited */
int need_tile_for = -1;
/* 0 is the current delayed queue, 1-3 are the extra queues */
static struct delayed_goto delayed_goto_list[DELAYED_GOTO_NUM];
static bool delayed_goto_is_initialized = FALSE;

/********************************************************************** 
  Return a text info about a tile.
***********************************************************************/
static char *get_tile_info(struct tile *ptile)
{
  static char buf[256];
  struct city *pcity;

  if (!ptile) {
    my_snprintf(buf, sizeof(buf), _("(unknown tile)"));
  } else if ((pcity = map_get_city(ptile))) {
    my_snprintf(buf, sizeof(buf), "%s", pcity->name);
  } else {
    my_snprintf(buf, sizeof(buf), "(%d, %d)", ptile->x, ptile->y);
  }
  return buf;
}

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

  delayed_goto_list[dest].pap->auto_filter
      |= delayed_goto_list[src].pap->auto_filter;
  auto_filter_normalize(delayed_goto_list[dest].pap);

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
    automatic_processus_remove(delayed_goto_list[i].pap);
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
  Accessor function.
***********************************************************************/
const struct delayed_goto *delayed_goto_get(int dg)
{
  dgassert(dg);

  return &delayed_goto_list[dg];
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
    if (i != 0) {
      char buf[256], buf2[256];

      my_snprintf(buf, sizeof(buf), "DELAYED_GOTO_DG%d_AUTOMATIC", i);
      my_snprintf(buf2, sizeof(buf2),
                  _("Delayed goto queue %d automatic execution"), i);
      delayed_goto_list[i].pap =
          automatic_processus_new(PAGE_NUM, AV_TO_FV(AUTO_WAR_DIPLSTATE),
                                  buf, buf2, i,
                                  AP_MAIN_CONNECT
                                  (request_execute_delayed_goto),
                                  AP_CONNECT(AUTO_WAR_DIPLSTATE,
                                             request_player_execute_delayed_goto),
                                  -1);
    } else {
      delayed_goto_list[0].pap =
          automatic_processus_new(PAGE_DG, AV_TO_FV(AUTO_WAR_DIPLSTATE),
                                  "DELAYED_GOTO_AUTOMATIC",
                                  _("Delayed goto automatic execution"),
                                  0,
                                  AP_MAIN_CONNECT
                                  (request_execute_delayed_goto),
                                  AP_CONNECT(AUTO_WAR_DIPLSTATE,
                                             request_player_execute_delayed_goto),
                                  -1);
    }
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
  ...
***********************************************************************/
void delayed_goto_set(int dg, const struct delayed_goto *pdg)
{
  dgassert(dg);
  assert(pdg);

  if (delayed_goto_data_list_size(pdg->dglist) == 0) {
    return;
  }
  delayed_goto_clear(dg);
  delayed_goto_data_list_iterate(pdg->dglist, dgd) {
    delayed_goto_add_unit(dg, dgd->id, dgd->type, dgd->ptile);
  } delayed_goto_data_list_iterate_end;
  delayed_goto_list[dg].pplayer = pdg->pplayer;
  update_delayed_goto_menu(dg);
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
    switch (pplayer_get_diplstate(get_player_ptr(), unit_owner(punit))->
            type) {
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
      delayed_goto_add_unit(0, punit->id, delayed_para_or_nuke, ptile);
      count++;
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
                                    delayed_goto_exclusive_filter)) {
        continue;
      }
      delayed_goto_add_unit(0, punit->id, delayed_para_or_nuke, ptile);
      count++;
    } unit_list_iterate_end;
  }

  if (count == 0) {
    append_output_window(_("Warclient: No units selected for "
                           "delayed goto."));
    return;
  }

  my_snprintf(buf, sizeof(buf),
              _("Warclient: Adding %d %s goto %s to queue."), count,
              PL_("unit", "units", count), get_tile_info(ptile));
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
    if (!dgd->ptile && dgd->type != 3) {
      hover_state = HOVER_DELAYED_GOTO;
      need_tile_for = dg;
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
    if (dgd->type == 3) {
      /* Pause */
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

    if (unit_limit && (++counter > unit_limit)) {
      /* Check the unit limit */
      break;
    }

    if (dgd->type == 2) {
      /* Airlift */
      need_city_for = dgd->id;
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

      if (dgd->type == 0 || !unit_flag(punit, F_PARATROOPERS)) {
        /* Normal move */
        send_goto_unit(punit, dgd->ptile);
        punit->is_new = FALSE;
      } else {
        /* Paradrop */
        do_unit_paradrop_to(punit, dgd->ptile);
        punit->is_new = FALSE;
      }
      if (dgd->type == 1 && unit_flag(punit, F_NUCLEAR)) {
        /* Explode nuke */
        do_unit_nuke(punit);
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
  delayed_goto_add_unit(0, need_city_for, 2, ptile);
  update_delayed_goto_menu(0);
  need_city_for = -1;
}

/********************************************************************** 
  Add a pause in the delayed queue.
***********************************************************************/
void add_pause_delayed_goto(void)
{
  append_output_window(_("Warclient: Adding pause in delayed goto."));
  delayed_goto_add_unit(0, 0, 3, NULL);
  update_delayed_goto_menu(0);
}


/********************************************************************** 
  Airlift queues.
***********************************************************************/
#define aqassert(aq) assert(airlift_is_initialized              \
                            && (aq) >= 0                        \
                            && (aq) < AIRLIFT_QUEUE_NUM)

int need_city_for = -1;
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
                airlift_queue_size(aq));
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
const struct airlift_queue *airlift_queue_get(int aq)
{
  aqassert(aq);

  return &airlift_queues[aq];
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
void airlift_queue_set(int aq, const struct airlift_queue *paq)
{
  aqassert(aq);

  if (tile_list_size(paq->tlist) > 0) {
    tile_list_unlink_all(airlift_queues[aq].tlist);
    tile_list_iterate(paq->tlist, ptile) {
      tile_list_append(airlift_queues[aq].tlist, ptile);
    } tile_list_iterate_end;
  }
  airlift_queues[aq].utype = paq->utype;
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
    cat_snprintf(buf, sizeof(buf), " %d:", aq);
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
                    "airlift queue."),
                  ptile->city->name);
    }
  } else {
    tile_list_prepend(airlift_queues[0].tlist, ptile);
    my_snprintf(buf, sizeof(buf),
                _("Warclient: Adding city %s to auto airlift queue."),
                ptile->city->name);
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
    need_city_for = aq;
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
                _("Warclient: You cannot airlift there (%d, %d); "
                  "no city exists."),
                TILE_XY(ptile));
    append_output_window(buf);
    return;
  }

  connection_do_buffer(&aconnection);
  if (need_city_for >= 0 && need_city_for < AIRLIFT_QUEUE_NUM) {
    do_airlift_for(need_city_for, ptile->city);
  } else if (airlift_queue_size(0) && airlift_queues[0].utype != U_LAST) {
    do_airlift_for(0, ptile->city);
  } else {
    int i;

    for (i = 1; i < AIRLIFT_QUEUE_NUM; i++) {
      do_airlift_for(i, ptile->city);
    }
  }
  need_city_for = -1;
  connection_do_unbuffer(&aconnection);
}
