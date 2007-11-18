# Makefile for program source directory in GNU NLS utilities package.
# Copyright (C) 1995-1997, 2000, 2001 by Ulrich Drepper <drepper@gnu.ai.mit.edu>
#
# This file file be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

PACKAGE = freeciv
VERSION = 2.0.9

# These two variables depend on the location of this directory.
subdir = po
top_builddir = ..

SHELL = /bin/sh


srcdir = .
top_srcdir = ..


prefix = /usr/local
exec_prefix = ${prefix}
datadir = ${prefix}/share
localedir = $(datadir)/locale
gettextsrcdir = $(datadir)/gettext/po

INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
MKINSTALLDIRS = bootstrap/mkinstalldirs
mkinstalldirs = $(SHELL) `case "$(MKINSTALLDIRS)" in /*) echo "$(MKINSTALLDIRS)" ;; *) echo "$(top_builddir)/$(MKINSTALLDIRS)" ;; esac`

CC = gcc
GMSGFMT = /usr/bin/msgfmt
MSGFMT = /usr/bin/msgfmt
XGETTEXT = /usr/bin/xgettext
MSGMERGE = msgmerge

DEFS = -DHAVE_CONFIG_H
CFLAGS = -g -O2 -Wall -Wpointer-arith -Wcast-align -Wmissing-prototypes -Wmissing-declarations
CPPFLAGS =  -DLOCALEDIR="\"${datarootdir}/locale\"" -DDEFAULT_DATA_PATH="\".:data:~/.freeciv:${datarootdir}/freeciv\""

INCLUDES = -I.. -I$(top_srcdir)/intl

COMPILE = $(CC) -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(XCFLAGS)

POFILES =  ar.po ca.po cs.po da.po de.po en_GB.po es.po et.po fa.po fi.po fr.po he_IL.po hu.po it.po ja.po nl.po nb.po no.po pl.po pt.po pt_BR.po ro.po ru.po sv.po uk.po
GMOFILES =  ar.gmo ca.gmo cs.gmo da.gmo de.gmo en_GB.gmo es.gmo et.gmo fa.gmo fi.gmo fr.gmo he_IL.gmo hu.gmo it.gmo ja.gmo nl.gmo nb.gmo no.gmo pl.gmo pt.gmo pt_BR.gmo ro.gmo ru.gmo sv.gmo uk.gmo
DISTFILES = ChangeLog Makefile.in.in POTFILES.in POTFILES.skip \
            $(PACKAGE).pot $(POFILES) $(GMOFILES)

