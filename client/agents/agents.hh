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

#ifndef WC_CLIENT_AGENTS_AGENTS_H
#define WC_CLIENT_AGENTS_AGENTS_H

#include "shared.hh"            /* bool type */

#include "wc_types.hh"

/*
 * Besides callback for convenience client/agents/agents also
 * implements a "flattening" of the call stack i.e. to ensure that
 * every agent is only called once at any time.
 */

/* Don't use the very last level unless you know what you're doing */
#define LAST_AGENT_LEVEL 99

#define MAX_AGENT_NAME_LEN 10

enum callback_type {
  CB_NEW, CB_REMOVE, CB_CHANGE, CB_LAST
};

struct agent_s {
  char name[MAX_AGENT_NAME_LEN];
  int level;

  void (*turn_start_notify) (void);
  void (*city_callbacks[CB_LAST]) (int);
  void (*unit_callbacks[CB_LAST]) (int);
  void (*tile_callbacks[CB_LAST]) (tile_t *ptile);
};

void agents_init(void);
void agents_free(void);
void register_agent(const struct agent_s *agent);
bool agents_busy(void);

/* called from client/packhand.c */
void agents_disconnect(void);
void agents_processing_started(void);
void agents_processing_finished(void);
void agents_freeze_hint(void);
void agents_thaw_hint(void);
void agents_game_joined(void);
void agents_game_start(void);
void agents_before_new_turn(void);
void agents_start_turn(void);
void agents_new_turn(void);

void agents_unit_changed(unit_t *punit);
void agents_unit_new(unit_t *punit);
void agents_unit_remove(unit_t *punit);

void agents_city_changed(city_t *pcity);
void agents_city_new(city_t *pcity);
void agents_city_remove(city_t *pcity);

void agents_tile_changed(tile_t *ptile);
void agents_tile_new(tile_t *ptile);
void agents_tile_remove(tile_t *ptile);

/* called from agents */
void cause_a_city_changed_for_agent(const char *name_of_calling_agent,
                                    city_t *pcity);
void cause_a_unit_changed_for_agent(const char *name_of_calling_agent,
                                    unit_t *punit);
void wait_for_requests(const char *agent_name, int first_request_id,
                       int last_request_id);
#endif  /* WC_CLIENT_AGENTS_AGENTS_H */
