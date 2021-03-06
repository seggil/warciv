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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "wc_iconv.hh"
#include "wc_intl.hh"
#include "mem.hh"
#include "shared.hh"
#include "support.hh"
#include "time.h"

#include "log.hh"

char const *loglevelstr[5] = {"FATAL","ERROR","NORMAL","VERBOSE","DEBUG"};

static const char *log_filename;
static log_callback_fn log_callback;
static const char *l_file;
static int l_line;
static const char *l_fncname;

int logd_init_counter = 1;
int wc_log_level;

struct logd_fileinfo {
  char *name;
  int min;
  int max;
};
static int logd_num_files;
static struct logd_fileinfo *logd_files;

/**************************************************************************
level_str should be either "0", "1", "2", "3", "4" or
"4:filename" or "4:file1:file2" or "4:filename,100,200" etc

If everything goes ok, returns the level.
If there was a parsing problem, prints to stderr, and returns -1.

Also sets up the logd_files data structure and increments
logd_init_counter.  Does _not_ set wc_log_level.
**************************************************************************/
int log_parse_level_str(const char *level_str)
{
  const char *c;
  const char *tok;
  char *dup;
  int n = 0;                    /* number of filenames */
  int i;
  int level;

#ifdef DEBUG
  const int max_level = LOG_DEBUG;
#else
  const int max_level = LOG_DEBUG;
  /* const int max_level = LOG_VERBOSE; */
#endif

  /* re-entrant: */
  logd_num_files = 0;
  if (logd_files) {
    free(logd_files);
    logd_files = NULL;
  }
  logd_init_counter++;

  c = level_str;
  n = 0;
  while((c = strchr(c, ':'))) {
    c++;
    n++;
  }
  if (n == 0) {
    if (sscanf(level_str, "%d", &level) != 1) {
      wc_fprintf(stderr, _("Bad log level \"%s\".\n"), level_str);
      return -1;
    }
    if (level >= LOG_FATAL && level <= max_level) {
      return level;
    } else {
      wc_fprintf(stderr, _("Bad log level %d in \"%s\".\n"), level, level_str);
      if (level == LOG_DEBUG && max_level < LOG_DEBUG) {
        wc_fprintf(stderr, _("Warciv must be compiled with the DEBUG flag"
                             " to use debug level %d.\n"), LOG_DEBUG);
      }
      return -1;
    }
  }

  c = level_str;
  if (c[0] == ('0' + LOG_DEBUG) && c[1] == ':') {
    level = LOG_DEBUG;
    if (max_level < LOG_DEBUG) {
      wc_fprintf(stderr, _("Warciv must be compiled with the DEBUG flag"
                           " to use debug level %d.\n"), LOG_DEBUG);
      return -1;
    }
  } else {
    wc_fprintf(stderr, _("Badly formed log level argument \"%s\".\n"),
               level_str);
    return -1;
  }
  logd_num_files = n;
  logd_files = (struct logd_fileinfo *)
    wc_malloc(n * sizeof(struct logd_fileinfo));

  dup = mystrdup(c+2);
  tok = strtok(dup, ":");

  if (!tok) {
    wc_fprintf(stderr, _("Badly formed log level argument \"%s\".\n"),
               level_str);
    level = -1;
    goto out;
  }
  i = 0;
  do {
    char *d = (char*)strchr(tok, ',');

    logd_files[i].min = logd_files[i].max = 0;
    if (d) {
      char *pc = d + 1;

      d[0] = '\0';
      d = strchr(d + 1, ',');
      if (d && *pc != '\0' && d[1] != '\0') {
        d[0] = '\0';
        if (sscanf(pc, "%d", &logd_files[i].min) != 1) {
          wc_fprintf(stderr, _("Not an integer: '%s'\n"), pc);
          level = -1;
          goto out;
        }
        if (sscanf(d + 1, "%d", &logd_files[i].max) != 1) {
          wc_fprintf(stderr, _("Not an integer: '%s'\n"), d + 1);
          level = -1;
          goto out;
        }
      }
    }
    if(strlen(tok)==0) {
      wc_fprintf(stderr, _("Empty filename in log level argument \"%s\".\n"),
                 level_str);
      level = -1;
      goto out;
    }
    logd_files[i].name = mystrdup(tok);
    i++;
    tok = strtok(NULL, ":");
  } while(tok);

  if (i!=logd_num_files) {
    wc_fprintf(stderr, _("Badly formed log level argument \"%s\".\n"),
               level_str);
    level = -1;
    goto out;
  }

 out:
  free(dup);
  return level;
}

/**************************************************************************
  Initialise the log module. Either 'filename' or 'callback' may be NULL.
  If both are NULL, print to stderr. If both are non-NULL, both callback,
  and fprintf to file.
**************************************************************************/
void log_init(const char *filename, int initial_level,
              log_callback_fn callback)
{
  wc_log_level = initial_level;
  if (filename && strlen(filename) > 0) {
    log_filename = filename;
  } else {
    log_filename = NULL;
  }
  log_callback = callback;
  freelog(LOG_VERBOSE, "log started");
  freelog(LOG_DEBUG, "LOG_DEBUG test");
}

