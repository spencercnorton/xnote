SUBDIRS = intl m4  doc src images po

desktopdir = $(shell $(top_srcdir)/scripts/finddesktopdir $(datadir)/applications)

desktop_DATA = xpad.desktop

EXTRA_DIST = scripts/config.rpath  xpad.desktop scripts/finddesktopdir scripts/install-sh\
 scripts/mkinstalldirs

ACLOCAL_AMFLAGS = -I m4
