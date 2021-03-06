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
#  include "../../config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "gtkpixcomm.h"

#include "game.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"
#include "unit.h"
#include "version.h"

#include "../climisc.h"
#include "colors.h"
#include "gui_main.h"
#include "../include/mapview_g.h"
#include "../options.h"
#include "../tilespec.h"

#include "graphics.h"

#include "../include/goto_cursor.xbm"
#include "../include/goto_cursor_mask.xbm"
#include "../include/drop_cursor.xbm"
#include "../include/drop_cursor_mask.xbm"
#include "../include/nuke_cursor.xbm"
#include "../include/nuke_cursor_mask.xbm"
#include "../include/patrol_cursor.xbm"
#include "../include/patrol_cursor_mask.xbm"
#include "../include/source_cursor.xbm"
#include "../include/source_cursor_mask.xbm"
#include "../include/dest_cursor.xbm"
#include "../include/dest_cursor_mask.xbm"
#include "../include/trade_cursor.xbm"
#include "../include/trade_cursor_mask.xbm"

wc_Sprite *intro_gfx_sprite;
wc_Sprite *radar_gfx_sprite;

GdkCursor *goto_cursor;
GdkCursor *drop_cursor;
GdkCursor *nuke_cursor;
GdkCursor *patrol_cursor;
GdkCursor *source_cursor;
GdkCursor *dest_cursor;
GdkCursor *trade_cursor;

/***************************************************************************
...
***************************************************************************/
bool isometric_view_supported(void)
{
  return TRUE;
}

/***************************************************************************
...
***************************************************************************/
bool overhead_view_supported(void)
{
  return TRUE;
}

/***************************************************************************
...
***************************************************************************/
#define COLOR_MOTTO_FACE_R    0x2D
#define COLOR_MOTTO_FACE_G    0x71
#define COLOR_MOTTO_FACE_B    0xE3

/**************************************************************************
...
**************************************************************************/
void gtk_draw_shadowed_string(GdkDrawable *drawable,
                              GdkGC *black_gc,
                              GdkGC *white_gc,
                              gint x, gint y, PangoLayout *layout)
{
  gdk_draw_layout(drawable, black_gc, x + 1, y + 1, layout);
  gdk_draw_layout(drawable, white_gc, x, y, layout);
}

/**************************************************************************
...
**************************************************************************/
void load_intro_gfx(void)
{
  int tot, y;
  char s[64];
  GdkColor face;
  GdkGC *face_gc;
  PangoContext *context;
  PangoLayout *layout;
  PangoRectangle rect;
  GdkScreen *screen;

  screen = gdk_screen_get_default();

  context = gdk_pango_context_get_for_screen(screen);
  layout = pango_layout_new(context);
  pango_layout_set_font_description(layout, main_font);

  /* get colors */
  face.red  = COLOR_MOTTO_FACE_R<<8;
  face.green= COLOR_MOTTO_FACE_G<<8;
  face.blue = COLOR_MOTTO_FACE_B<<8;
  face_gc = gdk_gc_new(root_window);

  /* Main graphic */
  intro_gfx_sprite = load_gfxfile(main_intro_filename);
  tot=intro_gfx_sprite->width;

  pango_layout_set_text(layout, warciv_motto(), -1);
  pango_layout_get_pixel_extents(layout, &rect, NULL);

  y = intro_gfx_sprite->height-45;

  gdk_gc_set_rgb_fg_color(face_gc, &face);
  gdk_draw_layout(intro_gfx_sprite->pixmap, face_gc,
                  (tot-rect.width) / 2, y, layout);
  g_object_unref(face_gc);

  /* Minimap graphic */
  radar_gfx_sprite = load_gfxfile(minimap_intro_filename);
  tot=radar_gfx_sprite->width;

  my_snprintf(s, sizeof(s), "%d.%d.%d%s",
              MAJOR_VERSION, MINOR_VERSION,
              PATCH_VERSION, VERSION_LABEL);
  pango_layout_set_text(layout, s, -1);
  pango_layout_get_pixel_extents(layout, &rect, NULL);

  y = radar_gfx_sprite->height - (rect.height + 6);

  gtk_draw_shadowed_string(radar_gfx_sprite->pixmap,
                           toplevel->style->black_gc,
                           toplevel->style->white_gc,
                           (tot - rect.width) / 2, y,
                           layout);

  pango_layout_set_text(layout, word_version(), -1);
  pango_layout_get_pixel_extents(layout, &rect, NULL);
  y-=rect.height+3;

  gtk_draw_shadowed_string(radar_gfx_sprite->pixmap,
                           toplevel->style->black_gc,
                           toplevel->style->white_gc,
                           (tot - rect.width) / 2, y,
                           layout);

  /* done */
  g_object_unref(layout);
  g_object_unref(context);
  return;
}

