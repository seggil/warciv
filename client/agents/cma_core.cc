/**********************************************************************
 Freeciv - Copyright (C) 2001 - R. Falke
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
// Citizen Management Agent(CMA)

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif

#include <assert.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "city.hh"
#include "dataio.hh"
#include "events.hh"
#include "wc_intl.hh"
#include "government.hh"
#include "hash.hh"
#include "log.hh"
#include "mem.hh"
#include "packets.hh"
#include "shared.hh"         /* for MIN() */
#include "support.hh"
#include "timing.hh"

#include "agents.hh"
#include "../attribute.hh"
#include "../include/chatline_g.hh"
#include "../include/citydlg_g.hh"
#include "../include/cityrep_g.hh"
#include "../civclient.hh"
#include "../climisc.hh"
#include "../clinet.hh"
#include "../include/messagewin_g.hh"
#include "../packhand.hh"

#include "cma_core.hh"

/*
 * The citizen management agent(CMA) is an agent. The CMA will subscribe
 * itself to all city events. So if a city changes the callback function
 * called. handle_city will be called from city_changed to update the
 * city_changed is given city. handle_city will call cma_query_result and
 * apply_result_on_server to update the server city state.
 */

/****************************************************************************
 defines, structs, globals, forward declarations
*****************************************************************************/

#define APPLY_RESULT_LOG_LEVEL                          LOG_DEBUG
#define HANDLE_CITY_LOG_LEVEL                           LOG_DEBUG
#define HANDLE_CITY_LOG_LEVEL2                          LOG_DEBUG
#define RESULTS_ARE_EQUAL_LOG_LEVEL                     LOG_DEBUG

#define SHOW_TIME_STATS                                 FALSE
#define SHOW_APPLY_RESULT_ON_SERVER_ERRORS              FALSE
#define ALWAYS_APPLY_AT_SERVER                          FALSE

#define SAVED_PARAMETER_SIZE                            29

/*
 * Misc statistic to analyze performance.
 */
static struct stats_s {
  struct timer *wall_timer;
  int apply_result_ignored;
  int apply_result_applied;
  int refresh_forced;
} stats;

#define my_city_map_iterate(pcity, cx, cy) {                     \
  city_map_checked_iterate(pcity->common.tile, cx, cy, _ptile) { \
    if(!is_city_center(cx, cy)) {

#define my_city_map_iterate_end    \
    }                              \
  } city_map_checked_iterate_end;  \
}

/****************************************************************************
 Returns TRUE iff the two results are equal. Both results have to be
 results for the given city.
*****************************************************************************/
static bool results_are_equal(city_t *pcity,
                             const struct cm_result *const result1,
                             const struct cm_result *const result2)
{
  int stat;

  //T(disorder);
  if (result1->disorder != result2->disorder) {
    freelog(RESULTS_ARE_EQUAL_LOG_LEVEL, "disorder");
    return FALSE;
  }

  //T(happy);
  if (result1->happy != result2->happy) {
    freelog(RESULTS_ARE_EQUAL_LOG_LEVEL, "happy");
    return FALSE;
  }
  //T(specialists[SP_ELVIS]);
  if (result1->specialists[SP_ELVIS] != result2->specialists[SP_ELVIS]) {
    freelog(RESULTS_ARE_EQUAL_LOG_LEVEL, "specialists[SP_ELVIS]");
    return FALSE;
  }
  //T(specialists[SP_SCIENTIST]);
  if (result1->specialists[SP_SCIENTIST] != result2->specialists[SP_SCIENTIST]) {
    freelog(RESULTS_ARE_EQUAL_LOG_LEVEL, "specialists[SP_SCIENTIST]");
    return FALSE;
  }
  //T(specialists[SP_TAXMAN]);
  if (result1->specialists[SP_TAXMAN] != result2->specialists[SP_TAXMAN]) {
    freelog(RESULTS_ARE_EQUAL_LOG_LEVEL, "specialists[SP_TAXMAN]");
    return FALSE;
  }

  for (stat = 0; stat < CM_NUM_STATS; stat++) {
    //T(surplus[stat]);
    if (result1->surplus[stat] != result2->surplus[stat]) {
      freelog(RESULTS_ARE_EQUAL_LOG_LEVEL, "surplus[stat]");
      return FALSE;
    }
  }

  my_city_map_iterate(pcity, x, y) {
    if (result1->worker_positions_used[x][y] !=
        result2->worker_positions_used[x][y]) {
      freelog(RESULTS_ARE_EQUAL_LOG_LEVEL, "worker_positions_used");
      return FALSE;
    }
  } my_city_map_iterate_end;

  return TRUE;
}

