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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32_NATIVE
#  include <signal.h>
#  include <unistd.h>
#endif

#ifdef GENERATING_MAC  /* mac header(s) */
#  include <Controls.h>
#  include <Dialogs.h>
#endif

#ifdef WIN32_NATIVE
#  include <winsock2.h>
#  include <windows.h>
#endif

#include "wc_iconv.hh"
#include "wc_intl.hh"
#include "log.hh"
#include "shared.hh"
#include "support.hh"
#include "version.hh"

#include "console.hh"
#include "meta.hh"
#include "sernet.hh"
#include "srv_main.hh"

#include "civserver.hh"

#ifdef GENERATING_MAC
static void Mac_options(int argc);  /* don't need argv */
#endif

/**************************************************************************
  ...
**************************************************************************/
#ifndef WIN32_NATIVE
static void signal_handler(int sig)
{
  /* Exit on SIGQUIT. */
  if (sig == SIGQUIT) {
    fprintf(stderr, "\nServer quitting on SIGQUIT.\n");

    /* Because the signal may have interrupted arbitrary code,
     * we use _exit here instead of exit so that we don't
     * accidentally call any "unsafe" functions here (see the
     * manual page for the signal function). */
    _exit(EXIT_SUCCESS);
  }

  if (sig == SIGPIPE) {
    if (signal(SIGPIPE, signal_handler) == SIG_ERR) {
      fprintf(stderr, "\nFailed to reset SIGPIPE handler"
              " while handling SIGPIPE.\n");
      _exit(EXIT_FAILURE);
    }
    return;
  }
}
#endif