/****************************************************************************
  Create a new sprite by cropping and taking only the given portion of
  the image.
  return: allocated Sprite
****************************************************************************/
struct Sprite *crop_sprite(struct Sprite *source,
                           int x, int y,
                           int width, int height,
                           struct Sprite *mask,
                           int mask_offset_x, int mask_offset_y)
{
  GdkPixmap *mypixmap;
  GdkPixmap *mymask = NULL;
  wc_Sprite *mysprite;

  mypixmap = gdk_pixmap_new(root_window, width, height, -1);

  gdk_draw_drawable(mypixmap, civ_gc, source->pixmap, x, y, 0, 0,
                    width, height);

  if (source->has_mask) {
    mymask = gdk_pixmap_new(NULL, width, height, 1);
    gdk_draw_rectangle(mymask, mask_bg_gc, TRUE, 0, 0, -1, -1);

    gdk_draw_drawable(mymask, mask_fg_gc, source->mask,
                      x, y, 0, 0, width, height);
  }

  if (mask) {
    if (mymask) {
      gdk_gc_set_function(mask_fg_gc, GDK_AND);
      gdk_draw_drawable(mymask, mask_fg_gc, mask->mask,
                        x - mask_offset_x, y - mask_offset_y,
                        0, 0, width, height);
      gdk_gc_set_function(mask_fg_gc, GDK_OR);
    } else {
      mymask = gdk_pixmap_new(NULL, width, height, 1);
      gdk_draw_rectangle(mymask, mask_bg_gc, TRUE, 0, 0, -1, -1);

      gdk_draw_drawable(mymask, mask_fg_gc, source->mask,
                        x, y, 0, 0, width, height);
    }
  }

  mysprite = wc_malloc(sizeof(wc_Sprite));

  mysprite->pixmap= mypixmap;
  mysprite->fogged = NULL;
  mysprite->has_mask = (mymask != NULL);
  mysprite->mask = mymask;

  mysprite->width= width;
  mysprite->height= height;

  mysprite->pixbuf= NULL;

  return mysprite;

}

/****************************************************************************
  Get the width and height of the sprite.
****************************************************************************/
void get_sprite_dimensions(struct Sprite *sprite, int *width, int *height)
{
  *width = sprite->width;
  *height = sprite->height;
}

