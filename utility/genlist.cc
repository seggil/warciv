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
#  include "../config.h"
#endif

#include <stdlib.h>
#include <assert.h>

#include "wc_intl.hh"
#include "log.hh"
#include "mem.hh"

#include "genlist.hh"

#define INIT_MAGIC 0xA1B1C1D1

/* A single element of a genlist, storing the pointer to user
   data, and pointers to the next and previous elements:
*/
struct _genlist_link {
  genlist_link *next;
  genlist_link *prev;
  void *dataptr;
};


/* A genlist, storing the number of elements (for quick retrieval and
   testing for empty lists), and pointers to the first and last elements
   of the list.
*/
struct _genlist {
  int nelements;
  genlist_link *head_link;
  genlist_link *tail_link;
};

static genlist_link *find_genlist_position(const genlist *pgenlist, int pos);

/************************************************************************
  Returns a new genlist, initialized.
************************************************************************/
genlist *genlist_new(void)
{
  genlist *pgenlist = (genlist *)wc_malloc(sizeof(*pgenlist));

  pgenlist->nelements = 0;
  pgenlist->head_link = NULL;
  pgenlist->tail_link = NULL;

  return pgenlist;
}

/************************************************************************
  Free a genlist, and unlink all datas.
************************************************************************/
void genlist_free(genlist *pgenlist)
{
  genlist_unlink_all(pgenlist);
  free(pgenlist);
}

/************************************************************************
  Returns the number of elements stored in the genlist.
************************************************************************/
int genlist_size(const genlist *pgenlist)
{
  return pgenlist->nelements;
}

/************************************************************************
  Returns the user-data pointer stored in the genlist at the position
  given by 'idx'.  For idx out of range (including an empty list),
  returns NULL.
  Recall 'idx' can be -1 meaning the last element.
************************************************************************/
void *genlist_get(const genlist *pgenlist, int idx)
{
  genlist_link *link = find_genlist_position(pgenlist, idx);

  if (link) {
    return link->dataptr;
  } else {
    return NULL;
  }
}

/************************************************************************
  Returns the head link.
************************************************************************/
const genlist_link *genlist_get_head(const genlist *pgenlist)
{
  return pgenlist->head_link;
}

/************************************************************************
  Returns the tail link.
************************************************************************/
const genlist_link *genlist_get_tail(const genlist *pgenlist)
{
  return pgenlist->tail_link;
}

/************************************************************************
  Frees all the internal data used by the genlist (but doesn't touch
  the user-data).  At the end the state of the genlist will be the
  same as when genlist_init() is called on a new genlist.
************************************************************************/
void genlist_unlink_all(genlist *pgenlist)
{
  if (pgenlist->nelements > 0) {
    genlist_link *plink = pgenlist->head_link, *plink2;

    do {
      plink2 = plink->next;
      free(plink);
    } while ((plink = plink2) != NULL);

    pgenlist->head_link = NULL;
    pgenlist->tail_link = NULL;

    pgenlist->nelements = 0;
  }
}

/************************************************************************
  Remove an element of the genlist with the specified user-data pointer
  given by 'punlink'.  If there is no such element, does nothing.
  If there are multiple such elements, removes the first one.
************************************************************************/
void genlist_unlink(genlist *pgenlist, void *punlink)
{
  if (pgenlist->nelements > 0) {
    genlist_link *plink = pgenlist->head_link;

    while (plink != NULL && plink->dataptr != punlink) {
      plink = plink->next;
    }

    if (plink) {
      if (pgenlist->head_link == plink) {
         pgenlist->head_link = plink->next;
      } else {
         plink->prev->next = plink->next;
      }

      if (pgenlist->tail_link == plink) {
         pgenlist->tail_link = plink->prev;
      } else {
         plink->next->prev = plink->prev;
      }
      free(plink);
      pgenlist->nelements--;
    }
  }
}

