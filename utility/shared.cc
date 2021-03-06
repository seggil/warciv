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

#ifdef HAVE_CONFIG_H
#  include "../config.hh"
#endif

#ifdef HAVE_SYS_TYPES_H
/* Under Mac OS X sys/types.h must be included before dirent.h */
#include <sys/types.h>
#endif

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef WIN32_NATIVE
# include <winsock2.h>
# include <windows.h>
# include <lmcons.h>     /* UNLEN */
#endif

#include "astring.hh"
#include "wc_iconv.hh"
#include "wc_intl.hh"
#include "iterator.hh"
#include "log.hh"
#include "mem.hh"
#include "support.hh"

#include "shared.hh"

#ifndef PATH_SEPARATOR
#if defined _WIN32 || defined __WIN32__ || defined __EMX__ || defined __DJGPP__
  /* Win32, OS/2, DOS */
# define PATH_SEPARATOR ";"
#else
  /* Unix */
# define PATH_SEPARATOR ":"
#endif
#endif

/* If no default data path is defined use the default default one */
#ifndef DEFAULT_DATA_PATH
#define DEFAULT_DATA_PATH "." PATH_SEPARATOR "data" PATH_SEPARATOR \
                          "~/.warciv"
#endif

/* Both of these are stored in the local encoding.  The grouping_sep must
 * be converted to the internal encoding when it's used. */
static char *grouping = NULL;
static char *grouping_sep = NULL;

/***************************************************************
  Take a string containing multiple lines and create a copy where
  each line is padded to the length of the longest line and centered.
  We do not cope with tabs etc.  Note that we're assuming that the
  last line does _not_ end with a newline.  The caller should
  free() the result.

  FIXME: This is only used in the Xaw client, and so probably does
  not belong in common.
***************************************************************/
char *create_centered_string(const char *s)
{
  /* Points to the part of the source that we're looking at. */
  const char *cp;

  /* Points to the beginning of the line in the source that we're
   * looking at. */
  const char *cp0;

  /* Points to the result. */
  char *r;

  /* Points to the part of the result that we're filling in right
     now. */
  char *rn;

  int i;

  int maxlen = 0;
  int curlen = 0;
  int nlines = 1;

  for(cp=s; *cp != '\0'; cp++) {
    if(*cp!='\n')
      curlen++;
    else {
      if(maxlen<curlen)
        maxlen=curlen;
      curlen=0;
      nlines++;
    }
  }
  if(maxlen<curlen)
    maxlen=curlen;

  r = rn = (char*)wc_malloc(nlines*(maxlen+1));

  curlen=0;
  for(cp0=cp=s; *cp != '\0'; cp++) {
    if(*cp!='\n')
      curlen++;
    else {
      for(i=0; i<(maxlen-curlen)/2; i++)
        *rn++=' ';
      memcpy(rn, cp0, curlen);
      rn+=curlen;
      *rn++='\n';
      curlen=0;
      cp0=cp+1;
    }
  }
  for(i=0; i<(maxlen-curlen)/2; i++)
    *rn++=' ';
  strcpy(rn, cp0);

  return r;
}

/**************************************************************************
  return a char * to the parameter of the option or NULL.
  *i can be increased to get next string in the array argv[].
  It is an error for the option to exist but be an empty string.
  This doesn't use freelog() because it is used before logging is set up.
**************************************************************************/
char *get_option(const char *option_name, char **argv, int *i, int argc)
{
  int len = strlen(option_name);

  if (strcmp(option_name, argv[*i]) == 0 ||
      (strncmp(option_name, argv[*i], len) == 0 && argv[*i][len] == '=') ||
      strncmp(option_name + 1, argv[*i], 2) == 0) {
    char *opt = argv[*i] + (argv[*i][1] != '-' ? 0 : len);

    if (*opt == '=') {
      opt++;
    } else {
      if (*i < argc - 1) {
        (*i)++;
        opt = argv[*i];
        if (strlen(opt)==0) {
          wc_fprintf(stderr, _("Empty argument for \"%s\".\n"), option_name);
          exit(EXIT_FAILURE);
        }
      } else {
        wc_fprintf(stderr, _("Missing argument for \"%s\".\n"), option_name);
        exit(EXIT_FAILURE);
      }
    }

    return opt;
  }

  return NULL;
}

/***************************************************************
...
***************************************************************/
bool is_option(const char *option_name,char *option)
{
  return (strcmp(option_name, option) == 0 ||
          strncmp(option_name + 1, option, 2) == 0);
}

/***************************************************************
  Like strcspn but also handles quotes, i.e. *reject chars are
  ignored if they are inside single or double quotes.
***************************************************************/
static size_t my_strcspn(const char *s, const char *reject)
{
  bool in_single_quotes = FALSE;
  bool in_double_quotes = FALSE;
  size_t i;
  size_t len = strlen(s);

  for (i = 0; i < len; i++) {
    if (s[i] == '"' && !in_single_quotes) {
      in_double_quotes = !in_double_quotes;
    } else if (s[i] == '\'' && !in_double_quotes) {
      in_single_quotes = !in_single_quotes;
    }

    if (in_single_quotes || in_double_quotes) {
      continue;
    }

    if (strchr(reject, s[i])) {
      break;
    }
  }

  return i;
}

/***************************************************************
 Splits the string into tokens. The individual tokens are
 returned. The delimiterset can freely be chosen.

 i.e. "34 abc 54 87" with a delimiterset of " " will yield
      tokens={"34", "abc", "54", "87"}

 Part of the input string can be quoted (single or double) to embedded
 delimiter into tokens. For example,
   command 'a name' hard "1,2,3,4,5" 99
   create 'Mack "The Knife"'
 will yield 5 and 2 tokens respectively using the delimiterset " ,".

 Tokens which aren't used aren't modified (and memory is not
 allocated). If the string would yield more tokens only the first
 num_tokens are extracted.

 The user has the responsiblity to free the memory allocated by
 **tokens, e.g. with the free_tokens functions.
***************************************************************/
int get_tokens(const char *str, char **tokens,
               size_t num_tokens, const char *delimiterset)
{
  return get_tokens_full(str, tokens, num_tokens,
                         delimiterset, FALSE);
}
/***************************************************************
  ...
***************************************************************/
int get_tokens_full(const char *str,
                    char **tokens,
                    size_t num_tokens,
                    const char *delimiterset,
                    bool fill_last)
{
  unsigned int token = 0;

  assert(str != NULL);

  for (;;) {
    size_t len;
    size_t padlength = 0;

    /* skip leading delimiters */
    str += strspn(str, delimiterset);

    if (*str == '\0') {
      break;
    }

    if (token >= num_tokens) {
      break;
    }

    if (fill_last && token == num_tokens - 1) {
      len = strlen(str);
    } else {
      len = my_strcspn(str, delimiterset);
    }

    /* strip start/end quotes if they exist */
    if (len >= 2) {
      if ((str[0] == '"' && str[len - 1] == '"')
          || (str[0] == '\'' && str[len - 1] == '\'')) {
        len -= 2;
        padlength = 1;          /* to set the string past the end quote */
        str++;
      }
    }

    tokens[token] = (char*)wc_malloc(len + 1);
    (void) mystrlcpy(tokens[token], str, len + 1);      /* adds the '\0' */

    token++;

    str += len + padlength;
  }

  return token;
}