/***************************************************************************
...
***************************************************************************/
void load_cursors(void)
{
  GdkBitmap *pixmap, *mask;
  GdkColor *white, *black;

  white = colors_standard[COLOR_STD_WHITE];
  black = colors_standard[COLOR_STD_BLACK];

  /* goto */
  pixmap = gdk_bitmap_create_from_data(root_window, goto_cursor_bits,
                                       goto_cursor_width,
                                       goto_cursor_height);
  mask   = gdk_bitmap_create_from_data(root_window, goto_cursor_mask_bits,
                                       goto_cursor_mask_width,
                                       goto_cursor_mask_height);
  goto_cursor = gdk_cursor_new_from_pixmap(pixmap, mask,
                                           white, black,
                                           goto_cursor_x_hot,
                                           goto_cursor_y_hot);
  g_object_unref(pixmap);
  g_object_unref(mask);

  /* drop */
  pixmap = gdk_bitmap_create_from_data(root_window, drop_cursor_bits,
                                       drop_cursor_width,
                                       drop_cursor_height);
  mask   = gdk_bitmap_create_from_data(root_window, drop_cursor_mask_bits,
                                       drop_cursor_mask_width,
                                       drop_cursor_mask_height);
  drop_cursor = gdk_cursor_new_from_pixmap(pixmap, mask,
                                           white, black,
                                           drop_cursor_x_hot,
                                           drop_cursor_y_hot);
  g_object_unref(pixmap);
  g_object_unref(mask);

  /* nuke */
  pixmap = gdk_bitmap_create_from_data(root_window, nuke_cursor_bits,
                                       nuke_cursor_width,
                                       nuke_cursor_height);
  mask   = gdk_bitmap_create_from_data(root_window, nuke_cursor_mask_bits,
                                       nuke_cursor_mask_width,
                                       nuke_cursor_mask_height);
  nuke_cursor = gdk_cursor_new_from_pixmap(pixmap, mask,
                                           white, black,
                                           nuke_cursor_x_hot, nuke_cursor_y_hot);
  g_object_unref(pixmap);
  g_object_unref(mask);

  /* patrol */
  pixmap = gdk_bitmap_create_from_data(root_window, patrol_cursor_bits,
                                       patrol_cursor_width,
                                       patrol_cursor_height);
  mask   = gdk_bitmap_create_from_data(root_window, patrol_cursor_mask_bits,
                                       patrol_cursor_mask_width,
                                       patrol_cursor_mask_height);
  patrol_cursor = gdk_cursor_new_from_pixmap(pixmap, mask,
                                             white, black,
                                             patrol_cursor_x_hot, patrol_cursor_y_hot);
  g_object_unref(pixmap);
  g_object_unref(mask);

  /* airlift source */
  pixmap = gdk_bitmap_create_from_data(root_window, source_cursor_bits,
                                       source_cursor_width,
                                       source_cursor_height);
  mask   = gdk_bitmap_create_from_data(root_window, source_cursor_mask_bits,
                                       source_cursor_mask_width,
                                       source_cursor_mask_height);
  source_cursor = gdk_cursor_new_from_pixmap(pixmap, mask,
                                             white, black,
                                             source_cursor_x_hot,
                                             source_cursor_y_hot);
  g_object_unref(pixmap);
  g_object_unref(mask);

  /* airlift destination */
  pixmap = gdk_bitmap_create_from_data(root_window, dest_cursor_bits,
                                       dest_cursor_width,
                                       dest_cursor_height);
  mask   = gdk_bitmap_create_from_data(root_window, dest_cursor_mask_bits,
                                       dest_cursor_mask_width,
                                       dest_cursor_mask_height);
  dest_cursor = gdk_cursor_new_from_pixmap(pixmap, mask,
                                           white, black,
                                           dest_cursor_x_hot,
                                           dest_cursor_y_hot);
  g_object_unref(pixmap);
  g_object_unref(mask);

  /*trade */
  pixmap = gdk_bitmap_create_from_data(root_window, trade_cursor_bits,
                                       trade_cursor_width,
                                       trade_cursor_height);
  mask   = gdk_bitmap_create_from_data(root_window, trade_cursor_mask_bits,
                                       trade_cursor_mask_width,
                                       trade_cursor_mask_height);
  trade_cursor = gdk_cursor_new_from_pixmap(pixmap, mask,
                                            white, black,
                                            trade_cursor_x_hot,
                                            trade_cursor_y_hot);
  g_object_unref(pixmap);
  g_object_unref(mask);
}


#if 0
/***************************************************************************
...
***************************************************************************/
void dtor_sprite( wc_Sprite *mysprite )
{
  free_sprite( mysprite );
  return;
}
#endif

/***************************************************************************
 Returns the filename extensions the client supports
 Order is important.
***************************************************************************/
const char **gfx_fileextensions(void)
{
  static const char *ext[] = {
    "png",
    "xpm",
    NULL
  };

  return ext;
}

