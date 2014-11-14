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
#ifndef WC_COMMON_IDEX_H
#define WC_COMMON_IDEX_H

/**************************************************************************
   idex = ident index: a lookup table for quick mapping of unit and city
   id values to unit and city pointers.
***************************************************************************/

#include "wc_types.hh"

void idex_init(void);
void idex_free(void);

void idex_register_city(city_t *pcity);
void idex_register_unit(unit_t *punit);
void idex_register_city_name(city_t *pcity);

void idex_unregister_city(city_t *pcity);
void idex_unregister_unit(unit_t *punit);
void idex_unregister_city_name(city_t *pcity);

city_t *idex_lookup_city(int id);
city_t *idex_lookup_city_by_name(const char *name);
unit_t *idex_lookup_unit(int id);

#endif  /* WC_COMMON_IDEX_H */
