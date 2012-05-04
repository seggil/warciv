/**********************************************************************
 Freeciv - Copyright (C) 2005  - M.C. Kaufman
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

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "log.h"
#include "shared.h"
#include "support.h"

#include "connection.h"

#include "script.h"


#define W_CONN_CLASS "warciv wrapped connection"
struct w_conn {
  struct connection *pconn;
};


/**************************************************************************
  Returns TRUE if the script was successfully loaded.
**************************************************************************/
bool script_load(lua_State *L, const char *filename)
{
  const char *dfn, *error;
  int rv;

  dfn = datafilename(filename);
  if (!dfn) {
    freelog(LOG_ERROR, "script_load: \"%s\" not in data path.", filename);
    return FALSE;
  }

  rv = luaL_loadfile(L, dfn);
  if (rv != 0) {
    error = lua_tostring(L, -1);
    if (rv == LUA_ERRSYNTAX) {
      freelog(LOG_ERROR, "script_load: syntax error: %s", error);
    } else if (rv == LUA_ERRFILE) {
      freelog(LOG_ERROR, "script_load: load failed: %s", error);
    } else if (rv == LUA_ERRMEM) {
      freelog(LOG_ERROR, "script_load: out of memory: %s", error);
    } else {
      freelog(LOG_ERROR, "script_load: unknown error (%d): %s", rv, error);
    }
    lua_pop(L, 1);
    return FALSE;
  }

  rv = lua_pcall(L, 0, 0, 0);
  if (rv != 0) {
    error = lua_tostring(L, -1);
    if (rv == LUA_ERRRUN) {
      freelog(LOG_ERROR, "script_load: runtime error: %s", error);
    } else if (rv == LUA_ERRMEM) {
      freelog(LOG_ERROR, "script_load: out of memory: %s", error);
    } else {
      freelog(LOG_ERROR, "script_load: unknown error (%d): %s", rv, error);
    }
    lua_pop(L, 1);
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static int w_freelog(lua_State *L)
{
  int loglevel = luaL_checkinteger(L, 1);
  const char *msg = luaL_checkstring(L, 2);
  freelog(loglevel, "%s", msg);
  return 0;
}

/**************************************************************************
  ...
**************************************************************************/
static struct w_conn *check_w_conn(lua_State *L)
{
  struct w_conn *wc;
  wc = (struct w_conn *) luaL_checkudata(L, 1, W_CONN_CLASS);
  luaL_argcheck(L, wc != NULL, 1, "warciv connection expected");
  luaL_argcheck(L, conn_is_valid(wc->pconn), 1, "invalid connection");
  return wc;
}

/**************************************************************************
  ...
**************************************************************************/
static int w_conn_get_username(lua_State *L)
{
  struct w_conn *wc = check_w_conn(L);
  lua_pushstring(L, wc->pconn->username);
  return 1;
}

/**************************************************************************
  ...
**************************************************************************/
static int w_conn_set_password(lua_State *L)
{
  struct w_conn *wc = check_w_conn(L);
  size_t len;
  const char *pass = luaL_checklstring(L, 2, &len);
  sz_strlcpy(wc->pconn->u.server.password, pass);
  return 0;
}

/**************************************************************************
  ...
**************************************************************************/
static int w_conn_get_password(lua_State *L)
{
  struct w_conn *wc = check_w_conn(L);
  lua_pushstring(L, wc->pconn->u.server.password);
  return 1;
}

/**************************************************************************
  ...
**************************************************************************/
static int w_conn_set_salt(lua_State *L)
{
  struct w_conn *wc = check_w_conn(L);
  int salt = luaL_checkinteger(L, 2);
  wc->pconn->u.server.salt = salt;
  return 0;
}

/**************************************************************************
  ...
**************************************************************************/
static int w_conn_get_salt(lua_State *L)
{
  struct w_conn *wc = check_w_conn(L);
  lua_pushinteger(L, wc->pconn->u.server.salt);
  return 1;
}

/**************************************************************************
  ...
**************************************************************************/
int luaopen_warciv(lua_State *L)
{
  static const struct luaL_reg warciv_funcs[] = {
    {"freelog", w_freelog},
    {NULL, NULL}
  };

  static const struct luaL_reg w_conn_methods[] = {
    {"get_username", w_conn_get_username},
    {"set_password", w_conn_set_password},
    {"get_password", w_conn_get_password},
    {"set_salt", w_conn_set_salt},
    {"get_salt", w_conn_get_salt},
    {NULL, NULL}
  };

  luaL_newmetatable(L, W_CONN_CLASS);
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);
  lua_settable(L, -3);
  luaL_register(L, NULL, w_conn_methods);

  luaL_register(L, "warciv", warciv_funcs);

#define SET_ENUM(x)\
  lua_pushliteral(L, #x);\
  lua_pushinteger(L, x);\
  lua_rawset(L, -3);

  SET_ENUM(LOG_FATAL);
  SET_ENUM(LOG_ERROR);
  SET_ENUM(LOG_NORMAL);
  SET_ENUM(LOG_VERBOSE);
  SET_ENUM(LOG_DEBUG);

#undef SET_ENUM

  return 1;
}

/**************************************************************************
  ...
**************************************************************************/
static int push_w_conn(lua_State *L, struct connection *pconn)
{
  struct w_conn *wc;

  wc = lua_newuserdata(L, sizeof(*wc));
  luaL_getmetatable(L, W_CONN_CLASS);
  lua_setmetatable(L, -2);

  wc->pconn = pconn;

  return 1;
}


/**************************************************************************
  Returns TRUE if the call succeeds, in which case the return values
  specified in 'sig' will be on the stack. Otherwise returns FALSE if there
  is an error, with the error message on the stack.

  'sig' is a format string consisting of argument type option characters,
  followed by a '>', followed by return type option characters. Valid
  argument types are:

    'i' - int
    'd' - double
    's' - char *
    'C' - struct connection *

  Return types are assigned by dereferncing va_list elements. Valid return
  types are:

    'i' - int
    'd' - double
    's' - char *

  NB: You must lua_pop() the return values specified in 'sig' yourself,
  since if this function did it, it would not be possible to return
  strings.
**************************************************************************/
bool script_call(lua_State *L, const char *func, const char *sig, ...)
{
  int narg, nres;
  va_list ap;

  lua_getglobal(L, func);
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 1);
    lua_pushfstring(L, "script_call: Function `%s' not defined.", func);
    return FALSE;
  }

  va_start(ap, sig);

  narg = 0;
  while (*sig) {
    switch (*sig++) {

    case 'd':
      lua_pushnumber(L, va_arg(ap, double));
      break;

    case 'i':
      lua_pushnumber(L, va_arg(ap, int));
      break;

    case 's':
      lua_pushstring(L, va_arg(ap, char *));
      break;

    case 'C':
      push_w_conn(L, va_arg(ap, struct connection *));
      break;

    case '>':
      goto END_OF_ARGS;

    default:
      va_end(ap);
      lua_pop(L, narg);
      lua_pushfstring(L, "script_call: Invalid function "
                      "argument type '%c'.", *(sig - 1));
      return FALSE;
    }
    if (!lua_checkstack(L, 1)) {
      va_end(ap);
      lua_pop(L, narg);
      lua_pushliteral(L, "script_call: Stack overflow!");
      return FALSE;
    }
    narg++;
  }

END_OF_ARGS:

  nres = strlen(sig);
  if (lua_pcall(L, narg, nres, 0) != 0) {
    va_end(ap);
    lua_pushfstring(L, "script_call: Error running function `%s': %s",
                    func, lua_tostring(L, -1));
    lua_remove(L, -2);
    return FALSE;
  }

  nres = -nres; /* stack index of first result */

  while (*sig) {
    switch (*sig++) {

    case 'd':
      *va_arg(ap, double *) = lua_tonumber(L, nres);
      break;

    case 'i':
      *va_arg(ap, int *) = (int) lua_tonumber(L, nres);
      break;

    case 's':
      *va_arg(ap, const char **) = lua_tostring(L, nres);
      break;

    default:
      va_end(ap);
      lua_pop(L, -nres);
      lua_pushfstring(L, "script_call: Invalid function "
                      "return value type '%c'.", *(sig - 1));
      return FALSE;
    }
    nres++;
  }

  va_end(ap);
  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
bool script_check_func(lua_State *L, const char *scriptname,
                       const char *funcname)
{
  bool defined;

  lua_getglobal(L, funcname);
  defined = lua_isfunction(L, -1);
  lua_pop(L, 1);

  if (!defined) {
    freelog(LOG_ERROR, "Database script \"%s\" does not define a \"%s\" "
            "function.", scriptname, funcname);
    return FALSE;
  }

  return TRUE;
}

