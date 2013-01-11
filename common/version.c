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
#  include "../config.h"
#endif

#include "wc_intl.h"
#include "shared.h"
#include "support.h"

#include "version.h"

#ifdef SVNREV
#  include "wc_svnrev_gen.h"
#endif /* SVNREV */


/**********************************************************************
  Returns a static string containing the full warclient project version
  information, including svn revision information if available, and
  correctly modified for the client and server.

  The returned string should generally be added after the string from
  warciv_name_version().
***********************************************************************/
const char *warclient_name_version(void)
{
  static char buf[256] = "";
  const char *progname = "Warclient";

#if defined(SVNREV) && !defined(WC_SVNREV_OFF)
  my_snprintf(buf, sizeof(buf), "%s %s (%s)", progname,
              WARCLIENT_VERSION_STRING, fc_svn_revision());
#else
  my_snprintf(buf, sizeof(buf), "%s %s ", progname,
              WARCLIENT_VERSION_STRING);
#endif

  return buf;
}

/**********************************************************************
  ...
***********************************************************************/
const char *warciv_name_version(void)
{
  static char msgbuf[128];

#if IS_BETA_VERSION
  my_snprintf(msgbuf, sizeof (msgbuf), _("Warciv version %s %s"),
              VERSION_STRING, _("(beta version)"));
#else
  my_snprintf(msgbuf, sizeof (msgbuf), _("Warciv version %s"),
              VERSION_STRING);
#endif

  return msgbuf;
}

/**********************************************************************
  ...
***********************************************************************/
const char *word_version(void)
{
#if IS_BETA_VERSION
  return _("betatest version ");
#else
  return _("version ");
#endif
}

/**********************************************************************
  Returns string with svn revision information if it is possible to
  determine. Can return also some fallback string or even NULL.
***********************************************************************/
const char *wc_svn_revision(void)
{
#if 0
//#if defined(SVNREV) && !defined(WC_SVNREV_OFF)
  return WC_SVNREV; /* Either revision, or modified revision */
#else  /* WC_SVNREV_OFF */
  return NULL;
#endif /* WC_SVNREV_OFF */
}

/**********************************************************************
  Return the BETA message.
  If returns NULL, not a beta version.
***********************************************************************/
const char *beta_message(void)
{
#if IS_BETA_VERSION
  static char msgbuf[128];
  static const char *month[] =
  {
    NULL,
    N_("January"),
    N_("February"),
    N_("March"),
    N_("April"),
    N_("May"),
    N_("June"),
    N_("July"),
    N_("August"),
    N_("September"),
    N_("October"),
    N_("November"),
    N_("December")
  };
  my_snprintf (msgbuf, sizeof (msgbuf),
               _("THIS IS A BETA VERSION\n"
                 "Warciv %s will be released in\n"
                 "%s, at %s"), /* No full stop here since it would be
                                  immediately following a URL, which
                                  would only cause confusion. */
               NEXT_STABLE_VERSION,
               _(NEXT_RELEASE_MONTH),
               WEBSITE_URL);
  return msgbuf;
#else
  return NULL;
#endif
}

/***************************************************************************
  Return the Warciv motto.
  (The motto is common code:
   only one instance of the string in the source;
   only one time gettext needs to translate it. --jjm)
***************************************************************************/
const char *warciv_motto(void)
{
  return _("'Cause civilization should be free!");
}
