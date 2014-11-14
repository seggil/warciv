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
#ifndef WC_UTILITY_GENLIST_H
#define WC_UTILITY_GENLIST_H

/**********************************************************************
  MODULE: genlist

  A "genlist" is a generic doubly-linked list.  That is:
    generic:        stores (void*) pointers to arbitrary user data;
    doubly-linked:  can be efficiently traversed both "forwards"
                    and "backwards".

  The list data structures are allocated dynamically, and list
  elements can be added or removed at arbitrary positions.

  Positions in the list are specified starting from 0, up to n-1
  for a list with n elements.  The position -1 can be used to
  refer to the last element (that is, the same as n-1, or n when
  adding a new element), but other negative numbers are not
  meaningful.

  There are two memory management issues:

  - The user-data pointed to by the genlist elements; these are
    entirely the user's responsibility, and the genlist code just
    treats these as opaque data, not doing any allocation or freeing.

  - The data structures used internally by the genlist to store
    data for the links etc.  These are allocated by genlist_insert(),
    and freed by genlist_unlink() and genlist_unlink_all().  That is,
    it is the responsibility of the user to call the unlink functions
    as necessary to avoid memory leaks.

  A trap to beware of with iterators is modifying the list while the
  iterator is active, in particular removing the next element pointed
  to by the iterator (see further comments below).

  See also the speclist module.
***********************************************************************/

/* Opaque types to avoid a non-pointer declarations */
typedef struct _genlist genlist;
typedef struct _genlist_link genlist_link;

genlist *genlist_new(void);
void genlist_free(genlist *pgenlist);

int genlist_size(const genlist *pgenlist);
void *genlist_get(const genlist *pgenlist, int idx);
const genlist_link *genlist_get_head(const genlist *pgenlist);
const genlist_link *genlist_get_tail(const genlist *pgenlist);
void genlist_insert(genlist *pgenlist, void *data, int pos);
void genlist_unlink(genlist *pgenlist, void *punlink);
void genlist_unlink_all(genlist *pgenlist);
void genlist_sort(genlist *pgenlist, int (*compar)(const void *, const void *));
bool genlist_search(const genlist *pgenlist, const void *data);

void *genlist_link_get_data(const genlist_link *plink);
const genlist_link *genlist_link_get_prev(const genlist_link *plink);
const genlist_link *genlist_link_get_next(const genlist_link *plink);

#define ITERATOR_PTR(iter) (genlist_link_get_data(iter))
#define ITERATOR_NEXT(iter) (iter = genlist_link_get_next(iter))
#define ITERATOR_PREV(iter) (iter = genlist_link_get_prev(iter))


/* This is to iterate for a type defined like:
     struct unit_list { struct genlist list; };
   where the pointers in the list are really pointers to "atype".
   Eg, see speclist.h, which is what this is really for.
*/
#define TYPED_LIST_ITERATE(atype, typed_list, var) {       \
  const genlist_link *myiter;                              \
  atype *var;                                              \
  myiter = genlist_get_head((const genlist *)typed_list);  \
  for(; ITERATOR_PTR(myiter);) {                           \
    var=(atype *)ITERATOR_PTR(myiter);                     \
    ITERATOR_NEXT(myiter);

/* Balance for above: */
#define LIST_ITERATE_END  }}


/* Same, but iterate backwards: */
#define TYPED_LIST_ITERATE_REV(atype, typed_list, var) {   \
  const genlist_link *myiter;                              \
  atype *var;                                              \
  myiter = genlist_get_tail((const genlist *)typed_list);  \
  for(; ITERATOR_PTR(myiter);) {                           \
    var=(atype *)ITERATOR_PTR(myiter);                     \
    ITERATOR_PREV(myiter);

/* Balance for above: */
#define LIST_ITERATE_REV_END  }}


#endif  /* WC_UTILITY_GENLIST_H */
