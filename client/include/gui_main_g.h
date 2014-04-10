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
#ifndef WC_CLIENT_INCLUDE_GUI_MAIN_G_H
#define WC_CLIENT_INCLUDE_GUI_MAIN_G_H

#include "shared.h"             /* bool type */

#include "wc_types.h"

void connection_init(void);
void ui_main(int argc, char *argv[]);
void update_connection_list_dialog(void);
void sound_bell(void);
void add_net_input(int);
void remove_net_input(void);

enum input_flags {
  INPUT_READ = 0x1,
  INPUT_WRITE = 0x2,
  INPUT_ERROR = 0x4,
  INPUT_CLOSED = 0x8,
};
typedef bool (*input_ready_callback_t) (int sock, int flags, void *data);
int add_net_input_callback (int sock, int flags, input_ready_callback_t cb,
                      void *data, data_free_func_t datafree);
void remove_net_input_callback (int id);

typedef bool (*timer_callback_t) (void *data);
int add_timer_callback (int millisecond_interval, timer_callback_t, void *data);
void remove_timer_callback (int id);

void set_unit_icon(int idx, unit_t *punit);
void set_unit_icons_more_arrow(bool onoff);

void update_info_table(void);

extern const char *client_string;

#endif  /* WC_CLIENT_INCLUDE_GUI_MAIN_G_H */
