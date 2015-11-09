SUBDIRS = autopackage doc src images po

autostartdir          = /etc/xdg/autostart
autostart_in_files    = xpad.desktop.in
autostart_DATA        = $(autostart_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = xpad.desktop.in
dist_desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

# Distribute pot file
dist-hook:
	$(MAKE) -C "$(srcdir)/po" "$(GETTEXT_PACKAGE).pot"
	cp "$(srcdir)/po/$(GETTEXT_PACKAGE).pot" "$(distdir)/po/"

EXTRA_DIST = $(desktop_in_files) intltool-extract.in intltool-merge.in intltool-update.in $(autostart_in_files)

CLEANFILES = $(autostart_DATA)

DISTCLEANFILES = xpad.desktop intltool-extract intltool-merge intltool-update po/xpad.pot \
Makefile.in aclocal.m4 autom4te.cache/* compile config.h.in configure depcomp install-sh \
intltool-extract.in intltool-merge.in intltool-update.in missing autopackage/Makefile.in \
doc/Makefile.in images/Makefile.in po/Makefile.in.in src/Makefile.in *.in~ *.~1~ $(autostart_in_files)
