## Process this file with automake to produce Makefile.in

if CLIENT
COMMON_CLIENT_FILES =			\
	isophex.tilespec		\
	isotrident.tilespec		\
	trident.tilespec		\
	trident_shields.tilespec	\
	helpdata.txt			\
	stdsounds.soundspec
if CLIENT_GUI_XAW
CLIENT_EXTRA_FILES = Freeciv
endif
if CLIENT_GUI_GTK
CLIENT_EXTRA_FILES = freeciv.rc
endif
if CLIENT_GUI_GTK_2_0
CLIENT_EXTRA_FILES = freeciv.rc-2.0
endif
CLIENT_FILES = $(COMMON_CLIENT_FILES) $(CLIENT_EXTRA_FILES)
else
CLIENT_FILES =
endif

if SERVER
SERVER_FILES = \
	civ1.serv	\
	civ2.serv	\
	history.serv	\
	warruleset-1.2.serv	\
	warruleset-1.3.serv	\
	nowonder+1.2.serv	\
	tournament.serv	\
	warserver.serv	\
	exemple-actions.txt	\
	exemple-welcome.txt
else
SERVER_FILES =
endif

pkgdata_DATA = $(SERVER_FILES) $(CLIENT_FILES)

EXTRA_DIST = \
	Freeciv	\
	freeciv.rc	\
	freeciv.rc-2.0	\
	isophex.tilespec	\
	isotrident.tilespec	\
	trident.tilespec	\
	trident_shields.tilespec	\
	helpdata.txt	\
	civ1.serv	\
	civ2.serv	\
	history.serv	\
	warruleset-1.2.serv	\
	warruleset-1.3.serv	\
	tournament.serv	\
	warserver.serv	\
	exemple-actions.txt	\
	exemple-welcome.txt

if CLIENT
CLIENTDATADIRS = misc flags isotrident trident isophex stdsounds
else
CLIENTDATADIRS = 
endif

if SERVER
SERVERDATADIRS = default civ1 civ2 warruleset-1.2 warruleset-1.3 nowonder+1.2 duel scenario nation history maps tournament
else
SERVERDATADIRS = 
endif

SUBDIRS = $(CLIENTDATADIRS) $(SERVERDATADIRS)