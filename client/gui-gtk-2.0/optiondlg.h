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
#ifndef FC__OPTIONDLG_H
#define FC__OPTIONDLG_H

#include <gtk/gtk.h>

void popup_option_dialog(void);
const GtkWidget *get_option_dialog_shell(void);

void fullscreen_mode_callback(struct client_option *poption);
void map_scrollbars_callback(struct client_option *poption);
void mapview_redraw_callback(struct client_option *poption);
void split_message_window_callback(struct client_option *poption);

#endif  /* FC__OPTIONDLG_H */
