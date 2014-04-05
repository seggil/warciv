/**********************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Poject
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef WC_CLIENT_TEXT_H
#define WC_CLIENT_TEXT_H

#include "wc_types.h"

struct player_spaceship;

/****************************************************************************
  These functions return static strings with generally useful text.
****************************************************************************/
const char *popup_info_text(tile_t *ptile);
const char *concat_tile_activity_text(tile_t *ptile);
const char *get_nearest_city_text(city_t *pcity, int sq_dist);
const char *unit_description(struct unit *punit);
const char *science_dialog_text(void);
const char *get_info_label_text(void);
const char *get_bulb_tooltip(void);
const char *get_global_warming_tooltip(void);
const char *get_nuclear_winter_tooltip(void);
const char *get_government_tooltip(void);
const char *get_unit_info_label_text1(struct unit *punit);
const char *get_unit_info_label_text2(struct unit *punit);
const char *get_spaceship_descr(struct player_spaceship *pship);
const char *get_timeout_label_text(void);
const char *format_duration(int duration);
const char *get_ping_time_text(player_t *pplayer);
const char *get_report_title(const char *report_name);
const char *get_happiness_buildings(const city_t *pcity);
const char *get_happiness_wonders(const city_t *pcity);
const char *get_ping_time_text(player_t *pplayer);

#endif /* WC_CLIENT_TEXT_H */
