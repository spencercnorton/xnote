xpaddir = $(libdir)/xpad
helpdir = $(xpaddir)/help

dist_man_MANS = xpad.1

install-data-local:
	@echo "Create help directory"
	$(MKDIR_P) $(helpdir)
	@echo "Copying help file"
	cp xpad-user-help.txt $(helpdir)

uninstall-local:
	@echo "Delete help directory"
	rm -rf $(xpaddir)
