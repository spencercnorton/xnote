SUBDIRS = autopackage doc src images po

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = xpad.desktop.in
dist_desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

EXTRA_DIST = $(desktop_in_files) scripts/intltool-extract.in scripts/intltool-merge.in scripts/intltool-update.in

DISTCLEANFILES = xpad.desktop intltool-extract intltool-merge intltool-update po/xpad.pot