/****************************************************************************
 Copy the current city state (citizen assignment, production stats and
 happy state) in the given result.
*****************************************************************************/
static void get_current_as_result(city_t *pcity,
                                  struct cm_result *result)
{
  int worker = 0, specialist = 0;

  memset(result->worker_positions_used, 0,
         sizeof(result->worker_positions_used));

  my_city_map_iterate(pcity, x, y) {
    result->worker_positions_used[x][y] =
        (pcity->common.city_map[x][y] == C_TILE_WORKER);
    if (result->worker_positions_used[x][y]) {
      worker++;
    }
  } my_city_map_iterate_end;

  specialist_type_iterate(sp) {
    result->specialists[sp] = pcity->common.specialists[sp];
    specialist += pcity->common.specialists[sp];
  } specialist_type_iterate_end;

  assert(worker + specialist == pcity->common.pop_size);

  result->found_a_valid = TRUE;

  cm_copy_result_from_city(pcity, result);
}

/****************************************************************************
  Returns TRUE if the city is valid for CMA. Fills parameter if TRUE
  is returned. Parameter can be NULL.
*****************************************************************************/
static bool check_city(int city_id, struct cm_parameter *parameter)
{
  city_t *pcity = find_city_by_id(city_id);
  struct cm_parameter *dummy;

  if (!parameter) {
    dummy = (cm_parameter*)wc_malloc( sizeof(struct cm_parameter));
    if ( dummy == 0) {
      printf("in %s, malloc failled\n", __FUNCTION__);
      exit(1);
    }
    parameter = dummy;
  }

  if (!pcity
      || !cma_get_parameter(ATTR_CITY_CMA_PARAMETER, city_id, parameter)) {
    return FALSE;
  }

  if (city_owner(pcity) != get_player_ptr()) {
    cma_release_city(pcity);
    create_event(pcity->common.tile, E_CITY_CMA_RELEASE,
                 _("CMA: You lost control of %s. Detaching from city."),
                 pcity->common.name);
    return FALSE;
  }

  return TRUE;
}

/****************************************************************************
 Change the actual city setting to the given result. Returns TRUE iff
 the actual data matches the calculated one.
*****************************************************************************/
static bool apply_result_on_server(city_t *pcity,
                                   const struct cm_result *const result)
{
  int first_request_id = 0, last_request_id = 0;
  int i, sp;
  struct cm_result current_state;
  bool success;

  assert(result->found_a_valid);
  get_current_as_result(pcity, &current_state);

  if (results_are_equal(pcity, result, &current_state)
      && !ALWAYS_APPLY_AT_SERVER) {
    stats.apply_result_ignored++;
    return TRUE;
  }

  stats.apply_result_applied++;

  freelog(APPLY_RESULT_LOG_LEVEL, "apply_result(city='%s'(%d))",
          pcity->common.name, pcity->common.id);

  connection_do_buffer(&aconnection);

  /* Do checks */
  if (pcity->common.pop_size
      != (cm_count_worker(pcity, result)
          + cm_count_specialist(pcity, result)))
  {
    cm_print_city(pcity);
    cm_print_result(pcity, result);
    assert(0);
  }

  /* Remove all surplus workers */
  my_city_map_iterate(pcity, x, y) {
    if ((pcity->common.city_map[x][y] == C_TILE_WORKER) &&
        !result->worker_positions_used[x][y]) {
      freelog(APPLY_RESULT_LOG_LEVEL, "Removing worker at %d,%d.", x, y);
      last_request_id = city_toggle_worker(pcity, x, y);
      if (first_request_id == 0) {
        first_request_id = last_request_id;
      }
    }
  } my_city_map_iterate_end;

  /* Change the excess non-elvis specialists to elvises. */
  assert(SP_ELVIS == 0);
  for (sp = 1; sp < SP_COUNT; sp++) {
    for (i = 0; i < pcity->common.specialists[sp] - result->specialists[sp]; i++) {
      freelog(APPLY_RESULT_LOG_LEVEL, "Change specialist from %d to %d.",
              sp, SP_ELVIS);
      last_request_id = city_change_specialist(pcity, (Specialist_type_id)sp, SP_ELVIS);
      if (first_request_id == 0) {
        first_request_id = last_request_id;
      }
    }
  }

  /* now all surplus people are enterainers */

  /* Set workers */
  /* FIXME: This code assumes that any toggled worker will turn into an
   * elvis! */
  my_city_map_iterate(pcity, x, y) {
    if (result->worker_positions_used[x][y] &&
        pcity->common.city_map[x][y] != C_TILE_WORKER) {
      assert(pcity->common.city_map[x][y] == C_TILE_EMPTY);
      freelog(APPLY_RESULT_LOG_LEVEL, "Putting worker at %d,%d.", x, y);
      last_request_id = city_toggle_worker(pcity, x, y);
      if (first_request_id == 0) {
        first_request_id = last_request_id;
      }
    }
  } my_city_map_iterate_end;

  /* Set all specialists except SP_ELVIS (all the unchanged ones remain
   * as elvises). */
  assert(SP_ELVIS == 0);
  for (sp = 1; sp < SP_COUNT; sp++) {
    for (i = 0; i < result->specialists[sp] - pcity->common.specialists[sp]; i++) {
      freelog(APPLY_RESULT_LOG_LEVEL, "Changing specialist from %d to %d.",
              SP_ELVIS, sp);
      last_request_id = city_change_specialist(pcity, SP_ELVIS, (Specialist_type_id)sp);
      if (first_request_id == 0) {
        first_request_id = last_request_id;
      }
    }
  }

  if (last_request_id == 0 || ALWAYS_APPLY_AT_SERVER) {
      /*
       * If last_request is 0 no change request was send. But it also
       * means that the results are different or the results_are_equal
       * test at the start of the function would be true. So this
       * means that the client has other results for the same
       * allocation of citizen than the server. We just send a
       * PACKET_CITY_REFRESH to bring them in sync.
       */
    first_request_id = last_request_id =
        dsend_packet_city_refresh(&aconnection, pcity->common.id);
    stats.refresh_forced++;
  }
  reports_freeze_till(last_request_id);

  connection_do_unbuffer(&aconnection);

  if (last_request_id != 0) {
    int city_id = pcity->common.id;

    wait_for_requests("CMA", first_request_id, last_request_id);
    if (!check_city(city_id, NULL)) {
      return FALSE;
    }
  }

  /* Return. */
  get_current_as_result(pcity, &current_state);

  freelog(APPLY_RESULT_LOG_LEVEL, "apply_result: return");

  success = results_are_equal(pcity, result, &current_state);
  if (!success) {
    cm_clear_cache(pcity);

    if (SHOW_APPLY_RESULT_ON_SERVER_ERRORS) {
      freelog(LOG_NORMAL, "expected");
      cm_print_result(pcity, result);
      freelog(LOG_NORMAL, "got");
      cm_print_result(pcity, &current_state);
    }
  }
  return success;
}

