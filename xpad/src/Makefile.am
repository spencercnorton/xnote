bin_PROGRAMS = xpad

xpad_SOURCES = \
	fio.c fio.h \
	pref.c pref.h \
	pad.c pad.h \
	help.c help.h \
	toolbar.c toolbar.h \
	settings.c settings.h \
	dashboard-frontend.c dashboard-frontend.h \
	defines.h \
	eggtrayicon.c eggtrayicon.h \
	properties.c properties.h \
	xpad-app.c xpad-app.h \
	xpad-dashboard-frontend.c xpad-dashboard-frontend.h \
	xpad-pad-group.c xpad-pad-group.h \
	xpad-session-manager.c xpad-session-manager.h \
	xpad-tray.c xpad-tray.h

localedir = $(datadir)/locale

AM_CFLAGS = @GTK_CFLAGS@ @X_CFLAGS@ -ansi -pedantic -Wall -Werror\
 -DPKGDATADIR=\"$(pkgdatadir)\" -DLOCALE_DIR=\"$(localedir)\"\
 -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED\
 -DGDK_PIXMAP_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED

xpad_LDADD = @X_PRE_LIBS@ @X_LIBS@ @X_EXTRA_LIBS@ @GTK_LIBS@ @LIBINTL@
