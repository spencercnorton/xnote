bin_PROGRAMS = xpad

xpad_SOURCES = main.c main.h fio.c fio.h pref.c pref.h pad.c pad.h \
 help.c help.h toolbar.c toolbar.h tray.c tray.h eggtrayicon.c \
 eggtrayicon.h defines.h gettext.h settings.c settings.h \
 properties.c properties.h

AM_CFLAGS = -ansi -pedantic -Wall `pkg-config --cflags gtk+-2.0`\
 -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED\
 -DGDK_PIXMAP_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED

xpad_LDFLAGS = `pkg-config --libs gtk+-2.0`

localedir = $(datadir)/locale

INCLUDES = -I. -I$(srcdir) -I.. -I../intl -I$(top_srcdir)/intl
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