/***************************************************************
  ...
***************************************************************/
void free_tokens(char **tokens, size_t ntokens)
{
  size_t i;
  for (i = 0; i < ntokens; i++) {
    if (tokens[i]) {
      free(tokens[i]);
    }
  }
}

/***************************************************************
  Returns a statically allocated string containing a nicely-formatted
  version of the given number according to the user's locale.  (Only
  works for numbers >= zero.)  The number is given in scientific notation
  as mantissa * 10^exponent.
***************************************************************/
const char *big_int_to_text(unsigned int mantissa, unsigned int exponent)
{
  static char buf[64]; /* Note that we'll be filling this in right to left. */
  char *grp = grouping;
  char *ptr;
  unsigned int cnt = 0;
  char sep[64];
  size_t seplen;

  /* We have to convert the encoding here (rather than when the locale
   * is initialized) because it can't be done before the charsets are
   * initialized. */
  local_to_internal_string_buffer(grouping_sep, sep, sizeof(sep));
  seplen = strlen(sep);

#if 0 /* Not needed while the values are unsigned. */
  assert(mantissa >= 0);
  assert(exponent >= 0);
#endif

  if (mantissa == 0) {
    return "0";
  }

  /* We fill the string in backwards, starting from the right.  So the first
   * thing we do is terminate it. */
  ptr = &buf[sizeof(buf)];
  *(--ptr) = '\0';

  while (mantissa != 0 && exponent >= 0) {
    int dig;

    if (ptr <= buf + seplen) {
      /* Avoid a buffer overflow. */
      assert(ptr > buf + seplen);
      return ptr;
    }

    /* Add on another character. */
    if (exponent > 0) {
      dig = 0;
      exponent--;
    } else {
      dig = mantissa % 10;
      mantissa /= 10;
    }
    *(--ptr) = '0' + dig;

    cnt++;
    if (mantissa != 0 && cnt == (unsigned int)*grp) {
      /* Reached count of digits in group: insert separator and reset count. */
      cnt = 0;
      if (*grp == CHAR_MAX) {
        /* This test is unlikely to be necessary since we would need at
           least 421-bit ints to break the 127 digit barrier, but why not. */
        break;
      }
      ptr -= seplen;
      assert(ptr >= buf);
      memcpy(ptr, sep, seplen);
      if (*(grp + 1) != 0) {
        /* Zero means to repeat the present group-size indefinitely. */
        grp++;
      }
    }
  }

  return ptr;
}


/****************************************************************************
  Return a prettily formatted string containing the given number.
****************************************************************************/
const char *int_to_text(unsigned int number)
{
  return big_int_to_text(number, 0);
}

/****************************************************************************
  Check whether or not the given char is a valid ascii character.  The
  character can be in any charset so long as it is a superset of ascii.
****************************************************************************/
static bool is_ascii(char ch)
{
  /* this works with both signed and unsigned char's. */
  return ch >= ' ' && ch <= '~';
}

/***************************************************************
  This is used in sundry places to make sure that names of cities,
  players etc. do not contain yucky characters of various sorts.
  Returns TRUE iff the name is acceptable.
  FIXME:  Not internationalised.
***************************************************************/
bool is_ascii_name(const char *name)
{
  const char illegal_chars[] = {'|', '%', '"', ',', '*', '<', '>', '\0'};
  int i;
  int j;

  /* must not be NULL or empty */
  if (!name || *name == '\0') {
    return FALSE;
  }

  /* must begin and end with some non-space character */
  if ((*name == ' ') || (*(strchr(name, '\0') - 1) == ' ')) {
    return FALSE;
  }

  /* must be composed entirely of printable ascii characters,
   * and no illegal characters which can break ranking scripts. */
  for (i = 0; name[i]; i++) {
    if (!is_ascii(name[i])) {
      return FALSE;
    }
    for (j = 0; illegal_chars[j]; j++) {
      if (name[i] == illegal_chars[j]) {
        return FALSE;
      }
    }
  }

  /* otherwise, it's okay... */
  return TRUE;
}

/***************************************************************
  Produce a statically allocated textual representation of the given
  year.
***************************************************************/
const char *textyear(int year)
{
  static char y[32];
  if (year<0)
    my_snprintf(y, sizeof(y), _("%d BC"), -year);
  else
    my_snprintf(y, sizeof(y), _("%d AD"), year);
  return y;
}

/**************************************************************************
  Compares two strings, in the collating order of the current locale,
  given pointers to the two strings (i.e., given "char *"s).
  Case-sensitive.  Designed to be called from qsort().
**************************************************************************/
int compare_strings(const void *first, const void *second)
{
#if defined(ENABLE_NLS) && defined(HAVE_STRCOLL)
  return strcoll((const char *)first, (const char *)second);
#else
  return strcmp((const char *)first, (const char *)second);
#endif
}

/**************************************************************************
  Compares two strings, in the collating order of the current locale,
  given pointers to the two string pointers (i.e., given "char **"s).
  Case-sensitive.  Designed to be called from qsort().
**************************************************************************/
int compare_strings_ptrs(const void *first, const void *second)
{
#if defined(ENABLE_NLS) && defined(HAVE_STRCOLL)
  return strcoll(*((const char **)first), *((const char **)second));
#else
  return strcmp(*((const char **)first), *((const char **)second));
#endif
}

/***************************************************************************
  Returns 's' incremented to first non-space character.
***************************************************************************/
const char *skip_leading_spaces(const char *s)
{
  assert(s!=NULL);
  while(*s != '\0' && my_isspace(*s)) {
    s++;
  }
  return s;
}

/***************************************************************************
  Removes leading spaces in string pointed to by 's'.
  Note 's' must point to writeable memory!
***************************************************************************/
static void remove_leading_spaces(char *s)
{
  const char *t;

  assert(s!=NULL);
  t = skip_leading_spaces(s);
  if (t != s) {
    while (*t != '\0') {
      *s++ = *t++;
    }
    *s = '\0';
  }
}

/***************************************************************************
  Terminates string pointed to by 's' to remove traling spaces;
  Note 's' must point to writeable memory!
***************************************************************************/
void remove_trailing_spaces(char *s)
{
  char *t;
  size_t len;

  assert(s!=NULL);
  len = strlen(s);
  if (len > 0) {
    t = s + len -1;
    while(my_isspace(*t)) {
      *t = '\0';
      if (t == s) {
        break;
      }
      t--;
    }
  }
}

/***************************************************************************
  Removes leading and trailing spaces in string pointed to by 's'.
  Note 's' must point to writeable memory!
***************************************************************************/
void remove_leading_trailing_spaces(char *s)
{
  remove_leading_spaces(s);
  remove_trailing_spaces(s);
}

