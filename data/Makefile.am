## Process this file with automake to produce Makefile.in

#pkgdatadir = $(datadir)/@PACKAGE@-@VERSION_WITHOUT_LABEL@

if CLIENT
COMMON_CLIENT_FILES =			\
	isophex.tilespec		\
	isotrident.tilespec		\
	trident.tilespec		\
	trident_shields.tilespec	\
	helpdata.txt			\
	stdsounds.soundspec

if CLIENT_GUI_GTK_2_0
CLIENT_EXTRA_FILES = warciv.rc-2.0
endif
CLIENT_FILES = $(COMMON_CLIENT_FILES) $(CLIENT_EXTRA_FILES)
else
CLIENT_FILES =
endif

if SERVER
SERVER_FILES =              \
	auth.sql	    \
	civ1.serv           \
	civ2.serv           \
	database.lua	\
	example-actions.txt \
	example-welcome.txt \
	wcdb.sql	    \
	history.serv	    \
	nowonder+1.2.serv   \
	nowonder+-1.3.serv  \
	nowonder+-1.4.serv  \
	team_names.txt	    \
	tournament.serv	    \
	warserver.serv	    \
	warruleset-1.2.serv \
	warruleset-1.3.serv
else
SERVER_FILES =
endif

pkgdata_DATA = $(SERVER_FILES) $(CLIENT_FILES)

EXTRA_DIST =            \
	Freeciv	        \
	warciv.rc-2.0	\
	isophex.tilespec	 \
	isotrident.tilespec      \
	trident.tilespec	 \
	trident_shields.tilespec \
	helpdata.txt	         \
	civ1.serv	\
	civ2.serv	\
	history.serv	        \
	warruleset-1.2.serv	\
	warruleset-1.3.serv	\
	nowonder+1.2.serv	\
	nowonder+-1.3.serv	\
	nowonder+-1.4.serv	\
	tournament.serv	\
	warserver.serv	\
	example-actions.txt	\
	example-welcome.txt	\
	auth.sql	        \
	wcdb.sql	        \
	database.lua		\
	stdsounds.soundspec

if CLIENT
CLIENTDATADIRS = misc flags isotrident trident isophex stdsounds
else
CLIENTDATADIRS = 
endif

if SERVER
SERVERDATADIRS =	\
	default	\
	civ1	\
	civ2	\
	warruleset-1.2	\
	warruleset-1.3	\
	nowonder+-1.2	\
	nowonder+-1.3	\
	nowonder+-1.4	\
	nowonder-1.2	\
	duel	        \
	experimental	\
	1.14style	\
	scenario	\
	nation	   \
	history	   \
	maps	   \
	tournament
else
SERVERDATADIRS = 
endif

SUBDIRS = $(CLIENTDATADIRS) $(SERVERDATADIRS)

