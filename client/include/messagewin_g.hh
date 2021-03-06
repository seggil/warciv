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
#ifndef WC_CLIENT_INCLUDE_MESSAGEWIN_G_H
#define WC_CLIENT_INCLUDE_MESSAGEWIN_G_H

#include "packets.hh"

#include "../messagewin_common.hh"

void popup_meswin_dialog(void);
bool is_meswin_open(void);
void real_update_meswin_dialog(void);

#endif  /* WC_CLIENT_INCLUDE_MESSAGEWIN_G_H */
