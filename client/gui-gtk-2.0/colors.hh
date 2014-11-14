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
#ifndef WC_CLIENT_GUI_COLORS_H
#define WC_CLIENT_GUI_COLORS_H

#include <gtk/gtk.h>

#include "../include/colors_g.h"

extern GdkColor *colors_standard[COLOR_STD_LAST];
GdkColor *color_from_str(const char *str);

#endif  /* WC_CLIENT_GUI_COLORS_H */
