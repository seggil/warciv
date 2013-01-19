# Do checks for Freeciv mysql database support
#
# Called without any parameters.

AC_DEFUN([WC_CHECK_MYSQL],
[
  dnl  no=compile mysql database support,  yes=compile in mysql database support,  *=error
  AC_ARG_ENABLE(mysql, 
    AS_HELP_STRING([--enable-mysql],
                   [enable database support via mysql]),
    [case "${enableval}" in
      yes) mysql=true ;;
      no)  mysql=false ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --enable-mysql) ;;
    esac], [mysql=false])

  AC_ARG_WITH(mysql-prefix,
              AS_HELP_STRING([--with-mysql-prefix=PFX],
                             [Prefix where MySQL is installed (optional)]),
              [mysql_prefix="$withval"],[mysql_prefix=""])

  if test x$mysql = xtrue; then

    if test x$mysql_prefix = x; then
      AC_CHECK_HEADER(mysql/mysql.h, , 
                      [AC_MSG_WARN([couldn't find mysql header: disabling mysql support]);
                       mysql=false])

      dnl we need to set -L correctly, we will check once in standard locations
      dnl then we will check with other LDFLAGS. if none of these work, we fail.
   
      AC_CHECK_LIB(mysqlclient, mysql_query, 
		   [MYSQL_LIBS="-lmysqlclient $MYSQL_LIBS"],
                   [AC_MSG_WARN([couldn't find mysql libs in normal locations]);
                    mysql=false])

      wc_preauth_LDFLAGS=$LDFLAGS
      wc_mysql_lib_loc="-L/usr/lib/mysql -L/usr/local/lib/mysql \
                        -L$HOME/lib -L$HOME/lib/mysql"

      for __ldpath in $wc_mysql_lib_loc; do
        unset ac_cv_lib_mysqlclient_mysql_query
        LDFLAGS="$LDFLAGS $__ldpath"
        mysql=true

        AC_CHECK_LIB(mysqlclient, mysql_query,
                     [MYSQL_LIBS="-lmysqlclient $MYSQL_LIBS";
                      AC_MSG_WARN([had to add $__ldpath to LDFLAGS])],
                      [AC_MSG_WARN([couldn't find mysql libs in $__ldpath]);
                       mysql=false])

        if test x$mysql = xtrue; then
          break
        else
          LDFLAGS=$wc_preauth_LDFLAGS
        fi
      done
      MYSQL_CFLAGS="$MYSQL_CFLAGS"
    else
      MYSQL_CFLAGS="-I$mysql_prefix/include $MYSQL_CFLAGS"
      MYSQL_LIBS="-L$mysql_prefix/lib/mysql -lmysqlclient $MYSQL_LIBS"
      mysql_saved_cflags="$CFLAGS"
      mysql_saved_cppflags="$CPPFLAGS"
      mysql_saved_libs="$LIBS"
      CFLAGS="$CFLAGS $MYSQL_CFLAGS"
      CPPFLAGS="$CPPFLAGS $MYSQL_CFLAGS"
      LIBS="$LIBS $MYSQL_LIBS"
      AC_CHECK_HEADER(mysql/mysql.h, , 
                      [AC_MSG_ERROR([couldn't find mysql header in $mysql_prefix/include]);
                       mysql=false])
      AC_CHECK_LIB(mysqlclient, mysql_query, ,
                   [AC_MSG_ERROR([couldn't find mysql libs in $mysql_prefix/lib/mysql]);
                    mysql=false])
      CFLAGS=$mysql_saved_cflags
      CPPFLAGS=$mysql_saved_cppflags
      LIBS=$mysql_saved_libs
    fi

    if test x$mysql = xfalse; then
      AC_MSG_WARN([can't find mysql -- disabling database support])
    fi

    AC_SUBST(LDFLAGS)
    AC_SUBST(MYSQL_CFLAGS)
    AC_SUBST(MYSQL_LIBS)
  fi

  if test x$mysql = xtrue; then
    AC_DEFINE(HAVE_MYSQL, 1, [have mysql for authentication, game logging, etc.])
  fi
  AM_CONDITIONAL([HAVE_MYSQL], [test x$mysql = xtrue])

])

