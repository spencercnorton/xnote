bin_PROGRAMS = xpad

xpad_SOURCES = main.c fio.c pref.c pad.c help.c toolbar.c tray.c\
 eggtrayicon.c

AM_CFLAGS = -ansi -pedantic -Wall `pkg-config --cflags gtk+-2.0`\
 -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED\
 -DGDK_PIXMAP_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED

xpad_LDFLAGS = `pkg-config --libs gtk+-2.0`