/***************************************************************************
  As remove_trailing_spaces(), for specified char.
***************************************************************************/
static void remove_trailing_char(char *s, char trailing)
{
  char *t;

  assert(s!=NULL);
  t = s + strlen(s) -1;
  while(t>=s && (*t) == trailing) {
    *t = '\0';
    t--;
  }
}

/***************************************************************************
  Change spaces in s into newlines, so as to keep lines length len
  or shorter.  That is, modifies s.
  Returns number of lines in modified s.
***************************************************************************/
int wordwrap_string(char *s, int len)
{
  int num_lines = 0;
  int slen = strlen(s);

  /* At top of this loop, s points to the rest of string,
   * either at start or after inserted newline: */
 top:
  if (s && *s != '\0' && slen > len) {
    char *c;

    num_lines++;

    /* check if there is already a newline: */
    for(c=s; c<s+len; c++) {
      if (*c == '\n') {
        slen -= c+1 - s;
        s = c+1;
        goto top;
      }
    }

    /* find space and break: */
    for(c=s+len; c>s; c--) {
      if (my_isspace(*c)) {
        *c = '\n';
        slen -= c+1 - s;
        s = c+1;
        goto top;
      }
    }

    /* couldn't find a good break; settle for a bad one... */
    for (c = s + len + 1; *c != '\0'; c++) {
      if (my_isspace(*c)) {
        *c = '\n';
        slen -= c+1 - s;
        s = c+1;
        goto top;
      }
    }
  }
  return num_lines;
}

/***************************************************************************
  Returns pointer to '\0' at end of string 'str', and decrements
  *nleft by the length of 'str'.  This is intended to be useful to
  allow strcat-ing without traversing the whole string each time,
  while still keeping track of the buffer length.
  Eg:
     char buf[128];
     int n = sizeof(buf);
     char *p = buf;

     my_snprintf(p, n, "foo%p", p);
     p = end_of_strn(p, &n);
     mystrlcpy(p, "yyy", n);
***************************************************************************/
char *end_of_strn(char *str, int *nleft)
{
  int len = strlen(str);
  *nleft -= len;
  assert((*nleft)>0);           /* space for the terminating nul */
  return str + len;
}

/**********************************************************************
  Check the length of the given string.  If the string is too long,
  log errmsg, which should be a string in printf-format taking up to
  two arguments: the string and the length.
**********************************************************************/
bool check_strlen(const char *str, size_t len, const char *errmsg)
{
  if (strlen(str) >= len) {
    freelog(LOG_ERROR, errmsg, str, len);
    assert(0);
    return TRUE;
  }
  return FALSE;
}

/**********************************************************************
  Call check_strlen() on str and then strlcpy() it into buffer.
**********************************************************************/
size_t loud_strlcpy(char *buffer, const char *str, size_t len,
                   const char *errmsg)
{
  (void) check_strlen(str, len, errmsg);
  return mystrlcpy(buffer, str, len);
}

/**********************************************************************
 cat_snprintf is like a combination of my_snprintf and mystrlcat;
 it does snprintf to the end of an existing string.

 Like mystrlcat, n is the total length available for str, including
 existing contents and trailing nul.  If there is no extra room
 available in str, does not change the string.

 Also like mystrlcat, returns the final length that str would have
 had without truncation.  I.e., if return is >= n, truncation occurred.
**********************************************************************/
int cat_snprintf(char *str, size_t n, const char *format, ...)
{
  size_t len;
  int ret;
  va_list ap;

  assert(format != NULL);
  assert(str != NULL);
  assert(n>0);

  len = strlen(str);
  assert(len < n);

  va_start(ap, format);
  ret = my_vsnprintf(str+len, n-len, format, ap);
  va_end(ap);
  return (int) (ret + len);
}

/***************************************************************************
  Exit because of a fatal error after printing a message about it.  This
  should only be called for code errors - user errors (like not being able
  to find a tileset) should just exit rather than dumping core.
***************************************************************************/
void real_die(const char *file, int line, const char *format, ...)
{
  va_list ap;

  freelog(LOG_FATAL, "Detected fatal error in %s line %d:", file, line);
  va_start(ap, format);
  vreal_freelog(LOG_FATAL, format, ap);
  va_end(ap);

  assert(FALSE);

  exit(EXIT_FAILURE);
}

/***************************************************************************
  Returns string which gives users home dir, as specified by $HOME.
  Gets value once, and then caches result.
  If $HOME is not set, give a log message and returns NULL.
  Note the caller should not mess with the returned string.
***************************************************************************/
char *user_home_dir(void)
{
#ifdef AMIGA
  return "PROGDIR:";
#else
  static bool init = FALSE;
  static char *home_dir = NULL;

  if (!init) {
    char *env = getenv("HOME");
    if (env) {
      home_dir = mystrdup(env);         /* never free()d */
      freelog(LOG_VERBOSE, "HOME is %s", home_dir);
    } else {
#ifdef WIN32_NATIVE
      home_dir = (char*)wc_malloc(PATH_MAX);
      if (!getcwd(home_dir, PATH_MAX)) {
        free(home_dir);
        home_dir = NULL;
        freelog(LOG_ERROR, "Could not find home directory (HOME is not set)");
      }
#else
      freelog(LOG_ERROR, "Could not find home directory (HOME is not set)");
      home_dir = NULL;
#endif
    }
    init = TRUE;
  }
  return home_dir;
#endif
}

/***************************************************************************
  Returns string which gives user's username, as specified by $USER or
  as given in password file for this user's uid, or a made up name if
  we can't get either of the above.
  Gets value once, and then caches result.
  Note the caller should not mess with returned string.
***************************************************************************/
const char *user_username(void)
{
  static char username[MAX_LEN_NAME];

  /* This function uses a number of different methods to try to find a
   * username.  This username then has to be truncated to MAX_LEN_NAME
   * characters (including terminator) and checked for sanity.  Note that
   * truncating a sane name can leave you with an insane name under some
   * charsets. */

  if (username[0] != '\0') {
    /* Username is already known; just return it. */
    return username;
  }

  /* If the environment variable $USER is present and sane, use it. */
  {
    char *env = getenv("USER");

    if (env) {
      sz_strlcpy(username, env);
      if (is_ascii_name(username)) {
        freelog(LOG_VERBOSE, "USER username is %s", username);
        return username;
      }
    }
  }

#ifdef HAVE_GETPWUID
  /* Otherwise if getpwuid() is available we can use it to find the true
   * username. */
  {
    struct passwd *pwent = getpwuid(getuid());

    if (pwent) {
      sz_strlcpy(username, pwent->pw_name);
      if (is_ascii_name(username)) {
        freelog(LOG_VERBOSE, "getpwuid username is %s", username);
        return username;
      }
    }
  }
#endif

#ifdef WIN32_NATIVE
  /* On win32 the GetUserName function will give us the login name. */
  {
    char name[UNLEN + 1];
    DWORD length = sizeof(name);

    if (GetUserName(name, &length)) {
      sz_strlcpy(username, name);
      if (is_ascii_name(username)) {
        freelog(LOG_VERBOSE, "GetUserName username is %s", username);
        return username;
      }
    }
  }
#endif

#ifdef ALWAYS_ROOT
  sz_strlcpy(username, "name");
#else
  my_snprintf(username, MAX_LEN_NAME, "name%d", (int)getuid());
#endif
  freelog(LOG_VERBOSE, "fake username is %s", username);
  assert(is_ascii_name(username));
  return username;
}

