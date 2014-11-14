## Process this file with automake to produce Makefile.in

SUBDIRS = aicore

noinst_LIBRARIES = libcivcommon.a

AM_CPPFLAGS = -I../intl -I$(top_srcdir)/utility

## Above, note -I../intl instead of -I$(top_srcdir/intl) is deliberate.

libcivcommon_a_SOURCES = \
  capstr.cc \
  capstr.hh \
  city.cc   \
  city.hh   \
  combat.cc \
  combat.hh \
  connection.cc \
  connection.hh \
  dataio.cc \
  dataio.hh \
  diptreaty.cc \
  diptreaty.hh \
  effects.cc \
  effects.hh \
  events.hh \
  wc_types.hh \
  game.cc  \
  game.hh  \
  generate_packets.py \
  government.cc \
  government.hh \
  idex.cc  \
  idex.hh  \
  improvement.cc \
  improvement.hh \
  map.cc  \
  map.hh  \
  nation.cc        \
  nation.hh        \
  packets.cc \
  packets.hh \
  packets_gen.cc    \
  packets_gen.hh    \
  packets_client.cc \
  packets.def \
  player.cc \
  player.hh \
  spaceship.cc \
  spaceship.hh \
  tech.cc  \
  tech.hh  \
  terrain.cc \
  terrain.hh \
  traderoute.cc \
  traderoute.hh \
  unit.cc  \
  unit.hh  \
  unittype.cc \
  unittype.hh \
  worklist.cc \
  worklist.hh \
  version.cc \
  version.hh

#BUILT_SOURCES = packets_gen.cc packets_gen.hh wc_svnrev_gen.hh
BUILT_SOURCES =

# ./generate_packets makes both packets_gen.[ch]
# Feed the dependence through a fake intermediate file to avoid
# a race condition in a parallel build (like make -j2).
#packets_gen.h packets_gen.c: packets_generate
#.INTERMEDIATE: packets_generate
#packets_generate: packets.def generate_packets.py
# ./generate_packets.py
# touch packets_generate

if SVNREV
# wc_svnrev_gen.h must be generated before it can be included.
# Automatic dependencies are only generated during first
# build. We need dependency for that first build.
BUILT_SOURCES += wc_svnrev_gen.h

.PHONY : wc_svnrev_gen.hh
wc_svnrev_gen.hh :
	$(top_srcdir)/bootstrap/generate_svnrev.sh $(top_srcdir) $(top_builddir)

endif

#libcivcommon_a_DEPENDENCIES = ../utility/libcivutility.a
#libcivcommon_a_LIBADD       = ../utility/libcivutility.a
