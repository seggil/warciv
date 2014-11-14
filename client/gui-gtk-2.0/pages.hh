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
#ifndef WC_CLIENT_GUI_PAGES_H
#define WC_CLIENT_GUI_PAGES_H

#include <gtk/gtk.h>

#include "../include/pages_g.hh"

extern GtkWidget *start_message_area;
extern GtkListStore *conn_model;

GtkWidget *create_main_page(void);
GtkWidget *create_start_page(void);
GtkWidget *create_scenario_page(void);
GtkWidget *create_load_page(void);
GtkWidget *create_network_page(void);
GtkWidget *create_nation_page(void);

GtkWidget *create_statusbar(void);
void append_network_statusbar(const char *text);
void popup_save_dialog(void);

#endif  /* WC_CLIENT_GUI_PAGES_H */
