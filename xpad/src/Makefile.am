bin_PROGRAMS = xpad

xpad_SOURCES = \
	fio.c fio.h \
	help.c help.h \
	eggtrayicon.c eggtrayicon.h \
	xpad-app.c xpad-app.h \
	xpad-grip-tool-item.c xpad-grip-tool-item.h \
	xpad-pad.c xpad-pad.h \
	xpad-pad-group.c xpad-pad-group.h \
	xpad-pad-properties.c xpad-pad-properties.h \
	xpad-preferences.c xpad-preferences.h \
	xpad-session-manager.c xpad-session-manager.h \
	xpad-settings.c xpad-settings.h \
	xpad-text-buffer.c xpad-text-buffer.h \
	xpad-text-view.c xpad-text-view.h \
	xpad-toolbar.c xpad-toolbar.h \
	xpad-tray.c xpad-tray.h

localedir = $(datadir)/locale

DEBUG_CFLAGS = 
DEBUG_CFLAGS = -ansi -pedantic -Wall -Werror -g\
 -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED\
 -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED

AM_CFLAGS = @GTK_CFLAGS@ @X_CFLAGS@ $(DEBUG_CFLAGS)\
 -DPKGDATADIR=\"$(pkgdatadir)\" -DLOCALE_DIR=\"$(localedir)\"

xpad_LDADD = @X_PRE_LIBS@ @X_LIBS@ @X_EXTRA_LIBS@ @GTK_LIBS@ @LIBINTL@
