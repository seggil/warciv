## Process this file with automake to produce Makefile.in

if CLIENT
CLIENTSUBDIRS = client
else
CLIENTSUBDIRS = 
endif

if SERVER
SERVERSUBDIRS = ai server
else
SERVERSUBDIRS =
endif

if SERVER
if CLIENT
MANUALSUBDIRS = manual
else
MANUALSUBDIRS = 
endif
else
MANUALSUBDIRS = 
endif

if MAKE_DATA
DATASUBDIRS = data
else
DATASUBDIRS =
endif


SUBDIRS= $(DATASUBDIRS) intl utility common tests win32 $(SERVERSUBDIRS) $(CLIENTSUBDIRS) \
         po doc $(MANUALSUBDIRS) contrib

EXTRA_DIST=	autogen.sh \
		config.mac.h   \
		configure.ac   \
		diff_ignore             \
		bootstrap/civ.in        \
		bootstrap/ser.in        \
		bootstrap/fc_svnrev_gen.h.in    \
		bootstrap/generate_svnrev.sh    \
		bootstrap/warciv.desktop.in    \
		bootstrap/warciv.spec.in       \
		bootstrap/undep.sh.in           \
		debian/changelog        \
		debian/compat           \
		debian/control          \
		debian/copyright        \
		debian/rules            \
		debian/warclient*       \
		debian/watch            \
		debian/patches/01_configure_ac_localedir.dpatch     \
		debian/patches/series       \
		m4/ac_path_lib.m4       \
		m4/alsa.m4              \
		m4/c99.m4               \
		m4/debug.m4             \
		m4/deprecated.m4        \
		m4/esd.m4               \
		m4/freetype2.m4         \
		m4/gettext.m4           \
		m4/gettimeofday.m4      \
		m4/glib-2.0.m4          \
		m4/glib-gettext.m4      \
		m4/glib.m4              \
		m4/glibc21.m4           \
		m4/gtk-2.0.m4           \
		m4/gtk2-client.m4       \
		m4/iconv.m4             \
		m4/imlib.m4             \
		m4/isc-posix.m4         \
		m4/lcmessage.m4         \
		m4/locale.m4            \
		m4/mysql.m4             \
		m4/ngettext.m4          \
		m4/no-client.m4         \
		m4/progtest.m4          \
		m4/readline.m4          \
		m4/sdl.m4               \
		m4/sound.m4             \
		m4/version.m4           \
		m4/vsnprintf.m4         \
		m4/win32-client.m4      \
		m4/x.m4                 \
		vms/config.h_vms        \
		vms/warciv_h.vms       \
		vms/imlib_config.h_vms  \
		vms/readme.vms          \
		vms/vms_build.com

noinst_HEADERS= version.in

src-check:
	cd tests && $(MAKE) $(AM_MAKEFLAGS) src-check

.PHONY: src-check

