## Process this file with automake to produce Makefile.in

## Below used to be just 
##    SUBDIRS = @gui_sources@
## but that doesn't work properly for "make dist"

if CLIENT_GUI_GTK_2_0
GUI_SUBDIR = gui-gtk-2.0
endif

ALL_ESD_FILES=audio_esd.c audio_esd.h
ALL_SDL_FILES=audio_sdl.c audio_sdl.h
ALL_ALSA_FILES=audio_alsa.c audio_alsa.h
ALL_WINMM_FILES=audio_winmm.c audio_winmm.h

if ESD
ESD_FILES=$(ALL_ESD_FILES)
endif
if SDL
SDL_FILES=$(ALL_SDL_FILES)
endif
if ALSA
ALSA_FILES=$(ALL_ALSA_FILES)
endif
if WINMM
WINMM_FILES=$(ALL_WINMM_FILES)
endif

if MINGW32
CLIENTICON=../win32/clienticon.o
endif

## This is usually false, so "include" is not recursed into 
## by 'make', but it can be enabled in configure, and it is
## necessary to have something like this so that the include 
## directory is included for 'make dist'.
if MAKE_CLIENT_INCLUDE
CLI_INCLUDE = include
else
CLI_INCLUDE = 
endif

SUBDIRS = $(GUI_SUBDIR) $(CLI_INCLUDE) agents

bin_PROGRAMS = civclient

AM_CPPFLAGS = -I$(top_srcdir)/utility -I$(srcdir)/include -I$(top_srcdir)/common -I$(top_srcdir)/common/aicore -I../intl -I$(srcdir)/agents @SOUND_CFLAGS@

## Above, note -I../intl instead of -I$(top_srdir/intl) is deliberate.

civclient_SOURCES = $(ESD_FILES) $(SDL_FILES) $(ALSA_FILES) $(WINMM_FILES) \
	attribute.h	\
	attribute.c	\
	audio.c         \
	audio.h         \
	audio_none.c    \
	audio_none.h	\
	citydlg_common.c \
	citydlg_common.h \
	cityrepdata.c	\
	cityrepdata.h	\
	civclient.c	\
	civclient.h	\
	chatline_common.c \
	chatline_common.h \
	connectdlg_common.c \
	connectdlg_common.h \
	climisc.c	\
	climisc.h	\
	climap.c	\
	climap.h	\
	clinet.c	\
	clinet.h	\
	control.c	\
	control.h	\
	goto.c		\
	goto.h		\
	helpdata.c	\
	helpdata.h	\
	mapctrl_common.c \
	mapctrl_common.h \
	mapview_common.c \
	mapview_common.h \
	messagewin_common.c \
	messagewin_common.h \
	multiselect.c	\
	multiselect.h	\
	options.c	\
	options.h	\
	packhand.c	\
	packhand.h	\
	packhand_gen.h	\
	packhand_gen.c	\
	plrdlg_common.c \
	plrdlg_common.h \
	repodlgs_common.c \
	repodlgs_common.h \
	text.c	\
	text.h	\
	tilespec.c	\
	tilespec.h	\
	trade.c	\
	trade.h

civclient_LDFLAGS = @CLIENT_LDFLAGS@
fc_civclient_libs =	../utility/libcivutility.a	\
			../common/libcivcommon.a	\
			../common/aicore/libaicore.a	\
			agents/libagents.a		\
		 	@gui_sources@/libguiclient.a
civclient_DEPENDENCIES = $(fc_civclient_libs)
civclient_LDADD        = $(fc_civclient_libs) $(fc_civclient_libs) \
	@INTLLIBS@ @CLIENT_LIBS@ @SOUND_LIBS@ $(CLIENTICON)
desktopfiledir = $(prefix)/share/applications
desktopfile_DATA = freeciv.desktop
