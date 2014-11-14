## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libagents.a

AM_CPPFLAGS = \
   -I. -I$(srcdir)/.. \
   -I$(top_srcdir)/common/aicore \
   -I$(srcdir)/../include \
   -I$(top_srcdir)/utility \
   -I$(top_srcdir)/common \
   -I../../intl \
   -I$(srcdir)/../gui-gtk \
   @CLIENT_CFLAGS@

libagents_a_SOURCES = 	\
	agents.cc	\
	agents.hh	\
	cma_core.cc 	\
	cma_core.hh 	\
	cma_fec.cc	\
	cma_fec.hh	\
	sha.cc           \
	sha.hh
