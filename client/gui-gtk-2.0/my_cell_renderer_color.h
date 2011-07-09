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
#ifndef FC__MY_CELL_RENDERER_COLOR_H
#define FC__MY_CELL_RENDERER_COLOR_H

#include <gtk/gtk.h>

#define TYPE_MY_CELL_RENDERER_COLOR\
  (my_cell_renderer_color_get_type())
#define MY_CELL_RENDERER_COLOR(obj)\
  (G_TYPE_CHECK_INSTANCE_CAST((obj),  TYPE_MY_CELL_RENDERER_COLOR, MyCellRendererColor))
#define MY_CELL_RENDERER_COLOR_CLASS(klass)\
  (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_MY_CELL_RENDERER_COLOR, MyCellRendererColorClass))
#define MY_IS_CELL_RENDERER_COLOR(obj)\
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MY_CELL_RENDERER_COLOR))
#define MY_IS_CELL_RENDERER_COLOR_CLASS(klass)\
  (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_MY_CELL_RENDERER_COLOR))
#define MY_CELL_RENDERER_COLOR_GET_CLASS(obj)\
  (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_MY_CELL_RENDERER_COLOR, MyCellRendererColorClass))

typedef struct _MyCellRendererColor
{
  GtkCellRenderer parent;
  GdkColor *color;
  gint xsize, ysize;
} MyCellRendererColor;


typedef struct _MyCellRendererColorClass
{
  GtkCellRendererClass  parent_class;
} MyCellRendererColorClass;


GType my_cell_renderer_color_get_type (void);

GtkCellRenderer *my_cell_renderer_color_new (void);


#endif /* FC__MY_CELL_RENDERER_COLOR_H */
