SUBDIRS = doc src images po

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = xpad.desktop.in
dist_desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

pkgdata_DATA = xpad.ui xpad-tray.ui

EXTRA_DIST = scripts/intltool-extract.in scripts/intltool-merge.in scripts/intltool-update.in