/***************************************************************************
  Returns a list of data directory paths, in the order in which they should
  be searched.  These paths are specified internally or may be set as the
  environment variable $WARCIV_PATH (a separated list of directories,
  where the separator itself is specified internally, platform-dependent).
  '~' at the start of a component (provided followed by '/' or '\0') is
  expanded as $HOME.

  The returned value is a static NULL-terminated list of strings.

  num_dirs, if not NULL, will be set to the number of entries in the list.
***************************************************************************/
static const char **get_data_dirs(int *num_dirs)
{
  const char *path;
  char *path2;
  char *tok;
  static int num = 0;
  static const char **dirs = NULL;

  /* The first time this function is called it will search and
   * allocate the directory listing.  Subsequently we will already
   * know the list and can just return it. */
  if (dirs) {
    if (num_dirs) {
      *num_dirs = num;
    }
    return dirs;
  }

  path = getenv("WARCIV_PATH");
  if (!path) {
    path = DEFAULT_DATA_PATH;
  } else if (*path == '\0') {
    freelog(LOG_ERROR, _("WARCIV_PATH is set but empty; "
                         "using default path instead."));
    path = DEFAULT_DATA_PATH;
  }
  assert(path != NULL);

  path2 = mystrdup(path);       /* something we can strtok */

  tok = strtok(path2, PATH_SEPARATOR);
  do {
    int i;                      /* strlen(tok), or -1 as flag */

    tok = (char *) skip_leading_spaces(tok);
    remove_trailing_spaces(tok);
    if (strcmp(tok, "/") != 0) {
      remove_trailing_char(tok, '/');
    }

    i = strlen(tok);
    if (tok[0] == '~') {
      if (i > 1 && tok[1] != '/') {
        freelog(LOG_ERROR, "For \"%s\" in data path cannot expand '~'"
                " except as '~/'; ignoring", tok);
        i = 0;   /* skip this one */
      } else {
        char *home = user_home_dir();

        if (!home) {
          freelog(LOG_VERBOSE,
                  "No HOME, skipping data path component %s", tok);
          i = 0;
        } else {
          int len = strlen(home) + i;      /* +1 -1 */
          char *tmp = (char*)wc_malloc(len);

          my_snprintf(tmp, len, "%s%s", home, tok + 1);
          tok = tmp;
          i = -1;               /* flag to free tok below */
        }
      }
    }

    if (i != 0) {
      /* We could check whether the directory exists and
       * is readable etc?  Don't currently. */
      num++;
      dirs = (const char**)wc_realloc(dirs, num * sizeof(char*));
      dirs[num - 1] = mystrdup(tok);
      freelog(LOG_VERBOSE, "Data path component (%d): %s", num - 1, tok);
      if (i == -1) {
        free(tok);
        tok = NULL;
      }
    }

    tok = strtok(NULL, PATH_SEPARATOR);
  } while(tok);

  /* NULL-terminate the list. */
  dirs = (const char**)wc_realloc(dirs, (num + 1) * sizeof(char*));
  dirs[num] = NULL;

  free(path2);

  if (num_dirs) {
    *num_dirs = num;
  }
  return dirs;
}

/***************************************************************************
  Returns a NULL-terminated list of filenames in the data directories
  matching the given suffix.

  The list is allocated when the function is called; it should either
  be stored permanently or de-allocated (by free'ing each element and
  the whole list).

  The suffixes are removed from the filenames before the list is
  returned.
***************************************************************************/
const char **datafilelist(const char* suffix)
{
  const char **dirs = get_data_dirs(NULL);
  char **file_list = NULL;
  int num_matches = 0;
  int list_size = 0;
  int dir_num;
  int i;
  int j;
  size_t suffix_len = strlen(suffix);

  assert(!strchr(suffix, '/'));

  /* First assemble a full list of names. */
  for (dir_num = 0; dirs[dir_num]; dir_num++) {
    DIR* dir;
    struct dirent *entry;

    /* Open the directory for reading. */
    dir = opendir(dirs[dir_num]);
    if (!dir) {
      if (errno == ENOENT) {
        freelog(LOG_VERBOSE, "Skipping non-existing data directory %s.",
                dirs[dir_num]);
      } else {
        freelog(LOG_ERROR, _("Could not read data directory %s: %s."),
                dirs[dir_num], mystrerror(myerrno()));
      }
      continue;
    }

    /* Scan all entries in the directory. */
    while ((entry = readdir(dir))) {
      size_t len = strlen(entry->d_name);

      /* Make sure the file name matches. */
      if (len > suffix_len
          && strcmp(suffix, entry->d_name + len - suffix_len) == 0) {
        /* Strdup the entry so we can safely write to it. */
        char *match = mystrdup(entry->d_name);

        /* Make sure the list is big enough; grow exponentially to keep
           constant ammortized overhead. */
        if (num_matches >= list_size) {
          list_size = list_size > 0 ? list_size * 2 : 10;
          file_list = (char**)wc_realloc(file_list, list_size * sizeof(*file_list));
        }

        /* Clip the suffix. */
        match[len - suffix_len] = '\0';

        file_list[num_matches++] = mystrdup(match);

        free(match);
      }
    }

    closedir(dir);
  }

  /* Sort the list. */
  qsort(file_list, num_matches, sizeof(*file_list), compare_strings_ptrs);

  /* Remove duplicates (easy since it's sorted). */
  i = j = 0;
  while (j < num_matches) {
    char *this_ = file_list[j];

    for (j++; j < num_matches && strcmp(this_, file_list[j]) == 0; j++) {
      free(file_list[j]);
    }

    file_list[i] = this_;

    i++;
  }
  num_matches = i;

  /* NULL-terminate the whole thing. */
  file_list = (char**)wc_realloc(file_list, (num_matches + 1) * sizeof(*file_list));
  file_list[num_matches] = NULL;

  return (const char **)file_list;
}