POTFILES = \
     ../common/city.c \
     ../common/connection.c \
     ../common/effects.c \
     ../common/game.c \
     ../common/map.c \
     ../common/player.c \
     ../common/tech.c \
     ../common/unit.c \
     ../common/unittype.c \
     ../common/version.c \
     ../common/aicore/cm.c \
     ../utility/astring.h \
     ../utility/fciconv.c \
     ../utility/fcintl.c \
     ../utility/fcintl.h \
     ../utility/hash.c \
     ../utility/inputfile.c \
     ../utility/log.c \
     ../utility/netintf.c \
     ../utility/registry.c \
     ../utility/registry.h \
     ../utility/shared.c \
     ../utility/shared.h \
     ../utility/speclist.h \
     ../utility/specvec.h \
     ../utility/support.c \
     ../utility/ftwl/be_common_24_sprite.c \
     ../utility/ftwl/be_sdl.c \
     ../utility/ftwl/theme_engine.c \
     ../utility/tadns.c \
     ../ai/advdiplomacy.c \
     ../ai/aicity.c \
     ../server/auth.c \
     ../server/autoattack.c \
     ../server/barbarian.c \
     ../server/cityhand.c \
     ../server/citytools.c \
     ../server/cityturn.c \
     ../server/commands.c \
     ../server/connecthand.c \
     ../server/console.c \
     ../server/diplhand.c \
     ../server/diplomats.c \
     ../server/gamehand.c \
     ../server/gamelog.c \
     ../server/handchat.c \
     ../server/maphand.c \
     ../server/meta.c \
     ../server/plrhand.c \
     ../server/report.c \
     ../server/ruleset.c \
     ../server/savegame.c \
     ../server/sernet.c \
     ../server/settings.c \
     ../server/spacerace.c \
     ../server/srv_main.c \
     ../server/stdinhand.c \
     ../server/unithand.c \
     ../server/unittools.c \
     ../server/civserver.c \
     ../server/generator/mapgen.c \
     ../server/generator/startpos.c \
     ../client/gui-mui/chatline.c \
     ../client/gui-mui/citydlg.c \
     ../client/gui-mui/cityrep.c \
     ../client/gui-mui/connectdlg.c \
     ../client/gui-mui/dialogs.c \
     ../client/gui-mui/diplodlg.c \
     ../client/gui-mui/finddlg.c \
     ../client/gui-mui/gamedlgs.c \
     ../client/gui-mui/gotodlg.c \
     ../client/gui-mui/graphics.c \
     ../client/gui-mui/gui_main.c \
     ../client/gui-mui/helpdlg.c \
     ../client/gui-mui/inputdlg.c \
     ../client/gui-mui/inteldlg.c \
     ../client/gui-mui/mapclass.c \
     ../client/gui-mui/mapctrl.c \
     ../client/gui-mui/mapview.c \
     ../client/gui-mui/messagedlg.c \
     ../client/gui-mui/messagewin.c \
     ../client/gui-mui/plrdlg.c \
     ../client/gui-mui/repodlgs.c \
     ../client/gui-mui/spaceshipdlg.c \
     ../client/gui-mui/wldlg.c \
     ../client/gui-mui/worklistclass.c \
     ../client/audio_esd.c \
     ../client/attribute.c \
     ../client/citydlg_common.c \
     ../client/cityrepdata.c \
     ../client/civclient.c \
     ../client/connectdlg_common.c \
     ../client/climisc.c \
     ../client/clinet.c \
     ../client/control.c \
     ../client/helpdata.c \
     ../client/mapctrl_common.c \
     ../client/mapview_common.c \
     ../client/messagewin_common.c \
     ../client/packhand.c \
     ../client/plrdlg_common.c \
     ../client/options.c \
     ../client/repodlgs_common.c \
     ../client/text.c \
     ../client/tilespec.c \
     ../client/audio.c \
     ../client/gui-ftwl/chatline.c \
     ../client/gui-ftwl/connectdlg.c \
     ../client/gui-ftwl/dialogs.c \
     ../client/gui-ftwl/gui_main.c \
     ../client/gui-ftwl/gui_text.c \
     ../client/gui-ftwl/helpdlg.c \
     ../client/gui-ftwl/mapview.c \
     ../client/gui-gtk-2.0/chatline.c \
     ../client/gui-gtk-2.0/citydlg.c \
     ../client/gui-gtk-2.0/cityrep.c \
     ../client/gui-gtk-2.0/cma_fe.c \
     ../client/gui-gtk-2.0/connectdlg.c \
     ../client/gui-gtk-2.0/dialogs.c \
     ../client/gui-gtk-2.0/diplodlg.c \
     ../client/gui-gtk-2.0/finddlg.c \
     ../client/gui-gtk-2.0/gamedlgs.c \
     ../client/gui-gtk-2.0/gotodlg.c \
     ../client/gui-gtk-2.0/gui_main.c \
     ../client/gui-gtk-2.0/gui_stuff.c \
     ../client/gui-gtk-2.0/happiness.c \
     ../client/gui-gtk-2.0/helpdlg.c \
     ../client/gui-gtk-2.0/inteldlg.c \
     ../client/gui-gtk-2.0/mapctrl.c \
     ../client/gui-gtk-2.0/mapview.c \
     ../client/gui-gtk-2.0/menu.c \
     ../client/gui-gtk-2.0/messagedlg.c \
     ../client/gui-gtk-2.0/messagewin.c \
     ../client/gui-gtk-2.0/pages.c \
     ../client/gui-gtk-2.0/plrdlg.c \
     ../client/gui-gtk-2.0/repodlgs.c \
     ../client/gui-gtk-2.0/spaceshipdlg.c \
     ../client/gui-gtk-2.0/wldlg.c \
     ../client/gui-gtk/chatline.c \
     ../client/gui-gtk/citydlg.c \
     ../client/gui-gtk/cityrep.c \
     ../client/gui-gtk/cma_fe.c \
     ../client/gui-gtk/connectdlg.c \
     ../client/gui-gtk/dialogs.c \
     ../client/gui-gtk/diplodlg.c \
     ../client/gui-gtk/finddlg.c \
     ../client/gui-gtk/gamedlgs.c \
     ../client/gui-gtk/gotodlg.c \
     ../client/gui-gtk/gui_main.c \
     ../client/gui-gtk/gui_stuff.c \
     ../client/gui-gtk/happiness.c \
     ../client/gui-gtk/helpdlg.c \
     ../client/gui-gtk/inputdlg.c \
     ../client/gui-gtk/inteldlg.c \
     ../client/gui-gtk/mapctrl.c \
     ../client/gui-gtk/mapview.c \
     ../client/gui-gtk/menu.c \
     ../client/gui-gtk/messagedlg.c \
     ../client/gui-gtk/messagewin.c \
     ../client/gui-gtk/plrdlg.c \
     ../client/gui-gtk/repodlgs.c \
     ../client/gui-gtk/spaceshipdlg.c \
     ../client/gui-gtk/wldlg.c \
     ../client/gui-sdl/chatline.c \
     ../client/gui-sdl/citydlg.c \
     ../client/gui-sdl/cityrep.c \
     ../client/gui-sdl/cma_fe.c \
     ../client/gui-sdl/connectdlg.c \
     ../client/gui-sdl/dialogs.c \
     ../client/gui-sdl/diplodlg.c \
     ../client/gui-sdl/finddlg.c \
     ../client/gui-sdl/gotodlg.c \
     ../client/gui-sdl/graphics.c \
     ../client/gui-sdl/gui_main.c \
     ../client/gui-sdl/helpdlg.c \
     ../client/gui-sdl/inteldlg.c \
     ../client/gui-sdl/mapctrl.c \
     ../client/gui-sdl/mapview.c \
     ../client/gui-sdl/menu.c \
     ../client/gui-sdl/messagewin.c \
     ../client/gui-sdl/optiondlg.c \
     ../client/gui-sdl/plrdlg.c \
     ../client/gui-sdl/repodlgs.c \
     ../client/gui-sdl/spaceshipdlg.c \
     ../client/gui-sdl/wldlg.c \
     ../client/gui-sdl/gui_string.c \
     ../client/gui-sdl/gui_mem.h \
     ../client/gui-stub/chatline.c \
     ../client/gui-stub/connectdlg.c \
     ../client/gui-stub/helpdlg.c \
     ../client/gui-stub/mapview.c \
     ../client/gui-win32/chatline.c \
     ../client/gui-win32/citydlg.c \
     ../client/gui-win32/cma_fe.c \
     ../client/gui-win32/cityrep.c \
     ../client/gui-win32/connectdlg.c \
     ../client/gui-win32/dialogs.c \
     ../client/gui-win32/diplodlg.c \
     ../client/gui-win32/finddlg.c \
     ../client/gui-win32/gotodlg.c \
     ../client/gui-win32/gui_main.c \
     ../client/gui-win32/happiness.c \
     ../client/gui-win32/helpdlg.c \
     ../client/gui-win32/inputdlg.c \
     ../client/gui-win32/inteldlg.c \
     ../client/gui-win32/mapctrl.c \
     ../client/gui-win32/mapview.c \
     ../client/gui-win32/menu.c \
     ../client/gui-win32/messagedlg.c \
     ../client/gui-win32/messagewin.c \
     ../client/gui-win32/optiondlg.c \
     ../client/gui-win32/plrdlg.c \
     ../client/gui-win32/ratesdlg.c \
     ../client/gui-win32/repodlgs.c \
     ../client/gui-win32/spaceshipdlg.c \
     ../client/gui-win32/wldlg.c \
     ../client/gui-xaw/chatline.c \
     ../client/gui-xaw/citydlg.c \
     ../client/gui-xaw/cityrep.c \
     ../client/gui-xaw/cma_fe.c \
     ../client/gui-xaw/connectdlg.c \
     ../client/gui-xaw/dialogs.c \
     ../client/gui-xaw/diplodlg.c \
     ../client/gui-xaw/graphics.c \
     ../client/gui-xaw/gui_main.c \
     ../client/gui-xaw/gui_stuff.c \
     ../client/gui-xaw/helpdlg.c \
     ../client/gui-xaw/inputdlg.c \
     ../client/gui-xaw/inteldlg.c \
     ../client/gui-xaw/mapctrl.c \
     ../client/gui-xaw/mapview.c \
     ../client/gui-xaw/menu.c \
     ../client/gui-xaw/messagedlg.c \
     ../client/gui-xaw/optiondlg.c \
     ../client/gui-xaw/plrdlg.c \
     ../client/gui-xaw/ratesdlg.c \
     ../client/gui-xaw/repodlgs.c \
     ../client/gui-xaw/wldlg.c \
     ../client/agents/cma_core.c \
     ../client/agents/cma_fec.c \
     ../client/myai.c \
     ../client/peptool.c \
     ../client/multiselect.c \
     ../client/multiselect.h \
     ../data/Freeciv.in \
     ../data/helpdata.txt \
     ../data/default/buildings.ruleset \
     ../data/default/cities.ruleset \
     ../data/default/governments.ruleset \
     ../data/default/techs.ruleset \
     ../data/default/terrain.ruleset \
     ../data/default/units.ruleset \
     ../data/civ1/buildings.ruleset \
     ../data/civ1/governments.ruleset \
     ../data/civ1/techs.ruleset \
     ../data/civ1/terrain.ruleset \
     ../data/civ1/units.ruleset \
     ../data/civ1/cities.ruleset \
     ../data/civ2/buildings.ruleset \
     ../data/civ2/governments.ruleset \
     ../data/civ2/techs.ruleset \
     ../data/civ2/terrain.ruleset \
     ../data/civ2/units.ruleset \
     ../data/civ2/cities.ruleset \
     ../data/nation/afghani.ruleset \
     ../data/nation/american.ruleset \
     ../data/nation/arab.ruleset \
     ../data/nation/argentine.ruleset \
     ../data/nation/assyrian.ruleset \
     ../data/nation/australian.ruleset \
     ../data/nation/austrian.ruleset \
     ../data/nation/aztec.ruleset \
     ../data/nation/babylonian.ruleset \
     ../data/nation/barbarian.ruleset \
     ../data/nation/bavarian.ruleset \
     ../data/nation/belgian.ruleset \
     ../data/nation/boer.ruleset \
     ../data/nation/bosnia.ruleset \
     ../data/nation/brazilian.ruleset \
     ../data/nation/canadian.ruleset \
     ../data/nation/carthaginian.ruleset \
     ../data/nation/catalan.ruleset \
     ../data/nation/chilean.ruleset \
     ../data/nation/chinese.ruleset \
     ../data/nation/columbian.ruleset \
     ../data/nation/cornish.ruleset \
     ../data/nation/croatian.ruleset \
     ../data/nation/czech.ruleset \
     ../data/nation/danish.ruleset \
     ../data/nation/dunedain.ruleset \
     ../data/nation/dutch.ruleset \
     ../data/nation/egyptian.ruleset \
     ../data/nation/elvish.ruleset \
     ../data/nation/english.ruleset \
     ../data/nation/estonian.ruleset \
     ../data/nation/ethiopian.ruleset \
     ../data/nation/filipino.ruleset \
     ../data/nation/finnish.ruleset \
     ../data/nation/french.ruleset \
     ../data/nation/galician.ruleset \
     ../data/nation/german.ruleset \
     ../data/nation/greek.ruleset \
     ../data/nation/hobbits.ruleset \
     ../data/nation/hungarian.ruleset \
     ../data/nation/icelandic.ruleset \
     ../data/nation/inca.ruleset \
     ../data/nation/indian.ruleset \
     ../data/nation/indonesian.ruleset \
     ../data/nation/irish.ruleset \
     ../data/nation/israeli.ruleset \
     ../data/nation/italian.ruleset \
     ../data/nation/japanese.ruleset \
     ../data/nation/kampuchean.ruleset \
     ../data/nation/kenyan.ruleset \
     ../data/nation/korean.ruleset \
     ../data/nation/latvian.ruleset \
     ../data/nation/lithuanian.ruleset \
     ../data/nation/malaysian.ruleset \
     ../data/nation/martian.ruleset \
     ../data/nation/mexican.ruleset \
     ../data/nation/mongol.ruleset \
     ../data/nation/mordor.ruleset \
     ../data/nation/nigerian.ruleset \
     ../data/nation/observer.ruleset \
     ../data/nation/persian.ruleset \
     ../data/nation/phoenician.ruleset \
     ../data/nation/polish.ruleset \
     ../data/nation/portuguese.ruleset \
     ../data/nation/quebecois.ruleset \
     ../data/nation/roman.ruleset \
     ../data/nation/russian.ruleset \
     ../data/nation/scottish.ruleset \
     ../data/nation/serbian.ruleset \
     ../data/nation/silesian.ruleset \
     ../data/nation/singaporean.ruleset \
     ../data/nation/sioux.ruleset \
     ../data/nation/slovenian.ruleset \
     ../data/nation/spanish.ruleset \
     ../data/nation/sumerian.ruleset \
     ../data/nation/swedish.ruleset \
     ../data/nation/swiss.ruleset \
     ../data/nation/taiwanese.ruleset \
     ../data/nation/thai.ruleset \
     ../data/nation/turk.ruleset \
     ../data/nation/ukrainian.ruleset \
     ../data/nation/vietnamese.ruleset \
     ../data/nation/viking.ruleset \
     ../data/nation/welsh.ruleset \
     ../data/nation/zulu.ruleset \
     ../data/nation/nato.ruleset \
     ../data/nation/chechenia.ruleset \
     ../data/nation/cuba.ruleset \
     ../data/nation/palestine.ruleset \
     ../data/nation/bulgaria.ruleset \
     ../data/nation/greenland.ruleset \
     ../data/nation/azerbaijan.ruleset \
     ../data/nation/armenian.ruleset \
     ../data/nation/united_kingdom.ruleset \
     ../data/nation/vietcongs.ruleset \
     ../data/nation/hongkong.ruleset \
     ../data/nation/pakistani.ruleset \
     ../data/nation/sudanese.ruleset \
     ../data/nation/soviet.ruleset \
     ../data/nation/european.ruleset \
     ../data/nation/macedonia.ruleset \
     ../data/nation/united_nations.ruleset \
     ../data/nation/loatian.ruleset \
     ../data/nation/syria.ruleset \
     ../data/nation/rwandan.ruleset \
     ../data/nation/iraq.ruleset \
     ../data/history/buildings.ruleset \
     ../data/history/game.ruleset \
     ../data/history/governments.ruleset \
     ../data/history/techs.ruleset \
     ../data/history/terrain.ruleset \
     ../data/history/units.ruleset \
     ../data/history/cities.ruleset \
     ../data/themes/morgan/640x480/mapview.screen \
     ../data/themes/morgan/640x480/connect.screen \
     ../data/themes/morgan/640x480/mapview.prop \
     ../data/nowonder+1.2/buildings.ruleset \
     ../data/nowonder+1.2/cities.ruleset \
     ../data/nowonder+1.2/units.ruleset \
     ../data/nowonder+1.2/terrain.ruleset \
     ../data/nowonder+1.2/techs.ruleset \
     ../data/nowonder+1.2/governments.ruleset \
     ../data/nowonder+1.2/game.ruleset \
     ../data/duel/buildings.ruleset \
     ../data/duel/cities.ruleset \
     ../data/duel/units.ruleset \
     ../data/duel/terrain.ruleset \
     ../data/duel/techs.ruleset \
     ../data/duel/governments.ruleset \
     ../data/duel/game.ruleset \
     ../data/tournament/buildings.ruleset \
     ../data/tournament/cities.ruleset \
     ../data/tournament/units.ruleset \
     ../data/tournament/terrain.ruleset \
     ../data/tournament/techs.ruleset \
     ../data/tournament/governments.ruleset \
     ../data/warruleset-1.2/buildings.ruleset \
     ../data/warruleset-1.2/cities.ruleset \
     ../data/warruleset-1.2/units.ruleset \
     ../data/warruleset-1.2/terrain.ruleset \
     ../data/warruleset-1.2/techs.ruleset \
     ../data/warruleset-1.2/governments.ruleset \
     ../data/warruleset-1.2/game.ruleset \
     ../data/warruleset-1.3/buildings.ruleset \
     ../data/warruleset-1.3/cities.ruleset \
     ../data/warruleset-1.3/units.ruleset \
     ../data/warruleset-1.3/terrain.ruleset \
     ../data/warruleset-1.3/techs.ruleset \
     ../data/warruleset-1.3/governments.ruleset \
     ../data/warruleset-1.3/game.ruleset \
     ../manual/civmanual.c

