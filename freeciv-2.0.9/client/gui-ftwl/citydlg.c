/********************************************************************** 
 Freeciv - Copyright (C) 2004 - The Freeciv Project
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

#include "log.h"
#include "city.h"

#include "citydlg.h"

/**************************************************************************
  Pop up (or bring to the front) a dialog for the given city.  It may or
  may not be modal.
**************************************************************************/
void popup_city_dialog(struct city *pcity, bool make_modal)
{
  freelog(LOG_NORMAL, "popup_city_dialog('%s')", pcity->name);
  /* PORTME */
}

/**************************************************************************
  Close the dialog for the given city.
**************************************************************************/
void popdown_city_dialog(struct city *pcity)
{
  /* PORTME */
}

/**************************************************************************
  Close the dialogs for all cities.
**************************************************************************/
void popdown_all_city_dialogs(void)
{
  /* PORTME */
}

/**************************************************************************
  Refresh (update) all data for the given city's dialog.
**************************************************************************/
void refresh_city_dialog(struct city *pcity)
{
  /* PORTME */
}

/**************************************************************************
  Update city dialogs when the given unit's status changes.  This
  typically means updating both the unit's home city (if any) and the
  city in which it is present (if any).
**************************************************************************/
void refresh_unit_city_dialogs(struct unit *punit)
{
  /* PORTME */
#if 0
  /* Demo code */
  struct city *pcity_sup, *pcity_pre;
  struct city_dialog *pdialog;

  pcity_sup = find_city_by_id(punit->homecity);
  pcity_pre = map_get_city(punit->tile);

  if (pcity_sup && (pdialog = get_city_dialog(pcity_sup))) {
    city_dialog_update_supported_units(pdialog);
  }

  if (pcity_pre && (pdialog = get_city_dialog(pcity_pre))) {
    city_dialog_update_present_units(pdialog);
  }
#endif
}

/**************************************************************************
  Return whether the dialog for the given city is open.
**************************************************************************/
bool city_dialog_is_open(struct city *pcity)
{
  /* PORTME */
  return FALSE;
}
