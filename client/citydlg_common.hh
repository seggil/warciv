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

#ifndef WC_CLIENT_CITYDLG_COMMON_H
#define WC_CLIENT_CITYDLG_COMMON_H

#include <stddef.h>             /* size_t */

#include "shared.hh"            /* bool type */

#include "city.hh"              /* Specialist_type_id */
#include "wc_types.hh"

struct canvas;
enum citizen_type_type {
  CITIZEN_SPECIALIST,
  CITIZEN_CONTENT,
  CITIZEN_HAPPY,
  CITIZEN_UNHAPPY,
  CITIZEN_ANGRY,
  CITIZEN_LAST
};

struct citizen_type {
  enum citizen_type_type type;
  Specialist_type_id spec_type;
};

int get_citydlg_canvas_width(void);
int get_citydlg_canvas_height(void);
void generate_citydlg_dimensions(void);

bool city_to_canvas_pos(int *canvas_x, int *canvas_y,
                        int city_x, int city_y);
bool canvas_to_city_pos(int *city_x, int *city_y,
                        int canvas_x, int canvas_y);
void city_dialog_redraw_map(city_t *pcity,
                            struct canvas *pcanvas);

void get_city_dialog_production(city_t *pcity,
                                char *buffer, size_t buffer_len);
void get_city_dialog_production_full(char *buffer, size_t buffer_len,
                                     int id, bool is_unit,
                                     city_t *pcity);
void get_city_dialog_production_row(char *buf[], size_t column_size, int id,
                                    bool is_unit, city_t *pcity);

void get_city_citizen_types(city_t *pcity, int index,
                            struct citizen_type *citizens);
void city_rotate_specialist(city_t *pcity, int citizen_index);

void activate_all_units(tile_t *ptile);

int city_change_production(city_t *pcity, bool is_unit, int build_id);
int city_set_worklist(city_t *pcity, struct worklist *pworklist);
bool city_queue_insert(city_t *pcity, int position,
                       bool item_is_unit, int item_id);
void city_get_queue(city_t *pcity, struct worklist *pqueue);
bool city_set_queue(city_t *pcity, struct worklist *pqueue);
bool city_can_buy(const city_t *pcity);
int city_sell_improvement(city_t *pcity, Impr_Type_id sell_id);
int city_buy_production(city_t *pcity);
int city_change_specialist(city_t *pcity, Specialist_type_id from,
                           Specialist_type_id to);
int city_toggle_worker(city_t *pcity, int city_x, int city_y);
int city_rename(city_t *pcity, const char *name);

void refresh_city_dialog_maps(tile_t *ptile);

#endif /* WC_CLIENT_CITYDLG_COMMON_H */