CATALOGS =  ar.gmo ca.gmo cs.gmo da.gmo de.gmo en_GB.gmo es.gmo et.gmo fa.gmo fi.gmo fr.gmo he_IL.gmo hu.gmo it.gmo ja.gmo nl.gmo nb.gmo no.gmo pl.gmo pt.gmo pt_BR.gmo ro.gmo ru.gmo sv.gmo uk.gmo

.SUFFIXES:
.SUFFIXES: .c .o .po .pox .gmo .mo

.c.o:
	$(COMPILE) $<

.po.pox:
	$(MAKE) $(PACKAGE).pot
	$(MSGMERGE) $< $(srcdir)/$(PACKAGE).pot -o $*.pox

.po.mo:
	$(MSGFMT) -o $@ $<

.po.gmo:
	file=$(srcdir)/`echo $* | sed 's,.*/,,'`.gmo \
	  && rm -f $$file && $(GMSGFMT) -c --statistics -o $$file $<


all: all-yes

all-yes: $(CATALOGS)
all-no:

# Note: Target 'all' must not depend on target '$(srcdir)/$(PACKAGE).pot',
# otherwise packages like GCC can not be built if only parts of the source
# have been downloaded.

$(srcdir)/$(PACKAGE).pot: $(POTFILES) $(srcdir)/POTFILES.in
	$(XGETTEXT) --default-domain=$(PACKAGE) --directory=$(top_srcdir) \
	  --add-comments="TRANS:" --escape --keyword=_ --keyword=N_ --keyword=Q_ --keyword=PL_:1,2 \
	  --language=C --files-from=$(srcdir)/POTFILES.in \
	&& test ! -f $(PACKAGE).po \
	   || ( rm -f $(srcdir)/$(PACKAGE).pot \
		&& mv $(PACKAGE).po $(srcdir)/$(PACKAGE).pot )


