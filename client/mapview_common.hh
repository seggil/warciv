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

#ifndef WC_CLIENT_MAPVIEW_COMMON_H
#define WC_CLIENT_MAPVIEW_COMMON_H

#include "shared.hh"         /* bool type */

#include "wc_types.hh"
#include "map.hh"

#include "include/colors_g.hh"

#include "tilespec.hh"

enum map_update_type {
  MAP_UPDATE_NORMAL, /* Mark the zone to be re-drawn and copied to screen later */
  MAP_UPDATE_DRAW,   /* Redraw the zone and will be copied to screen later */
  MAP_UPDATE_WRITE   /* Redraw and copy to screen */
};

enum draw_elements {
  DRAW_SPRITES    = 1 << 0, /* Draw the tiles + cities + units */
  DRAW_DECORATION = 1 << 1  /* Draw goto path, chat links, etc... */
};

struct canvas_store;          /* opaque type, real type is gui-dep */

struct mapview_canvas_s {
  int gui_x0;
  int gui_y0;
  int width;
  int height;                 /* Size in pixels. */
  int tile_width;
  int tile_height;            /* Size in tiles. Rounded up. */
  int store_width;
  int store_height;
  bool can_do_cached_drawing; /* TRUE if cached drawing is possible. */
  struct canvas *store;
  struct canvas *tmp_store;
};

/* Holds all information about the overview aka minimap. */
struct overview_s {
  /* The following fields are controlled by mapview_common.c. */
  int map_x0;
  int map_y0;
  int width;
  int height;            /* Size in pixels. */
  struct canvas *store;
};

extern struct mapview_canvas_s mapview_canvas;
extern struct overview_s       overview;

/* HACK: Callers can set this to FALSE to disable sliding.  It should be
 * reenabled afterwards. */
extern bool can_slide;

#define BORDER_WIDTH 2
#define GOTO_WIDTH 2

/*
 * Iterate over all map tiles that intersect with the given GUI rectangle.
 * The order of iteration is guaranteed to satisfy the painter's algorithm.
 *
 * gui_x0, gui_y0: gives the GUI origin of the rectangle.
 * width, height: gives the GUI width and height of the rectangle.  These
 * values may be negative.
 *
 * map_x, map_y: variables that will give the current tile of iteration.
 * These coordinates are unnormalized.
 *
 * draw: A variable that tells which parts of the tiles overlap with the
 * GUI rectangle.  Only applies in iso-view.
 *
 * The classic-view iteration is pretty simple.  Documentation of the
 * iso-view iteration is at
 * http://bugs.freeciv.org/Ticket/Attachment/51374/37363/isogrid.png.
 */
#define gui_rect_iterate(gui_x0, gui_y0, width, height, ptile)              \
{                                                                           \
  int _gui_x0 = (gui_x0), _gui_y0 = (gui_y0);                               \
  int _width = (width), _height = (height);                                 \
                                                                            \
  if (_width < 0) {                                                         \
    _gui_x0 += _width;                                                      \
    _width = -_width;                                                       \
  }                                                                         \
  if (_height < 0) {                                                        \
    _gui_y0 += _height;                                                     \
    _height = -_height;                                                     \
  }                                                                         \
  if (_width > 0 && _height > 0) {                                          \
    int W = (is_isometric ? (NORMAL_TILE_WIDTH / 2) : NORMAL_TILE_WIDTH);   \
    int H = (is_isometric ? (NORMAL_TILE_HEIGHT / 2) : NORMAL_TILE_HEIGHT); \
    int GRI_x0 = DIVIDE(_gui_x0, W);                                        \
    int GRI_y0 = DIVIDE(_gui_y0, H);                                        \
    int GRI_x1 = DIVIDE(_gui_x0 + _width + W - 1, W);                       \
    int GRI_y1 = DIVIDE(_gui_y0 + _height + H - 1, H);                      \
    int GRI_itr, GRI_x_itr, GRI_y_itr;                                      \
    int _map_x, _map_y;                                                     \
    int count;                                                              \
    tile_t *ptile;                                                          \
                                                                            \
    if (is_isometric) {                                                     \
      /* Tiles to the left/above overlap with us. */                        \
      GRI_x0--;                                                             \
      GRI_y0--;                                                             \
    }                                                                       \
    count = (GRI_x1 - GRI_x0) * (GRI_y1 - GRI_y0);                          \
    for (GRI_itr = 0; GRI_itr < count; GRI_itr++) {                         \
      GRI_x_itr = GRI_x0 + (GRI_itr % (GRI_x1 - GRI_x0));                   \
      GRI_y_itr = GRI_y0 + (GRI_itr / (GRI_x1 - GRI_x0));                   \
      if (is_isometric) {                                                   \
        if ((GRI_x_itr + GRI_y_itr) % 2 != 0) {                             \
          continue;                                                         \
        }                                                                   \
        _map_x = (GRI_x_itr + GRI_y_itr) / 2;                               \
        _map_y = (GRI_y_itr - GRI_x_itr) / 2;                               \
      } else {                                                              \
        _map_x = GRI_x_itr;                                                 \
        _map_y = GRI_y_itr;                                                 \
      }                                                                     \
      ptile = map_pos_to_tile(_map_x, _map_y);                              \
      if (!ptile) {                                                         \
        continue;                                                           \
      }

#define gui_rect_iterate_end                                                \
    }                                                                       \
  }                                                                         \
}

