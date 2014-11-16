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

/**********************************************************************
  Allocated/allocatable strings (and things?)
  original author: David Pfitzner <dwp@mso.anu.edu.au>

  A common technique is to have some memory dynamically allocated
  (using malloc etc), to avoid compiled-in limits, but only allocate
  enough space as initially needed, and then realloc later if/when
  require more space.  Typically, the realloc is made a bit more than
  immediately necessary, to avoid frequent reallocs if the object
  grows incementally.  Also, don't usually realloc at all if the
  object shrinks.  This is straightforward, but just requires a bit
  of book-keeping to keep track of how much has been allocated etc.
  This module provides some tools to make this a bit easier.

  This is deliberately simple and light-weight.  The user is allowed
  full access to the struct elements rather than use accessor
  functions etc.

  Note one potential hazard: when the size is increased (astr_minsize()),
  realloc (really wc_realloc) is used, which retains any data which
  was there previously, _but_: any external pointers into the allocated
  memory may then become wild.  So you cannot safely use such external
  pointers into the astring data, except strictly between times when
  the astring size may be changed.

***********************************************************************/

#ifdef HAVE_CONFIG_H
#  include "../config.hh"
#endif

#include <assert.h>
#include <stdarg.h>

#include "mem.hh"
#include "support.hh"            /* my_vsnprintf, mystrlcat */

#include "astring.hh"

/**********************************************************************
  Initialize the struct.
***********************************************************************/
void astr_init(struct astring *astr)
{
  struct astring zero_astr = ASTRING_INIT;
  assert(astr != NULL);
  *astr = zero_astr;
}

/**********************************************************************
  Check that astr has enough size to hold n, and realloc to a bigger
  size if necessary.  Here n must be big enough to include the trailing
  ascii-null if required.  The requested n is stored in astr->n.
  The actual amount allocated may be larger than n, and is stored
  in astr->n_alloc.
***********************************************************************/
void astr_minsize(struct astring *astr, size_t n)
{
  int n1;
  bool was_null = (astr->n == 0);

  assert(astr != NULL);

  astr->n = n;
  if (n <= astr->n_alloc) {
    return;
  }

  /* allocated more if this is only a small increase on before: */
  n1 = (3*(astr->n_alloc+10)) / 2;
  astr->n_alloc = (n > n1) ? n : n1;
  astr->str = (char *)wc_realloc(astr->str, astr->n_alloc);
  if (was_null) {
    astr_clear(astr);
  }
}

/**********************************************************************
  Free the memory associated with astr, and return astr to same
  state as after astr_init.
***********************************************************************/
void astr_free(struct astring *astr)
{
  struct astring zero_astr = ASTRING_INIT;

  assert(astr != NULL);

  if (astr->n_alloc > 0) {
    assert(astr->str != NULL);
    free(astr->str);
  }
  *astr = zero_astr;
}

/****************************************************************************
  Add the text to the string.
****************************************************************************/
static void vadd(struct astring *astr, const char *format, va_list ap)
{
  size_t new_len;
  size_t nb;
  char buf[1024];

  nb = my_vsnprintf(buf, sizeof(buf), format, ap);
  if (nb == -1) {
    die("Formatted string bigger than %lu bytes",
        (unsigned long)sizeof(buf));
  }

  /* Avoid calling strlen with NULL. */
  astr_minsize(astr, 1);

  new_len = astr->len + nb + 1;

  astr_minsize(astr, new_len);
  astr->len = mystrlcat(astr->str, buf, astr->n_alloc);
}

/****************************************************************************
  Append the text to the string.
****************************************************************************/
void astr_append(struct astring *astr, const char *s)
{
  size_t new_len;

  if (!astr || !astr->str || !s || s[0] == '\0') {
    return;
  }

  new_len = astr->len + strlen(s) + 1;

  astr_minsize(astr, new_len);
  astr->len = mystrlcat(astr->str, s, astr->n_alloc);
}

/****************************************************************************
  Append the text to the string printf style.
****************************************************************************/
void astr_append_printf(struct astring *astr, const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vadd(astr, format, args);
  va_end(args);
}

/**********************************************************************
  Sets the content to the empty string.
***********************************************************************/
void astr_clear(struct astring *astr)
{
  assert(astr != NULL);

  if (astr->n == 0) {
    /* astr_minsize is really astr_size, so we don't want to reduce the
     * size. */
    astr_minsize(astr, 1);
  }
  astr->str[0] = '\0';
  astr->len = 0;
}

/**********************************************************************
  Returns the length of the string, like strlen.
***********************************************************************/
size_t astr_size(struct astring *astr)
{
  if (!astr) {
    return 0;
  }
  return astr->len;
}

/**********************************************************************
  Returns the total size of the internal buffer.
***********************************************************************/
size_t astr_capacity(struct astring *astr)
{
  if (!astr) {
    return 0;
  }
  return astr->n_alloc;
}

/**********************************************************************
  Returns a const pointer to the internal buffer, or NULL if none.
***********************************************************************/
const char *astr_get_data(struct astring *astr)
{
  if (!astr) {
    return NULL;
  }
  return astr->str;
}
