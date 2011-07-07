## Process this file with automake to produce Makefile.in

if CLIENT
CSUBDIRS =
else 
CSUBDIRS =
endif

if SERVER
SSUBDIRS = daemon
else
SSUBDIRS = 
endif

## Override automake so that "make install"  put these in proper place
pkgdatadir = $(datadir)/@PACKAGE@/contrib

pkgdata_DATA = block_tor_exit_nodes.sh

EXTRA_DIST = $(pkgdata_DATA)

SUBDIRS = $(CSUBDIRS) $(SSUBDIRS)