/***************************************************************************
  Returns a filename to access the specified file from a data
  directory by searching all data directories (as specified by
  get_data_dirs) for the file.

  If the specified 'filename' is NULL, the returned string contains
  the effective data path.  (But this should probably only be used for
  debug output.)

  Returns NULL if the specified filename cannot be found in any of the
  data directories.  (A file is considered "found" if it can be
  read-opened.)  The returned pointer points to static memory, so this
  function can only supply one filename at a time.
***************************************************************************/
char *datafilename(const char *filename)
{
  int num_dirs;
  int i;
  const char **dirs = get_data_dirs(&num_dirs);
  static struct astring realfile = ASTRING_INIT;

  if (!filename) {
    size_t len = 1;             /* in case num_dirs==0 */
    size_t seplen = strlen(PATH_SEPARATOR);

    for (i = 0; i < num_dirs; i++) {
      len += strlen(dirs[i]) + MAX(1, seplen);  /* separator or '\0' */
    }
    astr_minsize(&realfile, len);
    realfile.str[0] = '\0';

    for (i = 0; i < num_dirs; i++) {
      (void) mystrlcat(realfile.str, dirs[i], len);
      if (i < num_dirs) {
        (void) mystrlcat(realfile.str, PATH_SEPARATOR, len);
      }
    }
    return realfile.str;
  }

  for (i = 0; i < num_dirs; i++) {
    struct stat buf;            /* see if we can open the file or directory */
    size_t len = strlen(dirs[i]) + strlen(filename) + 2;

    astr_minsize(&realfile, len);
    my_snprintf(realfile.str, len, "%s/%s", dirs[i], filename);
    if (stat(realfile.str, &buf) == 0) {
      return realfile.str;
    }
  }

  freelog(LOG_VERBOSE, "Could not find readable file \"%s\" in data path.",
          filename);

  return NULL;
}

/**************************************************************************
  Compare modification times.
**************************************************************************/
static int compare_file_mtime_ptrs(const struct datafile_s * const *ppa,
                                   const struct datafile_s * const *ppb)
{
  return ((*ppa)->mtime < (*ppb)->mtime);
}

/**************************************************************************
  Compare names.
**************************************************************************/
static int compare_file_name_ptrs(const struct datafile_s * const *ppa,
                                  const struct datafile_s * const *ppb)
{
  return compare_strings((*ppa)->name, (*ppb)->name);
}

/**************************************************************************
  To be called on the return value of datafilelist_infix when it is no
  longer needed.
**************************************************************************/
void free_datafile_list(struct datafile_list *pdl)
{
  if (!pdl) {
    return;
  }

  datafile_list_iterate(pdl, pfile) {
    if (pfile) {
      if (pfile->name) {
        free(pfile->name);
      }
      if (pfile->fullname) {
        free(pfile->fullname);
      }
      free(pfile);
    }
  } datafile_list_iterate_end;
  datafile_list_free(pdl);
}

/**************************************************************************
  Search for filenames with the "infix" substring in the "subpath"
  subdirectory of the data path.
  "nodups" removes duplicate names.
  The returned list will be sorted by name first and modification time
  second. Returned "name"s will be truncated starting at the "infix"
  substring. The returned list must be freed.
**************************************************************************/
struct datafile_list *datafilelist_infix(const char *subpath,
                                         const char *infix, bool nodups)
{
  const char **dirs = get_data_dirs(NULL);
  int num_matches = 0;
  int dir_num;
  struct datafile_list *res;

  res = datafile_list_new();

  /* First assemble a full list of names. */
  for (dir_num = 0; dirs[dir_num]; dir_num++) {
    char path[PATH_MAX];
    DIR *dir;
    struct dirent *entry;

    if (subpath) {
      my_snprintf(path, sizeof(path), "%s/%s", dirs[dir_num], subpath);
    } else {
      sz_strlcpy(path, dirs[dir_num]);
    }

    /* Open the directory for reading. */
    dir = opendir(path);
    if (!dir) {
      continue;
    }

    /* Scan all entries in the directory. */
    while ((entry = readdir(dir))) {
      struct datafile_s *file;
      char *ptr;
      /* Strdup the entry so we can safely write to it. */
      char *filename = mystrdup(entry->d_name);

      /* Make sure the file name matches. */
      if ((ptr = strstr(filename, infix))) {
        struct stat buf;
        char *fullname;
        int len = strlen(path) + strlen(filename) + 2;

        fullname = (char*)wc_malloc(len);
        my_snprintf(fullname, len, "%s/%s", path, filename);

        if (stat(fullname, &buf) == 0) {
          file = (struct datafile_s *)wc_malloc(sizeof(struct datafile_s));

          /* Clip the suffix. */
          *ptr = '\0';

          file->name = filename; /* ownership passed */
          file->fullname = fullname; /* ownership passed */
          file->mtime = buf.st_mtime;

          datafile_list_append(res, file);
          num_matches++;
        } else {
        free(fullname);
          free(filename);
      }
      } else {
      free(filename);
    }
    }

    closedir(dir);
  }

  /* Sort the list by name. */
  datafile_list_sort(res, compare_file_name_ptrs);

  if (nodups) {
    char *name = (char*)"";

    datafile_list_iterate(res, pfile) {
      if (compare_strings(name, pfile->name) != 0) {
        name = pfile->name;
      } else {
        free(pfile->name);
        free(pfile->fullname);
        datafile_list_unlink(res, pfile);
        free(pfile);
      }
    } datafile_list_iterate_end;
  }

  /* Sort the list by last modification time. */
  datafile_list_sort(res, compare_file_mtime_ptrs);

  return res;
}



/***************************************************************************
  As datafilename(), above, except die with an appropriate log
  message if we can't find the file in the datapath.
***************************************************************************/
char *datafilename_required(const char *filename)
{
  char *dname;

  assert(filename!=NULL);
  dname = datafilename(filename);

  if (dname) {
    return dname;
  } else {
    freelog(LOG_ERROR, _("The data path may be set via"
                         " the environment variable WARCIV_PATH."));
    freelog(LOG_ERROR, _("Current data path is: \"%s\""), datafilename(NULL));
    freelog(LOG_FATAL,
                 _("The \"%s\" file is required ... aborting!"), filename);
    exit(EXIT_FAILURE);
  }
}

