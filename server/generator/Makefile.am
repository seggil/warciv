## Process this file with automake to produce Makefile.in
noinst_LIBRARIES = libgenerator.a

AM_CPPFLAGS = -I$(top_srcdir)/utility -I$(top_srcdir)/common \
 -I$(top_srcdir)/server -I$(top_builddir)/intl \
 -I$(top_srcdir)/server/generator

libgenerator_a_SOURCES = \
 mapgen.cc \
 mapgen.hh \
 mapgen_topology.cc \
 mapgen_topology.hh \
 utilities.cc \
 utilities.hh \
 height_map.cc \
 height_map.hh \
 startpos.cc \
 startpos.hh \
 temperature_map.cc \
 temperature_map.hh
