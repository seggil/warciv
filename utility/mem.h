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

#ifndef WC_UTILITY_MEM_H
#define WC_UTILITY_MEM_H

#include <stdlib.h>             /* size_t; actually stddef.h, but stdlib.h
                                   might be more reliable? --dwp */

/* wc_malloc, wc_realloc, fc_calloc:
 * fc_ stands for freeciv; the return value is checked,
 * and freeciv-specific processing occurs if it is NULL:
 * a log message, possibly cleanup, and ending with exit(1)
 */

#define wc_malloc(sz)      wc_real_malloc((sz), "malloc", \
                                          __LINE__, __FILE__)
#define wc_realloc(ptr,sz) wc_real_realloc((ptr), (sz), "realloc", \
                                           __LINE__, __FILE__)
#define fc_calloc(n,esz)   wc_real_calloc((n), (esz), "calloc", \
                                           __LINE__, __FILE__)

#define mystrdup(str)      real_mystrdup((str), "strdup", \
                                         __LINE__, __FILE__)

/***********************************************************************/

/* You shouldn't call these functions directly;
 * use the macros above instead.
 */
void *wc_real_malloc(size_t size,
                     const char *called_as, int line, const char *file);
void *wc_real_realloc(void *ptr, size_t size,
                      const char *called_as, int line, const char *file);
void *wc_real_calloc(size_t nelem, size_t elsize,
                     const char *called_as, int line, const char *file);

char *real_mystrdup(const char *str,
                    const char *called_as, int line, const char *file);

#endif /* WC_UTILITY_MEM_H */