/***************************************************************************
  Setup for Native Language Support, if configured to use it.
  (Call this only once, or it may leak memory.)
***************************************************************************/
void init_nls(void)
{
  /*
   * Setup the cached locale numeric formatting information. Defaults
   * are as appropriate for the US.
   */
  grouping = mystrdup("\3");
  grouping_sep = mystrdup(",");

#ifdef ENABLE_NLS
#ifdef WIN32_NATIVE
  /* set LANG by hand if it is not set */
  if (!getenv("LANG")) {
    char langname[3];
    langname[0] = '\0';

    switch (PRIMARYLANGID(LANGIDFROMLCID(GetUserDefaultLCID()))) {
    case LANG_SPANISH:
      strcpy(langname,"es");
      break;
    case LANG_GERMAN:
      strcpy(langname,"de");
      break;
    case LANG_ENGLISH:
      strcpy(langname,"en");
      break;
    case LANG_FRENCH:
      strcpy(langname,"fr");
      break;
    case LANG_DUTCH:
      strcpy(langname,"nl");
      break;
    case LANG_POLISH:
      strcpy(langname,"pl");
      break;
    case LANG_HUNGARIAN:
      strcpy(langname,"hu");
      break;
    case LANG_NORWEGIAN:
      strcpy(langname,"no");
      break;
    case LANG_JAPANESE:
      strcpy(langname,"ja");
      break;
    case LANG_PORTUGUESE:
      strcpy(langname,"pt");
      break;
    case LANG_ROMANIAN:
      strcpy(langname,"ro");
      break;
    case LANG_RUSSIAN:
      strcpy(langname,"ru");
      break;
    }
    if (strlen(langname) == 0) {
      static char envstr[40];

      my_snprintf(envstr, sizeof(envstr), "LANG=%s", langname);
      putenv(envstr);
    }
  }
#endif

  (void) setlocale(LC_ALL, "");
  (void) bindtextdomain(PACKAGE, LOCALEDIR);
  (void) textdomain(PACKAGE);

  /* Don't touch the defaults when LC_NUMERIC == "C".
     This is intended to cater to the common case where:
       1) The user is from North America. ;-)
       2) The user has not set the proper environment variables.
          (Most applications are (unfortunately) US-centric
          by default, so why bother?)
     This would result in the "C" locale being used, with grouping ""
     and thousands_sep "", where we really want "\3" and ",". */

  if (strcmp(setlocale(LC_NUMERIC, NULL), "C") != 0) {
    struct lconv *lc = localeconv();

    if (lc->grouping[0] == '\0') {
      /* This actually indicates no grouping at all. */
      static char m = CHAR_MAX;
      grouping = &m;
    } else {
      size_t len;
      for (len = 0;
           lc->grouping[len] != '\0' && lc->grouping[len] != CHAR_MAX; len++) {
        /* nothing */
      }
      len++;
      free(grouping);
      grouping = (char*)wc_malloc(len);
      memcpy(grouping, lc->grouping, len);
    }
    free(grouping_sep);
    grouping_sep = mystrdup(lc->thousands_sep);
  }
#endif
}

/***************************************************************************
  If we have root privileges, die with an error.
  (Eg, for security reasons.)
  Param argv0 should be argv[0] or similar; fallback is
  used instead if argv0 is NULL.
  But don't die on systems where the user is always root...
  (a general test for this would be better).
  Doesn't use freelog() because gets called before logging is setup.
***************************************************************************/
void dont_run_as_root(const char *argv0, const char *fallback)
{
#if (defined(ALWAYS_ROOT) || defined(__EMX__) || defined(__BEOS__))
  return;
#else
  if (getuid()==0 || geteuid()==0) {
    wc_fprintf(stderr,
               _("%s: Fatal error: you're trying to run me as superuser!\n"),
               (argv0 ? argv0 : fallback ? fallback : "warciv"));
    wc_fprintf(stderr, _("Use a non-privileged account instead.\n"));
    exit(EXIT_FAILURE);
  }
#endif
}

/***************************************************************************
  Return a description string of the result.
  In English, form of description is suitable to substitute in, eg:
     prefix is <description>
  (N.B.: The description is always in English, but they have all been marked
   for translation.  If you want a localized version, use _() on the return.)
***************************************************************************/
const char *m_pre_description(enum m_pre_result result)
{
  static const char * const descriptions[] = {
    N_("exact match"),
    N_("only match"),
    N_("ambiguous"),
    N_("empty"),
    N_("too long"),
    N_("non-match")
  };
  assert(result >= 0 && result < ARRAY_SIZE(descriptions));
  return descriptions[result];
}

/***************************************************************************
  See match_prefix_full().
***************************************************************************/
enum m_pre_result match_prefix(m_pre_accessor_fn_t accessor_fn,
                               size_t n_names,
                               size_t max_len_name,
                               m_pre_strncmp_fn_t cmp_fn,
                               const char *prefix,
                               int *ind_result)
{
  return match_prefix_full(accessor_fn, n_names, max_len_name, cmp_fn,
                           prefix, ind_result, NULL, 0, NULL);
}

/***************************************************************************
  Given n names, with maximum length max_len_name, accessed by
  accessor_fn(0) to accessor_fn(n-1), look for matching prefix
  according to given comparison function.
  Returns type of match or fail, and for return <= M_PRE_AMBIGUOUS
  sets *ind_result with matching index (or for ambiguous, first match).
  If max_len_name==0, treat as no maximum.
  If the int array 'matches' is non-NULL, up to 'max_matches' ambiguous
  matching names indices will be inserted into it. If 'pnum_matches' is
  non-NULL, it will be set to the number of indices inserted into 'matches'.
***************************************************************************/
enum m_pre_result match_prefix_full(m_pre_accessor_fn_t accessor_fn,
                                    size_t n_names,
                                    size_t max_len_name,
                                    m_pre_strncmp_fn_t cmp_fn,
                                    const char *prefix,
                                    int *ind_result,
                                    int *matches,
                                    int max_matches,
                                    int *pnum_matches)
{
  unsigned int i;
  unsigned int len;
  int nmatches;

  len = strlen(prefix);
  if (len == 0) {
    return M_PRE_EMPTY;
  }
  if (len > max_len_name && max_len_name > 0) {
    return M_PRE_LONG;
  }

  nmatches = 0;
  for(i = 0; i < n_names; i++) {
    const char *name = accessor_fn(i);
    if (cmp_fn(name, prefix, len)==0) {
      if (strlen(name) == len) {
        *ind_result = i;
        return M_PRE_EXACT;
      }
      if (nmatches==0) {
        *ind_result = i;        /* first match */
      }
      if (matches != NULL && nmatches < max_matches) {
        matches[nmatches] = i;
      }
      nmatches++;
    }
  }

  if (nmatches == 1) {
    return M_PRE_ONLY;
  } else if (nmatches > 1) {
    if (pnum_matches != NULL) {
      *pnum_matches = MIN(max_matches, nmatches);
    }
    return M_PRE_AMBIGUOUS;
  } else {
    return M_PRE_FAIL;
  }
}

/***************************************************************************
 Return whether two vectors: vec1 and vec2 have common
 bits. I.e. (vec1 & vec2) != 0.

 Don't call this function directly, use BV_CHECK_MASK macro
 instead. Don't call this function with two different bitvectors.
***************************************************************************/
bool bv_check_mask(const unsigned char *vec1, const unsigned char *vec2,
                   size_t size1, size_t size2)
{
  size_t i;
  assert(size1 == size2);

  for (i = 0; i < size1; i++) {
    if ((vec1[0] & vec2[0]) != 0) {
      return TRUE;
    }
    vec1++;
    vec2++;
  }
  return FALSE;
}

bool bv_are_equal(const unsigned char *vec1, const unsigned char *vec2,
                  size_t size1, size_t size2)
{
  size_t i;
  assert(size1 == size2);

  for (i = 0; i < size1; i++) {
    if (vec1[0] != vec2[0]) {
      return FALSE;
    }
    vec1++;
    vec2++;
  }
  return TRUE;
}

/***************************************************************************
  Returns string which gives the multicast group IP address for finding
  servers on the LAN, as specified by $WARCIV_MULTICAST_GROUP.
  Gets value once, and then caches result.
***************************************************************************/
char *get_multicast_group(void)
{
  static bool init = FALSE;
  static char *group = NULL;
  static char *default_multicast_group = (char*)"225.0.0.1";

  if (!init) {
    char *env = getenv("WARCIV_MULTICAST_GROUP");
    if (env) {
      group = mystrdup(env);
    } else {
      group = mystrdup(default_multicast_group);
    }
    init = TRUE;
  }
  return group;
}

