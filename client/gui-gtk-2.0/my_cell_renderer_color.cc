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

#ifdef HAVE_CONFIG_H
# include "../../config.hh"
#endif

#include "my_cell_renderer_color.hh"

/* Adapted from http://scentric.net/tutorial/ */

static void my_cell_renderer_color_init(MyCellRendererColor *cell);
static void my_cell_renderer_color_class_init(MyCellRendererColorClass *klass);
static void my_cell_renderer_color_get_property(GObject *object,
                                                guint param_id,
                                                GValue *value,
                                                GParamSpec *pspec);

static void my_cell_renderer_color_set_property(GObject *object,
                                                guint param_id,
                                                const GValue *value,
                                                GParamSpec *pspec);

static void my_cell_renderer_color_finalize(GObject *gobject);


static void my_cell_renderer_color_get_size(GtkCellRenderer *cell,
                                            GtkWidget *widget,
                                            GdkRectangle *cell_area,
                                            gint *x_offset,
                                            gint *y_offset,
                                            gint *width, gint *height);

static void my_cell_renderer_color_render(GtkCellRenderer *cell,
                                          GdkWindow *window,
                                          GtkWidget *widget,
                                          GdkRectangle *background_area,
                                          GdkRectangle *cell_area,
                                          GdkRectangle *expose_area,
                                          GtkCellRendererState flags);
enum {
  PROP_COLOR = 1,
  PROP_XSIZE,
  PROP_YSIZE,
};

static gpointer parent_class;

/**************************************************************************
...
**************************************************************************/
GType my_cell_renderer_color_get_type(void)
{
  static GType cell_color_type = 0;

  if (cell_color_type){
    return cell_color_type;
  }

  {
    static const GTypeInfo info = {
      sizeof (MyCellRendererColorClass),
      NULL,                     /* base_init */
      NULL,                     /* base_finalize */
      (GClassInitFunc) my_cell_renderer_color_class_init,
      NULL,                     /* class_finalize */
      NULL,                     /* class_data */
      sizeof (MyCellRendererColor),
      0,                        /* n_preallocs */
      (GInstanceInitFunc) my_cell_renderer_color_init,
    };

    /* Derive from GtkCellRenderer */
    cell_color_type = g_type_register_static(GTK_TYPE_CELL_RENDERER,
                                             "MyCellRendererColor",
                                             &info, (GTypeFlags)0);
  }

  return cell_color_type;
}

/**************************************************************************
...
**************************************************************************/
static void my_cell_renderer_color_init(MyCellRendererColor *cell)
{
  GTK_CELL_RENDERER (cell)->mode = GTK_CELL_RENDERER_MODE_INERT;
  GTK_CELL_RENDERER (cell)->xpad = 2;
  GTK_CELL_RENDERER (cell)->ypad = 2;
  cell->color = NULL;
  cell->xsize = 24;
  cell->ysize = 16;
}

/**************************************************************************
...
**************************************************************************/
static void my_cell_renderer_color_class_init(MyCellRendererColorClass *klass)
{
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS(klass);
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  gpointer pspec;

  parent_class = g_type_class_peek_parent(klass);
  object_class->finalize = my_cell_renderer_color_finalize;

  /* Hook up functions to set and get our
   *  custom cell renderer properties */
  object_class->get_property = my_cell_renderer_color_get_property;
  object_class->set_property = my_cell_renderer_color_set_property;

  /* Override the two crucial functions that are the heart
   *  of a cell renderer in the parent class */
  cell_class->get_size = my_cell_renderer_color_get_size;
  cell_class->render = my_cell_renderer_color_render;

  /* Install our very own properties */
  pspec = g_param_spec_pointer("color",
                               "Color",
                               "The color to display (a pointer to GdkColor)",
                               (GParamFlags)G_PARAM_READWRITE);
  g_object_class_install_property(object_class, PROP_COLOR, (GParamSpec*)pspec);

  pspec = g_param_spec_int("xsize", "Xsize",
                           "Size in the x direction",
                           0, 1000, 24, (GParamFlags)G_PARAM_READWRITE);
  g_object_class_install_property(object_class, PROP_XSIZE, (GParamSpec*)pspec);

  pspec = g_param_spec_int("ysize", "Ysize",
                           "Size in the y direction",
                           0, 1000, 16, (GParamFlags)G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_YSIZE, (GParamSpec*)pspec);
}

