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
#ifndef WC_SERVER_LUA_SRC_LOCALLUACONF_H
#define WC_SERVER_LUA_SRC_LOCALLUACONF_H

#ifdef HAVE_CONFIG_H
# ifdef LOCAL_LUA_CONF
#  include "../../../config.hh"
# endif
#endif

#ifdef HAVE_MKSTEMP
# define LUA_USE_MKSTEMP
#endif
#ifdef HAVE_ISATTY
# define LUA_USE_ISATTY
#endif
#ifdef HAVE_POPEN
# define LUA_USE_POPEN
#endif
#ifdef HAVE__LONGJMP
# define LUA_USE_ULONGJMP
#endif

#endif /* WC_SERVER_LUA_SRC_LOCALLUACONF_H */