/***************************************************************************
...
***************************************************************************/
struct Sprite *load_gfxfile(const char *filename)
{
  GdkPixbuf  *im;
  GdkPixbuf  *pixbuf3;
  wc_Sprite     *mysprite;
  int         w;
  int         h;
  GError      *pixbuf_error = NULL;
  GdkColormap *colormap;
  GdkScreen   *screen;

  screen = gdk_screen_get_default();
  colormap = gdk_screen_get_default_colormap(screen);

  freelog(LOG_DEBUG, "load_gfxfile filename=\"%s\"", filename);

  im = gdk_pixbuf_new_from_file(filename,&pixbuf_error);
  if (!im) {
    freelog(LOG_FATAL, "Failed reading graphics file: %s \n"
            "Error : %s\n", filename,pixbuf_error->message);
    exit(EXIT_FAILURE);
  }

  mysprite = wc_calloc(1, sizeof(struct Sprite));

  w = gdk_pixbuf_get_width(im);
  h = gdk_pixbuf_get_height(im);
  if ( ! gdk_pixbuf_get_has_alpha(im) ) {
    pixbuf3 = gdk_pixbuf_add_alpha(im, FALSE, 0,0,0);
    gdk_pixbuf_render_pixmap_and_mask_for_colormap(pixbuf3, colormap,
                                                   &mysprite->pixmap,
                                                   &mysprite->mask, 1);
    g_object_unref(pixbuf3);
  }
  else
  {
    gdk_pixbuf_render_pixmap_and_mask_for_colormap(im, colormap,
                                                   &mysprite->pixmap,
                                                   &mysprite->mask, 1);
  }
  if (mysprite->mask != NULL) {
    mysprite->has_mask  = true;
    //printf("%s %s %d a mask for  %s\n", __FILE__, __FUNCTION__, __LINE__, filename);
  } else {
    mysprite->has_mask  = false;
    //printf("%s %s %d no mask for %s\n", __FILE__, __FUNCTION__, __LINE__, filename);
  }
  mysprite->width     = w;
  mysprite->height    = h;

  mysprite->pixbuf    = NULL;
  mysprite->fogged    = NULL;

  g_object_unref(im);

  return mysprite;
}

/***************************************************************************
   Deletes a sprite.  These things can use a lot of memory.
***************************************************************************/
void free_sprite(wc_Sprite *Sprite3)
{
  freelog(LOG_DEBUG, "free_sprite s=%p", Sprite3);

  if (Sprite3->pixmap) {
    g_object_unref(Sprite3->pixmap);
    /*Sprite3->pixmap = NULL;*/
  }
  if (Sprite3->mask) {
    g_object_unref(Sprite3->mask);
    /*Sprite3->mask = NULL;*/
  }
  if (Sprite3->pixbuf) {
    g_object_unref(Sprite3->pixbuf);
  }
  if (Sprite3->fogged) {
    g_object_unref(Sprite3->fogged);
  }
  free(Sprite3);
}

/***************************************************************************
 ...
***************************************************************************/
void create_overlay_unit(struct canvas *pcanvas, int i)
{
  enum color_std bg_color;
  int x1, x2, y1, y2;
  int width, height;
  struct unit_type *type = get_unit_type(i);

  freelog(LOG_DEBUG, "create_overlay_unit pcanvas=%p id=%d --> type=%p",
          pcanvas, i, type);

  sprite_get_bounding_box(type->sprite, &x1, &y1, &x2, &y2);
  if (pcanvas->type == CANVAS_PIXBUF) {
    width = gdk_pixbuf_get_width(pcanvas->v.pixbuf);
    height = gdk_pixbuf_get_height(pcanvas->v.pixbuf);
    gdk_pixbuf_fill(pcanvas->v.pixbuf, 0x00000000);
  } else {
    if (pcanvas->type == CANVAS_PIXCOMM) {
      gtk_pixcomm_clear(pcanvas->v.pixcomm);
    }

    /* Guess */
    width = UNIT_TILE_WIDTH;
    height = UNIT_TILE_HEIGHT;
  }

  if (solid_unit_icon_bg) {
    /* Give tile a background color, based on the type of unit */
    switch (type->move_type) {
    case LAND_MOVING: bg_color = COLOR_STD_GROUND; break;
    case SEA_MOVING:  bg_color = COLOR_STD_OCEAN;  break;
    case HELI_MOVING: bg_color = COLOR_STD_YELLOW; break;
    case AIR_MOVING:  bg_color = COLOR_STD_CYAN;   break;
    default:      bg_color = COLOR_STD_BLACK;  break;
    }
    canvas_put_rectangle(pcanvas, bg_color, 0, 0, width, height);
  }

  /* Finally, put a picture of the unit in the tile */
  canvas_put_sprite(pcanvas, 0, 0, type->sprite,
                    (x2 + x1 - width) / 2, (y1 + y2 - height) / 2,
                    UNIT_TILE_WIDTH - (x2 + x1 - width) / 2,
                    UNIT_TILE_HEIGHT - (y1 + y2 - height) / 2);
}

