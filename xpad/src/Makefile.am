bin_PROGRAMS = xpad

xpad_SOURCES = \
	fio.c fio.h \
	help.c help.h \
	eggstatusicon.c eggstatusicon.h \
	eggtrayicon.c eggtrayicon.h \
	prefix.c prefix.h \
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
	xpad-tray.c xpad-tray.h \
	$(MARSHAL_GENERATED)

BUILT_SOURCES = $(MARSHAL_GENERATED)
EXTRA_DIST = eggmarshalers.list

MARSHAL_GENERATED = eggmarshalers.c eggmarshalers.h
@XPAD_MARSHAL_RULE@

localedir = $(datadir)/locale

AM_CFLAGS = @GTK_CFLAGS@ @X_CFLAGS@ @DEBUG_CFLAGS@ -DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\"
xpad_LDADD = @X_PRE_LIBS@ @X_LIBS@ @X_EXTRA_LIBS@ @GTK_LIBS@ @INTLLIBS@ @BINRELOC_LIBS@
