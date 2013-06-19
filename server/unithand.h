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
#ifndef WC_SERVER_UNITHAND_H
#define WC_SERVER_UNITHAND_H

#include "packets.h"
#include "unit.h"

#include "hand_gen.h"

bool handle_unit_move_request(struct unit *punit, tile_t *ptile,
                             bool igzoc, bool move_diplomat_city);
void handle_unit_activity_request(struct unit *punit,
                                  enum unit_activity new_activity);

#endif  /* WC_SERVER_UNITHAND_H */
