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
#ifndef WC_CLIENT_GUI_CONNECTDLG_H
#define WC_CLIENT_GUI_CONNECTDLG_H

#include <gtk/gtk.h>

#include "shared.h"

#include "../include/connectdlg_g.h"

GtkWidget *create_file_selection(const char *title, bool is_save);

#endif  /* WC_CLIENT_GUI_CONNECTDLG_H */