/**************************************************************************
Adjust the logging level after initial log_init().
**************************************************************************/
void log_set_level(int level)
{
  wc_log_level=level;
}

/**************************************************************************
Adjust the callback function after initial log_init().
**************************************************************************/
void log_set_callback(log_callback_fn callback)
{
  log_callback=callback;
}

/**************************************************************************
  Return an updated struct logdebug_afile_info:
**************************************************************************/
struct logdebug_afile_info logdebug_update(const char *file)
{
  struct logdebug_afile_info ret;
  int i;

  ret.tthis = 1;
  ret.min = 0;
  ret.max = 0;
  if (logd_num_files==0) {
    return ret;
  }
  ret.tthis = 0;
  for (i = 0; i < logd_num_files; i++) {
    if((strstr(file, logd_files[i].name))) {
      ret.tthis = 1;
      ret.min = logd_files[i].min;
      ret.max = logd_files[i].max;
      return ret;
    }
  }
  return ret;
}

/**************************************************************************
Unconditionally print a simple string.
Let the callback do its own level formating and add a '\n' if it wants.
**************************************************************************/
static void log_write(FILE *fs, int level, char *message)
{
  static time_t ltime;
  char *timebuf;

  if ((!log_filename) && log_callback) {
    log_callback(level, message);
  }
  if (log_filename || (!log_callback)) {
    time(&ltime);
    timebuf = ctime(&ltime);
    timebuf[24]='\0';

    wc_fprintf(fs, "%-8s: %-26s: %-12s: line %-5i: %-25s - %s\n",
               loglevelstr[level],timebuf,l_file, l_line, l_fncname, message);
    fflush(fs);
  }
}

/**************************************************************************
Print a log message.
Only prints if level <= wc_log_level.
For repeat message, may wait and print instead
"last message repeated ..." at some later time.
Calls log_callback if non-null, else prints to stderr.
**************************************************************************/
#define MAX_LEN_LOG_LINE 512
void vreal_freelog(int level, const char *message, va_list ap)
{
  static char bufbuf[2][MAX_LEN_LOG_LINE];
  char buf[MAX_LEN_LOG_LINE];
  static bool bufbuf1 = FALSE;
  static unsigned int repeated=0; /* total times current message repeated */
  static unsigned int next=2;   /* next total to print update */
  static unsigned int prev=0;   /* total on last update */
  static int prev_level=-1;     /* only count as repeat if same level  */

  if(level<=wc_log_level) {
    FILE *fs;

    if (log_filename) {
      if(!(fs=fopen(log_filename, "a"))) {
        wc_fprintf(stderr, _("Couldn't open logfile: %s for appending.\n"),
                   log_filename);
        exit(EXIT_FAILURE);
      }
    } else {
      fs = stderr;
    }

    my_vsnprintf(bufbuf1 ? bufbuf[1] : bufbuf[0], MAX_LEN_LOG_LINE, message, ap);

    if(level==prev_level && 0==strncmp(bufbuf[0],bufbuf[1],MAX_LEN_LOG_LINE-1)){
      repeated++;
      if(repeated==next){
        my_snprintf(buf, sizeof(buf),
                    PL_("last message repeated %d time",
                        "last message repeated %d times",
                        repeated-prev), repeated-prev);
        if (repeated>2) {
          cat_snprintf(buf, sizeof(buf),
                       PL_(" (total %d repeat)", " (total %d repeats)", repeated),
                       repeated);
        }
        log_write(fs, prev_level, buf);
        prev=repeated;
        next*=2;
      }
    }else{
      if(repeated>0 && repeated!=prev){
        if(repeated==1) {
          /* just repeat the previous message: */
          log_write(fs, prev_level, bufbuf1 ? bufbuf[0] : bufbuf[1]);
        } else {
          my_snprintf(buf, sizeof(buf),
                      PL_("last message repeated %d time",
                          "last message repeated %d times", repeated - prev),
                      repeated - prev);
          if (repeated > 2) {
            cat_snprintf(buf, sizeof(buf),
                         PL_(" (total %d repeat)", " (total %d repeats)",
                             repeated),
                         repeated);
          }
          log_write(fs, prev_level, buf);
        }
      }
      prev_level=level;
      repeated=0;
      next=2;
      prev=0;
      log_write(fs, level, bufbuf1 ? bufbuf[1] : bufbuf[0]);
    }
    bufbuf1 = !bufbuf1;
    fflush(fs);
    if (log_filename) {
      fclose(fs);
    }
  }
}

/**************************************************************************
  ...
**************************************************************************/
void real_freelog(int level, const char *file, int line, const char *fncname, const char *message, ...)
{
  l_file = file;
  l_line = line;
  l_fncname = fncname;
  va_list ap;
  va_start(ap, message);
  vreal_freelog(level, message, ap);
  va_end(ap);
}
