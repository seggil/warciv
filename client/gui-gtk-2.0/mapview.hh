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
#ifndef WC_CLIENT_GUI_MAPVIEW_H
#define WC_CLIENT_GUI_MAPVIEW_H

#include <gtk/gtk.h>

#include "gtkpixcomm.hh"

#include "../citydlg_common.hh"
#include "../include/mapview_g.hh"
#include "../mapview_common.hh"

#include "graphics.hh"

GdkPixmap *get_thumb_pixmap(int onoff);

gboolean overview_canvas_expose(GtkWidget *w, GdkEventExpose *ev, gpointer data);
gboolean map_canvas_expose(GtkWidget *w, GdkEventExpose *ev, gpointer data);
gboolean map_canvas_configure(GtkWidget *w, GdkEventConfigure *ev,
                              gpointer data);

void put_unit_gpixmap(unit_t *punit, GtkPixcomm *p);

void put_unit_gpixmap_city_overlays(unit_t *punit, GtkPixcomm *p);

void scrollbar_jump_callback(GtkAdjustment *adj, gpointer hscrollbar);
void update_map_canvas_scrollbars_size(void);

void update_hover_cursor(void);

#endif  /* WC_CLIENT_GUI_MAPVIEW_H */
