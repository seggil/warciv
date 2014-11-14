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
#ifndef WC_CLIENT_GUI_GRAPHICS_H
#define WC_CLIENT_GUI_GRAPHICS_H

#include <gtk/gtk.h>

#include "../include/graphics_g.hh"
#include "../mapview_common.hh"

struct Sprite
{
  GdkPixmap *pixmap;
  GdkPixmap *fogged;
  GdkBitmap *mask;
  int        has_mask;
  int        width;
  int        height;

  GdkPixbuf *pixbuf;
};

typedef struct Sprite wc_Sprite;

void create_overlay_unit(struct canvas *pcanvas, int i);

extern wc_Sprite *    intro_gfx_sprite;
extern wc_Sprite *    radar_gfx_sprite;
extern GdkCursor * goto_cursor;
extern GdkCursor * drop_cursor;
extern GdkCursor * nuke_cursor;
extern GdkCursor * patrol_cursor;
extern GdkCursor * source_cursor;
extern GdkCursor * dest_cursor;
extern GdkCursor * trade_cursor;

void gtk_draw_shadowed_string(GdkDrawable *drawable,
                              GdkGC *black_gc,
                              GdkGC *white_gc,
                              gint x, gint y, PangoLayout *layout);

wc_Sprite* sprite_scale(wc_Sprite *src, int new_w, int new_h);
void sprite_get_bounding_box(wc_Sprite * sprite, int *start_x,
                             int *start_y, int *end_x, int *end_y);
wc_Sprite *crop_blankspace(wc_Sprite *s);

GdkPixbuf *gdk_pixbuf_new_from_sprite(wc_Sprite *src);

/********************************************************************
 NOTE: the pixmap and mask of a sprite must not change after this
       function is called!
 ********************************************************************/
GdkPixbuf *sprite_get_pixbuf(wc_Sprite *sprite);

#endif  /* WC_CLIENT_GUI_GRAPHICS_H */
