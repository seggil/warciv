## Process this file with automake to produce Makefile.in

RC_COMPILER = @RC_COMPILER@

dist_noinst_DATA = \
		client.ico \
		server.ico \
		clienticon.rc \
		servericon.rc

if MINGW32
nodist_noinst_DATA = \
		clienticon.o \
		servericon.o

CLEANFILES = $(nodist_noinst_DATA)

clienticon.o: clienticon.rc
	$(RC_COMPILER) -i $(srcdir)/clienticon.rc -o $(builddir)/clienticon.o

servericon.o: servericon.rc
	$(RC_COMPILER) -i $(srcdir)/servericon.rc -o $(builddir)/servericon.o
endif