enum color_std get_grid_color(tile_t *ptile, enum direction8 dir);

void map_to_gui_vector(int *gui_dx, int *gui_dy, int map_dx, int map_dy);
bool tile_to_canvas_pos(int *canvas_x, int *canvas_y, tile_t *ptile);
tile_t *canvas_pos_to_tile(int canvas_x, int canvas_y);
tile_t *canvas_pos_to_nearest_tile(int canvas_x, int canvas_y);

void get_mapview_scroll_window(int *xmin, int *ymin,
                               int *xmax, int *ymax,
                               int *xsize, int *ysize);
void get_mapview_scroll_step(int *xstep, int *ystep);
void get_mapview_scroll_pos(int *scroll_x, int *scroll_y);
void set_mapview_scroll_pos(int scroll_x, int scroll_y);

void set_mapview_origin(int gui_x0, int gui_y0);
tile_t *get_center_tile_mapcanvas(void);
void center_tile_mapcanvas(tile_t *ptile);

bool tile_visible_mapcanvas(tile_t *ptile);
bool tile_visible_and_not_on_border_mapcanvas(tile_t *ptile);

void put_unit(unit_t *punit,
              struct canvas *pcanvas, int canvas_x, int canvas_y);
void put_city(city_t *pcity,
              struct canvas *pcanvas, int canvas_x, int canvas_y);
void put_terrain(tile_t *ptile,
                 struct canvas *pcanvas, int canvas_x, int canvas_y);

void put_city_tile_output(city_t *pcity, int city_x, int city_y,
                          struct canvas *pcanvas,
                          int canvas_x, int canvas_y);
void put_unit_city_overlays(unit_t *punit,
                            struct canvas *pcanvas,
                            int canvas_x, int canvas_y);
void toggle_city_color(city_t *pcity);
void toggle_unit_color(unit_t *punit);
void put_red_frame_tile(struct canvas *pcanvas,
                        int canvas_x, int canvas_y);

void put_nuke_mushroom_pixmaps(tile_t *ptile);

void put_one_tile(struct canvas *pcanvas, tile_t *ptile,
                  int canvas_x, int canvas_y, bool citymode);
void put_one_tile_iso(struct canvas *pcanvas, tile_t *ptile,
                      int canvas_x, int canvas_y, bool citymode);
void tile_draw_grid(struct canvas *pcanvas, tile_t *ptile,
                    int canvas_x, int canvas_y, bool citymode);

void draw_map_canvas(int canvas_x, int canvas_y,
                     int width, int height, enum draw_elements elements);
void update_map_canvas(int canvas_x, int canvas_y,
                       int width, int height, enum map_update_type type);
void update_map_canvas_visible(enum map_update_type type);
void refresh_tile_mapcanvas(tile_t *ptile, enum map_update_type type);
void update_city_description(city_t *pcity);

void flush_dirty_overview(void);

void show_city_descriptions(int canvas_x, int canvas_y,
                            int width, int height);
bool show_unit_orders(unit_t *punit);

void draw_segment(tile_t *ptile, enum direction8 dir);
void undraw_segment(tile_t *ptile, enum direction8 dir);

void decrease_unit_hp_smooth(unit_t *punit0, int hp0,
                             unit_t *punit1, int hp1);
void move_unit_map_canvas(unit_t *punit,
                          tile_t *ptile, int dx, int dy);

city_t *find_city_or_settler_near_tile(tile_t *ptile,
                                       unit_t **punit);
city_t *find_city_near_tile(tile_t *ptile);

void get_city_mapview_production(city_t *pcity,
                                 char *buf, size_t buf_len);
void get_city_mapview_name_and_growth(city_t *pcity,
                                      char *name_buffer,
                                      size_t name_buffer_len,
                                      char *growth_buffer,
                                      size_t growth_buffer_len,
                                      enum color_std *grwoth_color);
void get_city_mapview_traderoutes(city_t *pcity,
                                  char *traderoutes_buffer,
                                  size_t traderoutes_len,
                                  enum color_std *traderoutes_color);

void map_to_overview_pos(int *overview_x, int *overview_y,
                         int map_x, int map_y);
void overview_to_map_pos(int *map_x, int *map_y,
                         int overview_x, int overview_y);

void refresh_overview_canvas(void);
void overview_update_tile(tile_t *ptile);
void set_overview_dimensions(int width, int height);

bool map_canvas_resized(int width, int height);
void init_mapcanvas_and_overview(void);

void draw_traderoutes(void);
void update_trade_route_line(struct trade_route *ptr);

#endif /* WC_CLIENT_MAPVIEW_COMMON_H */
