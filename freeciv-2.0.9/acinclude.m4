dnl @synopsis AC_PATH_LIB(LIBRARY [, MINIMUM-VERSION [, HEADERS [, CONFIG-SCRIPT [, MODULES [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]]]]]])
dnl
dnl Runs a LIBRARY-config script and defines LIBRARY_CFLAGS and LIBRARY_LIBS,
dnl saving you from writing your own macro specific to your library.
dnl
dnl The defaults:
dnl  $1 = LIBRARY             e.g. gtk, ncurses
dnl  $2 = MINIMUM-VERSION     x.y.z format e.g. 4.2.1
dnl                           Add ' -nocheck' e.g. '4.2.1 -nocheck' to avoid
dnl                           checking version with library-defined version
dnl                           numbers (see below) i.e. --version only
dnl  $3 = HEADER              Header to include in test program if not
dnl                           called LIBRARY/LIBRARY.h
dnl  $4 = CONFIG-SCRIPT       Name of libconfig script if not
dnl                           LIBRARY-config
dnl  $5 = MODULES             List of module names to pass to LIBRARY-config.
dnl                           It is probably best to use only one, to avoid
dnl                           two version numbers being reported.
dnl  $6 = ACTION-IF-FOUND     Shell script to run if library is found
dnl  $7 = ACTION-IF-NOT-FOUND Shell script to run if library is not found
dnl
dnl LIBRARY-config must support `--cflags' and `--libs' args.
dnl If MINIMUM-VERSION is specified, LIBRARY-config should also support the
dnl `--version' arg, and have version information embedded in its header
dnl as detailed below:
dnl
dnl Macros:
dnl  #define LIBRARY_MAJOR_VERSION       (@LIBRARY_MAJOR_VERSION@)
dnl  #define LIBRARY_MINOR_VERSION       (@LIBRARY_MINOR_VERSION@)
dnl  #define LIBRARY_MICRO_VERSION       (@LIBRARY_MICRO_VERSION@)
dnl
dnl Version numbers (defined in the library):
dnl  extern const unsigned int library_major_version;
dnl  extern const unsigned int library_minor_version;
dnl  extern const unsigned int library_micro_version;
dnl
dnl If the above are not defined, then use ' -nocheck'.
dnl
dnl If the `--with-library-[exec-]prefix' arguments to ./configure are
dnl given, it must also support `--prefix' and `--exec-prefix'.
dnl (In other words, it must be like gtk-config.)
dnl
dnl If modules are to be used, LIBRARY-config must support modules.
dnl 
dnl For example:
dnl
dnl  `AC_PATH_LIB(foo, 1.0.0)'
dnl
dnl would run `foo-config --version' and check that it is at least 1.0.0
dnl
dnl If so, the following would then be defined:
dnl
dnl  FOO_CFLAGS  to `foo-config --cflags`
dnl  FOO_LIBS    to `foo-config --libs`
dnl  FOO_VERSION to `foo-config --version`
dnl
dnl Based on `AM_PATH_GTK' (gtk.m4) by Owen Taylor, and `AC_PATH_GENERIC'
dnl (ac_path_generic.m4) by Angus Lees <gusl@cse.unsw.edu.au>
dnl
dnl @version $Id: ac_path_lib.m4 4430 2002-04-13 13:52:03Z rfalke $
dnl @author Roger Leigh <roger@whinlatter.uklinux.net>
dnl
AC_DEFUN([AC_PATH_LIB],[# check for presence of lib$1
dnl
dnl we're going to need uppercase, lowercase and user-friendly versions of the
dnl string `LIBRARY', and long and cache variants.
dnl
m4_pushdef([UP], m4_translit([$1], [a-z], [A-Z]))dnl
m4_pushdef([DOWN], m4_translit([$1], [A-Z], [a-z]))dnl
m4_pushdef([LDOWN], ac_path_lib_[]DOWN)dnl
m4_pushdef([LUP], ac_path_lib_[]UP)dnl
m4_pushdef([CACHEDOWN], ac_cv_path_lib_[]DOWN)dnl
m4_pushdef([CACHEUP], ac_cv_path_lib_[]UP)dnl
LDOWN[]_header="m4_default([$3], [$1/$1.h])"
LDOWN[]_config="m4_default([$4], [$1-config])"
dnl
dnl get the cflags and libraries from the LIBRARY-config script
dnl
AC_ARG_WITH(DOWN-prefix,
            AC_HELP_STRING([--with-DOWN-prefix=PFX],
	                   [prefix where UP is installed (optional)]),
            [LDOWN[]_config_prefix="$withval"],
	    [LDOWN[]_config_prefix=""])dnl
AC_ARG_WITH(DOWN-exec-prefix,
            AC_HELP_STRING([--with-DOWN-exec-prefix=PFX],
	                   [exec-prefix where UP is installed (optional)]),
            [LDOWN[]_config_exec_prefix="$withval"],
	    [LDOWN[]_config_exec_prefix=""])dnl
AC_ARG_ENABLE(DOWN[]test,
              AC_HELP_STRING([--disable-DOWN[]test],
	                     [do not try to compile and run a test UP program]),
              [LDOWN[]_test_enabled="no"],
              [LDOWN[]_test_enabled="yes"])dnl
dnl
dnl set up LIBRARY-config script parameters
dnl
m4_if([$5], , ,
      [LDOWN[]_config_args="$LDOWN[]_config_args $5"])
LDOWN[]_min_version=`echo "$2" | sed -e 's/ -nocheck//'`
m4_if([$2], , ,[if test "$LDOWN[]_min_version" = "$2" ; then
                  LDOWN[]_version_test_enabled="yes"
                fi])
if test x$LDOWN[]_config_exec_prefix != x ; then
  LDOWN[]_config_args="$LDOWN[]_config_args --exec-prefix=$LDOWN[]_config_exec_prefix"
fi
if test x$LDOWN[]_config_prefix != x ; then
  LDOWN[]_config_args="$LDOWN[]_config_args --prefix=$LDOWN[]_config_prefix"
fi
dnl
dnl find LIBRARY-config script
dnl
AC_PATH_PROG(UP[]_CONFIG, $LDOWN[]_config, no)dnl
dnl
dnl run the test, but cache results so it can be skipped next time
dnl
if test x$UP[]_CONFIG = xno ; then
  LDOWN[]_present_avoidcache="no"
else
  LDOWN[]_present_avoidcache="yes"
dnl
dnl set up variables, caching those needed later
dnl
  AC_CACHE_CHECK([for UP CFLAGS],
                 [CACHEDOWN[]_cflags],
                 [CACHEDOWN[]_cflags=`$UP[]_CONFIG $LDOWN[]_config_args --cflags`])
  AC_CACHE_CHECK([for UP LIBS],
                 [CACHEDOWN[]_libs],
                 [CACHEDOWN[]_libs=`$UP[]_CONFIG $LDOWN[]_config_args --libs`])
  AC_CACHE_CHECK([for UP version],
                 [CACHEDOWN[]_version],
                 [CACHEDOWN[]_version=`$UP[]_CONFIG $LDOWN[]_config_args --version`])
  UP[]_CFLAGS="$CACHEDOWN[]_cflags"
  UP[]_LIBS="$CACHEDOWN[]_libs"
  UP[]_VERSION="$CACHEDOWN[]_version"
  LDOWN[]_config_major_version=`$UP[]_CONFIG $LDOWN[]_config_args --version | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
  LDOWN[]_config_minor_version=`$UP[]_CONFIG $LDOWN[]_config_args --version | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
  LDOWN[]_config_micro_version=`$UP[]_CONFIG $LDOWN[]_config_args --version | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
  LDOWN[]_min_major_version=`echo "$LDOWN[]_min_version" | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
  LDOWN[]_min_minor_version=`echo "$LDOWN[]_min_version" | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
  LDOWN[]_min_micro_version=`echo "$LDOWN[]_min_version" | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
fi
dnl
dnl remove error log from previous runs
dnl
rm -f error.DOWN[]test
dnl
dnl now check if the installed UP is sufficiently new. (Also sanity
dnl checks the results of DOWN-config to some extent
dnl
AC_CACHE_CHECK([for UP - m4_if([$2], ,
                               [any version],
                               [version >= $LDOWN[]_min_version])],
               [CACHEDOWN[]_present],
[CACHEDOWN[]_present="$LDOWN[]_present_avoidcache"
if test x$CACHEDOWN[]_present = xyes -a x$LDOWN[]_test_enabled = xyes -a \
    x$LDOWN[]_version_test_enabled = xyes ; then
  ac_save_CFLAGS="$CFLAGS"
  ac_save_LIBS="$LIBS"
  CFLAGS="$CFLAGS $UP[]_CFLAGS"
  LIBS="$UP[]_LIBS $LIBS"
dnl
dnl now check if the installed UP is sufficiently new. (Also sanity
dnl checks the results of DOWN-config to some extent
dnl
  rm -f conf.DOWN[]test
  AC_TRY_RUN([
#include <$]LDOWN[_header>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;
  FILE *errorlog;

  if ((errorlog = fopen("error.]DOWN[test", "w")) == NULL) {
     exit(1);
   }

  system ("touch conf.]DOWN[test");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = strdup("$]LDOWN[_min_version");
  if (!tmp_version) {
     exit(1);
   }
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     fprintf(errorlog, "*** %s: bad version string\n", "$]LDOWN[_min_version");
     exit(1);
   }

  if ((]DOWN[_major_version != $]LDOWN[_config_major_version) ||
      (]DOWN[_minor_version != $]LDOWN[_config_minor_version) ||
      (]DOWN[_micro_version != $]LDOWN[_config_micro_version))
    {
      fprintf(errorlog, "*** '$]UP[_CONFIG --version' returned %d.%d.%d, but \n", \
        $]LDOWN[_config_major_version, $]LDOWN[_config_minor_version, \
	$]LDOWN[_config_micro_version);
      fprintf(errorlog, "*** ]UP[ (%d.%d.%d) was found!\n", \
        ]DOWN[_major_version, ]DOWN[_minor_version, ]DOWN[_micro_version);
      fprintf(errorlog, "***\n");
      fprintf(errorlog, "*** If $]LDOWN[_config was correct, then it is best to remove\n");
      fprintf(errorlog, "*** the old version of ]UP[.  You may also be able to\n");
      fprintf(errorlog, "*** fix the error by modifying your LD_LIBRARY_PATH enviroment\n");
      fprintf(errorlog, "*** variable, or by editing /etc/ld.so.conf.  Make sure you have\n");
      fprintf(errorlog, "*** run ldconfig if that is required on your system.\n");
      fprintf(errorlog, "*** If $]LDOWN[_config was wrong, set the environment\n");
      fprintf(errorlog, "*** variable ]UP[_CONFIG to point to the correct copy of\n");
      fprintf(errorlog, "*** $]LDOWN[_config, and remove the file config.cache\n");
      fprintf(errorlog, "*** before re-running configure.\n");
    } 
#if defined (]UP[_MAJOR_VERSION) && defined (]UP[_MINOR_VERSION) && defined (]UP[_MICRO_VERSION)
  else if ((]DOWN[_major_version != ]UP[_MAJOR_VERSION) ||
	   (]DOWN[_minor_version != ]UP[_MINOR_VERSION) ||
           (]DOWN[_micro_version != ]UP[_MICRO_VERSION))
    {
      fprintf(errorlog, "*** ]UP[ header files (version %d.%d.%d) do not match\n",
	     ]UP[_MAJOR_VERSION, ]UP[_MINOR_VERSION, ]UP[_MICRO_VERSION);
      fprintf(errorlog, "*** library (version %d.%d.%d)\n",
	     ]DOWN[_major_version, ]DOWN[_minor_version, ]DOWN[_micro_version);
    }
#endif /* defined (]UP[_MAJOR_VERSION) ... */
  else
    {
      if ((]DOWN[_major_version > major) ||
        ((]DOWN[_major_version == major) && (]DOWN[_minor_version > minor)) ||
        ((]DOWN[_major_version == major) && (]DOWN[_minor_version == minor) && (]DOWN[_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        fprintf(errorlog, "*** An old version of ]UP[ (%d.%d.%d) was found.\n",
               ]DOWN[_major_version, ]DOWN[_minor_version, ]DOWN[_micro_version);
        fprintf(errorlog, "*** You need a version of ]UP[ newer than %d.%d.%d.\n",
	       major, minor, micro);
        /*fprintf(errorlog, "*** The latest version of ]UP[ is always available from ftp://ftp.my.site\n");*/
        fprintf(errorlog, "***\n");
        fprintf(errorlog, "*** If you have already installed a sufficiently new version, this\n");
        fprintf(errorlog, "*** error probably means that the wrong copy of the $]LDOWN[_config\n");
        fprintf(errorlog, "*** shell script is being found.  The easiest way to fix this is to\n");
        fprintf(errorlog, "*** remove the old version of ]UP[, but you can also set the\n");
        fprintf(errorlog, "*** ]UP[_CONFIG environment variable to point to the correct\n");
	fprintf(errorlog, "*** copy of $]LDOWN[_config.  (In this case, you will have to\n");
        fprintf(errorlog, "*** modify your LD_LIBRARY_PATH environment variable, or edit\n");
        fprintf(errorlog, "*** /etc/ld.so.conf so that the correct libraries are found at\n");
	fprintf(errorlog, "*** run-time.)\n");
      }
    }
  return 1;
}
],, [CACHEDOWN[]_present="no"],
    [echo $ac_n "cross compiling; assumed OK... $ac_c" >>error.]DOWN[test])
  CFLAGS="$ac_save_CFLAGS"
  LIBS="$ac_save_LIBS"
else
dnl
dnl check version just using --version values
dnl
  m4_if([$2], , ,[
  if test x$LDOWN[]_present_avoidcache = xyes ; then
    if test \
        "$LDOWN[]_config_major_version" -lt "$LDOWN[]_min_major_version" -o \
        "$LDOWN[]_config_major_version" -eq "$LDOWN[]_min_major_version" -a \
        "$LDOWN[]_config_minor_version" -lt "$LDOWN[]_min_minor_version" -o \
        "$LDOWN[]_config_major_version" -eq "$LDOWN[]_min_major_version" -a \
        "$LDOWN[]_config_minor_version" -eq "$LDOWN[]_min_minor_version" -a \
        "$LDOWN[]_config_micro_version" -lt "$LDOWN[]_min_micro_version" ; then
      CACHEDOWN[]_present="no"
      LDOWN[]_version_test_error="yes"
      echo "*** '$UP[]_CONFIG --version' returned $LDOWN[]_config_major_version.$LDOWN[]_config_minor_version.$LDOWN[]_config_micro_version, but" >>error.]DOWN[test
      echo "*** UP (>= $LDOWN[]_min_version) was needed." >>error.]DOWN[test
      echo "***" >>error.]DOWN[test
      echo "*** If $]LDOWN[_config was wrong, set the environment" >>error.]DOWN[test
      echo "*** variable ]UP[_CONFIG to point to the correct copy of" >>error.]DOWN[test
      echo "*** $]LDOWN[_config, and remove the file config.cache" >>error.]DOWN[test
      echo "*** before re-running configure." >>error.]DOWN[test
    else
      CACHEDOWN[]_present="yes"
    fi
  fi])
dnl
dnl if the user allowed it, try linking with the library
dnl
  if test x$LDOWN[]_test_enabled = xyes ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $UP[]_CFLAGS"
    LIBS="$UP[]_LIBS $LIBS"
    AC_TRY_LINK([
#include <stdio.h>
],,,            [CACHEDOWN[]_present="no"
                 if test x$LDOWN[]_version_test_error = xyes ; then
                   echo "***" >>error.]DOWN[test
                 fi
                 echo "*** The test program failed to compile or link.  See the file" >>error.]DOWN[test
                 echo "*** config.log for the exact error that occured.  This usually" >>error.]DOWN[test
                 echo "*** means UP was not installed, was incorrectly installed" >>error.]DOWN[test
                 echo "*** or that you have moved UP since it was installed.  In" >>error.]DOWN[test
                 echo "*** the latter case, you may want to edit the $LDOWN[]_config" >>error.]DOWN[test
                 echo "*** script: $UP[]_CONFIG" >>error.]DOWN[test])
    CFLAGS="$ac_save_CFLAGS"
    LIBS="$ac_save_LIBS"
  fi  
fi
dnl
dnl the test failed; try to diagnose the cause of failure
dnl
if test x$CACHEDOWN[]_present = xno ; then
  if test x$UP[]_CONFIG = xno ; then
    echo "*** The $LDOWN[]_config script installed by UP could not be found" >>error.]DOWN[test
    echo "*** If UP was installed in PREFIX, make sure PREFIX/bin is in" >>error.]DOWN[test
    echo "*** your path, or set the UP[]_CONFIG environment variable to the" >>error.]DOWN[test
    echo "*** full path to $LDOWN[]_config." >>error.]DOWN[test
  else
    if test -f conf.DOWN[]test ; then
      :
    elif test x$LDOWN[]_version_test_enabled = xyes ; then
      echo "*** Could not run UP test program, checking why..." >>error.]DOWN[test
      echo "***" >>error.]DOWN[test
      CFLAGS="$CFLAGS $UP[]_CFLAGS"
      LIBS="$LIBS $UP[]_LIBS"
      AC_TRY_LINK([
#include <$]LDOWN[_header>
#include <stdio.h>
],      [ return ((]DOWN[_major_version) || (]DOWN[_minor_version) || (]DOWN[_micro_version)); ],
        [ echo "*** The test program compiled, but did not run.  This usually" >>error.]DOWN[test
          echo "*** means that the run-time linker is not finding UP or finding" >>error.]DOWN[test
          echo "*** finding the wrong version of UP.  If it is not finding" >>error.]DOWN[test
          echo "*** UP, you'll need to set your LD_LIBRARY_PATH environment" >>error.]DOWN[test
          echo "*** variable, or edit /etc/ld.so.conf to point to the installed" >>error.]DOWN[test
          echo "*** location.  Also, make sure you have run ldconfig if that is" >>error.]DOWN[test
	  echo "*** required on your system." >>error.]DOWN[test
	  echo "***" >>error.]DOWN[test
          echo "*** If you have an old version installed, it is best to remove" >>error.]DOWN[test
	  echo "*** it, although you may also be able to get things to work by" >>error.]DOWN[test
	  echo "*** modifying LD_LIBRARY_PATH" >>error.]DOWN[test],
        [ echo "*** The test program failed to compile or link.  See the file" >>error.]DOWN[test
	  echo "*** config.log for the exact error that occured.  This usually" >>error.]DOWN[test
          echo "*** means UP was incorrectly installed or that you have" >>error.]DOWN[test
          echo "*** moved UP since it was installed.  In the latter case," >>error.]DOWN[test
	  echo "*** you may want to edit the $LDOWN[]_config script:" >>error.]DOWN[test
	  echo "*** $UP[]_CONFIG" >>error.]DOWN[test])
      CFLAGS="$ac_save_CFLAGS"
      LIBS="$ac_save_LIBS"
    fi
  fi
  UP[]_CFLAGS=""
  UP[]_LIBS=""
  UP[]_VERSION=""
  m4_if([$7], , :, [$7])
fi])
dnl
dnl print the error log (after AC_CACHE_CHECK completes)
dnl
if test -f error.DOWN[]test ; then
  cat error.DOWN[]test
fi
dnl
dnl define variables and run extra code
dnl
UP[]_CFLAGS="$CACHEDOWN[]_cflags"
UP[]_LIBS="$CACHEDOWN[]_libs"
UP[]_VERSION="$CACHEDOWN[]_version"
AC_SUBST(UP[]_CFLAGS)dnl
AC_SUBST(UP[]_LIBS)dnl
AC_SUBST(UP[]_VERSION)dnl
# Run code which depends upon the test result.
if test x$CACHEDOWN[]_present = xyes ; then
  m4_if([$6], , :, [$6])     
else
  UP[]_CFLAGS=""
  UP[]_LIBS=""
  UP[]_VERSION=""
  m4_if([$7], , :, [$7])
fi
dnl
dnl clean up temporary files
dnl
rm -f conf.DOWN[]test
rm -f error.DOWN[]test
dnl
dnl pop the macros we defined earlier
dnl
m4_popdef([UP])dnl
m4_popdef([DOWN])dnl
m4_popdef([LDOWN])dnl
m4_popdef([LUP])dnl
m4_popdef([CACHEDOWN])dnl
m4_popdef([CACHEUP])dnl
])
dnl AM_ALSA_SUPPORT([ACTION-IF-SUPPORTS [, ACTION-IF-NOT-SUPPORTS]])
dnl Partially stolen from alsaplayer
dnl
AC_DEFUN([AM_ALSA_SUPPORT],
[dnl
  AC_MSG_CHECKING(for ALSA version)
  AC_EGREP_CPP([AP_maGiC_VALUE],
  [
#include <sys/asoundlib.h>
#if defined(SND_LIB_MAJOR) && defined(SND_LIB_MINOR)
#if SND_LIB_MAJOR>1 || (SND_LIB_MAJOR==1 && SND_LIB_MINOR>=0)
AP_maGiC_VALUE
#endif
#endif
  ],
  AC_MSG_RESULT([>= 1.0])
  AC_MSG_CHECKING(for Audio File Library version)
  AC_EGREP_CPP([AP_maGiC_VALUE],
  [
#include <audiofile.h>
#if defined(LIBAUDIOFILE_MAJOR_VERSION) && defined(LIBAUDIOFILE_MINOR_VERSION)
#if LIBAUDIOFILE_MAJOR_VERSION>0 || (LIBAUDIOFILE_MAJOR_VERSION==0 && LIBAUDIOFILE_MINOR_VERSION>=2)
AP_maGiC_VALUE
#endif
#endif
  ],
  AC_MSG_RESULT([>= 0.2])
  ALSA_CFLAGS=""
  ALSA_LIB="-laudiofile -lasound"
  ifelse([$1], , :, [$1]),
  AC_MSG_RESULT([no])
  ifelse([$2], , :, [$2])
  ),
  AC_MSG_RESULT([no])
  ifelse([$2], , :, [$2])
  )
])
# Do checks for Freeciv authentication support
#
# Called without any parameters.

AC_DEFUN([FC_CHECK_AUTH],
[
  dnl  no=do not compile in authentication,  yes=compile in auth,  *=error
  AC_ARG_ENABLE(auth, 
  [  --enable-auth        compile in authentication],
  [case "${enableval}" in
    yes) auth=true ;;
    no)  auth=false ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-auth) ;;
   esac], [auth=false])

  if test x$auth = xtrue; then

    AC_CHECK_HEADER(mysql/mysql.h, , 
                    [AC_MSG_WARN([couldn't find mysql header: disabling auth]);
                     auth=false])

    dnl we need to set -L correctly, we will check once in standard locations
    dnl then we will check with other LDFLAGS. if none of these work, we fail.
 
    AC_CHECK_LIB(mysqlclient, mysql_query, 
		 [AUTH_LIBS="-lmysqlclient $AUTH_LIBS"],
                 [AC_MSG_WARN([couldn't find mysql libs in normal locations]);
                  auth=false])

    fc_preauth_LDFLAGS=$LDFLAGS
    fc_mysql_lib_loc="-L/usr/lib/mysql -L/usr/local/lib/mysql \
                      -L$HOME/lib -L$HOME/lib/mysql"

    for __ldpath in $fc_mysql_lib_loc; do
      unset ac_cv_lib_mysqlclient_mysql_query
      LDFLAGS="$LDFLAGS $__ldpath"
      auth=true

      AC_CHECK_LIB(mysqlclient, mysql_query,
                   [AUTH_LIBS="-lmysqlclient $AUTH_LIBS";
                    AC_MSG_WARN([had to add $__ldpath to LDFLAGS])],
                    [AC_MSG_WARN([couldn't find mysql libs in $__ldpath]);
                     auth=false])

      if test x$auth = xtrue; then
        break
      else
        LDFLAGS=$fc_preauth_LDFLAGS
      fi

    done

    if test x$auth = xfalse; then
      AC_MSG_WARN([can't find mysql -- disabling authentication])
    fi

    AC_SUBST(LDFLAGS)
    AC_SUBST(AUTH_LIBS)
  fi

  if test x$auth = xtrue; then
    AC_DEFINE(HAVE_AUTH, 1, [can compile with authentication])
  fi

])
# Check for the presense of C99 features.  These may be optional or required.

# Check C99-style variadic macros (required):
#
#  #define PRINTF(msg, ...) (printf(msg, __VA_ARGS__)
#
AC_DEFUN([FC_VARIADIC_MACROS],
[
  dnl Check for variadic macros
  AC_CACHE_CHECK([for C99 variadic macros],
    [ac_cv_c99_variadic_macros],
     [AC_TRY_COMPILE(
          [#include <stdio.h>
           #define MSG(...) fprintf(stderr, __VA_ARGS__)
          ],
          [MSG("foo");
           MSG("%s", "foo");
           MSG("%s%d", "foo", 1);],
          ac_cv_c99_variadic_macros=yes,
          ac_cv_c99_variadic_macros=no)])
  if test "x${ac_cv_c99_variadic_macros}" != "xyes"; then
    AC_MSG_ERROR([A compiler supporting C99 variadic macros is required])
  fi
])

# Check C99-style variable-sized arrays (required):
#
#   char concat_str[strlen(s1) + strlen(s2) + 1];
#
AC_DEFUN([FC_VARIABLE_ARRAYS],
[
  dnl Check for variable arrays
  AC_CACHE_CHECK([for C99 variable arrays],
    [ac_cv_c99_variable_arrays],
    [AC_TRY_COMPILE(
        [],
        [char *s1 = "foo", *s2 = "bar";
         char s3[strlen(s1) + strlen(s2) + 1];
         sprintf(s3, "%s%s", s1, s2);],
        ac_cv_c99_variable_arrays=yes,
        ac_cv_c99_variable_arrays=no)])
  if test "x${ac_cv_c99_variable_arrays}" != "xyes"; then
    AC_MSG_ERROR([A compiler supporting C99 variable arrays is required])
  fi
])
AC_DEFUN([FC_DEBUG], [
AC_ARG_ENABLE(debug,
[  --enable-debug[[=no/some/yes]]  turn on debugging [[default=some]]],
[case "${enableval}" in   
  yes) enable_debug=yes ;;
  some) enable_debug=some ;;
  no)  enable_debug=no ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
esac], [enable_debug=some])

dnl -g is added by AC_PROG_CC if the compiler understands it
if test "x$enable_debug" = "xyes"; then
  AC_DEFINE(DEBUG, 1, [Define if you want extra debugging.])
  EXTRA_GCC_DEBUG_CFLAGS="$EXTRA_GCC_DEBUG_CFLAGS -Werror"
else
  if test "x$enable_debug" = "xno"; then
    AC_DEFINE(NDEBUG, 1, [Define if you want no debug support.])
    EXTRA_GCC_DEBUG_CFLAGS="-O3 -fomit-frame-pointer"
  fi
fi
])
# Configure paths for ESD
# Manish Singh    98-9-30
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_ESD([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for ESD, and define ESD_CFLAGS and ESD_LIBS
dnl
AC_DEFUN([AM_PATH_ESD],
[dnl 
dnl Get the cflags and libraries from the esd-config script
dnl
AC_ARG_WITH(esd-prefix,[  --with-esd-prefix=PFX   Prefix where ESD is installed (optional)],
            esd_prefix="$withval", esd_prefix="")
AC_ARG_WITH(esd-exec-prefix,[  --with-esd-exec-prefix=PFX Exec prefix where ESD is installed (optional)],
            esd_exec_prefix="$withval", esd_exec_prefix="")
AC_ARG_ENABLE(esdtest, [  --disable-esdtest       Do not try to compile and run a test ESD program],
		    , enable_esdtest=yes)

  if test x$esd_exec_prefix != x ; then
     esd_args="$esd_args --exec-prefix=$esd_exec_prefix"
     if test x${ESD_CONFIG+set} != xset ; then
        ESD_CONFIG=$esd_exec_prefix/bin/esd-config
     fi
  fi
  if test x$esd_prefix != x ; then
     esd_args="$esd_args --prefix=$esd_prefix"
     if test x${ESD_CONFIG+set} != xset ; then
        ESD_CONFIG=$esd_prefix/bin/esd-config
     fi
  fi

  AC_PATH_PROG(ESD_CONFIG, esd-config, no)
  min_esd_version=ifelse([$1], ,0.2.7,$1)
  AC_MSG_CHECKING(for ESD - version >= $min_esd_version)
  no_esd=""
  if test "$ESD_CONFIG" = "no" ; then
    no_esd=yes
  else
    AC_LANG_SAVE
    AC_LANG_C
    ESD_CFLAGS=`$ESD_CONFIG $esdconf_args --cflags`
    ESD_LIBS=`$ESD_CONFIG $esdconf_args --libs`

    esd_major_version=`$ESD_CONFIG $esd_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    esd_minor_version=`$ESD_CONFIG $esd_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    esd_micro_version=`$ESD_CONFIG $esd_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_esdtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $ESD_CFLAGS"
      LIBS="$LIBS $ESD_LIBS"
dnl
dnl Now check if the installed ESD is sufficiently new. (Also sanity
dnl checks the results of esd-config to some extent
dnl
      rm -f conf.esdtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esd.h>

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.esdtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_esd_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_esd_version");
     exit(1);
   }

   if (($esd_major_version > major) ||
      (($esd_major_version == major) && ($esd_minor_version > minor)) ||
      (($esd_major_version == major) && ($esd_minor_version == minor) && ($esd_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'esd-config --version' returned %d.%d.%d, but the minimum version\n", $esd_major_version, $esd_minor_version, $esd_micro_version);
      printf("*** of ESD required is %d.%d.%d. If esd-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If esd-config was wrong, set the environment variable ESD_CONFIG\n");
      printf("*** to point to the correct copy of esd-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_esd=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
       AC_LANG_RESTORE
     fi
  fi
  if test "x$no_esd" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$ESD_CONFIG" = "no" ; then
       echo "*** The esd-config script installed by ESD could not be found"
       echo "*** If ESD was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the ESD_CONFIG environment variable to the"
       echo "*** full path to esd-config."
     else
       if test -f conf.esdtest ; then
        :
       else
          echo "*** Could not run ESD test program, checking why..."
          CFLAGS="$CFLAGS $ESD_CFLAGS"
          LIBS="$LIBS $ESD_LIBS"
          AC_LANG_SAVE
          AC_LANG_C
          AC_TRY_LINK([
#include <stdio.h>
#include <esd.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding ESD or finding the wrong"
          echo "*** version of ESD. If it is not finding ESD, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means ESD was incorrectly installed"
          echo "*** or that you have moved ESD since it was installed. In the latter case, you"
          echo "*** may want to edit the esd-config script: $ESD_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
          AC_LANG_RESTORE
       fi
     fi
     ESD_CFLAGS=""
     ESD_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(ESD_CFLAGS)
  AC_SUBST(ESD_LIBS)
  rm -f conf.esdtest
])

dnl AM_ESD_SUPPORTS_MULTIPLE_RECORD([ACTION-IF-SUPPORTS [, ACTION-IF-NOT-SUPPORTS]])
dnl Test, whether esd supports multiple recording clients (version >=0.2.21)
dnl
AC_DEFUN([AM_ESD_SUPPORTS_MULTIPLE_RECORD],
[dnl
  AC_MSG_NOTICE([whether installed esd version supports multiple recording clients])
  ac_save_ESD_CFLAGS="$ESD_CFLAGS"
  ac_save_ESD_LIBS="$ESD_LIBS"
  AM_PATH_ESD(0.2.21,
    ifelse([$1], , [
      AM_CONDITIONAL(ESD_SUPPORTS_MULTIPLE_RECORD, true)
      AC_DEFINE(ESD_SUPPORTS_MULTIPLE_RECORD, 1,
	[Define if you have esound with support of multiple recording clients.])],
    [$1]),
    ifelse([$2], , [AM_CONDITIONAL(ESD_SUPPORTS_MULTIPLE_RECORD, false)], [$2])
    if test "x$ac_save_ESD_CFLAGS" != x ; then
       ESD_CFLAGS="$ac_save_ESD_CFLAGS"
    fi
    if test "x$ac_save_ESD_LIBS" != x ; then
       ESD_LIBS="$ac_save_ESD_LIBS"
    fi
  )
])
# Configure paths for FreeType2
# Marcelo Magallon 2001-10-26, based on gtk.m4 by Owen Taylor

dnl AC_CHECK_FT2([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for FreeType2, and define FT2_CFLAGS and FT2_LIBS
dnl
AC_DEFUN([AC_CHECK_FT2],
[dnl
dnl Get the cflags and libraries from the freetype-config script
dnl
AC_ARG_WITH(ft-prefix,
[  --with-ft-prefix=PREFIX
                          Prefix where FreeType is installed (optional)],
            ft_config_prefix="$withval", ft_config_prefix="")
AC_ARG_WITH(ft-exec-prefix,
[  --with-ft-exec-prefix=PREFIX
                          Exec prefix where FreeType is installed (optional)],
            ft_config_exec_prefix="$withval", ft_config_exec_prefix="")
AC_ARG_ENABLE(freetypetest,
[  --disable-freetypetest  Do not try to compile and run
                          a test FreeType program],
              [], enable_fttest=yes)

if test x$ft_config_exec_prefix != x ; then
  ft_config_args="$ft_config_args --exec-prefix=$ft_config_exec_prefix"
  if test x${FT2_CONFIG+set} != xset ; then
    FT2_CONFIG=$ft_config_exec_prefix/bin/freetype-config
  fi
fi
if test x$ft_config_prefix != x ; then
  ft_config_args="$ft_config_args --prefix=$ft_config_prefix"
  if test x${FT2_CONFIG+set} != xset ; then
    FT2_CONFIG=$ft_config_prefix/bin/freetype-config
  fi
fi
AC_PATH_PROG(FT2_CONFIG, freetype-config, no)

min_ft_version=ifelse([$1], ,6.1.0,$1)
AC_MSG_CHECKING(for FreeType - version >= $min_ft_version)
no_ft=""
if test "$FT2_CONFIG" = "no" ; then
  no_ft=yes
else
  FT2_CFLAGS=`$FT2_CONFIG $ft_config_args --cflags`
  FT2_LIBS=`$FT2_CONFIG $ft_config_args --libs`
  ft_config_major_version=`$FT2_CONFIG $ft_config_args --version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
  ft_config_minor_version=`$FT2_CONFIG $ft_config_args --version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
  ft_config_micro_version=`$FT2_CONFIG $ft_config_args --version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
  ft_min_major_version=`echo $min_ft_version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
  ft_min_minor_version=`echo $min_ft_version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
  ft_min_micro_version=`echo $min_ft_version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
  if test x$enable_fttest = xyes ; then
    ft_config_is_lt=""
    if test $ft_config_major_version -lt $ft_min_major_version ; then
      ft_config_is_lt=yes
    else
      if test $ft_config_major_version -eq $ft_min_major_version ; then
        if test $ft_config_minor_version -lt $ft_min_minor_version ; then
          ft_config_is_lt=yes
        else
          if test $ft_config_minor_version -eq $ft_min_minor_version ; then
            if test $ft_config_micro_version -lt $ft_min_micro_version ; then
              ft_config_is_lt=yes
            fi
          fi
        fi
      fi
    fi
    if test x$ft_config_is_lt = xyes ; then
      no_ft=yes
    else
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $FT2_CFLAGS"
      LIBS="$FT2_LIBS $LIBS"
dnl
dnl Sanity checks for the results of freetype-config to some extent
dnl
      AC_TRY_RUN([
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <stdlib.h>

int
main()
{
  FT_Library library;
  FT_Error error;

  error = FT_Init_FreeType(&library);

  if (error)
    return 1;
  else
  {
    FT_Done_FreeType(library);
    return 0;
  }
}
],, no_ft=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
      CFLAGS="$ac_save_CFLAGS"
      LIBS="$ac_save_LIBS"
    fi             # test $ft_config_version -lt $ft_min_version
  fi               # test x$enable_fttest = xyes
fi                 # test "$FT2_CONFIG" = "no"
if test x$no_ft = x ; then
   AC_MSG_RESULT(yes)
   ifelse([$2], , :, [$2])
else
   AC_MSG_RESULT(no)
   if test "$FT2_CONFIG" = "no" ; then
     echo "*** The freetype-config script installed by FreeType 2 could not be found."
     echo "*** If FreeType 2 was installed in PREFIX, make sure PREFIX/bin is in"
     echo "*** your path, or set the FT2_CONFIG environment variable to the"
     echo "*** full path to freetype-config."
   else
     if test x$ft_config_is_lt = xyes ; then
       echo "*** Your installed version of the FreeType 2 library is too old."
       echo "*** If you have different versions of FreeType 2, make sure that"
       echo "*** correct values for --with-ft-prefix or --with-ft-exec-prefix"
       echo "*** are used, or set the FT2_CONFIG environment variable to the"
       echo "*** full path to freetype-config."
     else
       echo "*** The FreeType test program failed to run.  If your system uses"
       echo "*** shared libraries and they are installed outside the normal"
       echo "*** system library path, make sure the variable LD_LIBRARY_PATH"
       echo "*** (or whatever is appropiate for your system) is correctly set."
     fi
   fi
   FT2_CFLAGS=""
   FT2_LIBS=""
   ifelse([$3], , :, [$3])
fi
AC_SUBST(FT2_CFLAGS)
AC_SUBST(FT2_LIBS)
])
# Macro to add for using GNU gettext.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU General Public
# License or the GNU Library General Public License but which still want
# to provide support for the GNU gettext functionality.
# Please note that the actual code of the GNU gettext library is covered
# by the GNU Library General Public License, and the rest of the GNU
# gettext package package is covered by the GNU General Public License.
# They are *not* in the public domain.

# serial 10

dnl Usage: AM_WITH_NLS([TOOLSYMBOL], [NEEDSYMBOL], [LIBDIR]).
dnl If TOOLSYMBOL is specified and is 'use-libtool', then a libtool library
dnl    $(top_builddir)/intl/libintl.la will be created (shared and/or static,
dnl    depending on --{enable,disable}-{shared,static} and on the presence of
dnl    AM-DISABLE-SHARED). Otherwise, a static library
dnl    $(top_builddir)/intl/libintl.a will be created.
dnl If NEEDSYMBOL is specified and is 'need-ngettext', then GNU gettext
dnl    implementations (in libc or libintl) without the ngettext() function
dnl    will be ignored.
dnl LIBDIR is used to find the intl libraries.  If empty,
dnl    the value `$(top_builddir)/intl/' is used.
dnl
dnl The result of the configuration is one of three cases:
dnl 1) GNU gettext, as included in the intl subdirectory, will be compiled
dnl    and used.
dnl    Catalog format: GNU --> install in $(datadir)
dnl    Catalog extension: .mo after installation, .gmo in source tree
dnl 2) GNU gettext has been found in the system's C library.
dnl    Catalog format: GNU --> install in $(datadir)
dnl    Catalog extension: .mo after installation, .gmo in source tree
dnl 3) No internationalization, always use English msgid.
dnl    Catalog format: none
dnl    Catalog extension: none
dnl The use of .gmo is historical (it was needed to avoid overwriting the
dnl GNU format catalogs when building on a platform with an X/Open gettext),
dnl but we keep it in order not to force irrelevant filename changes on the
dnl maintainers.
dnl
AC_DEFUN([AM_WITH_NLS],
  [AC_MSG_CHECKING([whether NLS is requested])
    dnl Default is enabled NLS
    AC_ARG_ENABLE(nls,
      [  --disable-nls           do not use Native Language Support],
      USE_NLS=$enableval, USE_NLS=yes)
    AC_MSG_RESULT($USE_NLS)
    AC_SUBST(USE_NLS)

    BUILD_INCLUDED_LIBINTL=no
    USE_INCLUDED_LIBINTL=no
    INTLLIBS=

    dnl If we use NLS figure out what method
    if test "$USE_NLS" = "yes"; then
      AC_DEFINE(ENABLE_NLS, 1,
        [Define to 1 if translation of program messages to the user's native language
   is requested.])
      AC_MSG_CHECKING([whether included gettext is requested])
      AC_ARG_WITH(included-gettext,
        [  --with-included-gettext use the GNU gettext library included here],
        nls_cv_force_use_gnu_gettext=$withval,
        nls_cv_force_use_gnu_gettext=no)
      AC_MSG_RESULT($nls_cv_force_use_gnu_gettext)

      nls_cv_use_gnu_gettext="$nls_cv_force_use_gnu_gettext"
      if test "$nls_cv_force_use_gnu_gettext" != "yes"; then
        dnl User does not insist on using GNU NLS library.  Figure out what
        dnl to use.  If GNU gettext is available we use this.  Else we have
        dnl to fall back to GNU NLS library.
	CATOBJEXT=NONE

        dnl Add a version number to the cache macros.
        define(gt_cv_func_gnugettext_libc, [gt_cv_func_gnugettext]ifelse([$2], need-ngettext, 2, 1)[_libc])
        define(gt_cv_func_gnugettext_libintl, [gt_cv_func_gnugettext]ifelse([$2], need-ngettext, 2, 1)[_libintl])

	AC_CHECK_HEADER(libintl.h,
	  [AC_CACHE_CHECK([for GNU gettext in libc], gt_cv_func_gnugettext_libc,
	    [AC_TRY_LINK([#include <libintl.h>
extern int _nl_msg_cat_cntr;],
	       [bindtextdomain ("", "");
return (int) gettext ("")]ifelse([$2], need-ngettext, [ + (int) ngettext ("", "", 0)], [])[ + _nl_msg_cat_cntr],
	       gt_cv_func_gnugettext_libc=yes,
	       gt_cv_func_gnugettext_libc=no)])

	   if test "$gt_cv_func_gnugettext_libc" != "yes"; then
	     AC_CACHE_CHECK([for GNU gettext in libintl],
	       gt_cv_func_gnugettext_libintl,
	       [gt_save_LIBS="$LIBS"
		LIBS="$LIBS -lintl $LIBICONV"
		AC_TRY_LINK([#include <libintl.h>
extern int _nl_msg_cat_cntr;],
		  [bindtextdomain ("", "");
return (int) gettext ("")]ifelse([$2], need-ngettext, [ + (int) ngettext ("", "", 0)], [])[ + _nl_msg_cat_cntr],
		  gt_cv_func_gnugettext_libintl=yes,
		  gt_cv_func_gnugettext_libintl=no)
		LIBS="$gt_save_LIBS"])
	   fi

	   dnl If an already present or preinstalled GNU gettext() is found,
	   dnl use it.  But if this macro is used in GNU gettext, and GNU
	   dnl gettext is already preinstalled in libintl, we update this
	   dnl libintl.  (Cf. the install rule in intl/Makefile.in.)
	   if test "$gt_cv_func_gnugettext_libc" = "yes" \
	      || { test "$gt_cv_func_gnugettext_libintl" = "yes" \
		   && test "$PACKAGE" != gettext; }; then
	     AC_DEFINE(HAVE_GETTEXT, 1,
               [Define if the GNU gettext() function is already present or preinstalled.])

	     if test "$gt_cv_func_gnugettext_libintl" = "yes"; then
	       dnl If iconv() is in a separate libiconv library, then anyone
	       dnl linking with libintl{.a,.so} also needs to link with
	       dnl libiconv.
	       INTLLIBS="-lintl $LIBICONV"
	     fi

	     gt_save_LIBS="$LIBS"
	     LIBS="$LIBS $INTLLIBS"
	     AC_CHECK_FUNCS(dcgettext)
	     LIBS="$gt_save_LIBS"

	     dnl Search for GNU msgfmt in the PATH.
	     AM_PATH_PROG_WITH_TEST(MSGFMT, msgfmt,
	       [$ac_dir/$ac_word --statistics /dev/null >/dev/null 2>&1], :)
	     AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)

	     dnl Search for GNU xgettext in the PATH.
	     AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
	       [$ac_dir/$ac_word --omit-header /dev/null >/dev/null 2>&1], :)

	     CATOBJEXT=.gmo
	   fi
	])

        if test "$CATOBJEXT" = "NONE"; then
	  dnl GNU gettext is not found in the C library.
	  dnl Fall back on GNU gettext library.
	  nls_cv_use_gnu_gettext=yes
        fi
      fi

      if test "$nls_cv_use_gnu_gettext" = "yes"; then
        dnl Mark actions used to generate GNU NLS library.
        INTLOBJS="\$(GETTOBJS)"
        AM_PATH_PROG_WITH_TEST(MSGFMT, msgfmt,
	  [$ac_dir/$ac_word --statistics /dev/null >/dev/null 2>&1], :)
        AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)
        AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
	  [$ac_dir/$ac_word --omit-header /dev/null >/dev/null 2>&1], :)
        AC_SUBST(MSGFMT)
	BUILD_INCLUDED_LIBINTL=yes
	USE_INCLUDED_LIBINTL=yes
        CATOBJEXT=.gmo
	INTLLIBS="ifelse([$3],[],\$(top_builddir)/intl,[$3])/libintl.ifelse([$1], use-libtool, [l], [])a $LIBICONV"
	LIBS=`echo " $LIBS " | sed -e 's/ -lintl / /' -e 's/^ //' -e 's/ $//'`
      fi

      dnl This could go away some day; the PATH_PROG_WITH_TEST already does it.
      dnl Test whether we really found GNU msgfmt.
      if test "$GMSGFMT" != ":"; then
	dnl If it is no GNU msgfmt we define it as : so that the
	dnl Makefiles still can work.
	if $GMSGFMT --statistics /dev/null >/dev/null 2>&1; then
	  : ;
	else
	  AC_MSG_RESULT(
	    [found msgfmt program is not GNU msgfmt; ignore it])
	  GMSGFMT=":"
	fi
      fi

      dnl This could go away some day; the PATH_PROG_WITH_TEST already does it.
      dnl Test whether we really found GNU xgettext.
      if test "$XGETTEXT" != ":"; then
	dnl If it is no GNU xgettext we define it as : so that the
	dnl Makefiles still can work.
	if $XGETTEXT --omit-header /dev/null >/dev/null 2>&1; then
	  : ;
	else
	  AC_MSG_RESULT(
	    [found xgettext program is not GNU xgettext; ignore it])
	  XGETTEXT=":"
	fi
      fi

      dnl We need to process the po/ directory.
      POSUB=po
    fi
    AC_OUTPUT_COMMANDS(
     [for ac_file in $CONFIG_FILES; do
        # Support "outfile[:infile[:infile...]]"
        case "$ac_file" in
          *:*) ac_file=`echo "$ac_file"|sed 's%:.*%%'` ;;
        esac
        # PO directories have a Makefile.in generated from Makefile.in.in.
        case "$ac_file" in */Makefile.in)
          # Adjust a relative srcdir.
          ac_dir=`echo "$ac_file"|sed 's%/[^/][^/]*$%%'`
          ac_dir_suffix="/`echo "$ac_dir"|sed 's%^\./%%'`"
          ac_dots=`echo "$ac_dir_suffix"|sed 's%/[^/]*%../%g'`
          # In autoconf-2.13 it is called $ac_given_srcdir.
          # In autoconf-2.50 it is called $srcdir.
          test -n "$ac_given_srcdir" || ac_given_srcdir="$srcdir"
          case "$ac_given_srcdir" in
            .)  top_srcdir=`echo $ac_dots|sed 's%/$%%'` ;;
            /*) top_srcdir="$ac_given_srcdir" ;;
            *)  top_srcdir="$ac_dots$ac_given_srcdir" ;;
          esac
          if test -f "$ac_given_srcdir/$ac_dir/POTFILES.in"; then
            rm -f "$ac_dir/POTFILES"
            test -n "$as_me" && echo "$as_me: creating $ac_dir/POTFILES" || echo "creating $ac_dir/POTFILES"
            sed -e "/^#/d" -e "/^[ 	]*\$/d" -e "s,.*,     $top_srcdir/& \\\\," -e "\$s/\(.*\) \\\\/\1/" < "$ac_given_srcdir/$ac_dir/POTFILES.in" > "$ac_dir/POTFILES"
            test -n "$as_me" && echo "$as_me: creating $ac_dir/Makefile" || echo "creating $ac_dir/Makefile"
            sed -e "/POTFILES =/r $ac_dir/POTFILES" "$ac_dir/Makefile.in" > "$ac_dir/Makefile"
          fi
          ;;
        esac
      done])


    dnl If this is used in GNU gettext we have to set BUILD_INCLUDED_LIBINTL
    dnl to 'yes' because some of the testsuite requires it.
    if test "$PACKAGE" = gettext; then
      BUILD_INCLUDED_LIBINTL=yes
    fi

    dnl intl/plural.c is generated from intl/plural.y. It requires bison,
    dnl because plural.y uses bison specific features. It requires at least
    dnl bison-1.26 because earlier versions generate a plural.c that doesn't
    dnl compile.
    dnl bison is only needed for the maintainer (who touches plural.y). But in
    dnl order to avoid separate Makefiles or --enable-maintainer-mode, we put
    dnl the rule in general Makefile. Now, some people carelessly touch the
    dnl files or have a broken "make" program, hence the plural.c rule will
    dnl sometimes fire. To avoid an error, defines BISON to ":" if it is not
    dnl present or too old.
    AC_CHECK_PROGS([INTLBISON], [bison])
    if test -z "$INTLBISON"; then
      ac_verc_fail=yes
    else
      dnl Found it, now check the version.
      AC_MSG_CHECKING([version of bison])
changequote(<<,>>)dnl
      ac_prog_version=`$INTLBISON --version 2>&1 | sed -n 's/^.*GNU Bison.* \([0-9]*\.[0-9.]*\).*$/\1/p'`
      case $ac_prog_version in
        '') ac_prog_version="v. ?.??, bad"; ac_verc_fail=yes;;
        1.2[6-9]* | 1.[3-9][0-9]* | [2-9].*)
changequote([,])dnl
           ac_prog_version="$ac_prog_version, ok"; ac_verc_fail=no;;
        *) ac_prog_version="$ac_prog_version, bad"; ac_verc_fail=yes;;
      esac
      AC_MSG_RESULT([$ac_prog_version])
    fi
    if test $ac_verc_fail = yes; then
      INTLBISON=:
    fi

    dnl These rules are solely for the distribution goal.  While doing this
    dnl we only have to keep exactly one list of the available catalogs
    dnl in configure.in.
    for lang in $ALL_LINGUAS; do
      GMOFILES="$GMOFILES $lang.gmo"
      POFILES="$POFILES $lang.po"
    done

    dnl Make all variables we use known to autoconf.
    AC_SUBST(BUILD_INCLUDED_LIBINTL)
    AC_SUBST(USE_INCLUDED_LIBINTL)
    AC_SUBST(CATALOGS)
    AC_SUBST(CATOBJEXT)
    AC_SUBST(GMOFILES)
    AC_SUBST(INTLLIBS)
    AC_SUBST(INTLOBJS)
    AC_SUBST(POFILES)
    AC_SUBST(POSUB)

    dnl For backward compatibility. Some configure.ins may be using this.
    nls_cv_header_intl=
    nls_cv_header_libgt=

    dnl For backward compatibility. Some Makefiles may be using this.
    DATADIRNAME=share
    AC_SUBST(DATADIRNAME)

    dnl For backward compatibility. Some Makefiles may be using this.
    INSTOBJEXT=.mo
    AC_SUBST(INSTOBJEXT)

    dnl For backward compatibility. Some Makefiles may be using this.
    GENCAT=gencat
    AC_SUBST(GENCAT)
  ])

dnl Usage: Just like AM_WITH_NLS, which see.
AC_DEFUN([AM_GNU_GETTEXT],
  [AC_REQUIRE([AC_PROG_MAKE_SET])dnl
   AC_REQUIRE([AC_PROG_CC])dnl
   AC_REQUIRE([AC_CANONICAL_HOST])dnl
   AC_REQUIRE([AC_PROG_RANLIB])dnl
   AC_REQUIRE([AC_ISC_POSIX])dnl
   AC_REQUIRE([AC_HEADER_STDC])dnl
   AC_REQUIRE([AC_C_CONST])dnl
   AC_REQUIRE([AC_C_INLINE])dnl
   AC_REQUIRE([AC_TYPE_OFF_T])dnl
   AC_REQUIRE([AC_TYPE_SIZE_T])dnl
   AC_REQUIRE([AC_FUNC_ALLOCA])dnl
   AC_REQUIRE([AC_FUNC_MMAP])dnl
   AC_REQUIRE([jm_GLIBC21])dnl

   AC_CHECK_HEADERS([argz.h limits.h locale.h nl_types.h malloc.h stddef.h \
stdlib.h string.h unistd.h sys/param.h])
   AC_CHECK_FUNCS([feof_unlocked fgets_unlocked getcwd getegid geteuid \
getgid getuid mempcpy munmap putenv setenv setlocale stpcpy strchr strcasecmp \
strdup strtoul tsearch __argz_count __argz_stringify __argz_next])

   AM_ICONV
   AM_LANGINFO_CODESET
   AM_LC_MESSAGES
   AM_WITH_NLS([$1],[$2],[$3])

   if test "x$CATOBJEXT" != "x"; then
     if test "x$ALL_LINGUAS" = "x"; then
       LINGUAS=
     else
       AC_MSG_CHECKING(for catalogs to be installed)
       NEW_LINGUAS=
       for presentlang in $ALL_LINGUAS; do
         useit=no
         for desiredlang in ${LINGUAS-$ALL_LINGUAS}; do
           # Use the presentlang catalog if desiredlang is
           #   a. equal to presentlang, or
           #   b. a variant of presentlang (because in this case,
           #      presentlang can be used as a fallback for messages
           #      which are not translated in the desiredlang catalog).
           case "$desiredlang" in
             "$presentlang"*) useit=yes;;
           esac
         done
         if test $useit = yes; then
           NEW_LINGUAS="$NEW_LINGUAS $presentlang"
         fi
       done
       LINGUAS=$NEW_LINGUAS
       AC_MSG_RESULT($LINGUAS)
     fi

     dnl Construct list of names of catalog files to be constructed.
     if test -n "$LINGUAS"; then
       for lang in $LINGUAS; do CATALOGS="$CATALOGS $lang$CATOBJEXT"; done
     fi
   fi

   dnl If the AC_CONFIG_AUX_DIR macro for autoconf is used we possibly
   dnl find the mkinstalldirs script in another subdir but $(top_srcdir).
   dnl Try to locate is.
   MKINSTALLDIRS=
   if test -n "$ac_aux_dir"; then
     MKINSTALLDIRS="$ac_aux_dir/mkinstalldirs"
   fi
   if test -z "$MKINSTALLDIRS"; then
     MKINSTALLDIRS="\$(top_srcdir)/mkinstalldirs"
   fi
   AC_SUBST(MKINSTALLDIRS)

   dnl Enable libtool support if the surrounding package wishes it.
   INTL_LIBTOOL_SUFFIX_PREFIX=ifelse([$1], use-libtool, [l], [])
   AC_SUBST(INTL_LIBTOOL_SUFFIX_PREFIX)
  ])
dnl FC_CHECK_GETTIMEOFDAY_RUNTIME(EXTRA-LIBS, ACTION-IF-FOUND, ACTION-IF-NOT-FOUND)
dnl
dnl This tests whether gettimeofday works at runtime.  Here, "works"
dnl means: time doesn't go backward and time doesn't jump forward by
dnl a huge amount. It seems that glibc 2.3.1 is broken in this respect.

AC_DEFUN([FC_CHECK_GETTIMEOFDAY_RUNTIME],
[
templibs="$LIBS"
LIBS="$1 $LIBS"
AC_TRY_RUN([
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define VERBOSE 0
#define SECONDS	3

int main(int argc, char **argv)
{
  struct timeval tv[2], start, end;
  int calls;

  if (gettimeofday(&start, NULL) == -1) {
    return 1;
  }
  end = start;
  end.tv_sec += SECONDS;

  tv[0] = start;
  tv[1] = start;

  for (calls = 0;; calls++) {
    time_t sec;

    if (gettimeofday(&tv[0], NULL) == -1) {
      return 1;
    }

    if (tv[0].tv_sec < tv[1].tv_sec) {
#if VERBOSE
      double diff =
	  (tv[1].tv_sec - start.tv_sec) +
	  ((tv[1].tv_usec - start.tv_usec) / 1e6);
      printf("after %fs: going backward by %lds\n", diff,
	     tv[1].tv_sec - tv[0].tv_sec);
#endif
      return 1;
    }

    if (tv[0].tv_sec == tv[1].tv_sec && tv[0].tv_usec < tv[1].tv_usec) {
#if VERBOSE
      double diff =
	  (tv[1].tv_sec - start.tv_sec) +
	  ((tv[1].tv_usec - start.tv_usec) / 1e6);
      printf("after %fs: going backward by %ldus\n", diff,
	     tv[1].tv_usec - tv[0].tv_usec);
#endif
      return 1;
    }

    if (tv[0].tv_sec > tv[1].tv_sec + 1) {
#if VERBOSE
      double diff =
	  (tv[1].tv_sec - start.tv_sec) +
	  ((tv[1].tv_usec - start.tv_usec) / 1e6);
      printf("after %fs: going forward by %lds\n", diff,
	     tv[0].tv_sec - tv[1].tv_sec);
#endif
      return 1;
    }

    sec = time(NULL);

    if (abs(sec - tv[0].tv_sec) > 1) {
#if VERBOSE
      double diff =
	  (tv[1].tv_sec - start.tv_sec) +
	  ((tv[1].tv_usec - start.tv_usec) / 1e6);
      printf("after %fs: time() = %ld, gettimeofday = %ld, diff = %ld\n", diff,
	     (long)sec, (long)tv[0].tv_sec, sec - (long)tv[0].tv_sec);
#endif
      return 1;
    }

    if (timercmp(&tv[0], &end, >)) {
      break;
    }
    tv[1] = tv[0];
  }

#if VERBOSE
  {
    double diff =
	(tv[1].tv_sec - start.tv_sec) +
	((tv[1].tv_usec - start.tv_usec) / 1e6);
    printf("%d calls in %fs = %fus/call\n", calls, diff, 1e6 * diff / calls);
  }
#endif
  return 0;
}
],
[AC_MSG_RESULT(yes)
  [$2]],
[AC_MSG_RESULT(no)
  [$3]],
[AC_MSG_RESULT(unknown: cross-compiling)
  [$3]])
LIBS="$templibs"
])
# Configure paths for GLIB
# Owen Taylor     1997-2001

dnl AM_PATH_GLIB_2_0([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GLIB, and define GLIB_CFLAGS and GLIB_LIBS, if gmodule, gobject or 
dnl gthread is specified in MODULES, pass to pkg-config
dnl
AC_DEFUN([AM_PATH_GLIB_2_0],
[dnl 
dnl Get the cflags and libraries from pkg-config
dnl
AC_ARG_ENABLE(glibtest, [  --disable-glibtest      do not try to compile and run a test GLIB program],
		    , enable_glibtest=yes)

  pkg_config_args=glib-2.0
  for module in . $4
  do
      case "$module" in
         gmodule) 
             pkg_config_args="$pkg_config_args gmodule-2.0"
         ;;
         gobject) 
             pkg_config_args="$pkg_config_args gobject-2.0"
         ;;
         gthread) 
             pkg_config_args="$pkg_config_args gthread-2.0"
         ;;
      esac
  done

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

  no_glib=""

  if test x$PKG_CONFIG != xno ; then
    if $PKG_CONFIG --atleast-pkgconfig-version 0.7 ; then
      :
    else
      echo *** pkg-config too old; version 0.7 or better required.
      no_glib=yes
      PKG_CONFIG=no
    fi
  else
    no_glib=yes
  fi

  min_glib_version=ifelse([$1], ,2.0.0,$1)
  AC_MSG_CHECKING(for GLIB - version >= $min_glib_version)

  if test x$PKG_CONFIG != xno ; then
    ## don't try to run the test against uninstalled libtool libs
    if $PKG_CONFIG --uninstalled $pkg_config_args; then
	  echo "Will use uninstalled version of GLib found in PKG_CONFIG_PATH"
	  enable_glibtest=no
    fi

    if $PKG_CONFIG --atleast-version $min_glib_version $pkg_config_args; then
	  :
    else
	  no_glib=yes
    fi
  fi

  if test x"$no_glib" = x ; then
    GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
    GOBJECT_QUERY=`$PKG_CONFIG --variable=gobject_query glib-2.0`
    GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`

    GLIB_CFLAGS=`$PKG_CONFIG --cflags $pkg_config_args`
    GLIB_LIBS=`$PKG_CONFIG --libs $pkg_config_args`
    glib_config_major_version=`$PKG_CONFIG --modversion glib-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    glib_config_minor_version=`$PKG_CONFIG --modversion glib-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    glib_config_micro_version=`$PKG_CONFIG --modversion glib-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_glibtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GLIB_CFLAGS"
      LIBS="$GLIB_LIBS $LIBS"
dnl
dnl Now check if the installed GLIB is sufficiently new. (Also sanity
dnl checks the results of pkg-config to some extent)
dnl
      rm -f conf.glibtest
      AC_TRY_RUN([
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.glibtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_glib_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_glib_version");
     exit(1);
   }

  if ((glib_major_version != $glib_config_major_version) ||
      (glib_minor_version != $glib_config_minor_version) ||
      (glib_micro_version != $glib_config_micro_version))
    {
      printf("\n*** 'pkg-config --modversion glib-2.0' returned %d.%d.%d, but GLIB (%d.%d.%d)\n", 
             $glib_config_major_version, $glib_config_minor_version, $glib_config_micro_version,
             glib_major_version, glib_minor_version, glib_micro_version);
      printf ("*** was found! If pkg-config was correct, then it is best\n");
      printf ("*** to remove the old version of GLib. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If pkg-config was wrong, set the environment variable PKG_CONFIG_PATH\n");
      printf("*** to point to the correct configuration files\n");
    } 
  else if ((glib_major_version != GLIB_MAJOR_VERSION) ||
	   (glib_minor_version != GLIB_MINOR_VERSION) ||
           (glib_micro_version != GLIB_MICRO_VERSION))
    {
      printf("*** GLIB header files (version %d.%d.%d) do not match\n",
	     GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     glib_major_version, glib_minor_version, glib_micro_version);
    }
  else
    {
      if ((glib_major_version > major) ||
        ((glib_major_version == major) && (glib_minor_version > minor)) ||
        ((glib_major_version == major) && (glib_minor_version == minor) && (glib_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GLIB (%d.%d.%d) was found.\n",
               glib_major_version, glib_minor_version, glib_micro_version);
        printf("*** You need a version of GLIB newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GLIB is always available from ftp://ftp.gtk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the pkg-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GLIB, but you can also set the PKG_CONFIG environment to point to the\n");
        printf("*** correct copy of pkg-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_glib=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_glib" = x ; then
     AC_MSG_RESULT(yes (version $glib_config_major_version.$glib_config_minor_version.$glib_config_micro_version))
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$PKG_CONFIG" = "no" ; then
       echo "*** A new enough version of pkg-config was not found."
       echo "*** See http://www.freedesktop.org/software/pkgconfig/"
     else
       if test -f conf.glibtest ; then
        :
       else
          echo "*** Could not run GLIB test program, checking why..."
          ac_save_CFLAGS="$CFLAGS"
          ac_save_LIBS="$LIBS"
          CFLAGS="$CFLAGS $GLIB_CFLAGS"
          LIBS="$LIBS $GLIB_LIBS"
          AC_TRY_LINK([
#include <glib.h>
#include <stdio.h>
],      [ return ((glib_major_version) || (glib_minor_version) || (glib_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GLIB or finding the wrong"
          echo "*** version of GLIB. If it is not finding GLIB, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GLIB is incorrectly installed."])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GLIB_CFLAGS=""
     GLIB_LIBS=""
     GLIB_GENMARSHAL=""
     GOBJECT_QUERY=""
     GLIB_MKENUMS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GLIB_CFLAGS)
  AC_SUBST(GLIB_LIBS)
  AC_SUBST(GLIB_GENMARSHAL)
  AC_SUBST(GOBJECT_QUERY)
  AC_SUBST(GLIB_MKENUMS)
  rm -f conf.glibtest
])
# Macro to add for using GNU gettext.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
#
# Modified to never use included libintl. 
# Owen Taylor <otaylor@redhat.com>, 12/15/1998
#
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.
#
#
# If you make changes to this file, you MUST update the copy in
# acinclude.m4. [ aclocal dies on duplicate macros, so if
# we run 'aclocal -I macros/' then we'll run into problems
# once we've installed glib-gettext.m4 :-( ]
#

# serial 5

AC_DEFUN([AM_GLIB_WITH_NLS],
  dnl NLS is obligatory
  [USE_NLS=yes
    AC_SUBST(USE_NLS)

    dnl Figure out what method
    nls_cv_force_use_gnu_gettext="no"

    nls_cv_use_gnu_gettext="$nls_cv_force_use_gnu_gettext"
    if test "$nls_cv_force_use_gnu_gettext" != "yes"; then
      dnl User does not insist on using GNU NLS library.  Figure out what
      dnl to use.  If gettext or catgets are available (in this order) we
      dnl use this.  Else we have to fall back to GNU NLS library.
      dnl catgets is only used if permitted by option --with-catgets.
      nls_cv_header_intl=
      nls_cv_header_libgt=
      CATOBJEXT=NONE

      AC_CHECK_HEADER(libintl.h,
        [AC_CACHE_CHECK([for dgettext in libc], gt_cv_func_dgettext_libc,
	  [AC_TRY_LINK([#include <libintl.h>], [return (int) dgettext ("","")],
	    gt_cv_func_dgettext_libc=yes, gt_cv_func_dgettext_libc=no)])

	  if test "$gt_cv_func_dgettext_libc" != "yes"; then
	    AC_CHECK_LIB(intl, bindtextdomain,
	      [AC_CACHE_CHECK([for dgettext in libintl],
	        gt_cv_func_dgettext_libintl,
	        [AC_CHECK_LIB(intl, dgettext,
		  gt_cv_func_dgettext_libintl=yes,
		  gt_cv_func_dgettext_libintl=no)],
	        gt_cv_func_dgettext_libintl=no)])
	  fi

          if test "$gt_cv_func_dgettext_libintl" = "yes"; then
	    LIBS="$LIBS -lintl";
          fi

	  if test "$gt_cv_func_dgettext_libc" = "yes" \
	    || test "$gt_cv_func_dgettext_libintl" = "yes"; then
	    AC_DEFINE(HAVE_GETTEXT,1,
              [Define if the GNU gettext() function is already present or preinstalled.])
	    AM_PATH_PROG_WITH_TEST(MSGFMT, msgfmt,
 	      [test -z "`$ac_dir/$ac_word -h 2>&1 | grep 'dv '`"], no)dnl
	    if test "$MSGFMT" != "no"; then
	      AC_CHECK_FUNCS(dcgettext)
	      AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)
	      AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
	        [test -z "`$ac_dir/$ac_word -h 2>&1 | grep '(HELP)'`"], :)
	      AC_TRY_LINK(, [extern int _nl_msg_cat_cntr;
		 	     return _nl_msg_cat_cntr],
	        [CATOBJEXT=.gmo
	         DATADIRNAME=share],
	        [CATOBJEXT=.mo
	         DATADIRNAME=lib])
	      INSTOBJEXT=.mo
	    fi
	  fi

	  # Added by Martin Baulig 12/15/98 for libc5 systems
	  if test "$gt_cv_func_dgettext_libc" != "yes" \
	    && test "$gt_cv_func_dgettext_libintl" = "yes"; then
	    INTLLIBS=-lintl
	    LIBS=`echo $LIBS | sed -e 's/-lintl//'`
	  fi
      ])

      if test "$CATOBJEXT" = "NONE"; then
        dnl Neither gettext nor catgets in included in the C library.
        dnl Fall back on GNU gettext library.
        nls_cv_use_gnu_gettext=yes
      fi
    fi

    if test "$nls_cv_use_gnu_gettext" != "yes"; then
      AC_DEFINE(ENABLE_NLS, 1,
        [always defined to indicate that i18n is enabled])
    else
      dnl Unset this variable since we use the non-zero value as a flag.
      CATOBJEXT=
    fi

    dnl Test whether we really found GNU xgettext.
    if test "$XGETTEXT" != ":"; then
      dnl If it is no GNU xgettext we define it as : so that the
      dnl Makefiles still can work.
      if $XGETTEXT --omit-header /dev/null 2> /dev/null; then
        : ;
      else
        AC_MSG_RESULT(
	  [found xgettext program is not GNU xgettext; ignore it])
        XGETTEXT=":"
      fi
    fi

    # We need to process the po/ directory.
    POSUB=po

    AC_OUTPUT_COMMANDS(
      [case "$CONFIG_FILES" in *po/Makefile.in*)
        sed -e "/POTFILES =/r po/POTFILES" po/Makefile.in > po/Makefile
      esac])

    dnl These rules are solely for the distribution goal.  While doing this
    dnl we only have to keep exactly one list of the available catalogs
    dnl in configure.in.
    for lang in $ALL_LINGUAS; do
      GMOFILES="$GMOFILES $lang.gmo"
      POFILES="$POFILES $lang.po"
    done

    dnl Make all variables we use known to autoconf.
    AC_SUBST(CATALOGS)
    AC_SUBST(CATOBJEXT)
    AC_SUBST(DATADIRNAME)
    AC_SUBST(GMOFILES)
    AC_SUBST(INSTOBJEXT)
    AC_SUBST(INTLDEPS)
    AC_SUBST(INTLLIBS)
    AC_SUBST(INTLOBJS)
    AC_SUBST(POFILES)
    AC_SUBST(POSUB)
  ])

AC_DEFUN([AM_GLIB_GNU_GETTEXT],
  [AC_REQUIRE([AC_PROG_MAKE_SET])dnl
   AC_REQUIRE([AC_PROG_CC])dnl
   AC_REQUIRE([AC_PROG_RANLIB])dnl
   AC_REQUIRE([AC_HEADER_STDC])dnl
   AC_REQUIRE([AC_C_CONST])dnl
   AC_REQUIRE([AC_C_INLINE])dnl
   AC_REQUIRE([AC_TYPE_OFF_T])dnl
   AC_REQUIRE([AC_TYPE_SIZE_T])dnl
   AC_REQUIRE([AC_FUNC_ALLOCA])dnl
   AC_REQUIRE([AC_FUNC_MMAP])dnl

   AC_CHECK_HEADERS([argz.h limits.h locale.h nl_types.h malloc.h string.h \
unistd.h sys/param.h])
   AC_CHECK_FUNCS([getcwd munmap putenv setenv setlocale strchr strcasecmp \
strdup __argz_count __argz_stringify __argz_next])

   AM_LC_MESSAGES
   AM_GLIB_WITH_NLS

   if test "x$CATOBJEXT" != "x"; then
     if test "x$ALL_LINGUAS" = "x"; then
       LINGUAS=
     else
       AC_MSG_CHECKING(for catalogs to be installed)
       NEW_LINGUAS=
       for lang in ${LINGUAS=$ALL_LINGUAS}; do
         case "$ALL_LINGUAS" in
          *$lang*) NEW_LINGUAS="$NEW_LINGUAS $lang" ;;
         esac
       done
       LINGUAS=$NEW_LINGUAS
       AC_MSG_RESULT($LINGUAS)
     fi

     dnl Construct list of names of catalog files to be constructed.
     if test -n "$LINGUAS"; then
       for lang in $LINGUAS; do CATALOGS="$CATALOGS $lang$CATOBJEXT"; done
     fi
   fi

   dnl Determine which catalog format we have (if any is needed)
   dnl For now we know about two different formats:
   dnl   Linux libc-5 and the normal X/Open format
   test -d po || mkdir po
   if test "$CATOBJEXT" = ".cat"; then
     AC_CHECK_HEADER(linux/version.h, msgformat=linux, msgformat=xopen)

     dnl Transform the SED scripts while copying because some dumb SEDs
     dnl cannot handle comments.
     sed -e '/^#/d' $srcdir/po/$msgformat-msg.sed > po/po2msg.sed
   fi

   dnl If the AC_CONFIG_AUX_DIR macro for autoconf is used we possibly
   dnl find the mkinstalldirs script in another subdir but ($top_srcdir).
   dnl Try to locate is.
   MKINSTALLDIRS=
   if test -n "$ac_aux_dir"; then
     MKINSTALLDIRS="$ac_aux_dir/mkinstalldirs"
   fi
   if test -z "$MKINSTALLDIRS"; then
     MKINSTALLDIRS="\$(top_srcdir)/mkinstalldirs"
   fi
   AC_SUBST(MKINSTALLDIRS)

   dnl Generate list of files to be processed by xgettext which will
   dnl be included in po/Makefile.
   test -d po || mkdir po
   if test "x$srcdir" != "x."; then
     if test "x`echo $srcdir | sed 's@/.*@@'`" = "x"; then
       posrcprefix="$srcdir/"
     else
       posrcprefix="../$srcdir/"
     fi
   else
     posrcprefix="../"
   fi
   rm -f po/POTFILES
   sed -e "/^#/d" -e "/^\$/d" -e "s,.*,	$posrcprefix& \\\\," -e "\$s/\(.*\) \\\\/\1/" \
	< $srcdir/po/POTFILES.in > po/POTFILES
  ])

# Configure paths for GLIB
# Owen Taylor     97-11-3

dnl AM_PATH_GLIB([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GLIB, and define GLIB_CFLAGS and GLIB_LIBS, if "gmodule" or 
dnl gthread is specified in MODULES, pass to glib-config
dnl
AC_DEFUN([AM_PATH_GLIB],
[dnl 
dnl Get the cflags and libraries from the glib-config script
dnl
AC_ARG_WITH(glib-prefix,[  --with-glib-prefix=PFX   Prefix where GLIB is installed (optional)],
            glib_config_prefix="$withval", glib_config_prefix="")
AC_ARG_WITH(glib-exec-prefix,[  --with-glib-exec-prefix=PFX Exec prefix where GLIB is installed (optional)],
            glib_config_exec_prefix="$withval", glib_config_exec_prefix="")
AC_ARG_ENABLE(glibtest, [  --disable-glibtest       Do not try to compile and run a test GLIB program],
		    , enable_glibtest=yes)

  if test x$glib_config_exec_prefix != x ; then
     glib_config_args="$glib_config_args --exec-prefix=$glib_config_exec_prefix"
     if test x${GLIB_CONFIG+set} != xset ; then
        GLIB_CONFIG=$glib_config_exec_prefix/bin/glib-config
     fi
  fi
  if test x$glib_config_prefix != x ; then
     glib_config_args="$glib_config_args --prefix=$glib_config_prefix"
     if test x${GLIB_CONFIG+set} != xset ; then
        GLIB_CONFIG=$glib_config_prefix/bin/glib-config
     fi
  fi

  for module in . $4
  do
      case "$module" in
         gmodule) 
             glib_config_args="$glib_config_args gmodule"
         ;;
         gthread) 
             glib_config_args="$glib_config_args gthread"
         ;;
      esac
  done

  AC_PATH_PROG(GLIB_CONFIG, glib-config, no)
  min_glib_version=ifelse([$1], ,0.99.7,$1)
  AC_MSG_CHECKING(for GLIB - version >= $min_glib_version)
  no_glib=""
  if test "$GLIB_CONFIG" = "no" ; then
    no_glib=yes
  else
    GLIB_CFLAGS=`$GLIB_CONFIG $glib_config_args --cflags`
    GLIB_LIBS=`$GLIB_CONFIG $glib_config_args --libs`
    glib_config_major_version=`$GLIB_CONFIG $glib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    glib_config_minor_version=`$GLIB_CONFIG $glib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    glib_config_micro_version=`$GLIB_CONFIG $glib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_glibtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GLIB_CFLAGS"
      LIBS="$GLIB_LIBS $LIBS"
dnl
dnl Now check if the installed GLIB is sufficiently new. (Also sanity
dnl checks the results of glib-config to some extent
dnl
      rm -f conf.glibtest
      AC_TRY_RUN([
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.glibtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_glib_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_glib_version");
     exit(1);
   }

  if ((glib_major_version != $glib_config_major_version) ||
      (glib_minor_version != $glib_config_minor_version) ||
      (glib_micro_version != $glib_config_micro_version))
    {
      printf("\n*** 'glib-config --version' returned %d.%d.%d, but GLIB (%d.%d.%d)\n", 
             $glib_config_major_version, $glib_config_minor_version, $glib_config_micro_version,
             glib_major_version, glib_minor_version, glib_micro_version);
      printf ("*** was found! If glib-config was correct, then it is best\n");
      printf ("*** to remove the old version of GLIB. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If glib-config was wrong, set the environment variable GLIB_CONFIG\n");
      printf("*** to point to the correct copy of glib-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    } 
  else if ((glib_major_version != GLIB_MAJOR_VERSION) ||
	   (glib_minor_version != GLIB_MINOR_VERSION) ||
           (glib_micro_version != GLIB_MICRO_VERSION))
    {
      printf("*** GLIB header files (version %d.%d.%d) do not match\n",
	     GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     glib_major_version, glib_minor_version, glib_micro_version);
    }
  else
    {
      if ((glib_major_version > major) ||
        ((glib_major_version == major) && (glib_minor_version > minor)) ||
        ((glib_major_version == major) && (glib_minor_version == minor) && (glib_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GLIB (%d.%d.%d) was found.\n",
               glib_major_version, glib_minor_version, glib_micro_version);
        printf("*** You need a version of GLIB newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GLIB is always available from ftp://ftp.gtk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the glib-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GLIB, but you can also set the GLIB_CONFIG environment to point to the\n");
        printf("*** correct copy of glib-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_glib=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_glib" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$GLIB_CONFIG" = "no" ; then
       echo "*** The glib-config script installed by GLIB could not be found"
       echo "*** If GLIB was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the GLIB_CONFIG environment variable to the"
       echo "*** full path to glib-config."
     else
       if test -f conf.glibtest ; then
        :
       else
          echo "*** Could not run GLIB test program, checking why..."
          CFLAGS="$CFLAGS $GLIB_CFLAGS"
          LIBS="$LIBS $GLIB_LIBS"
          AC_TRY_LINK([
#include <glib.h>
#include <stdio.h>
],      [ return ((glib_major_version) || (glib_minor_version) || (glib_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GLIB or finding the wrong"
          echo "*** version of GLIB. If it is not finding GLIB, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
          echo "***"
          echo "*** If you have a RedHat 5.0 system, you should remove the GTK package that"
          echo "*** came with the system with the command"
          echo "***"
          echo "***    rpm --erase --nodeps gtk gtk-devel" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GLIB was incorrectly installed"
          echo "*** or that you have moved GLIB since it was installed. In the latter case, you"
          echo "*** may want to edit the glib-config script: $GLIB_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GLIB_CFLAGS=""
     GLIB_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GLIB_CFLAGS)
  AC_SUBST(GLIB_LIBS)
  rm -f conf.glibtest
])
#serial 2
# Test for the GNU C Library, version 2.1 or newer.
# From Bruno Haible.

AC_DEFUN([jm_GLIBC21],
  [
    AC_CACHE_CHECK(whether we are using the GNU C Library 2.1 or newer,
      ac_cv_gnu_library_2_1,
      [AC_EGREP_CPP([Lucky GNU user],
	[
#include <features.h>
#ifdef __GNU_LIBRARY__
 #if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1) || (__GLIBC__ > 2)
  Lucky GNU user
 #endif
#endif
	],
	ac_cv_gnu_library_2_1=yes,
	ac_cv_gnu_library_2_1=no)
      ]
    )
    AC_SUBST(GLIBC21)
    GLIBC21="$ac_cv_gnu_library_2_1"
  ]
)
# Configure paths for GTK+
# Owen Taylor     1997-2001

dnl AM_PATH_GTK_2_0([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GTK+, and define GTK_CFLAGS and GTK_LIBS, if gthread is specified in MODULES, 
dnl pass to pkg-config
dnl
AC_DEFUN([AM_PATH_GTK_2_0],
[dnl 
dnl Get the cflags and libraries from pkg-config
dnl
AC_ARG_ENABLE(gtktest, [  --disable-gtktest       do not try to compile and run a test GTK+ program],
		    , enable_gtktest=yes)

  pkg_config_args=gtk+-2.0
  for module in . $4
  do
      case "$module" in
         gthread) 
             pkg_config_args="$pkg_config_args gthread-2.0"
         ;;
      esac
  done

  no_gtk=""

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

  if test x$PKG_CONFIG != xno ; then
    if pkg-config --atleast-pkgconfig-version 0.7 ; then
      :
    else
      echo *** pkg-config too old; version 0.7 or better required.
      no_gtk=yes
      PKG_CONFIG=no
    fi
  else
    no_gtk=yes
  fi

  min_gtk_version=ifelse([$1], ,2.0.0,$1)
  AC_MSG_CHECKING(for GTK+ - version >= $min_gtk_version)

  if test x$PKG_CONFIG != xno ; then
    ## don't try to run the test against uninstalled libtool libs
    if $PKG_CONFIG --uninstalled $pkg_config_args; then
	  echo "Will use uninstalled version of GTK+ found in PKG_CONFIG_PATH"
	  enable_gtktest=no
    fi

    if $PKG_CONFIG --atleast-version $min_gtk_version $pkg_config_args; then
	  :
    else
	  no_gtk=yes
    fi
  fi

  if test x"$no_gtk" = x ; then
    GTK_CFLAGS=`$PKG_CONFIG $pkg_config_args --cflags`
    GTK_LIBS=`$PKG_CONFIG $pkg_config_args --libs`
    gtk_config_major_version=`$PKG_CONFIG --modversion gtk+-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gtk_config_minor_version=`$PKG_CONFIG --modversion gtk+-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gtk_config_micro_version=`$PKG_CONFIG --modversion gtk+-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_gtktest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GTK_CFLAGS"
      LIBS="$GTK_LIBS $LIBS"
dnl
dnl Now check if the installed GTK+ is sufficiently new. (Also sanity
dnl checks the results of pkg-config to some extent)
dnl
      rm -f conf.gtktest
      AC_TRY_RUN([
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.gtktest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_gtk_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_gtk_version");
     exit(1);
   }

  if ((gtk_major_version != $gtk_config_major_version) ||
      (gtk_minor_version != $gtk_config_minor_version) ||
      (gtk_micro_version != $gtk_config_micro_version))
    {
      printf("\n*** 'pkg-config --modversion gtk+-2.0' returned %d.%d.%d, but GTK+ (%d.%d.%d)\n", 
             $gtk_config_major_version, $gtk_config_minor_version, $gtk_config_micro_version,
             gtk_major_version, gtk_minor_version, gtk_micro_version);
      printf ("*** was found! If pkg-config was correct, then it is best\n");
      printf ("*** to remove the old version of GTK+. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If pkg-config was wrong, set the environment variable PKG_CONFIG_PATH\n");
      printf("*** to point to the correct configuration files\n");
    } 
  else if ((gtk_major_version != GTK_MAJOR_VERSION) ||
	   (gtk_minor_version != GTK_MINOR_VERSION) ||
           (gtk_micro_version != GTK_MICRO_VERSION))
    {
      printf("*** GTK+ header files (version %d.%d.%d) do not match\n",
	     GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     gtk_major_version, gtk_minor_version, gtk_micro_version);
    }
  else
    {
      if ((gtk_major_version > major) ||
        ((gtk_major_version == major) && (gtk_minor_version > minor)) ||
        ((gtk_major_version == major) && (gtk_minor_version == minor) && (gtk_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GTK+ (%d.%d.%d) was found.\n",
               gtk_major_version, gtk_minor_version, gtk_micro_version);
        printf("*** You need a version of GTK+ newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GTK+ is always available from ftp://ftp.gtk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the pkg-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GTK+, but you can also set the PKG_CONFIG environment to point to the\n");
        printf("*** correct copy of pkg-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_gtk=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_gtk" = x ; then
     AC_MSG_RESULT(yes (version $gtk_config_major_version.$gtk_config_minor_version.$gtk_config_micro_version))
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$PKG_CONFIG" = "no" ; then
       echo "*** A new enough version of pkg-config was not found."
       echo "*** See http://pkgconfig.sourceforge.net"
     else
       if test -f conf.gtktest ; then
        :
       else
          echo "*** Could not run GTK+ test program, checking why..."
	  ac_save_CFLAGS="$CFLAGS"
	  ac_save_LIBS="$LIBS"
          CFLAGS="$CFLAGS $GTK_CFLAGS"
          LIBS="$LIBS $GTK_LIBS"
          AC_TRY_LINK([
#include <gtk/gtk.h>
#include <stdio.h>
],      [ return ((gtk_major_version) || (gtk_minor_version) || (gtk_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GTK+ or finding the wrong"
          echo "*** version of GTK+. If it is not finding GTK+, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GTK+ is incorrectly installed."])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GTK_CFLAGS=""
     GTK_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GTK_CFLAGS)
  AC_SUBST(GTK_LIBS)
  rm -f conf.gtktest
])
# Try to configure the GTK+-1.2 client (gui-gtk)

# FC_GTK_CLIENT
# Test for GTK+-1.2 libraries needed for gui-gtk

AC_DEFUN([FC_GTK_CLIENT],
[
  if test "$client" = gtk || test "$client" = yes ; then
    AM_PATH_GTK(1.2.5,
      [
        AM_PATH_GDK_IMLIB(1.9.2,
          [
            client=gtk
            CLIENT_CFLAGS="$GDK_IMLIB_CFLAGS"
            CLIENT_LIBS="$GDK_IMLIB_LIBS"
          ],
          [
            FC_NO_CLIENT([gtk], [Imlib is needed])
          ])
      ],
      [
        FC_NO_CLIENT([gtk], [GTK libraries not found])
      ])
  fi
])
# Configure paths for GTK+
# Owen Taylor     97-11-3

dnl AM_PATH_GTK([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GTK, and define GTK_CFLAGS and GTK_LIBS
dnl
AC_DEFUN([AM_PATH_GTK],
[dnl 
dnl Get the cflags and libraries from the gtk-config script
dnl
AC_ARG_WITH(gtk-prefix,[  --with-gtk-prefix=PFX   Prefix where GTK is installed (optional)],
            gtk_config_prefix="$withval", gtk_config_prefix="")
AC_ARG_WITH(gtk-exec-prefix,[  --with-gtk-exec-prefix=PFX Exec prefix where GTK is installed (optional)],
            gtk_config_exec_prefix="$withval", gtk_config_exec_prefix="")
AC_ARG_ENABLE(gtktest, [  --disable-gtktest       Do not try to compile and run a test GTK program],
		    , enable_gtktest=yes)

  for module in . $4
  do
      case "$module" in
         gthread) 
             gtk_config_args="$gtk_config_args gthread"
         ;;
      esac
  done

  if test x$gtk_config_exec_prefix != x ; then
     gtk_config_args="$gtk_config_args --exec-prefix=$gtk_config_exec_prefix"
     if test x${GTK_CONFIG+set} != xset ; then
        GTK_CONFIG=$gtk_config_exec_prefix/bin/gtk-config
     fi
  fi
  if test x$gtk_config_prefix != x ; then
     gtk_config_args="$gtk_config_args --prefix=$gtk_config_prefix"
     if test x${GTK_CONFIG+set} != xset ; then
        GTK_CONFIG=$gtk_config_prefix/bin/gtk-config
     fi
  fi

  AC_PATH_PROGS(GTK_CONFIG, gtk-config gtk12-config gtk13-config, no)
  min_gtk_version=ifelse([$1], ,0.99.7,$1)
  AC_MSG_CHECKING(for GTK - version >= $min_gtk_version)
  no_gtk=""
  if test "$GTK_CONFIG" = "no" ; then
    no_gtk=yes
  else
    GTK_CFLAGS=`$GTK_CONFIG $gtk_config_args --cflags`
    GTK_LIBS=`$GTK_CONFIG $gtk_config_args --libs`
    gtk_config_major_version=`$GTK_CONFIG $gtk_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gtk_config_minor_version=`$GTK_CONFIG $gtk_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gtk_config_micro_version=`$GTK_CONFIG $gtk_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_gtktest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GTK_CFLAGS"
      LIBS="$GTK_LIBS $LIBS"
dnl
dnl Now check if the installed GTK is sufficiently new. (Also sanity
dnl checks the results of gtk-config to some extent
dnl
      rm -f conf.gtktest
      AC_TRY_RUN([
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.gtktest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_gtk_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_gtk_version");
     exit(1);
   }

  if ((gtk_major_version != $gtk_config_major_version) ||
      (gtk_minor_version != $gtk_config_minor_version) ||
      (gtk_micro_version != $gtk_config_micro_version))
    {
      printf("\n*** 'gtk-config --version' returned %d.%d.%d, but GTK+ (%d.%d.%d)\n", 
             $gtk_config_major_version, $gtk_config_minor_version, $gtk_config_micro_version,
             gtk_major_version, gtk_minor_version, gtk_micro_version);
      printf ("*** was found! If gtk-config was correct, then it is best\n");
      printf ("*** to remove the old version of GTK+. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If gtk-config was wrong, set the environment variable GTK_CONFIG\n");
      printf("*** to point to the correct copy of gtk-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    } 
#if defined (GTK_MAJOR_VERSION) && defined (GTK_MINOR_VERSION) && defined (GTK_MICRO_VERSION)
  else if ((gtk_major_version != GTK_MAJOR_VERSION) ||
	   (gtk_minor_version != GTK_MINOR_VERSION) ||
           (gtk_micro_version != GTK_MICRO_VERSION))
    {
      printf("*** GTK+ header files (version %d.%d.%d) do not match\n",
	     GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     gtk_major_version, gtk_minor_version, gtk_micro_version);
    }
#endif /* defined (GTK_MAJOR_VERSION) ... */
  else
    {
      if ((gtk_major_version > major) ||
        ((gtk_major_version == major) && (gtk_minor_version > minor)) ||
        ((gtk_major_version == major) && (gtk_minor_version == minor) && (gtk_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GTK+ (%d.%d.%d) was found.\n",
               gtk_major_version, gtk_minor_version, gtk_micro_version);
        printf("*** You need a version of GTK+ newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GTK+ is always available from ftp://ftp.gtk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the gtk-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GTK+, but you can also set the GTK_CONFIG environment to point to the\n");
        printf("*** correct copy of gtk-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_gtk=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_gtk" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$GTK_CONFIG" = "no" ; then
       echo "*** The gtk-config script installed by GTK could not be found"
       echo "*** If GTK was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the GTK_CONFIG environment variable to the"
       echo "*** full path to gtk-config."
     else
       if test -f conf.gtktest ; then
        :
       else
          echo "*** Could not run GTK test program, checking why..."
          CFLAGS="$CFLAGS $GTK_CFLAGS"
          LIBS="$LIBS $GTK_LIBS"
          AC_TRY_LINK([
#include <gtk/gtk.h>
#include <stdio.h>
],      [ return ((gtk_major_version) || (gtk_minor_version) || (gtk_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GTK or finding the wrong"
          echo "*** version of GTK. If it is not finding GTK, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
          echo "***"
          echo "*** If you have a RedHat 5.0 system, you should remove the GTK package that"
          echo "*** came with the system with the command"
          echo "***"
          echo "***    rpm --erase --nodeps gtk gtk-devel" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GTK was incorrectly installed"
          echo "*** or that you have moved GTK since it was installed. In the latter case, you"
          echo "*** may want to edit the gtk-config script: $GTK_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GTK_CFLAGS=""
     GTK_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GTK_CFLAGS)
  AC_SUBST(GTK_LIBS)
  rm -f conf.gtktest
])
# Try to configure the GTK+-2.0 client (gui-gtk-2.0)

# FC_GTK_CLIENT
# Test for GTK+-2.0 libraries needed for gui-gtk-2.0

AC_DEFUN([FC_GTK2_CLIENT],
[
  if test "$client" = "gtk-2.0" || test "$client" = yes ; then
    AM_PATH_GTK_2_0(2.2.1,
      [
        client="gtk-2.0"
        CLIENT_CFLAGS="$GTK_CFLAGS"
        CLIENT_LIBS="$GTK_LIBS"
        if test x"$MINGW32" = "xyes"; then
          dnl Required to compile gtk2 on Windows platform
          CFLAGS="$CFLAGS -mms-bitfields"
          CLIENT_LDFLAGS="$LDFLAGS -mwindows"
        fi
      ],
      [
        FC_NO_CLIENT([gtk-2.0], [GTK+-2.0 libraries not found])
      ])
  fi
])
#serial AM2
dnl From Bruno Haible.

AC_DEFUN([AM_ICONV],
[
  dnl Some systems have iconv in libc, some have it in libiconv (OSF/1 and
  dnl those with the standalone portable GNU libiconv installed).

  AC_ARG_WITH([libiconv-prefix],
[  --with-libiconv-prefix=DIR  search for libiconv in DIR/include and DIR/lib], [
    for dir in `echo "$withval" | tr : ' '`; do
      if test -d $dir/include; then CPPFLAGS="$CPPFLAGS -I$dir/include"; fi
      if test -d $dir/lib; then LDFLAGS="$LDFLAGS -L$dir/lib"; fi
    done
   ])

  AC_CACHE_CHECK(for iconv, am_cv_func_iconv, [
    am_cv_func_iconv="no, consider installing GNU libiconv"
    am_cv_lib_iconv=no
    AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
      [iconv_t cd = iconv_open("","");
       iconv(cd,NULL,NULL,NULL,NULL);
       iconv_close(cd);],
      am_cv_func_iconv=yes)
    if test "$am_cv_func_iconv" != yes; then
      am_save_LIBS="$LIBS"
      LIBS="$LIBS -liconv"
      AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
        [iconv_t cd = iconv_open("","");
         iconv(cd,NULL,NULL,NULL,NULL);
         iconv_close(cd);],
        am_cv_lib_iconv=yes
        am_cv_func_iconv=yes)
      LIBS="$am_save_LIBS"
    fi
  ])
  if test "$am_cv_func_iconv" = yes; then
    AC_DEFINE(HAVE_ICONV, 1, [Define if you have the iconv() function.])
    AC_MSG_CHECKING([for iconv declaration])
    AC_CACHE_VAL(am_cv_proto_iconv, [
      AC_TRY_COMPILE([
#include <stdlib.h>
#include <iconv.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
#else
size_t iconv();
#endif
], [], am_cv_proto_iconv_arg1="", am_cv_proto_iconv_arg1="const")
      am_cv_proto_iconv="extern size_t iconv (iconv_t cd, $am_cv_proto_iconv_arg1 char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);"])
    am_cv_proto_iconv=`echo "[$]am_cv_proto_iconv" | tr -s ' ' | sed -e 's/( /(/'`
    AC_MSG_RESULT([$]{ac_t:-
         }[$]am_cv_proto_iconv)
    AC_DEFINE_UNQUOTED(ICONV_CONST, $am_cv_proto_iconv_arg1,
      [Define as const if the declaration of iconv() needs const.])
  fi
  LIBICONV=
  if test "$am_cv_lib_iconv" = yes; then
    LIBICONV="-liconv"
  fi
  AC_SUBST(LIBICONV)
])
# Configure paths for IMLIB
# Frank Belew     98-8-31
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_IMLIB([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for IMLIB, and define IMLIB_CFLAGS and IMLIB_LIBS
dnl
AC_DEFUN([AM_PATH_IMLIB],
[dnl 
dnl Get the cflags and libraries from the imlib-config script
dnl
AC_ARG_WITH(imlib-prefix,[  --with-imlib-prefix=PFX   Prefix where IMLIB is installed (optional)],
            imlib_prefix="$withval", imlib_prefix="")
AC_ARG_WITH(imlib-exec-prefix,[  --with-imlib-exec-prefix=PFX Exec prefix where IMLIB is installed (optional)],
            imlib_exec_prefix="$withval", imlib_exec_prefix="")
AC_ARG_ENABLE(imlibtest, [  --disable-imlibtest       Do not try to compile and run a test IMLIB program],
		    , enable_imlibtest=yes)

  if test x$imlib_exec_prefix != x ; then
     imlib_args="$imlib_args --exec-prefix=$imlib_exec_prefix"
     if test x${IMLIB_CONFIG+set} != xset ; then
        IMLIB_CONFIG=$imlib_exec_prefix/bin/imlib-config
     fi
  fi
  if test x$imlib_prefix != x ; then
     imlib_args="$imlib_args --prefix=$imlib_prefix"
     if test x${IMLIB_CONFIG+set} != xset ; then
        IMLIB_CONFIG=$imlib_prefix/bin/imlib-config
     fi
  fi

  AC_PATH_PROG(IMLIB_CONFIG, imlib-config, no)
  min_imlib_version=ifelse([$1], ,1.8.2,$1)
  AC_MSG_CHECKING(for IMLIB - version >= $min_imlib_version)
  no_imlib=""
  if test "$IMLIB_CONFIG" = "no" ; then
    no_imlib=yes
  else
    IMLIB_CFLAGS=`$IMLIB_CONFIG $imlibconf_args --cflags`
    IMLIB_LIBS=`$IMLIB_CONFIG $imlibconf_args --libs`

    imlib_major_version=`$IMLIB_CONFIG $imlib_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    imlib_minor_version=`$IMLIB_CONFIG $imlib_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    imlib_micro_version=`$IMLIB_CONFIG $imlib_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_imlibtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $IMLIB_CFLAGS"
      LIBS="$LIBS $IMLIB_LIBS"
dnl
dnl Now check if the installed IMLIB is sufficiently new. (Also sanity
dnl checks the results of imlib-config to some extent
dnl
      rm -f conf.imlibtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Imlib.h>

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.imlibtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_imlib_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_imlib_version");
     exit(1);
   }

    if (($imlib_major_version > major) ||
        (($imlib_major_version == major) && ($imlib_minor_version > minor)) ||
	(($imlib_major_version == major) && ($imlib_minor_version == minor) &&
	($imlib_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'imlib-config --version' returned %d.%d, but the minimum version\n", $imlib_major_version, $imlib_minor_version);
      printf("*** of IMLIB required is %d.%d. If imlib-config is correct, then it is\n", major, minor);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If imlib-config was wrong, set the environment variable IMLIB_CONFIG\n");
      printf("*** to point to the correct copy of imlib-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_imlib=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_imlib" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$IMLIB_CONFIG" = "no" ; then
       echo "*** The imlib-config script installed by IMLIB could not be found"
       echo "*** If IMLIB was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the IMLIB_CONFIG environment variable to the"
       echo "*** full path to imlib-config."
     else
       if test -f conf.imlibtest ; then
        :
       else
          echo "*** Could not run IMLIB test program, checking why..."
          CFLAGS="$CFLAGS $IMLIB_CFLAGS"
          LIBS="$LIBS $IMLIB_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <Imlib.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding IMLIB or finding the wrong"
          echo "*** version of IMLIB. If it is not finding IMLIB, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means IMLIB was incorrectly installed"
          echo "*** or that you have moved IMLIB since it was installed. In the latter case, you"
          echo "*** may want to edit the imlib-config script: $IMLIB_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     IMLIB_CFLAGS=""
     IMLIB_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(IMLIB_CFLAGS)
  AC_SUBST(IMLIB_LIBS)
  rm -f conf.imlibtest
])

# Check for gdk-imlib
AC_DEFUN([AM_PATH_GDK_IMLIB],
[dnl 
dnl Get the cflags and libraries from the imlib-config script
dnl
AC_ARG_WITH(imlib-prefix,[  --with-imlib-prefix=PFX   Prefix where IMLIB is installed (optional)],
            imlib_prefix="$withval", imlib_prefix="")
AC_ARG_WITH(imlib-exec-prefix,[  --with-imlib-exec-prefix=PFX Exec prefix where IMLIB is installed (optional)],
            imlib_exec_prefix="$withval", imlib_exec_prefix="")
AC_ARG_ENABLE(imlibtest, [  --disable-imlibtest       Do not try to compile and run a test IMLIB program],
		    , enable_imlibtest=yes)

  if test x$imlib_exec_prefix != x ; then
     imlib_args="$imlib_args --exec-prefix=$imlib_exec_prefix"
     if test x${IMLIB_CONFIG+set} != xset ; then
        IMLIB_CONFIG=$imlib_exec_prefix/bin/imlib-config
     fi
  fi
  if test x$imlib_prefix != x ; then
     imlib_args="$imlib_args --prefix=$imlib_prefix"
     if test x${IMLIB_CONFIG+set} != xset ; then
        IMLIB_CONFIG=$imlib_prefix/bin/imlib-config
     fi
  fi

  AC_PATH_PROG(IMLIB_CONFIG, imlib-config, no)
  min_imlib_version=ifelse([$1], ,1.8.2,$1)
  AC_MSG_CHECKING(for IMLIB - version >= $min_imlib_version)
  no_imlib=""
  if test "$IMLIB_CONFIG" = "no" ; then
    no_imlib=yes
  else
    GDK_IMLIB_CFLAGS=`$IMLIB_CONFIG $imlibconf_args --cflags-gdk`
    GDK_IMLIB_LIBS=`$IMLIB_CONFIG $imlibconf_args --libs-gdk`

    imlib_major_version=`$IMLIB_CONFIG $imlib_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    imlib_minor_version=`$IMLIB_CONFIG $imlib_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    if test "x$enable_imlibtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GDK_IMLIB_CFLAGS"
      LIBS="$LIBS $GDK_IMLIB_LIBS"
dnl
dnl Now check if the installed IMLIB is sufficiently new. (Also sanity
dnl checks the results of imlib-config to some extent
dnl
      rm -f conf.imlibtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <gdk_imlib.h>

int main ()
{
  int major, minor;
  char *tmp_version;

  system ("touch conf.gdkimlibtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_imlib_version");
  if (sscanf(tmp_version, "%d.%d", &major, &minor) != 2) {
     printf("%s, bad version string\n", "$min_imlib_version");
     exit(1);
   }

    if (($imlib_major_version > major) ||
        (($imlib_major_version == major) && ($imlib_minor_version >= minor)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'imlib-config --version' returned %d.%d, but the minimum version\n", $imlib_major_version, $imlib_minor_version);
      printf("*** of IMLIB required is %d.%d. If imlib-config is correct, then it is\n", major, minor);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If imlib-config was wrong, set the environment variable IMLIB_CONFIG\n");
      printf("*** to point to the correct copy of imlib-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_imlib=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_imlib" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$IMLIB_CONFIG" = "no" ; then
       echo "*** The imlib-config script installed by IMLIB could not be found"
       echo "*** If IMLIB was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the IMLIB_CONFIG environment variable to the"
       echo "*** full path to imlib-config."
     else
       if test -f conf.gdkimlibtest ; then
        :
       else
          echo "*** Could not run IMLIB test program, checking why..."
          CFLAGS="$CFLAGS $GDK_IMLIB_CFLAGS"
          LIBS="$LIBS $GDK_IMLIB_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <gdk_imlib.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding IMLIB or finding the wrong"
          echo "*** version of IMLIB. If it is not finding IMLIB, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means IMLIB was incorrectly installed"
          echo "*** or that you have moved IMLIB since it was installed. In the latter case, you"
          echo "*** may want to edit the imlib-config script: $IMLIB_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     IMLIB_CFLAGS=""
     IMLIB_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GDK_IMLIB_CFLAGS)
  AC_SUBST(GDK_IMLIB_LIBS)
  rm -f conf.gdkimlibtest
])
#serial 1
# This test replaces the one in autoconf.
# Currently this macro should have the same name as the autoconf macro
# because gettext's gettext.m4 (distributed in the automake package)
# still uses it.  Otherwise, the use in gettext.m4 makes autoheader
# give these diagnostics:
#   configure.in:556: AC_TRY_COMPILE was called before AC_ISC_POSIX
#   configure.in:556: AC_TRY_RUN was called before AC_ISC_POSIX

undefine([AC_ISC_POSIX])

AC_DEFUN([AC_ISC_POSIX],
  [
    dnl This test replaces the obsolescent AC_ISC_POSIX kludge.
    AC_CHECK_LIB(cposix, strerror, [LIBS="$LIBS -lcposix"])
  ]
)

# Check whether LC_MESSAGES is available in <locale.h>.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU General Public
# License or the GNU Library General Public License but which still want
# to provide support for the GNU gettext functionality.
# Please note that the actual code of the GNU gettext library is covered
# by the GNU Library General Public License, and the rest of the GNU
# gettext package package is covered by the GNU General Public License.
# They are *not* in the public domain.

# serial 2

AC_DEFUN([AM_LC_MESSAGES],
  [if test $ac_cv_header_locale_h = yes; then
    AC_CACHE_CHECK([for LC_MESSAGES], am_cv_val_LC_MESSAGES,
      [AC_TRY_LINK([#include <locale.h>], [return LC_MESSAGES],
       am_cv_val_LC_MESSAGES=yes, am_cv_val_LC_MESSAGES=no)])
    if test $am_cv_val_LC_MESSAGES = yes; then
      AC_DEFINE(HAVE_LC_MESSAGES, 1,
        [Define if your <locale.h> file defines LC_MESSAGES.])
    fi
  fi])
#serial AM1
dnl From Bruno Haible.

AC_DEFUN([AM_LANGINFO_CODESET],
[
  AC_CACHE_CHECK([for nl_langinfo and CODESET], am_cv_langinfo_codeset,
    [AC_TRY_LINK([#include <langinfo.h>],
      [char* cs = nl_langinfo(CODESET);],
      am_cv_langinfo_codeset=yes,
      am_cv_langinfo_codeset=no)
    ])
  if test $am_cv_langinfo_codeset = yes; then
    AC_DEFINE(HAVE_LANGINFO_CODESET, 1,
      [Define if you have <langinfo.h> and nl_langinfo(CODESET).])
  fi
])

AC_DEFUN([AM_LIBCHARSET],
[
  AC_CACHE_CHECK([for libcharset], am_cv_libcharset,
    [lc_save_LIBS="$LIBS"
     LIBS="$LIBS $LIBICONV"
     AC_TRY_LINK([#include <libcharset.h>],
      [locale_charset()],
      am_cv_libcharset=yes,
      am_cv_libcharset=no) 
      LIBS="$lc_save_LIBS" 
    ])
  if test $am_cv_libcharset = yes; then
    AC_DEFINE(HAVE_LIBCHARSET, 1,
      [Define if you have <libcharset.h> and locale_charset().])
  fi
])
dnl FC_CHECK_NGETTEXT_RUNTIME(EXTRA-LIBS, ACTION-IF-FOUND, ACTION-IF-NOT-FOUND)
dnl
dnl This tests whether ngettext works at runtime.  Here, "works"
dnl means "doesn't dump core", as some versions (for exmaple the 
dnl version which comes with glibc 2.2.5 is broken, gettext 
dnl version 0.10.38 however is ok).

AC_DEFUN([FC_CHECK_NGETTEXT_RUNTIME],
[
templibs="$LIBS"
LIBS="$1 $LIBS"
templang="$LANG"
LANG="de_DE"
AC_TRY_RUN([
/*
 * Check to make sure that ngettext works at runtime. Specifically,
 * some gettext versions dump core if the ngettext function is called.
 * (c) 2002 Raimar Falke <rf13@inf.tu-dresden.de>
 */
#include <string.h>
#include <libintl.h>
#include <locale.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  setlocale(LC_ALL, "");

  if (strcmp(ngettext("unit", "units", 1), "unit") == 0 &&
      strcmp(ngettext("unit", "units", 2), "units") == 0) {
    return 0;
  } else {
    return 1;
  }
}
],
[AC_MSG_RESULT(yes)
  [$2]],
[AC_MSG_RESULT(no)
  [$3]],
[AC_MSG_RESULT(unknown: cross-compiling)
  [$2]])
LIBS="$templibs"
LANG="$templang"
])
# Fail because a client wasn't found, if it was requested

# FC_NO_CLIENT($which_client, $error_message)
# The $1 client has failed its configure check; it cannot be compiled.  This
# simple macro will exit if this was the requested client, giving an error
# message including $2.  If this wasn't the specified client, it will do
# nothing and configure should continue...

AC_DEFUN([FC_NO_CLIENT],
[
  if test "$client" = "$1"; then
    AC_MSG_ERROR([specified client '$1' not configurable ($2)])
  fi
])
# Search path for a program which passes the given test.
# Ulrich Drepper <drepper@cygnus.com>, 1996.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU General Public
# License or the GNU Library General Public License but which still want
# to provide support for the GNU gettext functionality.
# Please note that the actual code of the GNU gettext library is covered
# by the GNU Library General Public License, and the rest of the GNU
# gettext package package is covered by the GNU General Public License.
# They are *not* in the public domain.

# serial 2

dnl AM_PATH_PROG_WITH_TEST(VARIABLE, PROG-TO-CHECK-FOR,
dnl   TEST-PERFORMED-ON-FOUND_PROGRAM [, VALUE-IF-NOT-FOUND [, PATH]])
AC_DEFUN([AM_PATH_PROG_WITH_TEST],
[# Extract the first word of "$2", so it can be a program name with args.
set dummy $2; ac_word=[$]2
AC_MSG_CHECKING([for $ac_word])
AC_CACHE_VAL(ac_cv_path_$1,
[case "[$]$1" in
  /*)
  ac_cv_path_$1="[$]$1" # Let the user override the test with a path.
  ;;
  *)
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:"
  for ac_dir in ifelse([$5], , $PATH, [$5]); do
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$ac_word; then
      if [$3]; then
	ac_cv_path_$1="$ac_dir/$ac_word"
	break
      fi
    fi
  done
  IFS="$ac_save_ifs"
dnl If no 4th arg is given, leave the cache variable unset,
dnl so AC_PATH_PROGS will keep looking.
ifelse([$4], , , [  test -z "[$]ac_cv_path_$1" && ac_cv_path_$1="$4"
])dnl
  ;;
esac])dnl
$1="$ac_cv_path_$1"
if test ifelse([$4], , [-n "[$]$1"], ["[$]$1" != "$4"]); then
  AC_MSG_RESULT([$]$1)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST($1)dnl
])

dnl FC_CHECK_READLINE_RUNTIME(EXTRA-LIBS, ACTION-IF-FOUND, ACTION-IF-NOT-FOUND)
dnl
dnl This tests whether readline works at runtime.  Here, "works"
dnl means "doesn't dump core", as some versions do if linked
dnl against wrong ncurses library.  Compiles with LIBS modified 
dnl to included -lreadline and parameter EXTRA-LIBS.
dnl Should already have checked that header and library exist.
dnl
AC_DEFUN([FC_CHECK_READLINE_RUNTIME],
[AC_MSG_CHECKING(whether readline works at runtime)
templibs="$LIBS"
LIBS="-lreadline $1 $LIBS"
AC_TRY_RUN([
/*
 * testrl.c
 * File revision 0
 * Check to make sure that readline works at runtime.
 * (Specifically, some readline packages link against a wrong 
 * version of ncurses library and dump core at runtime.)
 * (c) 2000 Jacob Lundberg, jacob@chaos2.org
 */

#include <stdio.h>
/* We assume that the presence of readline has already been verified. */
#include <readline/readline.h>
#include <readline/history.h>

/* Setup for readline. */
#define TEMP_FILE "./conftest.readline.runtime"

static void handle_readline_input_callback(char *line) {
/* Generally taken from freeciv-1.11.4/server/sernet.c. */
  if(line) {
    if(*line)
      add_history(line);
    /* printf(line); */
  }
}

int main(void) {
/* Try to init readline and see if it barfs. */
  using_history();
  read_history(TEMP_FILE);
  rl_initialize();
  rl_callback_handler_install("_ ", handle_readline_input_callback);
  rl_callback_handler_remove();  /* needed to re-set terminal */
  return(0);
}
],
[AC_MSG_RESULT(yes)
  [$2]],
[AC_MSG_RESULT(no)
  [$3]],
[AC_MSG_RESULT(unknown: cross-compiling)
  [$2]])
LIBS="$templibs"
])

AC_DEFUN([FC_HAS_READLINE],
[
    dnl Readline library and header files.
    if test "$WITH_READLINE" = "yes" || test "$WITH_READLINE" = "maybe"; then
       HAVE_TERMCAP="";
       dnl Readline header
       AC_CHECK_HEADER(readline/readline.h,
                       have_readline_header=1,
                       have_readline_header=0)
       if test "$have_readline_header" = "0"; then
           if test "$WITH_READLINE" = "yes"; then
               AC_MSG_ERROR(Did not find readline header file. 
You may need to install a readline \"development\" package.)
           else
               AC_MSG_WARN(Did not find readline header file. 
Configuring server without readline support.)
           fi
       else
           dnl Readline lib
           AC_CHECK_LIB(readline, completion_matches, 
                         have_readline_lib=1, have_readline_lib=0)
           dnl Readline lib >= 4.2
           AC_CHECK_LIB(readline, rl_completion_matches, 
                         have_new_readline_lib=1, have_new_readline_lib=0)
           if test "$have_readline_lib" != "1" && test "$have_new_readline_lib" != "1"; then
               dnl Many readline installations are broken in that they
               dnl don't set the dependency on the curses lib up correctly.
               dnl We give them a hand by trying to guess what might be needed.
               dnl
               dnl Some older Unices may need both -lcurses and -ltermlib,
               dnl but we don't support that just yet....

               AC_CHECK_LIB(tinfo, tgetent, HAVE_TERMCAP="-ltinfo")
               AC_CHECK_LIB(termlib, tgetent, HAVE_TERMCAP="-ltermlib")
               AC_CHECK_LIB(termcap, tgetent, HAVE_TERMCAP="-ltermcap")
               AC_CHECK_LIB(curses, tgetent, HAVE_TERMCAP="-lcurses")
               AC_CHECK_LIB(ncurses, tgetent, HAVE_TERMCAP="-lncurses")

               if test x"$HAVE_TERMCAP" != "x"; then
                   dnl We can't check for completion_matches() again,
                   dnl cause the result is cached. And autoconf doesn't
                   dnl seem to have a way to uncache it.
                   AC_CHECK_LIB(readline, filename_completion_function,
                         have_readline_lib=1, have_readline_lib=0,
                        "$HAVE_TERMCAP")
                   if test "$have_readline_lib" = "1"; then
                       AC_MSG_WARN(I had to manually add $HAVE_TERMCAP dependency to 
make readline library pass the test.)
                   fi
                   dnl We can't check for rl_completion_matches() again,
                   dnl cause the result is cached. And autoconf doesn't
                   dnl seem to have a way to uncache it.
                   AC_CHECK_LIB(readline, rl_filename_completion_function,
                         have_new_readline_lib=1, have_new_readline_lib=0,
                        "$HAVE_TERMCAP")
                   if test "$have_new_readline_lib" = "1"; then
                       AC_MSG_WARN(I had to manually add $HAVE_TERMCAP dependency to 
make readline library pass the test.)
                   fi
               fi
           fi

           if test "$have_new_readline_lib" = "1"; then
               FC_CHECK_READLINE_RUNTIME($HAVE_TERMCAP,
                         have_new_readline_lib=1, have_new_readline_lib=0)
               if test "$have_new_readline_lib" = "1"; then
                   SERVER_LIBS="-lreadline $SERVER_LIBS $HAVE_TERMCAP"
                   AC_DEFINE_UNQUOTED(HAVE_LIBREADLINE, 1, [Readline support])
                   AC_DEFINE_UNQUOTED(HAVE_NEWLIBREADLINE, 1, [Modern readline])
               else
                   if test "$WITH_READLINE" = "yes"; then
                       AC_MSG_ERROR(Specified --with-readline but the 
runtime test of readline failed.)
                   else
                       AC_MSG_WARN(Runtime test of readline failed. 
Configuring server without readline support.)
                   fi
               fi
           else
               if test "$have_readline_lib" = "1"; then
                   FC_CHECK_READLINE_RUNTIME($HAVE_TERMCAP,
                       have_readline_lib=1, have_readline_lib=0)
                   if test "$have_readline_lib" = "1"; then
                       SERVER_LIBS="-lreadline $SERVER_LIBS $HAVE_TERMCAP"
                       AC_DEFINE_UNQUOTED(HAVE_LIBREADLINE, 1, [Readline support])
                   else
                       if test "$WITH_READLINE" = "yes"; then
                           AC_MSG_ERROR(Specified --with-readline but the 
runtime test of readline failed.)
                       else
                           AC_MSG_WARN(Runtime test of readline failed. 
Configuring server without readline support.)
                       fi
                   fi
               else
                   if test "$WITH_READLINE" = "yes"; then
                       AC_MSG_ERROR(Specified --with-readline but the 
test to link against the library failed.)
                   else
                       AC_MSG_WARN(Test to link against readline library failed. 
Configuring server without readline support.)
                   fi
               fi
           fi
       fi
    fi
])
# Try to configure the SDL client (gui-sdl)

dnl FC_SDL_CLIENT
dnl Test for SDL and needed libraries for gui-sdl

AC_DEFUN([FC_SDL_CLIENT],
[
  if test "$client" = yes; then
    AC_MSG_WARN([Not checking for SDL; use --enable-client=sdl to enable])
  elif test "$client" = sdl ; then
    AM_PATH_SDL([1.1.4], [sdl_found="yes"], [sdl_found="no"])
    if test "$sdl_found" = yes; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      ac_save_CPPFLAGS="$CPPFLAGS"
      CPPFLAGS="$CPPFLAGS $SDL_CFLAGS"
      CFLAGS="$CFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $SDL_LIBS"
      AC_CHECK_LIB([SDL_image], [IMG_Load],
                   [sdl_image_found="yes"], [sdl_image_found="no"])
      if test "$sdl_image_found" = "yes"; then
        AC_CHECK_HEADER([SDL/SDL_image.h],
                        [sdl_image_h_found="yes"], [sdl_image_h_found="no"])
    	if test "$sdl_image_h_found" = yes; then
	  AC_CHECK_FT2([2.1.3], [freetype_found="yes"],[freetype_found="no"])
            if test "$freetype_found" = yes; then
	        LIBS=""
	        CLIENT_CFLAGS="$SDL_CFLAGS $FT2_CFLAGS"
	        CLIENT_LIBS="$SDL_LIBS -lSDL_image $FT2_LIBS"
	        found_client=yes
            elif test "$client" = "sdl"; then
              AC_MSG_ERROR([specified client 'sdl' not configurable (FreeType2 >= 2.1.3 is needed (www.freetype.org))])
            fi    
	elif test "$client" = "sdl"; then
	    AC_MSG_ERROR([specified client 'sdl' not configurable (SDL_image-devel is needed (www.libsdl.org))])
	fi
      elif test "$client" = "sdl"; then
        AC_MSG_ERROR([specified client 'sdl' not configurable (SDL_image is needed (www.libsdl.org))])
      fi
      CPPFLAGS="$ac_save_CPPFLAGS"
      CFLAGS="$ac_save_CFLAGS"
      LIBS="$ac_save_LIBS"
    fi

    if test "$found_client" = yes; then
      client=sdl

      dnl Check for libiconv (which is usually included in glibc, but may
      dnl be distributed separately).
      AM_ICONV
      AM_LIBCHARSET
      AM_LANGINFO_CODESET
      CLIENT_LIBS="$LIBICONV $CLIENT_LIBS"

      dnl Check for some other libraries - needed under BeOS for instance.
      dnl These should perhaps be checked for in all cases?
      AC_CHECK_LIB(socket, connect, CLIENT_LIBS="-lsocket $CLIENT_LIBS")
      AC_CHECK_LIB(bind, gethostbyaddr, CLIENT_LIBS="-lbind $CLIENT_LIBS")

    elif test "$client" = "sdl"; then
      AC_MSG_ERROR([specified client 'sdl' not configurable (SDL >= 1.1.4 is needed (www.libsdl.org))])
    fi
  fi
])
# Configure paths for SDL
# Sam Lantinga 9/21/99
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_SDL([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for SDL, and define SDL_CFLAGS and SDL_LIBS
dnl
AC_DEFUN([AM_PATH_SDL],
[dnl 
dnl Get the cflags and libraries from the sdl-config script
dnl
AC_ARG_WITH(sdl-prefix,[  --with-sdl-prefix=PFX   Prefix where SDL is installed (optional)],
            sdl_prefix="$withval", sdl_prefix="")
AC_ARG_WITH(sdl-exec-prefix,[  --with-sdl-exec-prefix=PFX Exec prefix where SDL is installed (optional)],
            sdl_exec_prefix="$withval", sdl_exec_prefix="")
AC_ARG_ENABLE(sdltest, [  --disable-sdltest       Do not try to compile and run a test SDL program],
		    , enable_sdltest=yes)

  if test x$sdl_exec_prefix != x ; then
     sdl_args="$sdl_args --exec-prefix=$sdl_exec_prefix"
     if test x${SDL_CONFIG+set} != xset ; then
        SDL_CONFIG=$sdl_exec_prefix/bin/sdl-config
     fi
  fi
  if test x$sdl_prefix != x ; then
     sdl_args="$sdl_args --prefix=$sdl_prefix"
     if test x${SDL_CONFIG+set} != xset ; then
        SDL_CONFIG=$sdl_prefix/bin/sdl-config
     fi
  fi

  AC_REQUIRE([AC_CANONICAL_TARGET])
  AC_PATH_PROG(SDL_CONFIG, sdl-config, no)
  min_sdl_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for SDL - version >= $min_sdl_version)
  no_sdl=""
  if test "$SDL_CONFIG" = "no" ; then
    no_sdl=yes
  else
    SDL_CFLAGS=`$SDL_CONFIG $sdlconf_args --cflags`
    SDL_LIBS=`$SDL_CONFIG $sdlconf_args --libs`

    sdl_major_version=`$SDL_CONFIG $sdl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    sdl_minor_version=`$SDL_CONFIG $sdl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    sdl_micro_version=`$SDL_CONFIG $sdl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_sdltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $SDL_LIBS"
dnl
dnl Now check if the installed SDL is sufficiently new. (Also sanity
dnl checks the results of sdl-config to some extent
dnl
      rm -f conf.sdltest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.sdltest");
  */
  { FILE *fp = fopen("conf.sdltest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_sdl_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_sdl_version");
     exit(1);
   }

   if (($sdl_major_version > major) ||
      (($sdl_major_version == major) && ($sdl_minor_version > minor)) ||
      (($sdl_major_version == major) && ($sdl_minor_version == minor) && ($sdl_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'sdl-config --version' returned %d.%d.%d, but the minimum version\n", $sdl_major_version, $sdl_minor_version, $sdl_micro_version);
      printf("*** of SDL required is %d.%d.%d. If sdl-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If sdl-config was wrong, set the environment variable SDL_CONFIG\n");
      printf("*** to point to the correct copy of sdl-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_sdl=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_sdl" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$SDL_CONFIG" = "no" ; then
       echo "*** The sdl-config script installed by SDL could not be found"
       echo "*** If SDL was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the SDL_CONFIG environment variable to the"
       echo "*** full path to sdl-config."
     else
       if test -f conf.sdltest ; then
        :
       else
          echo "*** Could not run SDL test program, checking why..."
          CFLAGS="$CFLAGS $SDL_CFLAGS"
          LIBS="$LIBS $SDL_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "SDL.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding SDL or finding the wrong"
          echo "*** version of SDL. If it is not finding SDL, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means SDL was incorrectly installed"
          echo "*** or that you have moved SDL since it was installed. In the latter case, you"
          echo "*** may want to edit the sdl-config script: $SDL_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SDL_CFLAGS=""
     SDL_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(SDL_CFLAGS)
  AC_SUBST(SDL_LIBS)
  rm -f conf.sdltest
])
AC_DEFUN([FC_CHECK_SOUND],[
 AC_ARG_ENABLE(esd,
   [  --disable-esd           Do not try to use Esound],
   USE_SOUND=no, USE_SOUND_ESD=yes)

 AC_ARG_ENABLE(sdl-mixer,
   [  --disable-sdl-mixer     Do not try to use the SDL mixer],
   USE_SOUND=no, USE_SOUND_SDL=yes)

 AC_ARG_ENABLE(alsa,
   [  --disable-alsa          Do not try to use ALSA],
   USE_SOUND=no, USE_SOUND_ALSA=yes)

 AC_ARG_ENABLE(winmm,
   [  --disable-winmm         Do not try to use WinMM for sound],
   USE_SOUND=no, USE_SOUND_WINMM=yes)

 if test "x$USE_SOUND_ESD" = "xyes"; then
  dnl Add esound support to client
  ESD_VERSION=0.0.20
  AM_PATH_ESD($ESD_VERSION, ESD=yes, ESD=no)
  if test "x$ESD" != "xno"; then
     SOUND_CFLAGS="$SOUND_CFLAGS $ESD_CFLAGS"
     SOUND_LIBS="$SOUND_LIBS $ESD_LIBS"
     AC_DEFINE(ESD, 1, [Esound support])
     AC_MSG_CHECKING(building ESOUND support)
     AC_MSG_RESULT(yes)
  fi
 fi

 if test "x$USE_SOUND_SDL" = "xyes"; then
  dnl Add SDL support to client
  SDL_VERSION=1.0.0
  AM_PATH_SDL($SDL_VERSION, SDL=yes, SDL=no)
  if test "x$SDL" != "xno"; then
    AC_CHECK_HEADER(SDL/SDL_mixer.h, SDL_mixer_h=1, SDL_mixer_h=0)
    AC_CHECK_LIB(SDL_mixer, Mix_OpenAudio, SDL_mixer=yes)
    AC_MSG_CHECKING(building SDL_mixer support)
    if test "x$SDL_mixer_h" = "x1"; then
      if test "x$SDL_mixer" = "xyes"; then
        SOUND_CFLAGS="$SOUND_CFLAGS $SDL_CFLAGS"
        SOUND_LIBS="$SOUND_LIBS $SDL_LIBS -lSDL_mixer"
        AC_DEFINE(SDL, 1, [SDL_Mixer support])
        AC_MSG_RESULT(yes)
      else
        AC_MSG_RESULT([no, found header but not library!])
      fi
    else
      AC_MSG_RESULT([no, install SDL_mixer first: http://www.libsdl.org/projects/SDL_mixer/index.html])
      SDL_mixer="xno"
    fi
  fi
 fi

 if test "x$USE_SOUND_ALSA" = "xyes"; then
  dnl Add ALSA support to client
  AM_ALSA_SUPPORT(ALSA=yes, ALSA=no)
  if test "x$ALSA" != "xno"; then
    SOUND_CFLAGS="$SOUND_CFLAGS $ALSA_CFLAGS"
    SOUND_LIBS="$SOUND_LIBS $ALSA_LIB"
    AC_DEFINE(ALSA, 1, [ALSA support])
    AC_MSG_CHECKING(building ALSA support)
    AC_MSG_RESULT(yes)
  fi
 fi

 if test "x$USE_SOUND_WINMM" = "xyes"; then
  dnl Add WinMM sound support to client
  if test x"$MINGW32" = "xyes"; then
    SOUND_LIBS="$SOUND_LIBS -lwinmm"
    AC_DEFINE(WINMM, 1, [Windows MultiMedia sound support])
    WINMM="yes"
  fi
 fi
])
#
# These macros are used in version.in and they just set
# version information to form understandable for configure.ac and
# configure.in. Other systems define these macros differently
# before reading version.in and thus get version information
# in different form.
# 

AC_DEFUN([FREECIV_VERSION_COMMENT])

AC_DEFUN([FREECIV_VERSION_INFO],
[
 MAJOR_VERSION="$1"
 MINOR_VERSION="$2"
 PATCH_VERSION="$3"
 VERSION_LABEL="$4"
])
AC_DEFUN([FREECIV_DEVEL_VERSION], [IS_DEVEL_VERSION="$1"])
AC_DEFUN([FREECIV_BETA_VERSION], [IS_BETA_VERSION="$1"])

dnl @synopsis AC_FUNC_VSNPRINTF
dnl
dnl Check whether there is a reasonably sane vsnprintf() function installed.
dnl "Reasonably sane" in this context means never clobbering memory beyond
dnl the buffer supplied, and having a sensible return value.  It is
dnl explicitly allowed not to NUL-terminate the return value, however.
dnl
dnl @version $Id: vsnprintf.m4 4430 2002-04-13 13:52:03Z rfalke $
dnl @author Gaute Strokkenes <gs234@cam.ac.uk>
dnl
AC_DEFUN([AC_FUNC_VSNPRINTF],
[AC_CACHE_CHECK(for working vsnprintf,
  ac_cv_func_working_vsnprintf,
[AC_TRY_RUN(
[#include <stdio.h>
#include <stdarg.h>

int
doit(char * s, ...)
{
  char buffer[32];
  va_list args;
  int r;

  buffer[5] = 'X';

  va_start(args, s);
  r = vsnprintf(buffer, 5, s, args);
  va_end(args);

  /* -1 is pre-C99, 7 is C99. */

  if (r != -1 && r != 7)
    exit(1);

  /* We deliberately do not care if the result is NUL-terminated or
     not, since this is easy to work around like this.  */

  buffer[4] = 0;

  /* Simple sanity check.  */

  if (strcmp(buffer, "1234"))
    exit(1);

  if (buffer[5] != 'X')
    exit(1);

  exit(0);
}

int
main(void)
{
  doit("1234567");
  exit(1);
}], ac_cv_func_working_vsnprintf=yes, ac_cv_func_working_vsnprintf=no, ac_cv_func_working_vsnprintf=no)])
dnl Note that the default is to be pessimistic in the case of cross compilation.
dnl If you know that the target has a sensible vsnprintf(), you can get around this
dnl by setting ac_func_vsnprintf to yes, as described in the Autoconf manual.
if test $ac_cv_func_working_vsnprintf = yes; then
  AC_DEFINE(HAVE_WORKING_VSNPRINTF, 1,
            [Define if you have a version of the 'vsnprintf' function
             that honours the size argument and has a proper return value.])
fi
])# AC_FUNC_VSNPRINTF
# Try to configure the Win32 client (gui-win32)

# FC_WIN32_CLIENT
# Test for Win32 and needed libraries for gui-win32

AC_DEFUN([FC_WIN32_CLIENT],
[
  if test "$client" = "win32" || test "$client" = "yes" ; then
    if test "$MINGW32" = "yes"; then
      AC_CHECK_LIB([z], [gzgets],
        [
          AC_CHECK_HEADER([zlib.h],
            [
              AC_CHECK_LIB([png], [png_read_image],
                [
                  AC_CHECK_HEADER([png.h],
                    [
                      found_client=yes
                      client=win32
                      CLIENT_LIBS="-lwsock32 -lcomctl32  -lpng -lz -mwindows"
                    ],
                    [
                      FC_NO_CLIENT([win32], [libpng-dev is needed])
                    ])
                ],
                [
                  FC_NO_CLIENT([win32], [libpng is needed])
                ], [-lz])
            ],
            [
              FC_NO_CLIENT([win32], [zlib-dev is needed])
            ])
        ],
        [
          FC_NO_CLIENT([win32], [zlib is needed])
        ])
    else
      FC_NO_CLIENT([win32], [mingw32 is needed])
    fi
  fi
])

dnl FC_CHECK_X_LIB(LIBRARY, FUNCTION [, ACTION-IF-FOUND [,
dnl   ACTION-IF-NOT-FOUND]])
dnl
dnl This macro is intended to search for X11-related libraries.  It takes the
dnl following variables for input:
dnl   X_LIBS		-- prefixed to all linker lines
dnl   X_EXTRA_LIBS	-- suffixed to all linker lines
dnl   LIBS		-- suffixed to all linker lines (after X_EXTRA_LIBS)
dnl Thus, the trial linker line will be "$X_LIBS -l$1 $X_EXTRA_LIBS $LIBS".
dnl
dnl The following variables are output:
dnl   X_EXTRA_LIBS	-- contains "-l$1 $X_EXTRA_LIBS" if the link succeeds
dnl
dnl Thus, the intended usage of this macro is something like this:
dnl   AC_PATH_XTRA
dnl   X_LIBS="$X_LIBS $X_PRE_LIBS"
dnl	dnl Is it just me or is AC_PATH_XTRA broken?
dnl   FC_CHECK_X_LIB(X11, XOpenDisplay, , AC_MSG_ERROR("Need X11"))
dnl   FC_CHECK_X_LIB(Xext, XShapeCombineMask)
dnl     [etc.]
dnl   LIBS="$X_LIBS $X_EXTRA_LIBS $LIBS"
dnl
AC_DEFUN([FC_CHECK_X_LIB], [
 AC_MSG_CHECKING([for $2 in X library -l$1])

 dnl Use a cache variable name containing both the library and function name,
 dnl because the test really is for library $1 defining function $2, not
 dnl just for library $1.  Separate tests with the same $1 and different $2s
 dnl may have different results.

 ac_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
 AC_CACHE_VAL(ac_cv_lib_$ac_lib_var,
  [ac_save_LIBS="$LIBS"
   LIBS="$X_LIBS -l$1 $X_EXTRA_LIBS $LIBS"
   AC_TRY_LINK(dnl
    ifelse([$2], [main], ,
     [#ifdef __cplusplus
       extern "C"
      #endif]
     [/* We use char because int might match the return type of a gcc2
      builtin and then its argument prototype would still apply.  */
      char $2();]
    )
   , [$2()],
   eval "ac_cv_lib_$ac_lib_var=yes",
   eval "ac_cv_lib_$ac_lib_var=no")
   LIBS="$ac_save_LIBS"
  ])dnl
 if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$3], ,
   [changequote(, )dnl
    ac_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
    changequote([, ])dnl

    # The HAVE_LIBX** values are defined in FC_CHECK_X_LIB, but we need an
    # AH_TEMPLATE for them so that autoheader will know about them.
    AH_TEMPLATE([HAVE_LIBX11], [Define if the X11 lib is available])
    AH_TEMPLATE([HAVE_LIBXEXT], [Define if the Xext lib is available])
    AH_TEMPLATE([HAVE_LIBXT], [Define if the Xt lib is available])
    AH_TEMPLATE([HAVE_LIBXMU], [Define if the Xmu lib is available])
    AH_TEMPLATE([HAVE_LIBXPM], [Define if the Xpm lib is available])
    AH_TEMPLATE([HAVE_LIBXAW], [Define if the Xaw lib is to be used])
    AH_TEMPLATE([HAVE_LIBXAW3D], [Define if the Xaw3d lib is to be used])
    if (test $ac_tr_lib == HAVE_LIBX11     \
        || test $ac_tr_lib == HAVE_LIBXEXT \
        || test $ac_tr_lib == HAVE_LIBXT   \
        || test $ac_tr_lib == HAVE_LIBXMU  \
        || test $ac_tr_lib == HAVE_LIBXPM  \
        || test $ac_tr_lib == HAVE_LIBXAW  \
        || test $ac_tr_lib == HAVE_LIBXAW3D); then
      AC_DEFINE_UNQUOTED($ac_tr_lib)
      X_EXTRA_LIBS="-l$1 $X_EXTRA_LIBS"
    else
      AC_MSG_ERROR([Invalid define of $ac_tr_lib in $1])
    fi
   ], [$3])
 else
  AC_MSG_RESULT(no)
 ifelse([$4], , , [$4
 ])dnl
 fi
])

dnl FC_EXPAND_DIR(VARNAME, DIR)
dnl expands occurrences of ${prefix} and ${exec_prefix} in the given DIR,
dnl and assigns the resulting string to VARNAME
dnl example: FC_EXPAND_DIR(LOCALEDIR, "$datadir/locale")
dnl eg, then: AC_DEFINE_UNQUOTED(LOCALEDIR, "$LOCALEDIR")
dnl by Alexandre Oliva 
dnl from http://www.cygnus.com/ml/automake/1998-Aug/0040.html
AC_DEFUN([FC_EXPAND_DIR], [
        $1=$2
        $1=`(
            test "x$prefix" = xNONE && prefix="$ac_default_prefix"
            test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
            eval echo \""[$]$1"\"
        )`
])


dnl FC_XPM_PATHS
dnl Allow user to specify extra include/lib paths for Xpm, with
dnl --with-xpm=prefix  --with-xpm-lib=dir  --with-xpm-include=dir
dnl The latter two override the prefix form.
dnl Sets variables xpm_libdir and xpm_incdir
dnl If user supplies a path, use that.
dnl If user specifies "no", set that, meaning "no extra path"
dnl If user specifies "yes" (default), then use /usr/local if it looks
dnl likely, else set to "no".
dnl Doesn't do any cache stuff.
dnl
AC_DEFUN([FC_XPM_PATHS],
[AC_MSG_CHECKING(extra paths for Xpm)
dnl General Xpm prefix:
dnl "no" means no prefix is required, "yes" means try /usr/local
AC_ARG_WITH(xpm-prefix,
    [  --with-xpm-prefix=DIR   Xpm files are in DIR/lib and DIR/include,
                          or use the following to set them separately:],
    xpm_prefix="$withval", 
    xpm_prefix="yes"
)
if test "$xpm_prefix" = "yes" || test "$xpm_prefix" = "no"; then
    xpm_libdir="$xpm_prefix"
    xpm_incdir="$xpm_prefix"
else
    xpm_libdir="$xpm_prefix/lib"
    xpm_incdir="$xpm_prefix/include"
fi
dnl May override general Xpm prefix with explicit individual paths:
AC_ARG_WITH(xpm-lib,
    [  --with-xpm-lib=DIR      Xpm library is in DIR],
    xpm_libdir="$withval" 
)
AC_ARG_WITH(xpm-include,
    [  --with-xpm-include=DIR  Xpm header file is in DIR (that is, DIR/X11/xpm.h)],
    xpm_incdir="$withval" 
)
dnl If xpm-lib path was not specified, try /usr/local/lib if that 
dnl looks likely; we don't actually try to link.
fc_xpm_default=/usr/local
if test "$xpm_libdir" = "yes"; then
    xpm_libdir="no"
    fc_xpm_default_lib="$fc_xpm_default/lib"
    for fc_extension in a so sl; do
        if test -r $fc_xpm_default_lib/libXpm.$fc_extension; then
            xpm_libdir=$fc_xpm_default_lib
            break
        fi
    done
fi
dnl Likewise for xpm-include with /usr/local/include;
dnl we don't actually try to include.
if test "$xpm_incdir" = "yes"; then
    xpm_incdir="no"
    fc_xpm_default_inc="$fc_xpm_default/include"
    if test -r $fc_xpm_default_inc/X11/xpm.h; then
        xpm_incdir=$fc_xpm_default_inc
    elif test -r $fc_xpm_default_inc/xpm.h; then
    	xpm_incdir=$fc_xpm_default_inc
	xpm_h_no_x11=yes
    fi
fi
AC_MSG_RESULT([library $xpm_libdir, include $xpm_incdir])
])


dnl FC_CHECK_X_PROTO_DEFINE(DEFINED-VARIABLE)
dnl
dnl This macro determines the value of the given defined
dnl variable needed by Xfuncproto.h in order to compile correctly.
dnl
dnl Typical DEFINED-VARIABLEs are:
dnl   FUNCPROTO
dnl   NARROWPROTO
dnl
dnl The following variables are output:
dnl   fc_x_proto_value		-- contains the value to which
dnl				the DEFINED-VARIABLE is set,
dnl				or "" if it has no known value.
dnl
dnl Example use:
dnl   FC_CHECK_X_PROTO_DEFINE(FUNCPROTO)
dnl   if test -n "$fc_x_proto_value"; then
dnl     AC_DEFINE_UNQUOTED(FUNCPROTO, $fc_x_proto_value)
dnl   fi
dnl
AC_DEFUN([FC_CHECK_X_PROTO_DEFINE],
[AC_REQUIRE([FC_CHECK_X_PROTO_FETCH])dnl
AC_MSG_CHECKING(for Xfuncproto control definition $1)
# Search for the requested defined variable; return it's value:
fc_x_proto_value=
for fc_x_define in $fc_x_proto_defines; do
  fc_x_val=1
  eval `echo $fc_x_define | sed -e 's/=/ ; fc_x_val=/' | sed -e 's/^/fc_x_var=/'`
  if test "x$fc_x_var" = "x$1"; then
    fc_x_proto_value=$fc_x_val
    break
  fi
done
if test -n "$fc_x_proto_value"; then
  AC_MSG_RESULT([yes: $fc_x_proto_value])
else
  AC_MSG_RESULT([no])
fi
])

dnl FC_CHECK_X_PROTO_FETCH
dnl
dnl This macro fetches the Xfuncproto control definitions.
dnl (Intended to be called once from FC_CHECK_X_PROTO_DEFINE.)
dnl
dnl The following variables are output:
dnl   fc_x_proto_defines	-- contains the list of defines of
dnl				Xfuncproto control definitions
dnl				(defines may or may not include
dnl				the -D prefix, or an =VAL part).
dnl
dnl Example use:
dnl   AC_REQUIRE([FC_CHECK_X_PROTO_FETCH])
dnl
AC_DEFUN([FC_CHECK_X_PROTO_FETCH],
[AC_REQUIRE([AC_PATH_X])dnl
AC_MSG_CHECKING(whether Xfuncproto was supplied)
dnl May override determined defines with explicit argument:
AC_ARG_WITH(x-funcproto,
    [  --with-x-funcproto=DEFS Xfuncproto control definitions are DEFS
                          (e.g.: --with-x-funcproto='FUNCPROTO=15 NARROWPROTO']dnl
)
if test "x$with_x_funcproto" = "x"; then
  fc_x_proto_defines=
  rm -fr conftestdir
  if mkdir conftestdir; then
    cd conftestdir
    # Make sure to not put "make" in the Imakefile rules, since we grep it out.
    cat > Imakefile <<'EOF'
fcfindpd:
	@echo 'fc_x_proto_defines=" ${PROTO_DEFINES}"'
EOF
    if (xmkmf) >/dev/null 2>/dev/null && test -f Makefile; then
      # GNU make sometimes prints "make[1]: Entering...", which would confuse us.
      eval `${MAKE-make} fcfindpd 2>/dev/null | grep -v make | sed -e 's/ -D/ /g'`
      AC_MSG_RESULT([no, found: $fc_x_proto_defines])
      cd ..
      rm -fr conftestdir
    else
      dnl Oops -- no/bad xmkmf... Time to go a-guessing...
      AC_MSG_RESULT([no])
      cd ..
      rm -fr conftestdir
      dnl First, guess something for FUNCPROTO:
      AC_MSG_CHECKING([for compilable FUNCPROTO definition])
      dnl Try in order of preference...
      for fc_x_value in 15 11 3 1 ""; do
	FC_CHECK_X_PROTO_FUNCPROTO_COMPILE($fc_x_value)
	if test "x$fc_x_proto_FUNCPROTO" != "xno"; then
	  break
	fi
      done
      if test "x$fc_x_proto_FUNCPROTO" != "xno"; then
	fc_x_proto_defines="$fc_x_proto_defines FUNCPROTO=$fc_x_proto_FUNCPROTO"
	AC_MSG_RESULT([yes, determined: $fc_x_proto_FUNCPROTO])
      else
	AC_MSG_RESULT([no, cannot determine])
      fi
      dnl Second, guess something for NARROWPROTO:
      AC_MSG_CHECKING([for workable NARROWPROTO definition])
      dnl Try in order of preference...
      for fc_x_value in 1 ""; do
	FC_CHECK_X_PROTO_NARROWPROTO_WORKS($fc_x_value)
	if test "x$fc_x_proto_NARROWPROTO" != "xno"; then
	  break
	fi
      done
      if test "x$fc_x_proto_NARROWPROTO" != "xno"; then
	fc_x_proto_defines="$fc_x_proto_defines NARROWPROTO=$fc_x_proto_NARROWPROTO"
	AC_MSG_RESULT([yes, determined: $fc_x_proto_NARROWPROTO])
      else
	AC_MSG_RESULT([no, cannot determine])
      fi
      AC_MSG_CHECKING(whether Xfuncproto was determined)
      if test -n "$fc_x_proto_defines"; then
	AC_MSG_RESULT([yes: $fc_x_proto_defines])
      else
	AC_MSG_RESULT([no])
      fi
    fi
  else
    AC_MSG_RESULT([no, examination failed])
  fi
else
  fc_x_proto_defines=$with_x_funcproto
  AC_MSG_RESULT([yes, given: $fc_x_proto_defines])
fi
])

dnl FC_CHECK_X_PROTO_FUNCPROTO_COMPILE(FUNCPROTO-VALUE)
dnl
dnl This macro determines whether or not Xfuncproto.h will
dnl compile given a value to use for the FUNCPROTO definition.
dnl
dnl Typical FUNCPROTO-VALUEs are:
dnl   15, 11, 3, 1, ""
dnl
dnl The following variables are output:
dnl   fc_x_proto_FUNCPROTO	-- contains the passed-in
dnl				FUNCPROTO-VALUE if Xfuncproto.h
dnl				compiled, or "no" if it did not.
dnl
dnl Example use:
dnl   FC_CHECK_X_PROTO_FUNCPROTO_COMPILE($fc_x_value)
dnl   if test "x$fc_x_proto_FUNCPROTO" != "xno"; then
dnl     echo Compile using FUNCPROTO=$fc_x_proto_FUNCPROTO
dnl   fi
dnl
AC_DEFUN([FC_CHECK_X_PROTO_FUNCPROTO_COMPILE],
[AC_REQUIRE([AC_PATH_XTRA])dnl
AC_LANG_SAVE
AC_LANG_C
fc_x_proto_FUNCPROTO=no
if test "x$1" = "x"; then
  fc_x_compile="#undef FUNCPROTO"
else
  fc_x_compile="#define FUNCPROTO $1"
fi
fc_x_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $X_CFLAGS"
AC_TRY_COMPILE([
$fc_x_compile
#include <X11/Xfuncproto.h>
  ],[
exit (0)
  ],
  [fc_x_proto_FUNCPROTO=$1])
CFLAGS="$fc_x_save_CFLAGS"
AC_LANG_RESTORE
])

dnl FC_CHECK_X_PROTO_NARROWPROTO_WORKS(NARROWPROTO-VALUE)
dnl
dnl This macro determines whether or not NARROWPROTO is required
dnl to get a typical X function (XawScrollbarSetThumb) to work.
dnl
dnl Typical NARROWPROTO-VALUEs are:
dnl   1, ""
dnl
dnl The following variables are required for input:
dnl   fc_x_proto_FUNCPROTO	-- the value to use for FUNCPROTO.
dnl
dnl The following variables are output:
dnl   fc_x_proto_NARROWPROTO	-- contains the passed-in
dnl				NARROWPROTO-VALUE if the test
dnl				worked, or "no" if it did not.
dnl
dnl Example use:
dnl   FC_CHECK_X_PROTO_NARROWPROTO_WORKS($fc_x_value)
dnl   if test "x$fc_x_proto_NARROWPROTO" != "xno"; then
dnl     echo Compile using NARROWPROTO=$fc_x_proto_NARROWPROTO
dnl   fi
dnl
AC_DEFUN([FC_CHECK_X_PROTO_NARROWPROTO_WORKS],
[AC_REQUIRE([AC_PATH_XTRA])dnl
AC_LANG_SAVE
AC_LANG_C
fc_x_proto_NARROWPROTO=no
if test "x$1" = "x"; then
  fc_x_works="#undef NARROWPROTO"
else
  fc_x_works="#define NARROWPROTO $1"
fi
if test "x$fc_x_proto_FUNCPROTO" = "x"; then
  fc_x_compile="#define FUNCPROTO 1"
else
  fc_x_compile="#define FUNCPROTO $fc_x_proto_FUNCPROTO"
fi
fc_x_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $X_CFLAGS $X_LIBS $X_PRE_LIBS -lXaw -lXt -lX11 $X_EXTRA_LIBS"
AC_TRY_RUN([
$fc_x_works
$fc_x_compile
#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Scrollbar.h>
#define TOP_VAL 0.125
#define SHOWN_VAL 0.25
int main (int argc, char ** argv)
{
  Widget toplevel;
  XtAppContext appcon;
  Widget scrollbar;
  double topbuf;
  double shownbuf;
  float * top = (float *)(&topbuf);
  float * shown = (float *)(&shownbuf);
  toplevel =
    XtAppInitialize
    (
     &appcon,
     "FcXTest",
     NULL, 0,
     &argc, argv,
     NULL,
     NULL, 0
    );
  scrollbar =
    XtVaCreateManagedWidget
    (
     "my_scrollbar",
     scrollbarWidgetClass,
     toplevel,
     NULL
    );
  XawScrollbarSetThumb (scrollbar, TOP_VAL, SHOWN_VAL);
  XtVaGetValues
  (
   scrollbar,
   XtNtopOfThumb, top,
   XtNshown, shown,
   NULL
  );
  if ((*top == TOP_VAL) && (*shown == SHOWN_VAL))
    {
      exit (0);
    }
  else
    {
      exit (1);
    }
  return (0);
}
  ],
  [fc_x_proto_NARROWPROTO=$1], [], [:])
CFLAGS="$fc_x_save_CFLAGS"
AC_LANG_RESTORE
])

# Try to configure the XAW client (gui-xaw)

# FC_XAW_CLIENT
# Test for X and XAW libraries needed for gui-xaw

AC_DEFUN([FC_XAW_CLIENT],
[
  if test "$client" = yes ; then
    AC_MSG_WARN([Not checking for XAW; use --enable-client=xaw to enable])
  elif test "$client" = xaw ; then
    dnl Checks for X:
    AC_PATH_XTRA

    dnl Determine the Xfuncproto control definitions:
    FC_CHECK_X_PROTO_DEFINE(FUNCPROTO)
    if test -n "$fc_x_proto_value"; then
      AC_DEFINE_UNQUOTED(FUNCPROTO, $fc_x_proto_value, [Xfuncproto])
    fi
    FC_CHECK_X_PROTO_DEFINE(NARROWPROTO)
    if test -n "$fc_x_proto_value"; then
      AC_DEFINE_UNQUOTED(NARROWPROTO, $fc_x_proto_value, [Narrowproto])
    fi

    dnl Check for libpng
    AC_CHECK_LIB(png, png_read_image, [X_LIBS="$X_LIBS -lpng -lm"],
	AC_MSG_ERROR([Could not find PNG library (libpng).]), [-lm -lz])
    AC_CHECK_HEADER(png.h, ,
	AC_MSG_ERROR([libpng found but not png.h.
You may need to install a libpng \"development\" package.]))

    dnl Try to get additional Xpm paths:
    FC_XPM_PATHS

    if test "$xpm_incdir" != "no"; then
      X_CFLAGS="$X_CFLAGS -I$xpm_incdir"
    fi
    if test "$xpm_libdir" != "no"; then
      X_LIBS="$X_LIBS -L$xpm_libdir"
      dnl Try using R values set in AC_PATH_XTRA:
      if test "$ac_R_nospace" = "yes"; then
        X_LIBS="$X_LIBS -R$xpm_libdir"
      elif test "$ac_R_space" = "yes"; then
        X_LIBS="$X_LIBS -R $xpm_libdir"
      fi
      dnl Some sites may put xpm.h in a directory whose parent isn't "X11"
      if test "x$xpm_h_no_x11" = "xyes"; then
        AC_DEFINE(XPM_H_NO_X11, 1, [XPM support])
      fi
    fi

    dnl Checks for X libs:
    fc_save_X_LIBS="$X_LIBS"
    X_LIBS="$X_LIBS $X_PRE_LIBS"
    FC_CHECK_X_LIB(X11, XOpenDisplay, , haveX11=no)
    if test "x$haveX11" != "xno"; then
      FC_CHECK_X_LIB(Xext, XShapeCombineMask)

      dnl Insert X_PRE_LIBS (eg -lSM -lICE) into X_EXTRA_LIBS here:
      X_EXTRA_LIBS="$X_PRE_LIBS $X_EXTRA_LIBS"
      X_LIBS="$fc_save_X_LIBS"

      FC_CHECK_X_LIB(Xt, main)
      FC_CHECK_X_LIB(Xmu, main)
      FC_CHECK_X_LIB(Xpm, XpmReadFileToPixmap, , haveXpm=no)
      if test "x$haveXpm" != "xno"; then
	dnl Xaw or Xaw3d:
	if test -n "$WITH_XAW3D"; then
	  FC_CHECK_X_LIB(Xaw3d, main, , AC_MSG_ERROR(did not find Xaw3d library))
	elif test "$client" = "xaw"; then
	  FC_CHECK_X_LIB(Xaw, main, , AC_MSG_ERROR(did not find Xaw library))
	else
	  FC_CHECK_X_LIB(Xaw3d, main, , noXaw3d=1)
	  if test -n "$noXaw3d"; then
	    FC_CHECK_X_LIB(Xaw, main, ,
	      AC_MSG_ERROR(did not find either Xaw or Xaw3d library))
	  fi
	fi

	CLIENT_CFLAGS="$X_CFLAGS"
	CLIENT_LIBS="$X_LIBS $X_EXTRA_LIBS"

	found_client=yes
      fi
    fi

    if test "x$found_client" = "xyes"; then
      client=xaw
    elif test "$client" = "xaw"; then
      if test "x$haveXpm" = "xno"; then
	AC_MSG_ERROR(specified client 'xaw' not configurable -- need Xpm library and development headers; perhaps try/adjust --with-xpm-lib)
      else
	AC_MSG_ERROR(specified client 'xaw' not configurable -- need X11 libraries and development headers; perhaps try/adjust --x-libraries)
      fi
    fi
  fi
])