/****************************************************************************
 Prints the data of the stats struct via freelog(LOG_NORMAL,...).
*****************************************************************************/
static void report_stats(void)
{
#if SHOW_TIME_STATS
  int total, per_mill;

  freelog(LOG_NORMAL, "CMA: overall=%fs queries=%d %fms / query",
          read_timer_seconds(stats.wall_timer), stats.queries,
          (1000.0 * read_timer_seconds(stats.wall_timer)) /
          ((double) stats.queries));
  total = stats.apply_result_ignored + stats.apply_result_applied;
  per_mill = (stats.apply_result_ignored * 1000) / (total ? total : 1);

  freelog(LOG_NORMAL,
          "CMA: apply_result: ignored=%2d.%d%% (%d) "
          "applied=%2d.%d%% (%d) total=%d",
          per_mill / 10, per_mill % 10, stats.apply_result_ignored,
          (1000 - per_mill) / 10, (1000 - per_mill) % 10,
          stats.apply_result_applied, total);
#endif
}

/****************************************************************************
...
*****************************************************************************/
static void release_city(int city_id)
{
  attr_city_set(ATTR_CITY_CMA_PARAMETER, city_id, 0, NULL);
}

/****************************************************************************
                           algorithmic functions
*****************************************************************************/

/****************************************************************************
 The given city has changed. handle_city ensures that either the city
 follows the set CMA goal or that the CMA detaches itself from the
 city.
*****************************************************************************/
static void handle_city(city_t *pcity)
{
  struct cm_result result;
  bool handled;
  int i;
  int city_id2;
  int city_id = city_id2 = pcity->common.id;
  struct cm_parameter parameter;


  freelog(HANDLE_CITY_LOG_LEVEL,
          "handle_city(city='%s'(%d) pos=(%d,%d) owner=%s)", pcity->common.name,
          pcity->common.id, TILE_XY(pcity->common.tile), city_owner(pcity)->name);

  freelog(HANDLE_CITY_LOG_LEVEL2, "START handle city='%s'(%d)",
          pcity->common.name, pcity->common.id);

  handled = FALSE;
  for (i = 0; i < 5; i++) {
    freelog(HANDLE_CITY_LOG_LEVEL2, "  try %d", i);

    if (!check_city(city_id, &parameter)) {
      handled = TRUE;
      break;
    }

    if ( city_id2 != city_id) {
      pcity = find_city_by_id(city_id);
      printf("in %s, using find_city_by_id\n", __FUNCTION__);
    }

    cm_query_result(pcity, &parameter, &result);
    if (!result.found_a_valid) {
      freelog(HANDLE_CITY_LOG_LEVEL2, "  no valid found result");

      cma_release_city(pcity);

      create_event(pcity->common.tile, E_CITY_CMA_RELEASE,
                   _("CMA: The agent can't fulfill the requirements "
                     "for %s. Passing back control."),
                   pcity->common.name);
      handled = TRUE;
      break;
    } else {
      if (!apply_result_on_server(pcity, &result)) {
        freelog(HANDLE_CITY_LOG_LEVEL2, "  doesn't cleanly apply");
        if (check_city(city_id, NULL) && i == 0) {
          create_event(pcity->common.tile, E_NOEVENT,
                       _("CMA: %s has changed and the calculated "
                         "result can't be applied. Will retry."),
                       pcity->common.name);
        }
      } else {
        freelog(HANDLE_CITY_LOG_LEVEL2, "  ok");
        /* Everything ok */
        handled = TRUE;
        break;
      }
    }
  }

  if ( city_id2 != city_id) {
    pcity = find_city_by_id(city_id);
    printf("in %s, using find_city_by_id\n", __FUNCTION__);
  }
  if (!handled) {
    assert(pcity != NULL);
    freelog(HANDLE_CITY_LOG_LEVEL2, "  not handled");

    create_event(pcity->common.tile, E_CITY_CMA_RELEASE,
                 _("CMA: %s has changed multiple times. This may be "
                   "an error in Warciv or bad luck. The CMA will detach "
                   "itself from the city now."),
                 pcity->common.name);

    cma_release_city(pcity);

    freelog(LOG_ERROR, "CMA: %s has changed multiple times due to "
            "an error in Warciv. Please send a savegame that can reproduce "
            "this bug at %s. Thank you.",
            pcity->common.name, BUG_URL);
  }

  freelog(HANDLE_CITY_LOG_LEVEL2, "END handle city=(%d)", city_id);
}

