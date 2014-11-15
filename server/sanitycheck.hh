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
#ifndef WC_SERVER_SANITYCHECK_H
#define WC_SERVER_SANITYCHECK_H

#include "wc_types.hh"

#if ( (defined(IS_BETA_VERSION) || defined(IS_DEVEL_VERSION)) \
      && !defined(NDEBUG) ) \
    || defined DEBUG
#  define SANITY_CHECKING
#endif

#ifdef SANITY_CHECKING

#  define sanity_check_city(x) real_sanity_check_city(x, __FILE__, __LINE__)
void real_sanity_check_city(city_t *pcity, const char *file, int line);

void sanity_check(void);

#else /* SANITY_CHECKING */

#  define sanity_check_city(x)
#  define sanity_check()

#endif /* SANITY_CHECKING */


#endif  /* WC_SERVER_SANITYCHECK_H */
