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

#ifndef WC_UTILITY_SUPPORT_H
#define WC_UTILITY_SUPPORT_H

/**********************************************************************
  Replacements for functions which are not available on all platforms.
  Where the functions are available natively, these are just wrappers.
  See also mem.h, netintf.h, rand.h, and see support.c for more comments.
***********************************************************************/

#include <stdarg.h>
#include <stdlib.h>             /* size_t */
#include <stdio.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include "shared.hh"           /* bool type and wc__attribute */

int mystrcasecmp(const char *str0, const char *str1);
int mystrncasecmp(const char *str0, const char *str1, size_t n);

const char *mystrerror(long err_no);
const char *mystrsocketerror(long err_no);
long myerrno(void);
long mysocketerrno(void);
bool is_interrupted_errno(long err_no);
void myusleep(unsigned long usec);

size_t mystrlcpy(char *dest, char const *src, size_t n);
size_t mystrlcat(char *dest, char const *src, size_t n);

/* convenience macros for use when dest is a char ARRAY: */
#define sz_strlcpy(dest,src) ((void)mystrlcpy((dest),(src),sizeof(dest)))
#define sz_strlcat(dest,src) ((void)mystrlcat((dest),(src),sizeof(dest)))

int my_snprintf(char *str, size_t n, const char *format, ...)
     wc__attribute((__format__ (__printf__, 3, 4)));

int my_vsnprintf(char *str, size_t n, const char *format, va_list ap );

int my_gethostname(char *buf, size_t len);

#ifdef SOCKET_ZERO_ISNT_STDIN
/* Support for console I/O in case SOCKET_ZERO_ISNT_STDIN. */
void my_init_console(void);
char *my_read_console(void);
#endif

bool is_reg_file_for_access(const char *name, bool write_access);

bool my_isalnum(char c);
bool my_isalpha(char c);
bool my_isdigit(char c);
bool my_isprint(char c);
bool my_isspace(char c);
bool my_isupper(char c);
char my_toupper(char c);
char my_tolower(char c);

FILE *my_tmpfile (void);

#endif  /* WC_UTILITY_SUPPORT_H */