/***************************************************************************
  This function is so that packhand.c can be gui-independent, and
  not have to deal with Sprites itself.
***************************************************************************/
void free_intro_radar_sprites(void)
{
  if (intro_gfx_sprite) {
    free_sprite(intro_gfx_sprite);
    intro_gfx_sprite=NULL;
  }
  if (radar_gfx_sprite) {
    free_sprite(radar_gfx_sprite);
    radar_gfx_sprite=NULL;
  }
}

/***************************************************************************
  Scales a sprite. If the sprite contains a mask, the mask is scaled
  as as well.
***************************************************************************/
wc_Sprite* sprite_scale(wc_Sprite *src, int new_w, int new_h)
{
  GdkPixbuf *original, *im;
  wc_Sprite    *mysprite;
  GdkColormap *colormap;
  GdkScreen *screen;

  screen = gdk_screen_get_default();
  colormap = gdk_screen_get_default_colormap(screen);

  original = sprite_get_pixbuf(src);
  im = gdk_pixbuf_scale_simple(original, new_w, new_h, GDK_INTERP_BILINEAR);

  mysprite = wc_malloc(sizeof(struct Sprite));

  gdk_pixbuf_render_pixmap_and_mask_for_colormap(im, colormap,
                                                 &mysprite->pixmap,
                                                 &mysprite->mask, 1);

  mysprite->has_mask  = (mysprite->mask != NULL);
  mysprite->width     = new_w;
  mysprite->height    = new_h;

  mysprite->pixbuf    = NULL;
  mysprite->fogged = NULL;
  g_object_unref(im);

  return mysprite;
}