/**************************************************************************
 Entry point for Warciv server.  Basically, does two things:
  1. Parses command-line arguments (possibly dialog, on mac).
  2. Calls the main server-loop routine.
**************************************************************************/
int main(int argc, char *argv[])
{
  int inx;
  bool showhelp = FALSE;
  bool showvers = FALSE;
  char *option = NULL;

  /* Load win32 post-crash debugger */
#ifdef WIN32_NATIVE
# ifndef NDEBUG
  if (LoadLibrary("exchndl.dll") == NULL) {
#  ifdef DEBUG
    fprintf(stderr, "exchndl.dll could not be loaded, no crash debugger\n");
#  endif
  }
# endif
#endif

  /* initialize server */
  server_init();

  /* parse command-line arguments... */

#ifdef GENERATING_MAC
  Mac_options(argc);
#endif
  /* no  we don't use GNU's getopt or even the "standard" getopt */
  /* yes we do have reasons ;)                                   */
  inx = 1;
  while (inx < argc) {
    if ((option = get_option("--file", argv, &inx, argc))) {
      sz_strlcpy(server_arg.load_filename, option);
    } else if (is_option("--help", argv[inx])) {
      showhelp = TRUE;
      break;
    } else if (is_option("--Hack-off", argv[inx])) {
      server_arg.hack_request_disabled = TRUE;
    } else if ((option = get_option("--log", argv, &inx, argc)))
      server_arg.log_filename = option;
    else if ((option = get_option("--gamelog", argv, &inx, argc)))
      server_arg.gamelog_filename = option;
    else if (is_option("--meta", argv[inx]))
      server_arg.metaserver_no_send = FALSE;
    else if ((option = get_option("--Metaserver", argv, &inx, argc))) {
      sz_strlcpy(server_arg.metaserver_addr, argv[inx]);
      server_arg.metaserver_no_send = FALSE;      /* --Metaserver implies --meta */
    } else if ((option = get_option("--xHost", argv, &inx, argc))) {
            sz_strlcpy(server_arg.metasendhost, argv[inx]); /* -- hostname to send to metaserver */
    } else if (is_option("--no-dns-lookup", argv[inx])) {
      server_arg.no_dns_lookup = TRUE;
    } else if ((option = get_option("--port", argv, &inx, argc))) {
      if (sscanf(option, "%d", &server_arg.port) != 1) {
        showhelp = TRUE;
        break;
      }
    } else if ((option = get_option("--bind", argv, &inx, argc))) {
      server_arg.bind_addr = option;
    } else if ((option = get_option("--read", argv, &inx, argc)))
      server_arg.script_filename = option;
    else if ((option = get_option("--quitidle", argv, &inx, argc))) {
      if (sscanf(option, "%d", &server_arg.quitidle) != 1) {
        showhelp = TRUE;
        break;
      }
    } else if (is_option("--exit-on-end", argv[inx])) {
      server_arg.exit_on_end = TRUE;
    } else if ((option = get_option("--debug", argv, &inx, argc))) {
      server_arg.loglevel = log_parse_level_str(option);
      if (server_arg.loglevel == -1) {
        server_arg.loglevel = LOG_NORMAL;
        showhelp = TRUE;
        break;
      }
#ifdef HAVE_MYSQL
    } else if (is_option("--auth", argv[inx])) {
      server_arg.auth.enabled = TRUE;
    } else if (is_option("--Guests", argv[inx])) {
      server_arg.auth.allow_guests = TRUE;
    } else if (is_option("--Newusers", argv[inx])) {
      server_arg.auth.allow_newusers = TRUE;
#endif
    } else if (is_option("--Ppm", argv[inx])) {
      server_arg.save_ppm = TRUE;
    } else if ((option = get_option("--Serverid", argv, &inx, argc))) {
      sz_strlcpy(server_arg.serverid, option);
    } else if ((option = get_option("--saves", argv, &inx, argc))) {
      server_arg.saves_pathname = option;
    } else if ((option = get_option("--Require", argv, &inx, argc))) {
      sz_strlcpy(server_arg.required_cap, option);
    } else if (is_option("--version", argv[inx]))
      showvers = TRUE;
    else {
      wc_fprintf(stderr, _("Error: unknown option '%s'\n"), argv[inx]);
      showhelp = TRUE;
      break;
    }
    inx++;
  }

  if (showvers && !showhelp) {
    wc_fprintf(stderr, "%s %s server\n", warciv_name_version(),
               warclient_name_version());
    exit(EXIT_SUCCESS);
  }
  con_write(C_VERSION, _("This is the server for %s %s"),
            warciv_name_version(), warclient_name_version());
  con_write(C_COMMENT, _("You can learn a lot about Warciv at %s"),
            WEBSITE_URL);

  if (showhelp) {
    wc_fprintf(stderr,
               _("Usage: %s [option ...]\nValid options are:\n"), argv[0]);
#ifdef HAVE_MYSQL
    wc_fprintf(stderr, _("  -a  --auth\t\tEnable server authentication.\n"));
    wc_fprintf(stderr, _("  -G  --Guests\t\tAllow guests to "
                         "login if auth is enabled.\n"));
    wc_fprintf(stderr, _("  -N  --Newusers\tAllow new users to "
                         "login if auth is enabled.\n"));
#endif
    wc_fprintf(stderr, _("  -b  --bind ADDR\tListen for clients on ADDR\n"));
#ifdef DEBUG
    wc_fprintf(stderr, _("  -d, --debug NUM\tSet debug log level (0 to 4,"
                      " or 4:file1,min,max:...)\n"));
#else
    wc_fprintf(stderr,
               _("  -d, --debug NUM\tSet debug log level (0 to 3)\n"));
#endif
    wc_fprintf(stderr, _("  -f, --file FILE\tLoad saved game FILE\n"));
    wc_fprintf(stderr,
               _("  -g, --gamelog FILE\tUse FILE as game logfile\n"));
    wc_fprintf(stderr,
               _("  -h, --help\t\tPrint a summary of the options\n"));
    wc_fprintf(stderr, _("  -H, --Hack-off\tDisable hack challenge\n"));
    wc_fprintf(stderr, _("  -l, --log FILE\tUse FILE as logfile\n"));
    wc_fprintf(stderr, _("  -m, --meta\t\tNotify metaserver and "
                         "send server's info\n"));
    wc_fprintf(stderr, _("  -M, --Metaserver ADDR\tSet ADDR "
                         "as metaserver address\n"));
    wc_fprintf(stderr, _("  -x, --xHost <fqdn hostname>\tSend this hostname "
                    "to Metaserver\n"));
    wc_fprintf(stderr, _("  -n, --no-dns-lookup\tDo not lookup "
                         "the hostname of new connections\n"));

    wc_fprintf(stderr, _("  -p, --port PORT\tListen for clients on "
                         "port PORT\n"));
    wc_fprintf(stderr, _("  -q, --quitidle TIME\tQuit if no players "
                         "for TIME seconds\n"));
    wc_fprintf(stderr, _("  -e, --exit-on-end\t"
                      "When a game ends, exit instead of restarting\n"));
    wc_fprintf(stderr,
               _("  -s, --saves DIR\tSave games to directory DIR\n"));
    wc_fprintf(stderr,
               _("  -S, --Serverid ID\tSets the server id to ID\n"));
    wc_fprintf(stderr,
             _("  -P, --Ppm\t\tSave ppms of the map when saving the game.\n"));
    wc_fprintf(stderr, _("  -r, --read FILE\tRead startup script FILE\n"));
    wc_fprintf(stderr, _("  -R, --Require CAP\tThe users will need this "
                      "capability(ies) to be able to play.\n"));
    wc_fprintf(stderr, _("  -v, --version\t\tPrint the version number\n"));
    wc_fprintf(stderr, _("Report bugs at %s.\n"), BUG_URL);
    exit(EXIT_SUCCESS);
  }

  /* disallow running as root -- too dangerous */
  dont_run_as_root(argv[0], "warciv_server");

#ifndef WIN32_NATIVE
  if (signal(SIGQUIT, signal_handler) == SIG_ERR) {
    wc_fprintf(stderr, _("Failed to install SIGQUIT handler: %s\n"),
               mystrerror(myerrno()));
    exit(EXIT_FAILURE);
  }

  /* Ignore SIGPIPE, the error is handled by the return value
   * of the write call. */
  if (signal(SIGPIPE, signal_handler) == SIG_ERR) {
    wc_fprintf(stderr, _("Failed to ignore SIGPIPE: %s\n"),
               mystrerror(myerrno()));
    exit(EXIT_FAILURE);
  }
#endif

  /* have arguments, call the main server loop... */
  server_main();

  /* Technically, we won't ever get here. We exit via server_quit. */

  /* done */
  exit(EXIT_SUCCESS);
}

