/**********************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef WC_SERVER_STDINHAND_C_H
#define WC_SERVER_STDINHAND_C_H

bool sset_is_changeable(int idx);

#define SSET_MAX_LEN  16             /* max setting name length (plus nul) */
#define TOKEN_DELIMITERS " \t\n,"

#endif
