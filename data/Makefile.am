desktopdir = $(datadir)/applications
desktop_in_files = xnote.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

# AppStream wants the installed file named after the component id
# (tech.norvi.xnote.metainfo.xml); the template keeps its historical name.
appdatadir = $(datadir)/metainfo
appdata_in_files = xnote.appdata.xml.in
appdata_DATA = tech.norvi.xnote.metainfo.xml

CLEANFILES = \
	$(appdata_DATA) \
	$(desktop_DATA)

EXTRA_DIST = \
	$(appdata_in_files) \
	$(desktop_in_files)

$(desktop_DATA): $(desktop_in_files)
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

# Explicit rule, not a suffix rule: the output name is the component id, the
# template keeps its historical name, and no same-stem .in is needed.
tech.norvi.xnote.metainfo.xml: xnote.appdata.xml.in
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@