/***************************************************************************
 Method returns the bounding box of a sprite. It assumes a rectangular
 object/mask. The bounding box contains the border (pixel which have
 unset pixel as neighbours) pixel.
***************************************************************************/
void sprite_get_bounding_box(wc_Sprite * sprite, int *start_x,
                             int *start_y, int *end_x, int *end_y)
{
  GdkImage *mask_image;
  int i, j;

  //printf("has_mask=%d, mask=%p;", sprite->has_mask, sprite->mask);
  if (!sprite->has_mask || !sprite->mask) {
    *start_x = 0;
    *start_y = 0;
    *end_x = sprite->width - 1;
    *end_y = sprite->height - 1;
    //printf("\n");
    return;
  }

  //if (0 >= sprite->width || 0 >= sprite->height) {
  //printf("gdk_drawable_get_image %u %u; ", sprite->width, sprite->height);
  //}
  if ( GDK_IS_DRAWABLE(sprite->mask) ) {
    mask_image =
      gdk_drawable_get_image(sprite->mask, 0, 0, sprite->width, sprite->height);
  } else {
    /* something wrong with setting mask field */
    printf("sprite_get_bounding_box sprite->mask is not drawable\n" );
    *start_x = 0;
    *start_y = 0;
    *end_x = sprite->width - 1;
    *end_y = sprite->height - 1;
    return;
  }

  /* parses mask image for the first column that contains a visible pixel */
  *start_x = -1;
  for (i = 0; i < sprite->width && *start_x == -1; i++) {
    for (j = 0; j < sprite->height; j++) {
      if (gdk_image_get_pixel(mask_image, i, j) != 0) {
        *start_x = i;
        break;
      }
    }
  }

  /* parses mask image for the last column that contains a visible pixel */
  *end_x = -1;
  if ( *start_x != -1 ) {  // the mask is all transparent
    for (i = sprite->width - 1; i >= *start_x && *end_x == -1; i--) {
      for (j = 0; j < sprite->height; j++) {
        if (gdk_image_get_pixel(mask_image, i, j) != 0) {
          *end_x = i;
          break;
        }
      }
    }

    /* parses mask image for the first row that contains a visible pixel */
    *start_y = -1;
    for (i = 0; i < sprite->height && *start_y == -1; i++) {
      for (j = *start_x; j <= *end_x; j++) {
        if (gdk_image_get_pixel(mask_image, j, i) != 0) {
          *start_y = i;
          break;
        }
      }
    }

    /* parses mask image for the last row that contains a visible pixel */
    *end_y = -1;
    for (i = sprite->height - 1; i >= *end_y && *end_y == -1; i--) {
      for (j = *start_x; j <= *end_x; j++) {
        if (gdk_image_get_pixel(mask_image, j, i) != 0) {
          *end_y = i;
          break;
        }
      }
    }
  }
  else {
    *start_y = -1;
    *end_y = -1;
  }
  g_object_unref(mask_image);
}

/*********************************************************************
 Crops all blankspace from a sprite (insofar as is possible as a rectangle)
*********************************************************************/
wc_Sprite *crop_blankspace(wc_Sprite *s)
{
  int x1, y1, x2, y2;

  sprite_get_bounding_box(s, &x1, &y1, &x2, &y2);
  if ( x1 == -1 || y1 == -1) {
    return s;
  }
  else {
    /* return value need to be freed */
    return crop_sprite(s, x1, y1, x2 - x1 + 1, y2 - y1 + 1, NULL, -1, -1);
  }
}

/*********************************************************************
 Converts a sprite to a GdkPixbuf.
*********************************************************************/
GdkPixbuf *gdk_pixbuf_new_from_sprite(wc_Sprite *src)
{
  GdkPixbuf *dst;
  int w, h;

  freelog(LOG_DEBUG, "gdk_pixbuf_new_from_sprite wc_Sprite src=%p", src);

  w = src->width;
  h = src->height;

  /* convert pixmap */
  dst = gdk_pixbuf_new(GDK_COLORSPACE_RGB, src->mask != NULL, 8, w, h);
  gdk_pixbuf_get_from_drawable(dst, src->pixmap, NULL, 0, 0, 0, 0, w, h);

  /* convert mask */
  if (src->mask) {
    GdkImage *img;
    int x, y, rowstride;
    guchar *pixels;

    img = gdk_drawable_get_image(src->mask, 0, 0, w, h);

    pixels = gdk_pixbuf_get_pixels(dst);
    rowstride = gdk_pixbuf_get_rowstride(dst);

    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
        guchar *pixel = pixels + y * rowstride + x * 4 + 3;

        //printf("gdk_image_get_pixel in gdk_pixbuf_new_from_sprite; ");
        if (gdk_image_get_pixel(img, x, y)) {
          *pixel = 255;
        } else {
          *pixel = 0;
        }
        //printf("end\n");
      }
    }
    g_object_unref(img);
  }

  return dst;
}

/********************************************************************
 NOTE: the pixmap and mask of a sprite must not change after this
       function is called!
********************************************************************/
GdkPixbuf *sprite_get_pixbuf(wc_Sprite *sprite)
{
  freelog(LOG_DEBUG, "sprite_get_pixbuf sprite=%p", sprite);

  if (!sprite) {
    return NULL;
  }

  if (!sprite->pixbuf) {
    sprite->pixbuf = gdk_pixbuf_new_from_sprite(sprite);
  }
  return sprite->pixbuf;
}
