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
#ifndef WC_CLIENT_INCLUDE_MAPCTRL_G_H
#define WC_CLIENT_INCLUDE_MAPCTRL_G_H

#include "shared.hh"           /* bool type */

#include "wc_types.hh"

#include "../mapctrl_common.hh"

void popup_newcity_dialog(unit_t *punit, char *suggestname);

void set_turn_done_button_state(bool state);

void create_line_at_mouse_pos(void);
void update_rect_at_mouse_pos(void);

#endif  /* WC_CLIENT_INCLUDE_MAPCTRL_G_H */
