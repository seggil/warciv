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
  civserver.cc \
  civserver.hh

libcivserver_a_SOURCES = \
  airgoto.cc \
  airgoto.hh \
  autoattack.cc \
  autoattack.hh \
  barbarian.cc \
  barbarian.hh \
  cityhand.cc \
  cityhand.hh \
  citytools.cc \
  citytools.hh \
  cityturn.cc \
  cityturn.hh \
  commands.cc \
  commands.hh \
  connecthand.cc \
  connecthand.hh \
  console.cc \
  console.hh \
  database.cc \
  database.hh \
  diplhand.cc \
  diplhand.hh \
  diplomats.cc \
  diplomats.hh \
  game_load.cc \
  game_load.hh \
  game_save.cc \
  game_save.hh \
  gamehand.cc \
  gamehand.hh \
  gamelog.cc \
  gamelog.hh \
  gotohand.cc \
  gotohand.hh \
  handchat.cc \
  handchat.hh \
  hand_gen.cc \
  hand_gen.hh \
  load_save_aux.hh \
  maphand.cc \
  maphand.hh \
  meta.cc  \
  meta.hh  \
  plrhand.cc \
  plrhand.hh \
  report.cc \
  report.hh \
  ruleset.cc \
  ruleset.hh \
  sanitycheck.cc \
  sanitycheck.hh \
  score.cc  \
  score.hh  \
  script.cc \
  script.hh \
  sernet.cc \
  sernet.hh \
  settings.cc \
  settings.hh \
  settlers.cc \
  settlers.hh \
  spacerace.cc \
  spacerace.hh \
  srv_main.cc \
  srv_main.hh \
  stdinhand.cc \
  stdinhand.hh \
  stdinhand_info.hh \
  tradehand.cc \
  tradehand.hh \
  unithand.cc \
  unithand.hh \
  unittools.cc \
  unittools.hh \
  vote.cc  \
  vote.hh

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
