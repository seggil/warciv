## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libaicore.a

AM_CPPFLAGS = -I$(top_srcdir)/utility -I.. -I$(top_srcdir)/common -I../../intl
SRC2_CFLAGS = -01
libaicore_a_SOURCES = $(SRC1) $(SRC2)
SRC1 =	aisupport.c		\
	aisupport.h		\
	path_finding.c		\
	path_finding.h		\
	pf_tools.c		\
	pf_tools.h		\
	cm.h			\
	citymap.c		\
	citymap.h
SRC2 = 	cm.c

