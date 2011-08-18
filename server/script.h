/**********************************************************************
 Freeciv - Copyright (C) 2005 - M.C. Kaufman
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef WC_SERVER_SCRIPT_H
#define WC_SERVER_SCRIPT_H


int luaopen_freeciv(lua_State *L);

bool script_load(lua_State *L, const char *filename);
bool script_check_func(lua_State *L, const char *scriptname,
                       const char *funcname);
bool script_call(lua_State *L, const char *func, const char *sig, ...)
                 wc__attribute((__format__ (__printf__, 3, 4)));

#endif /* WC_SERVER_SCRIPT_H */