/****************************************************************************
 Callback for the agent interface.
*****************************************************************************/
static void city_changed(int city_id)
{
  city_t *pcity = find_city_by_id(city_id);

  if (pcity) {
    cm_clear_cache(pcity);
    handle_city(pcity);
  }
}

/****************************************************************************
 Callback for the agent interface.
*****************************************************************************/
static void city_remove(int city_id)
{
  release_city(city_id);
}

/****************************************************************************
 Callback for the agent interface.
*****************************************************************************/
static void new_turn(void)
{
  report_stats();
}

/*************************** public interface *******************************/
/****************************************************************************
...
*****************************************************************************/
void cma_init(void)
{
  struct agent_s self;
  struct timer *timer = stats.wall_timer;

  freelog(LOG_DEBUG, "sizeof(struct cm_result)=%d",
          (unsigned int) sizeof(struct cm_result));
  freelog(LOG_DEBUG, "sizeof(struct cm_parameter)=%d",
          (unsigned int) sizeof(struct cm_parameter));

  /* reset cache counters */
  memset(&stats, 0, sizeof(stats));
  stats.wall_timer = renew_timer(timer, TIMER_USER, TIMER_ACTIVE);

  memset(&self, 0, sizeof(self));
  strcpy(self.name, "CMA");
  self.level = 1;
  self.city_callbacks[CB_CHANGE] = city_changed;
  self.city_callbacks[CB_NEW] = city_changed;
  self.city_callbacks[CB_REMOVE] = city_remove;
  self.turn_start_notify = new_turn;
  register_agent(&self);
}

/****************************************************************************
...
*****************************************************************************/
bool cma_apply_result(city_t *pcity,
                     const struct cm_result *const result)
{
  assert(!cma_is_city_under_agent(pcity, NULL));
  if (result->found_a_valid) {
    return apply_result_on_server(pcity, result);
  } else
    return TRUE; /* ???????? */
}

