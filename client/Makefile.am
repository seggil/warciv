## Process this file with automake to produce Makefile.in

## Below used to be just 
##    SUBDIRS = @gui_sources@
## but that doesn't work properly for "make dist"

if CLIENT_GUI_GTK_2_0
GUI_SUBDIR = gui-gtk-2.0
endif

if CLIENT_GUI_GTK_3_0
GUI_SUBDIRS = gui-gtk-3.0
endif

ALL_ESD_FILES=audio_esd.cc audio_esd.hh
ALL_SDL_FILES=audio_sdl.cc audio_sdl.hh
ALL_ALSA_FILES=audio_alsa.cc audio_alsa.hh
ALL_WINMM_FILES=audio_winmm.cc audio_winmm.hh

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

if CLIENT_GUI_GTK_2_0
bin_PROGRAMS = civclient
endif

if CLIENT_GUI_GTK_3_0
bin_PROGRAMS = civclient-gtk3
endif


#    -I$(top_srcdir)/common/aicore \
#    -I$(srcdir)/agents -I$(srcdir)/include
AM_CPPFLAGS = \
    -I$(top_srcdir)/utility \
    -I$(top_srcdir)/common \
    -I../intl @SOUND_CFLAGS@

## Above, note -I../intl instead of -I$(top_srdir/intl) is deliberate.

civclient_SOURCES = $(ESD_FILES) $(SDL_FILES) $(ALSA_FILES) $(WINMM_FILES) \
 attribute.hh      \
 attribute.cc      \
 audio.cc          \
 audio.hh          \
 audio_none.cc     \
 audio_none.hh     \
 citydlg_common.cc \
 citydlg_common.hc \
 cityrepdata.cc    \
 cityrepdata.hh    \
 civclient.cc      \
 civclient.hh      \
 chatline_common.cc   \
 chatline_common.hh   \
 connectdlg_common.cc \
 connectdlg_common.hh \
 climisc.cc      \
 climisc.hh      \
 climap.cc       \
 climap.hh       \
 clinet.cc       \
 clinet.hh       \
 control.cc      \
 control.hh      \
 goto.cc         \
 goto.hh         \
 helpdata.cc     \
 helpdata.hh     \
 mapctrl_common.cc \
 mapctrl_common.hh \
 mapview_common.cc \
 mapview_common.hh \
 messagewin_common.cc \
 messagewin_common.hh \
 multiselect.cc  \
 multiselect.hh  \
 options.cc      \
 options.h       \
 packhand.cc     \
 packhand.hh     \
 packhand_gen.cc \
 packhand_gen.hh    \
 plrdlg_common.cc   \
 plrdlg_common.hh   \
 repodlgs_common.cc \
 repodlgs_common.hh \
 text.cc \
 text.hh \
 tilespec.cc  \
 tilespec.hh  \
 trade.cc     \
 trade.hh

civclient_LDFLAGS = @CLIENT_LDFLAGS@
wc_civclient_libs =	../utility/libcivutility.a	\
			../common/libcivcommon.a	\
			../common/aicore/libaicore.a	\
			agents/libagents.a		\
		 	@gui_sources@/libguiclient.a
civclient_DEPENDENCIES = $(wc_civclient_libs)
civclient_LDADD        = $(wc_civclient_libs) $(wc_civclient_libs) \
	@INTLLIBS@ @CLIENT_LIBS@ \
	@SOUND_LIBS@ $(CLIENTICON)
desktopfiledir = $(prefix)/share/applications
desktopfile_DATA = warciv.desktop

.c.o:
	$(AM_V_CC)$(CXXCOMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	$(AM_V_at)$(am__mv) $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