/************************************************************************
  Insert a new element in the list, at position 'pos', with the specified
  user-data pointer 'data'.  Existing elements at >= pos are moved one
  space to the "right".  Recall 'pos' can be -1 meaning add at the
  end of the list.  For an empty list pos has no effect.
  A bad 'pos' value for a non-empty list is treated as -1 (is this
  a good idea?)
************************************************************************/
void genlist_insert(genlist *pgenlist, void *data, int pos)
{
  if (pgenlist->nelements == 0) { /*list is empty, ignore pos */

    genlist_link *plink = (genlist_link *)wc_malloc(sizeof(*plink));

    plink->dataptr = data;
    plink->next = NULL;
    plink->prev = NULL;

    pgenlist->head_link = plink;
    pgenlist->tail_link = plink;

  } else {
    genlist_link *plink = (genlist_link *)wc_malloc(sizeof(*plink));
    plink->dataptr = data;

    if (pos == 0) {
      plink->next = pgenlist->head_link;
      plink->prev = NULL;
      pgenlist->head_link->prev = plink;
      pgenlist->head_link = plink;
    } else if (pos <= -1 || pos >= pgenlist->nelements) {
      plink->next = NULL;
      plink->prev = pgenlist->tail_link;
      pgenlist->tail_link->next = plink;
      pgenlist->tail_link = plink;
    } else {
      genlist_link *left, *right;     /* left and right of new element */
      right = find_genlist_position(pgenlist, pos);
      left = right->prev;
      plink->next = right;
      plink->prev = left;
      right->prev = plink;
      left->next = plink;
    }
  }

  pgenlist->nelements++;
}


/************************************************************************
  Returns a pointer to the genlist link structure at the specified
  position.  Recall 'pos' -1 refers to the last position.
  For pos out of range returns NULL.
  Traverses list either forwards or backwards for best efficiency.
************************************************************************/
static genlist_link *find_genlist_position(const genlist *pgenlist, int pos)
{
  genlist_link *plink;

  if (pos == 0) {
    return pgenlist->head_link;
  } else if (pos == -1) {
    return pgenlist->tail_link;
  } else if (pos < -1 || pos >= pgenlist->nelements) {
    return NULL;
  }

  if (pos<pgenlist->nelements / 2) {  /* fastest to do forward search */
    for (plink = pgenlist->head_link; pos != 0; pos--) {
      plink = plink->next;
    }
  } else {                            /* fastest to do backward search */
    for (plink = pgenlist->tail_link, pos = pgenlist->nelements - pos - 1;
         pos != 0; pos--) {
      plink = plink->prev;
    }
  }

  return plink;
}

/************************************************************************
 Sort the elements of a genlist.

 The comparison function should be a function usable by qsort; note
 that the const void * arguments to compar should really be "pointers to
 void*", where the void* being pointed to are the genlist dataptrs.
 That is, there are two levels of indirection.
 To do the sort we first construct an array of pointers corresponding
 the the genlist dataptrs, then sort those and put them back into
 the genlist.
************************************************************************/
void genlist_sort(genlist *pgenlist, int (*compar)(const void *, const void *))
{
  const int n = genlist_size(pgenlist);
  void *sortbuf[n];
  genlist_link *myiter;
  int i;

  if (n <= 1) {
    return;
  }

  for (i = 0, myiter = pgenlist->head_link; i < n; i++, myiter = myiter->next) {
    sortbuf[i] = genlist_link_get_data(myiter);
  }

  qsort(sortbuf, n, sizeof(*sortbuf), compar);

  for (i = 0, myiter = pgenlist->head_link; i < n; i++, myiter = myiter->next) {
    myiter->dataptr = sortbuf[i];
  }
}

/************************************************************************
  Returns TRUE iff the data is an element of this list.
************************************************************************/
bool genlist_search(const genlist *pgenlist, const void *data)
{
  genlist_link *plink;

  for (plink = pgenlist->head_link; plink && plink->dataptr;
       plink = plink->next) {
    if (plink->dataptr == data) {
      return TRUE;
    }
  }
  return FALSE;
}

/************************************************************************
  Accessor functions for struct genlist_link.
************************************************************************/
void *genlist_link_get_data(const genlist_link *plink)
{
  return plink ? plink->dataptr : NULL;
}

/************************************************************************
  ...
************************************************************************/
const genlist_link *genlist_link_get_prev(const genlist_link *plink)
{
  return plink ? plink->prev : NULL;
}

/************************************************************************
  ...
************************************************************************/
const genlist_link *genlist_link_get_next(const genlist_link *plink)
{
  return plink ? plink->next : NULL;
}