/***************************************************************************
  Interpret ~/ in filename as home dir
  New path is returned in buf of size buf_size
***************************************************************************/
void interpret_tilde(char* buf, size_t buf_size, const char* filename)
{
  if (filename[0] == '~' && filename[1] == '/') {
    my_snprintf(buf, buf_size, "%s/%s", user_home_dir(), filename + 2);
  } else if (filename[0] == '~' && filename[1] == '\0') {
    strncpy(buf, user_home_dir(), buf_size);
  } else  {
    strncpy(buf, filename, buf_size);
  }
}

/**************************************************************************
  If the directory "pathname" does not exist, recursively create all
  directories until it does.

  Returns FALSE if an error occurs. Use myerrno() and mystrerror() to
  find out what the problem was.
**************************************************************************/
bool make_dir(const char *pathname)
{
  char *dir;
  char file[PATH_MAX];
  char path[PATH_MAX];
  int rv = 0;
  struct stat buf;

  interpret_tilde(file, sizeof(file), pathname);
  path[0] = '\0';

#ifndef WIN32_NATIVE
  /* Ensure we are starting from the root in absolute pathnames. */
  if (file[0] == '/') {
    sz_strlcat(path, "/");
  }
#endif

  for (dir = strtok(file, "/"); dir; dir = strtok(NULL, "/")) {
    sz_strlcat(path, dir);

    rv = stat(path, &buf);
    if (rv == -1 && myerrno() != ENOENT) {
      break;
    }

    /* If a file with the same name as 'path' exists,
     * we want to call mkdir anyway so that it sets
     * errno for us. */

    if (rv == -1 || !S_ISDIR(buf.st_mode)) {
#ifdef WIN32_NATIVE
      rv = mkdir(path);
#else
      rv = mkdir(path, 0755);
#endif
      if (rv == -1) {
        break;
      }
    }

    sz_strlcat(path, "/");
  }

  return rv != -1;
}

/**************************************************************************
  Returns TRUE if the filename's path is absolute.
**************************************************************************/
bool path_is_absolute(const char *filename)
{
  if (!filename) {
    return FALSE;
  }

  if (filename[0] == '/') {
    return TRUE;
  }

  return FALSE;
}

/**************************************************************************
  Returns the file's size in bytes.
**************************************************************************/
long get_file_size (const char *pathname)
{
  struct stat buf;
  int ret;

  ret = stat (pathname, &buf);
  return ret == -1 ? -1 : buf.st_size;
}
/**************************************************************************
  Counts the number of occurrences of a character in a string.
**************************************************************************/
int strchrcount (const char *str, char c)
{
  int n = 0;
  for (; *str; ++str)
    if (*str == c)
      n++;
  return n;
}

/**************************************************************************
  Calls free on every string in the list and then frees the list
  itself with string_list_free.
**************************************************************************/
void string_list_free_all(struct string_list *sl)
{
  char *s;

  while (string_list_size(sl) > 0) {
    s = string_list_get(sl, 0);
    string_list_unlink(sl, s);
    if (s != NULL) {
      free(s);
    }
  }
  string_list_free(sl);
}



/**************************************************************************
  String vector: utility to make an easy resizable vector of string.
  Note that all string are duplicate in the vector.
**************************************************************************/
struct string_vector
{
  char **vec;
  size_t size;
};

/**************************************************************************
  Free a string.
**************************************************************************/
static void string_free(char *string)
{
  if (string) {
    free(string);
  }
}

/**************************************************************************
  Duplicate a string.
**************************************************************************/
static char *string_duplicate(const char *string)
{
  if (string) {
    return mystrdup(string);
  }
  return NULL;
}

/**************************************************************************
  Create a new string vector.
**************************************************************************/
struct string_vector *string_vector_new(void)
{
  struct string_vector *psv = (struct string_vector *)
      wc_malloc(sizeof(struct string_vector));

  psv->vec = NULL;
  psv->size = 0;

  return psv;
}

/**************************************************************************
  Destroy a string vector.
**************************************************************************/
void string_vector_destroy(struct string_vector *psv)
{
  string_vector_remove_all(psv);
  free(psv);
}

/**************************************************************************
  Set the size of the vector.
**************************************************************************/
void string_vector_reserve(struct string_vector *psv, size_t reserve)
{
  if (reserve == psv->size) {
    return;
  } else if (reserve == 0) {
    string_vector_remove_all(psv);
    return;
  } else if (!psv->vec) {
    /* Initial reserve */
    psv->vec = (char**)wc_calloc(reserve, sizeof(char *));
  } else if (reserve > psv->size) {
    /* Expand the vector. */
    psv->vec = (char**)wc_realloc(psv->vec, reserve * sizeof(char *));
    memset(psv->vec + psv->size, 0, (reserve - psv->size) * sizeof(char *));
  } else {
    /* Shrink the vector: free the extra strings. */
    size_t i;

    for (i = psv->size - 1; i >= reserve; i--) {
      string_free(psv->vec[i]);
    }
    psv->vec = (char**)wc_realloc(psv->vec, reserve * sizeof(char *));
  }
  psv->size = reserve;
}

/**************************************************************************
  Stores the string vector from a normal vector. If size == -1, it will
  assume it is a NULL terminated vector.
**************************************************************************/
void string_vector_store(struct string_vector *psv,
                         const char *const *vec, size_t size)
{
  if (size == (size_t)-1) {
    string_vector_remove_all(psv);
    for (; *vec; vec++) {
      string_vector_append(psv, *vec);
    }
  } else {
    size_t i;

    string_vector_reserve(psv, size);
    for (i = 0; i < size; i++, vec++) {
      string_vector_set(psv, i, *vec);
    }
  }
}

/**************************************************************************
  Insert a string at the start of the vector.
**************************************************************************/
void string_vector_prepend(struct string_vector *psv, const char *string)
{
  string_vector_reserve(psv, psv->size + 1);
  memmove(psv->vec + 1, psv->vec, (psv->size - 1) * sizeof(char *));
  psv->vec[0] = string_duplicate(string);
}

/**************************************************************************
  Insert a string at the end of the vector.
**************************************************************************/
void string_vector_append(struct string_vector *psv, const char *string)
{
  string_vector_reserve(psv, psv->size + 1);
  psv->vec[psv->size - 1] = string_duplicate(string);
}

/**************************************************************************
  Insert a string at the index of the vector.
**************************************************************************/
void string_vector_insert(struct string_vector *psv,
                          size_t index, const char *string)
{
  if (index <= 0) {
    string_vector_prepend(psv, string);
  } else if (index >= psv->size) {
    string_vector_append(psv, string);
  } else {
    string_vector_reserve(psv, psv->size + 1);
    memmove(psv->vec + index + 1, psv->vec + index,
            (psv->size - index - 1) * sizeof(char *));
    psv->vec[index] = string_duplicate(string);
  }
}

