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
*
* This module contains freeciv-specific memory management functions
*
**********************************************************************/

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "wc_intl.h"
#include "log.h"
#include "shared.h"             /* TRUE, FALSE */

#include "mem.h"

/**********************************************************************
 Do whatever we should do when malloc fails.
 At the moment this just prints a log message and calls exit(EXIT_FAILURE)
**********************************************************************/
static void handle_alloc_failure(size_t size, const char *called_as,
                                 int line, const char *file)
{
  die("Out of memory trying to %s %lu bytes at line %d of %s.", called_as,
      (unsigned long) size, line, file);
}

/****************************************************************************
  Check the size for sanity.  The program will exit rather than allocate a
  dangerously large amount of memory.
****************************************************************************/
static void sanity_check_size(size_t size, const char *called_as,
                              int line, const char *file)
{
  /* There used to be a sanity check here that would abort if more than
   * 30 megabytes were allocated.  Unfortunately this didn't work because
   * savegame loading can potentially use a huge amount of memory for large
   * games.  Another problem is it doesn't help much because there's nothing
   * preventing a large number of smaller allocations. */

  if (size == 0) {
    freelog(LOG_ERROR, "Warning: %s with size %lu at line %d of %s",
            called_as, (unsigned long)size, line, file);
  }
}


/**********************************************************************
 Function used by wc_malloc macro, malloc() replacement
 No need to check return value.
**********************************************************************/
void *wc_real_malloc(size_t size,
                     const char *called_as, int line, const char *file)
{
  void *ptr;

  sanity_check_size(size, called_as, line, file);

  ptr = malloc(size);
  if (!ptr) {
    handle_alloc_failure(size, called_as, line, file);
  }
  return ptr;
}

/**********************************************************************
 Function used by wc_realloc macro, realloc() replacement
 No need to check return value.
**********************************************************************/
void *wc_real_realloc(void *ptr, size_t size,
                      const char *called_as, int line, const char *file)
{
  void *new_ptr;

  if (!ptr) {
    return wc_real_malloc(size, called_as, line, file);
  }

  sanity_check_size(size, called_as, line, file);

  new_ptr = realloc(ptr, size);
  if (!new_ptr) {
    handle_alloc_failure(size, called_as, line, file);
  }
  return new_ptr;
}

/**********************************************************************
 Function used by wc_calloc macro, calloc() replacement
 No need to check return value.

 I'm pretty sure only the product of nelem and elsize can ever
 matter here, and not their individual values.  (As a matter of C.)
 Except this function doesn't support calloc-ing more memory than
 can be expressing using a single size_t, but that's not likely
 to be a problem.
**********************************************************************/
void *wc_real_calloc(size_t nelem, size_t elsize,
                     const char *called_as, int line, const char *file)
{
  size_t size = nelem*elsize;   /* potential overflow */
  void *ptr;

  ptr = wc_real_malloc(size, called_as, line, file);
  memset(ptr, 0, size);
  return ptr;
}

/***************************************************************
 Function used by mystrdup macro, strdup() replacement
 No need to check return value.
***************************************************************/
char *real_mystrdup(const char *str,
                    const char *called_as, int line, const char *file)
{
  char *dest = wc_real_malloc(strlen(str)+1, called_as, line, file);

  /* no need to check whether dest is non-NULL! */
  strcpy(dest, str);
  return dest;
}