#ifdef GENERATING_MAC
/*************************************************************************
  generate an option dialog if no options have been passed in
*************************************************************************/
static void Mac_options(int argc)
{
#define HARDCODED_OPT
  /*temporary hack since GetNewDialog() doesn't want to work*/
#ifdef HARDCODED_OPT
  server_arg.log_filename="log.out";
  server_arg.loglevel=LOG_DEBUG;
#else
  if (argc == 0)
  {
    OSErr err;
    DialogPtr  optptr;
    Ptr storage;
    Handle ditl;
    Handle dlog;
    short the_type;
    Handle the_handle;
    Rect the_rect;
    short the_item, old_item=16;
    int done = false;
    Str255 the_string;
    /*load/init the stuff for the dialog*/
    storage =NewPtr(sizeof(DialogRecord));
    if (storage == 0)
    {
      exit(EXIT_FAILURE);
    }
    ditl=Get1Resource('DITL',200);
    if ((ditl == 0)||(ResError()))
    {
      exit(EXIT_FAILURE);
    }
    dlog=Get1Resource('DLOG',200);
    if ((dlog == 0)||(ResError()))
    {
      exit(EXIT_FAILURE);
    }
    /*make the dialog*/
    optptr=GetNewDialog(200, storage, (WindowPtr)-1L);
    /*setup the dialog*/
    err=SetDialogDefaultItem(optptr, 1);
    if (err != 0)
    {
      exit(EXIT_FAILURE);
    }
    /*insert default highlight draw code?*/
    err=SetDialogCancelItem(optptr, 2);
    if (err != 0)
    {
      exit(EXIT_FAILURE);
    }
    err=SetDialogTracksCursor(optptr, true);
    if (err != 0)
    {
      exit(EXIT_FAILURE);
    }
    GetDItem(optptr, 16/*normal radio button*/, &the_type, &the_handle, &the_rect);
    SetCtlValue((ControlHandle)the_handle, true);

    while(!done)/*loop*/
    {
      ModalDialog(0L, &the_item);/*don't feed 0 where a upp is expected?*/
        /*old book sugests using OL(NIL) as the first argument, but
        It doesn't include anything about UPPs either, so...*/
      switch (the_item)
      {
        case 1:
          done = true;
        break;
        case 2:
          exit(EXIT_SUCCESS);
        break;
        case 13:
          GetDItem(optptr, 13, &the_type, &the_handle, &the_rect);
          server_arg.metaserver_no_send=GetCtlValue((ControlHandle)the_handle);
          SetCtlValue((ControlHandle)the_handle, !server_arg.metaserver_no_send);
        break;
        case 15:
        case 16:
        case 17:
          GetDItem(optptr, old_item, &the_type, &the_handle, &the_rect);
          SetCtlValue((ControlHandle)the_handle, false);
          old_item=the_item;
          GetDItem(optptr, the_item, &the_type, &the_handle, &the_rect);
          SetCtlValue((ControlHandle)the_handle, true);
        break;
      }
    }
    /*now, load the dialog items into the corect variables interpritation*/
    GetDItem( optptr, 4, &the_type, &the_handle, &the_rect);
    GetIText( the_handle, (unsigned char *)server_arg.load_filename);
    GetDItem( optptr, 6, &the_type, &the_handle, &the_rect);
    GetIText( the_handle, (unsigned char *)server_arg.gamelog_filename);
    GetDItem( optptr, 8, &the_type, &the_handle, &the_rect);
    GetIText( the_handle, (unsigned char *)server_arg.log_filename);
    GetDItem( optptr, 12, &the_type, &the_handle, &the_rect);
    GetIText( the_handle, the_string);
    sscanf(the_string, "%d", server_arg.port);
    GetDItem( optptr, 10, &the_type, &the_handle, &the_rect);
    GetIText( the_handle, (unsigned char *)server_arg.script_filename);
    GetDItem(optptr, 15, &the_type, &the_handle, &the_rect);
    if(GetControlValue((ControlHandle)the_handle))
    {
      server_arg.loglevel=LOG_FATAL;
    }
    GetDItem(optptr, 16, &the_type, &the_handle, &the_rect);
    if(GetControlValue((ControlHandle)the_handle))
    {
      server_arg.loglevel=LOG_NORMAL;
    }
    GetDItem(optptr, 17, &the_type, &the_handle, &the_rect);
    if(GetControlValue((ControlHandle)the_handle))
    {
      server_arg.loglevel=LOG_VERBOSE;
    }
    DisposeDialog(optptr);/*get rid of the dialog after sorting out the options*/
    DisposePtr(storage);/*clean up the allocated memory*/
  }
#endif
#undef  HARDCODED_OPT
}
#endif