/**************************************************************************
  Replace a string at the index of the vector.
  Returns TRUE if the element has been really set.
**************************************************************************/
bool string_vector_set(struct string_vector *psv,
                       size_t index, const char *string)
{
  if (string_vector_index_valid(psv, index)) {
    string_free(psv->vec[index]);
    psv->vec[index] = string_duplicate(string);
    return TRUE;
  }
  return FALSE;
}

/**************************************************************************
  Remove the string at the index from the vector.
  Returns TRUE if the element has been really removed.
**************************************************************************/
bool string_vector_remove(struct string_vector *psv, size_t index)
{
  if (!string_vector_index_valid(psv, index)) {
    return FALSE;
  }

  if (psv->size == 1) {
    /* It is the last. */
    string_vector_remove_all(psv);
    return TRUE;
  }

  string_free(psv->vec[index]);
  memmove(psv->vec + index, psv->vec + index + 1,
          (psv->size - index - 1) * sizeof(char *));
  psv->vec[psv->size - 1] = NULL; /* Do not attempt to free this data. */
  string_vector_reserve(psv, psv->size - 1);

  return TRUE;
}

/**************************************************************************
  Remove all strings from the vector.
**************************************************************************/
void string_vector_remove_all(struct string_vector *psv)
{
  size_t i;
  char **p;

  if (!psv->vec) {
    return;
  }

  for (i = 0, p = psv->vec; i < psv->size; i++, p++) {
    string_free(*p);
  }
  free(psv->vec);
  psv->vec = NULL;
  psv->size = 0;
}

/**************************************************************************
  Remove all empty strings from the vector and removes all leading and
  trailing spaces.
**************************************************************************/
void string_vector_remove_empty(struct string_vector *psv)
{
  size_t i;
  char *str;

  if (!psv->vec) {
    return;
  }

  for (i = 0; i < psv->size;) {
    str = psv->vec[i];

    if (!str) {
      string_vector_remove(psv, i);
      continue;
    }

    remove_leading_trailing_spaces(str);
    if (str[0] == '\0') {
      string_vector_remove(psv, i);
      continue;
    }

    i++;
  }
}

/**************************************************************************
  Copy a string vector.
**************************************************************************/
void string_vector_copy(struct string_vector *dest,
                        const struct string_vector *src)
{
  size_t i;
  char **p;
  char *const *l;

  if (!src->vec) {
    string_vector_remove_all(dest);
    return;
  }

  string_vector_reserve(dest, src->size);
  for (i = 0, p = dest->vec, l = src->vec; i < dest->size; i++, p++, l++) {
    string_free(*p);
    *p = string_duplicate(*l);
  }
}

/**************************************************************************
  Returns the size of the vector.
**************************************************************************/
size_t string_vector_size(const struct string_vector *psv)
{
  return psv->size;
}

/**************************************************************************
  Returns the datas of the vector.
**************************************************************************/
const char *const *string_vector_data(const struct string_vector *psv)
{
  return (const char **) psv->vec;
}

/**************************************************************************
  Returns TRUE if the index is valid.
**************************************************************************/
bool string_vector_index_valid(const struct string_vector *psv, size_t index)
{
  return index >= 0 && index < psv->size;
}

/**************************************************************************
  Returns the string at the index of the vector.
**************************************************************************/
const char *string_vector_get(const struct string_vector *psv, size_t index)
{
  return string_vector_index_valid(psv, index) ? psv->vec[index] : NULL;
}

/**************************************************************************
  String vector iterator, derived from generic iterator. See also struct
  iterator in iterator.h for more informations.
**************************************************************************/
struct string_iter
{
  struct iterator vtable;
  union string_iter_u {
    struct string_vector *psv;
    const struct string_vector *pcsv;
  } u;
  size_t index;
  bool removed;
};

#define STRING_ITER(p) ((struct string_iter *)(p))

/**************************************************************************
  Returns the size of the string vector iterator.
**************************************************************************/
size_t string_iter_sizeof(void)
{
  return sizeof(struct string_iter);
}

/**************************************************************************
  If the previous iterator has been removed with string_iter_remove(),
  we just point to the next string, which have the same index.
**************************************************************************/
static void string_iter_next(struct iterator *string_iter)
{
  struct string_iter *iter = STRING_ITER(string_iter);

  if (iter->removed) {
    iter->removed = FALSE;
  } else {
    iter->index++;
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void *string_iter_get(const struct iterator *string_iter)
{
  struct string_iter *iter = STRING_ITER(string_iter);

  return (void *) string_vector_get(iter->u.pcsv, iter->index);
}

/**************************************************************************
  Returns TRUE if the iterator is valid.
**************************************************************************/
static bool string_iter_valid(const struct iterator *string_iter)
{
  struct string_iter *iter = STRING_ITER(string_iter);

  return string_vector_index_valid(iter->u.pcsv, iter->index);
}

/**************************************************************************
  Initialize a vector for string iteration.
**************************************************************************/
struct iterator *string_iter_init(struct string_iter *iter,
                                  const struct string_vector *psv)
{
  iter->vtable.next = string_iter_next;
  iter->vtable.get = string_iter_get;
  iter->vtable.valid = string_iter_valid;
  iter->u.pcsv = psv;
  iter->index = 0;
  iter->removed = FALSE;

  return ITERATOR(iter);
}

/**************************************************************************
  Returns the string of the iterator.
**************************************************************************/
const char *string_iter_get_string(const struct iterator *string_iter)
{
  struct string_iter *iter = STRING_ITER(string_iter);

  return string_vector_get(iter->u.pcsv, iter->index);
}

/**************************************************************************
  Returns the index of the iterator.
**************************************************************************/
size_t string_iter_get_index(const struct iterator *string_iter)
{
  return STRING_ITER(string_iter)->index;
}

/**************************************************************************
  Insert a string before the iterator.
**************************************************************************/
void string_iter_insert_before(const struct iterator *string_iter,
                               const char *string)
{
  struct string_iter *iter = STRING_ITER(string_iter);

  string_vector_insert(iter->u.psv, iter->index, string);
}

/**************************************************************************
  Insert a string after the iterator.
**************************************************************************/
void string_iter_insert_after(const struct iterator *string_iter,
                              const char *string)
{
  struct string_iter *iter = STRING_ITER(string_iter);

  string_vector_insert(iter->u.psv, iter->index + 1, string);
}

/**************************************************************************
  Initialize a vector for string iteration.
**************************************************************************/
void string_iter_set(struct iterator *string_iter, const char *string)
{
  struct string_iter *iter = STRING_ITER(string_iter);

  string_vector_set(iter->u.psv, iter->index, string);
}

/**************************************************************************
  Initialize a vector for string iteration.
**************************************************************************/
void string_iter_remove(struct iterator *string_iter)
{
  struct string_iter *iter = STRING_ITER(string_iter);

  if (string_vector_remove(iter->u.psv, iter->index)) {
    /* Prevent to jump over a string in the iteration. */
    iter->removed = TRUE;
  }
}
