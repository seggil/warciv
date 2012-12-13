## Process this file with automake to produce Makefile.in

SUBDIRS = generator lua

bin_PROGRAMS = civserver
noinst_LIBRARIES = libcivserver.a
# -I$(top_srcdir)/common/aicore
AM_CPPFLAGS = \
    -I$(top_srcdir)/utility \
    -I$(top_srcdir)/common \
    -I$(top_srcdir)/ai \
    -I../intl  \
    -I$(srcdir)/generator \
    -I$(srcdir)/lua/src @MYSQL_CFLAGS@


## Above, note -I../intl instead of -I$(top_srdir/intl) is deliberate.

if MINGW32
SERVERICON=../win32/servericon.o
endif

civserver_SOURCES = \
		civserver.c	\
		civserver.h

libcivserver_a_SOURCES = \
		airgoto.c	\
		airgoto.h	\
		autoattack.c	\
		autoattack.h	\
		barbarian.c	\
		barbarian.h	\
		cityhand.c	\
		cityhand.h	\
		citytools.c	\
		citytools.h	\
		cityturn.c	\
		cityturn.h	\
		commands.c	\
		commands.h	\
		connecthand.c	\
		connecthand.h	\
		console.c	\
		console.h	\
		database.c      \
		database.h      \
		diplhand.c	\
		diplhand.h	\
		diplomats.c	\
		diplomats.h	\
		game_load.c	\
		game_load.h	\
		game_save.c	\
		game_save.h	\
		gamehand.c	\
		gamehand.h	\
		gamelog.c	\
		gamelog.h	\
		gotohand.c	\
		gotohand.h	\
		handchat.c	\
		handchat.h	\
		hand_gen.h	\
		hand_gen.c	\
        load_save_aux.h \
		maphand.c	\
		maphand.h	\
		meta.c		\
		meta.h		\
		plrhand.c	\
		plrhand.h	\
		report.c	\
		report.h	\
		ruleset.c	\
		ruleset.h	\
		sanitycheck.c	\
		sanitycheck.h	\
		score.c		\
		score.h		\
		script.c	\
		script.h	\
		sernet.c	\
		sernet.h	\
		settings.c	\
		settings.h	\
		settlers.c	\
		settlers.h	\
		spacerace.c	\
		spacerace.h	\
		srv_main.c	\
		srv_main.h	\
		stdinhand.c	\
		stdinhand.h	\
		stdinhand_info.h\
		tradehand.c	\
		tradehand.h	\
		unithand.c	\
		unithand.h	\
		unittools.c	\
		unittools.h	\
		vote.c		\
		vote.h

if HAVE_MYSQL
LUASQL_MYSQL_LIB=./lua/mysql/libluasql_mysql.a
endif

civserver_DEPENDENCIES = ../utility/libcivutility.a ../common/libcivcommon.a \
      ../ai/libcivai.a ../utility/libcivutility.a ./libcivserver.a \
      ../utility/libcivutility.a ../common/aicore/libaicore.a \
      ./generator/libgenerator.a ./lua/src/liblua.a \
      $(LUASQL_MYSQL_LIB)
civserver_LDADD        = ../utility/libcivutility.a ../common/libcivcommon.a \
      ../ai/libcivai.a ../utility/libcivutility.a ./libcivserver.a @INTLLIBS@ \
      ../utility/libcivutility.a ../common/libcivcommon.a ../ai/libcivai.a \
      ../utility/libcivutility.a ./libcivserver.a ../utility/libcivutility.a \
      ../common/aicore/libaicore.a ./generator/libgenerator.a \
      ./lua/src/liblua.a $(MYSQL_LIBS) $(SERVER_LIBS) $(SERVERICON) \
      $(LUASQL_MYSQL_LIB)