/**************************************************************************
...
**************************************************************************/
static void my_cell_renderer_color_finalize(GObject *object)
{
  MyCellRendererColor *cell = MY_CELL_RENDERER_COLOR(object);

  if (cell->color) {
    gdk_color_free (cell->color);
  }

  (*G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/**************************************************************************
...
**************************************************************************/
static void my_cell_renderer_color_get_property(GObject *object,
                                                guint param_id,
                                                GValue *value,
                                                GParamSpec *psec)
{
  MyCellRendererColor *cell = MY_CELL_RENDERER_COLOR(object);

  switch (param_id) {
  case PROP_COLOR:
    g_value_set_pointer (value, cell->color);
    break;
  case PROP_XSIZE:
    g_value_set_int (value, cell->xsize);
    break;
  case PROP_YSIZE:
    g_value_set_int (value, cell->ysize);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, psec);
    break;
  }
}

/**************************************************************************
...
**************************************************************************/
static void my_cell_renderer_color_set_property(GObject *object,
                                                guint param_id,
                                                const GValue *value,
                                                GParamSpec *pspec)
{
  MyCellRendererColor *cell = MY_CELL_RENDERER_COLOR(object);
  GdkColor *color;

  switch (param_id) {
  case PROP_COLOR:
    color = (GdkColor*)g_value_get_pointer (value);
    if (color != NULL) {
      cell->color = gdk_color_copy (color);
    } else {
      cell->color = NULL;
    }
    break;
  case PROP_XSIZE:
    cell->xsize = g_value_get_int (value);
    break;
  case PROP_YSIZE:
    cell->ysize = g_value_get_int (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    break;
  }
}

/**************************************************************************
...
**************************************************************************/
GtkCellRenderer *my_cell_renderer_color_new(void)
{
  return (GtkCellRenderer*)g_object_new(TYPE_MY_CELL_RENDERER_COLOR, NULL);
}

/**************************************************************************
...
**************************************************************************/
static void my_cell_renderer_color_get_size(GtkCellRenderer *cell,
                                            GtkWidget *widget,
                                            GdkRectangle *cell_area,
                                            gint *x_offset,
                                            gint *y_offset,
                                            gint *width,
                                            gint *height)
{
  MyCellRendererColor *mycell = MY_CELL_RENDERER_COLOR(cell);

  gint calc_width;
  gint calc_height;

  calc_width = (gint) cell->xpad * 2 + mycell->xsize;
  calc_height = (gint) cell->ypad * 2 + mycell->ysize;

  if (width) {
    *width = calc_width;
  }

  if (height) {
    *height = calc_height;
  }
  if (cell_area) {
    if (x_offset) {
      *x_offset = cell->xalign *(cell_area->width - calc_width);
      *x_offset = MAX (*x_offset, 0);
    }

    if (y_offset) {
      *y_offset = cell->yalign *(cell_area->height - calc_height);
      *y_offset = MAX (*y_offset, 0);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
static void my_cell_renderer_color_render(GtkCellRenderer *cellrend,
                                          GdkWindow *window,
                                          GtkWidget *widget,
                                          GdkRectangle *background_area,
                                          GdkRectangle *cell_area,
                                          GdkRectangle *expose_area,
                                          GtkCellRendererState flags)
{
  MyCellRendererColor *cell = MY_CELL_RENDERER_COLOR(cellrend);
  gint width, height;
  gint x_offset = 0;
  gint y_offset = 0 ;
  GdkGC *gc;
  GdkColor black;

  if (!cell->color) {
    return;
  }

  my_cell_renderer_color_get_size(cellrend, widget, cell_area,
                                  &x_offset, &y_offset, &width, &height);

  width -= cellrend->xpad * 2;
  height -= cellrend->ypad * 2;

  gc = gdk_gc_new(window);
  gdk_gc_set_rgb_fg_color(gc, cell->color);
  gdk_draw_rectangle(window, gc, TRUE,
                     cell_area->x + x_offset + cellrend->xpad,
                     cell_area->y + y_offset + cellrend->ypad,
                     width, height);

  black.red = black.green = black.blue = 0;
  gdk_gc_set_rgb_fg_color (gc, &black);
  gdk_draw_rectangle(window, gc, FALSE,
                     cell_area->x + x_offset + cellrend->xpad,
                     cell_area->y + y_offset + cellrend->ypad,
                     width, height);
  g_object_unref(gc);
}
