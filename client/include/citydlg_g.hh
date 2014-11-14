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
#ifndef WC_CLIENT_INCLUDE_CITYDLG_G_H
#define WC_CLIENT_INCLUDE_CITYDLG_G_H

#include "wc_types.hh"

#include "../citydlg_common.hh"

enum city_update {
  UPDATE_NOTHING         = 0,

  UPDATE_TITLE           = 1 << 0,
  UPDATE_INFORMATION     = 1 << 1,
  UPDATE_CITIZENS        = 1 << 2,
  UPDATE_MAP             = 1 << 3,
  UPDATE_BUILDING        = 1 << 4,
  UPDATE_IMPROVEMENTS    = 1 << 5,
  UPDATE_WORKLIST        = 1 << 6,
  UPDATE_HAPPINESS       = 1 << 7,
  UPDATE_CMA             = 1 << 8,
  UPDATE_TRADE           = 1 << 9,
  UPDATE_PRESENT_UNITS   = 1 << 10,
  UPDATE_SUPPORTED_UNITS = 1 << 11,

  UPDATE_ALL             = (1 << 12) - 1
};

void popup_city_dialog(city_t *pcity, bool make_modal);
void popdown_city_dialog(city_t *pcity);
void popdown_all_city_dialogs(void);
void refresh_city_dialog(city_t *pcity, enum city_update update);
void refresh_all_city_dialogs(enum city_update update);
void refresh_unit_city_dialogs(unit_t *punit);
bool city_dialog_is_open(city_t *pcity);

#endif  /* WC_CLIENT_INCLUDE_CITYDLG_G_H */
