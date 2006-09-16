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
#ifndef FC__HASH_H
#define FC__HASH_H

/**************************************************************************
   An improved general-purpose hash table implementation.
   See comments in hash.c.
***************************************************************************/

#include "shared.h"		/* bool type */

/* Function typedefs: */
typedef unsigned int (*hash_val_fn_t)(const void *, unsigned int);
typedef int (*hash_cmp_fn_t)(const void *, const void *);

/* Supplied functions (matching above typedefs) appropriate for
   keys being normal nul-terminated strings: */
unsigned int hash_fval_string(const void *vkey, unsigned int num_buckets);
unsigned int hash_fval_string2(const void *vkey, unsigned int num_buckets);
int hash_fcmp_string(const void *vkey1, const void *vkey2);
/* Compare strings without regard to case */
int hash_fcmp_string_ci(const void *vkey1, const void *vkey2);

/* Appropriate for int values: */
unsigned int hash_fval_int (const void *vkey, unsigned int num_buckets);
int hash_fcmp_int (const void *vkey1, const void *vkey2);

unsigned int hash_fval_ptr_int (const void *vkey, unsigned int num_buckets);
int hash_fcmp_ptr_int (const void *vkey1, const void *vkey2);

unsigned int hash_fval_uint16_t (const void *vkey, unsigned int num_buckets);
int hash_fcmp_uint16_t (const void *vkey1, const void *vkey2);
unsigned int hash_fval_uint32_t (const void *vkey, unsigned int num_buckets);
int hash_fcmp_uint32_t (const void *vkey1, const void *vkey2);

/* Appropriate for void pointers or casted longs, used as keys
   directly instead of by reference. */
unsigned int hash_fval_keyval(const void *vkey, unsigned int num_buckets);
int hash_fcmp_keyval(const void *vkey1, const void *vkey2);

/* General functions: */
struct hash_table *hash_new(hash_val_fn_t fval, hash_cmp_fn_t fcmp);
struct hash_table *hash_new_nentries(hash_val_fn_t fval, hash_cmp_fn_t fcmp,
				     unsigned int nentries);

void hash_free(struct hash_table *h);

bool hash_insert(struct hash_table *h, const void *key, const void *data);
void *hash_replace(struct hash_table *h, const void *key, const void *data);

bool hash_key_exists(const struct hash_table *h, const void *key);
void *hash_lookup_data(const struct hash_table *h, const void *key);

void *hash_delete_entry(struct hash_table *h, const void *key);
void *hash_delete_entry_full(struct hash_table *h, const void *key,
			     void **deleted_key);
void hash_delete_all_entries(struct hash_table *h);

const void *hash_key_by_number(const struct hash_table *h,
			       unsigned int entry_number);
const void *hash_value_by_number(const struct hash_table *h,
				 unsigned int entry_number);

unsigned int hash_num_entries(const struct hash_table *h);
unsigned int hash_num_buckets(const struct hash_table *h);
unsigned int hash_num_deleted(const struct hash_table *h);

enum Bucket_State { BUCKET_UNUSED=0, BUCKET_USED, BUCKET_DELETED };

/* internal bucket (don't use!) */
struct hash_bucket {
  enum Bucket_State used;
  const void *key;
  const void *data;
  unsigned int hash_val;	/* to avoid recalculating, or an extra fcmp,
                             in lookup */
};

/* opaque type (don't peek!) */
struct hash_table {
  struct hash_bucket *buckets;
  hash_val_fn_t fval;
  hash_cmp_fn_t fcmp;
  unsigned int num_buckets;
  unsigned int num_entries;	/* does not included deleted entries */
  unsigned int num_deleted;
  bool frozen;			/* do not auto-resize when set */
};


/**************************************************************************
  Iterates over all keys, values in the hash table. It is safe to delete
  entries in the table while iterating.
***************************************************************************/
#define hash_iterate(phash_table, key_type, keyvar, value_type, valuevar)\
{\
  int hi__i;\
  key_type keyvar;\
  value_type valuevar;\
  for (hi__i = 0; hi__i < (phash_table)->num_buckets; ++hi__i) {\
    struct hash_bucket *hi__bucket = &(phash_table)->buckets[hi__i];\
    if (hi__bucket->used != BUCKET_USED)\
      continue;\
    keyvar = (key_type) hi__bucket->key;\
    valuevar = (value_type) hi__bucket->data;

#define hash_iterate_end \
  }\
}
    

#endif  /* FC__HASH_H */