/****************************************************************************
...
*****************************************************************************/
void cma_put_city_under_agent(city_t *pcity,
                              const struct cm_parameter *const parameter)
{
  freelog(LOG_DEBUG, "cma_put_city_under_agent(city='%s'(%d))",
          pcity->common.name, pcity->common.id);

  assert(city_owner(pcity) == get_player_ptr() || !get_player_ptr());

  cma_set_parameter(ATTR_CITY_CMA_PARAMETER, pcity->common.id, parameter);

  if (can_client_issue_orders()) {
    cause_a_city_changed_for_agent("CMA", pcity);
  }

  if (server_has_extglobalinfo && !client_is_observer()) {
    if (cma_is_city_under_agent(pcity, NULL)) {
      struct packet_city_manager_param packet;
      int i;

      packet.id = pcity->common.id;
      for (i = 0; i < CM_NUM_STATS; i++) {
        packet.minimal_surplus[i] = parameter->minimal_surplus[i];
        packet.factor[i] = parameter->factor[i];
      }
      packet.require_happy = parameter->require_happy;
      packet.allow_disorder = parameter->allow_disorder;
      packet.allow_specialists = parameter->allow_specialists;
      packet.happy_factor = parameter->happy_factor;

      send_packet_city_manager_param(&aconnection, &packet);
    } else {
      /* Failed */
      dsend_packet_city_no_manager_param(&aconnection, pcity->common.id);
    }
  }

  freelog(LOG_DEBUG, "cma_put_city_under_agent: return");
}

/****************************************************************************
...
*****************************************************************************/
void cma_release_city(city_t *pcity)
{
  release_city(pcity->common.id);
  refresh_city_dialog(pcity, UPDATE_CMA);
  city_report_dialog_update_city(pcity);
  if (server_has_extglobalinfo && !client_is_observer()) {
    dsend_packet_city_no_manager_param(&aconnection, pcity->common.id);
  }
}

/****************************************************************************
...
*****************************************************************************/
bool cma_is_city_under_agent(const city_t *pcity,
                             struct cm_parameter *parameter)
{
  struct cm_parameter my_parameter;

  if (!cma_get_parameter(ATTR_CITY_CMA_PARAMETER, pcity->common.id, &my_parameter)) {
    return FALSE;
  }

  if (parameter) {
    memcpy(parameter, &my_parameter, sizeof(struct cm_parameter));
  }
  return TRUE;
}

/**************************************************************************
  Get the parameter.

  Don't bother to cm_init_parameter, since we set all the fields anyway.
  But leave the comment here so we can find this place when searching
  for all the creators of a parameter.
**************************************************************************/
bool cma_get_parameter(enum attr_city attr, int city_id,
                       struct cm_parameter *parameter)
{
  size_t len;
  char buffer[SAVED_PARAMETER_SIZE];
  struct data_in din;
  int i, version, dummy;

  /* Changing this function is likely to break compatability with old
   * savegames that store these values. */

  len = attr_city_get(attr, city_id, sizeof(buffer), buffer);
  if (len == 0) {
    return FALSE;
  }
  assert(len == SAVED_PARAMETER_SIZE);

  dio_input_init(&din, buffer, len);

  dio_get_uint8(&din, &version);
  assert(version == 2);

  for (i = 0; i < CM_NUM_STATS; i++) {
    dio_get_sint16(&din, &parameter->minimal_surplus[i]);
    dio_get_sint16(&din, &parameter->factor[i]);
  }

  dio_get_sint16(&din, &parameter->happy_factor);
  dio_get_uint8(&din, &dummy); /* Dummy value; used to be factor_target. */
  dio_get_bool8(&din, &parameter->require_happy);
  /* These options are only for server-AI use. */
  parameter->allow_disorder = FALSE;
  parameter->allow_specialists = TRUE;

  return TRUE;
}

/**************************************************************************
 ...
**************************************************************************/
void cma_set_parameter(enum attr_city attr, int city_id,
                       const struct cm_parameter *parameter)
{
  char buffer[SAVED_PARAMETER_SIZE];
  struct data_out dout;
  int i;

  /* Changing this function is likely to break compatability with old
   * savegames that store these values. */

  dio_output_init(&dout, buffer, sizeof(buffer));

  dio_put_uint8(&dout, 2);

  for (i = 0; i < CM_NUM_STATS; i++) {
    dio_put_sint16(&dout, parameter->minimal_surplus[i]);
    dio_put_sint16(&dout, parameter->factor[i]);
  }

  dio_put_sint16(&dout, parameter->happy_factor);
  dio_put_uint8(&dout, 0); /* Dummy value; used to be factor_target. */
  dio_put_bool8(&dout, parameter->require_happy);

  assert(dio_output_used(&dout) == SAVED_PARAMETER_SIZE);

  attr_city_set(attr, city_id, SAVED_PARAMETER_SIZE, buffer);
}
