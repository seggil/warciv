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
#ifndef WC_CLIENT_CHATLINE_COMMON_H
#define WC_CLIENT_CHATLINE_COMMON_H

#include "shared.h"             /* bool type */

#include "wc_types.h"           /* city_t, tile and unit */

void send_chat(const char *message);

void chatline_common_init(void);

void append_output_window(const char *astring);
void append_output_window_full(const char *astring, int conn_id);

void output_window_freeze(void);
void output_window_thaw(void);
void output_window_force_thaw(void);
bool is_output_window_frozen(void);

int insert_city_link(char *buf, size_t buflen, city_t *pcity);
int insert_tile_link(char *buf, size_t buflen, tile_t *ptile);
int insert_unit_link(char *buf, size_t buflen, struct unit *punit);

#endif  /* WC_CLIENT_CHATLINE_COMMON_H */