install: install-exec install-data
install-exec:
install-data: install-data-yes
	if test "$(PACKAGE)" = "gettext"; then \
	  $(mkinstalldirs) $(DESTDIR)$(gettextsrcdir); \
	  $(INSTALL_DATA) $(srcdir)/Makefile.in.in \
			  $(DESTDIR)$(gettextsrcdir)/Makefile.in.in; \
	else \
	  : ; \
	fi
install-data-no: all
install-data-yes: all
	$(mkinstalldirs) $(DESTDIR)$(datadir)
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\.gmo$$//'`; \
	  dir=$(localedir)/$$lang/LC_MESSAGES; \
	  $(mkinstalldirs) $(DESTDIR)$$dir; \
	  if test -r $$cat; then \
	    $(INSTALL_DATA) $$cat $(DESTDIR)$$dir/$(PACKAGE).mo; \
	    echo "installing $$cat as $(DESTDIR)$$dir/$(PACKAGE).mo"; \
	  else \
	    $(INSTALL_DATA) $(srcdir)/$$cat $(DESTDIR)$$dir/$(PACKAGE).mo; \
	    echo "installing $(srcdir)/$$cat as" \
		 "$(DESTDIR)$$dir/$(PACKAGE).mo"; \
	  fi; \
	done

# Define this as empty until I found a useful application.
installcheck:

