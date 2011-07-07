## Process this file with automake to produce Makefile.in

## Override automake so that "make install"  put these in proper place
pkgdatadir = $(datadir)/@PACKAGE@/contrib/daemon

pkgdata_DATA= action.list \
			auth.serv \
			README.daemon \
			warserverd \
			warserver.default \
			warserver.serv \
			warserver.sysv.sh \
			welcome

EXTRA_DIST = $(pkgdata_DATA)