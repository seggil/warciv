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

/**************************************************************************
   idex = ident index: a lookup table for quick mapping of unit and city
   id values to unit and city pointers.

   Method: use separate hash tables for each type.
   Means code duplication for city/unit cases, but simplicity advantages.
   Don't have to manage memory at all: store pointers to unit and city
   structs allocated elsewhere, and keys are pointers to id values inside
   the structs.

   Note id values should probably be unsigned int: here leave as plain int
   so can use pointers to pcity->common.id etc.

   On probable errors, print LOG_ERROR messages and persevere,
   unless IDEX_DIE set.
***************************************************************************/

#ifdef HAVE_CONFIG_H
# include "../config.hh"
#endif

#include <assert.h>

#include "city.hh"
#include "hash.hh"
#include "log.hh"
#include "unit.hh"

#include "idex.hh"


#ifndef IDEX_DIE
#define IDEX_DIE FALSE
#endif
#define LOG_IDEX_ERR (IDEX_DIE ? LOG_FATAL : LOG_ERROR)


/* "Global" data: */
static struct hash_table *idex_city_hash = NULL;
static struct hash_table *idex_unit_hash = NULL;

static struct hash_table *idex_city_name_hash = NULL;

/**************************************************************************
   Initialize.  Should call this at the start before use.
***************************************************************************/
void idex_init(void)
{
  idex_free();

  idex_city_hash = hash_new(hash_fval_ptr_int, hash_fcmp_ptr_int);
  idex_unit_hash = hash_new(hash_fval_ptr_int, hash_fcmp_ptr_int);
  idex_city_name_hash = hash_new(hash_fval_string2, hash_fcmp_string);
}

/**************************************************************************
   Free the hashs.
***************************************************************************/
void idex_free(void)
{
  if (idex_city_hash) {
    hash_free(idex_city_hash);
    idex_city_hash = NULL;
  }

  if (idex_unit_hash) {
    hash_free(idex_unit_hash);
    idex_unit_hash = NULL;
  }

  if (idex_city_name_hash) {
    hash_keys_iterate(idex_city_name_hash, key) {
      if (key) {
        free(key);
      }
    } hash_keys_iterate_end;
    hash_free(idex_city_name_hash);
    idex_city_name_hash = NULL;
  }
}

/**************************************************************************
   Register a city into idex, with current pcity->common.id.
   Call this when pcity created.
***************************************************************************/
void idex_register_city(city_t *pcity)
{
  city_t *old = (city_t *)
      hash_replace(idex_city_hash, &pcity->common.id, pcity);
  if (old) {
    /* error */
    freelog(LOG_IDEX_ERR, "IDEX: city collision: new %d %p %s, old %d %p %s",
            pcity->common.id, (void *) pcity, pcity->common.name,
            old->common.id, (void *) old, old->common.name);
    if (IDEX_DIE) {
      die("byebye");
    }
  }
}

/**************************************************************************
   Register a unit into idex, with current punit->id.
   Call this when punit created.
***************************************************************************/
void idex_register_unit(unit_t *punit)
{
  unit_t *old = (unit_t *)
      hash_replace(idex_unit_hash, &punit->id, punit);
  if (old) {
    /* error */
    freelog(LOG_IDEX_ERR, "IDEX: unit collision: new %d %p %s, old %d %p %s",
            punit->id, (void *) punit, unit_name(punit->type),
            old->id, (void *) old, unit_name(old->type));
    if (IDEX_DIE) {
      die("byebye");
    }
  }
}

/**************************************************************************
   Remove a city from idex, with current pcity->common.id.
   Call this when pcity deleted.
***************************************************************************/
void idex_unregister_city(city_t *pcity)
{
  city_t *old = (city_t *)
      hash_delete_entry(idex_city_hash, &pcity->common.id);
  if (!old) {
    /* error */
    freelog(LOG_IDEX_ERR, "IDEX: city unreg missing: %d %p %s",
            pcity->common.id, (void *) pcity, pcity->common.name);
    if (IDEX_DIE) {
      die("byebye");
    }
  } else if (old != pcity) {
    /* error */
    freelog(LOG_IDEX_ERR,
            "IDEX: city unreg mismatch: unreg %d %p %s, old %d %p %s",
            pcity->common.id, (void *) pcity, pcity->common.name,
            old->common.id, (void *) old, old->common.name);
    if (IDEX_DIE) {
      die("byebye");
    }
  }
}

/**************************************************************************
   Remove a unit from idex, with current punit->id.
   Call this when punit deleted.
***************************************************************************/
void idex_unregister_unit(unit_t *punit)
{
  unit_t *old = (unit_t *)
      hash_delete_entry(idex_unit_hash, &punit->id);
  if (!old) {
    /* error */
    freelog(LOG_IDEX_ERR, "IDEX: unit unreg missing: %d %p %s",
            punit->id, (void *) punit, unit_name(punit->type));
    if (IDEX_DIE) {
      die("byebye");
    }
  } else if (old != punit) {
    /* error */
    freelog(LOG_IDEX_ERR,
            "IDEX: unit unreg mismatch: unreg %d %p %s, old %d %p %s",
            punit->id, (void *) punit, unit_name(punit->type),
            old->id, (void *) old, unit_name(old->type));
    if (IDEX_DIE) {
      die("byebye");
    }
  }
}

/**************************************************************************
   Lookup city with given id.
   Returns NULL if the city is not registered (which is not an error).
***************************************************************************/
city_t *idex_lookup_city(int id)
{
  return (city_t *) hash_lookup_data(idex_city_hash, &id);
}

/**************************************************************************
   Lookup unit with given id.
   Returns NULL if the unit is not registered (which is not an error).
***************************************************************************/
unit_t *idex_lookup_unit(int id)
{
  return (unit_t *) hash_lookup_data(idex_unit_hash, &id);
}

/**************************************************************************
  ...
***************************************************************************/
city_t *idex_lookup_city_by_name(const char *name)
{
  int id;

  if (!name || !name[0]) {
    return NULL;
  }

  id = PTR_TO_INT(hash_lookup_data(idex_city_name_hash, name));
  if (!id) {
    return NULL;
  }

  return idex_lookup_city(id);
}

/**************************************************************************
  ...
***************************************************************************/
void idex_register_city_name(city_t *pcity)
{
  void *old;
  if (!pcity || !pcity->common.name || !pcity->common.name[0]) {
    return;
  }

  freelog(LOG_DEBUG,
          "idex_register_city_name pcity=%p pcity->common.id=%d pcity->common.name=\"%s\"",
          pcity, pcity->common.id, pcity->common.name);
  if (hash_delete_entry_full(idex_city_name_hash, pcity->common.name, &old)) {
    free(old);
  }
  hash_insert(idex_city_name_hash, mystrdup(pcity->common.name),
              INT_TO_PTR(pcity->common.id));
}

/**************************************************************************
  ...
***************************************************************************/
void idex_unregister_city_name(city_t *pcity)
{
  void *old;

  if (!pcity || !pcity->common.name || !pcity->common.name[0]) {
    return;
  }

  freelog(LOG_DEBUG,
          "idex_unregister_city_name pcity=%p pcity->common.id=%d pcity->common.name=\"%s\"",
          pcity, pcity->common.id, pcity->common.name);
  if (hash_delete_entry_full(idex_city_name_hash, pcity->common.name, &old)) {
    free(old);
  }
}
