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
#ifndef WC_CLIENT_INCLUDE_DIALOGS_G_H
#define WC_CLIENT_INCLUDE_DIALOGS_G_H

#include "shared.h"             /* bool type */

#include "wc_types.h"
#include "nation.h"             /* Nation_Type_id */
#include "terrain.h"            /* enum tile_special_type */

struct packet_nations_selected_info;

void popup_notify_goto_dialog(const char *headline, const char *lines,
                              tile_t *ptile);
void popup_notify_dialog(const char *caption, const char *headline,
                         const char *lines);

void popup_races_dialog(void);
void popdown_races_dialog(void);

void popup_unit_select_dialog(tile_t *ptile);

void races_toggles_set_sensitive(bool *nations_used);

void popup_caravan_dialog(unit_t *punit,
                          city_t *phomecity, city_t *pdestcity);
bool caravan_dialog_is_open(void);
void popup_diplomat_dialog(unit_t *punit, tile_t *ptile);
bool diplomat_dialog_is_open(void);
void popup_incite_dialog(city_t *pcity, int cost);
void popup_bribe_dialog(unit_t *punit, int cost);
void popup_sabotage_dialog(city_t *pcity);
void popup_pillage_dialog(unit_t *punit,
                          enum tile_special_type may_pillage);

void popup_trade_planning_calculation_info(void);
void popdown_trade_planning_calculation_info(void);
void update_trade_planning_calculation_info(void);

void request_trade_planning_calculation_resume(void);
void remove_trade_planning_calculation_resume_request(void);

void popup_disband_unit(const unit_t *punit);
void popup_disband_units_focus(void);

void popdown_all_game_dialogs(void);
void popdown_all_game_dialogs_except_option_dialog(void);

#endif  /* WC_CLIENT_INCLUDE_DIALOGS_G_H */