uninstall:
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\.gmo$$//'`; \
	  rm -f $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(PACKAGE).mo; \
	done
	if test "$(PACKAGE)" = "gettext"; then \
	  rm -f $(DESTDIR)$(gettextsrcdir)/Makefile.in.in; \
	else \
	  : ; \
	fi

check: all

dvi info tags TAGS ID:

mostlyclean:
	rm -f core core.* *.pox $(PACKAGE).po *.new.po
	rm -fr *.o

clean: mostlyclean

distclean: clean
	rm -f Makefile Makefile.in POTFILES *.mo

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f $(GMOFILES)

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)
dist distdir:
	$(MAKE) update-po
	@$(MAKE) dist2
# This is a separate target because 'update-po' must be executed before.
dist2: $(DISTFILES)
	dists="$(DISTFILES)"; \
	for file in $$dists; do \
	  if test -f $$file; then dir=.; else dir=$(srcdir); fi; \
	  cp -p $$dir/$$file $(distdir); \
	done

update-po: Makefile
	$(MAKE) $(PACKAGE).pot
	if test "$(PACKAGE)" = "gettext"; then PATH=`pwd`/../src:$$PATH; fi; \
	cd $(srcdir); \
	catalogs='$(GMOFILES)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\.gmo$$//'`; \
	  echo "$$lang:"; \
	  if $(MSGMERGE) $$lang.po $(PACKAGE).pot -o $$lang.new.po; then \
	    mv -f $$lang.new.po $$lang.po; \
	  else \
	    echo "msgmerge for $$cat failed!"; \
	    rm -f $$lang.new.po; \
	  fi; \
	done
	$(MAKE) update-gmo

update-gmo: Makefile $(GMOFILES)
	@:

nb.po: no.po
	cp $(srcdir)/no.po nb.po

BUILT_SOURCES = nb.po

Makefile: Makefile.in.in $(top_builddir)/config.status POTFILES.in
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@.in CONFIG_HEADERS= \
	       $(SHELL) ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
