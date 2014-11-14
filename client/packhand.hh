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
#ifndef WC_CLIENT_PACKHAND_H
#define WC_CLIENT_PACKHAND_H

#include "map.hh"

#include "packhand_gen.hh"

void notify_about_incoming_packet(connection_t *pconn,
                                   int packet_type, int size);
void notify_about_outgoing_packet(connection_t *pconn,
                                  int packet_type, int size,
                                  int request_id);
void set_reports_thaw_request(int request_id);

void play_sound_for_event(enum event_type type);
void target_government_init(void);
void set_government_choice(int government);
void start_revolution(void);

#endif /* WC_CLIENT_PACKHAND_H */
