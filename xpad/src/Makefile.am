bin_PROGRAMS = xpad

xpad_SOURCES = main.c main.h fio.c fio.h pref.c pref.h pad.c pad.h \
 help.c help.h toolbar.c toolbar.h xpad-tray.c xpad-tray.h eggtrayicon.c \
 eggtrayicon.h defines.h settings.c settings.h \
 properties.c properties.h sm.c sm.h xpad-dashboard-frontend.c dashboard-frontend.c

localedir = $(datadir)/locale

AM_CFLAGS = @GTK_CFLAGS@ @X_CFLAGS@ -ansi -pedantic -Wall -Werror\
 -DPKGDATADIR=\"$(pkgdatadir)\" -DLOCALEDIR=\"$(localedir)\"\
 -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED\
 -DGDK_PIXMAP_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED

xpad_LDADD = @X_PRE_LIBS@ @X_LIBS@ @X_EXTRA_LIBS@ @GTK_LIBS@ @LIBINTL@
