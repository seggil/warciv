## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libaicore.a

AM_CPPFLAGS = -I$(top_srcdir)/utility -I.. -I$(top_srcdir)/common -I../../intl
SRC2_CFLAGS = -01
libaicore_a_SOURCES = $(SRC1) $(SRC2)
SRC1 = aisupport.cc \
 aisupport.hh    \
 path_finding.cc \
 path_finding.hh \
 pf_tools.cc     \
 pf_tools.hh     \
 cm.hh           \
 citymap.cc      \
 citymap.hh
SRC2 = 	cm.cc
