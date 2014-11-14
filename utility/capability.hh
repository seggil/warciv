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

#ifndef WC_UTILITY_CAPABILITY_H
#define WC_UTILITY_CAPABILITY_H

#include "shared.hh"           /* bool type */

bool has_capability(const char *cap, const char *capstr);
bool has_capabilities(const char *us, const char *them);
bool has_svn_revision(int svnrev, const char *capstr);

#endif  /* WC_UTILITY_CAPABILITY_H */
