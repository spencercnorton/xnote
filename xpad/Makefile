CC=gcc
OBJECTS=main.o \
	fio.o \
	pref.o \
	pad.o \
	help.o
CFLAGS=-pedantic -ansi -Wall -O3 `pkg-config --cflags gtk+-2.0` -fexpensive-optimizations
LDFLAGS=`pkg-config --libs gtk+-2.0`

#debian-friendly var
DESTDIR=

#where to put binary
BIN=$(DESTDIR)/usr/bin
MANDIR=$(DESTDIR)/usr/share/man/man1
ICONDIR=$(DESTDIR)/usr/share/pixmaps/xpad

xpad-target: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o xpad

# must be root for this
install:	xpad-target
		install -d $(BIN) $(MANDIR) $(ICONDIR)
		install ./xpad $(BIN)
		install ./xpad.1 $(MANDIR)
		install ./xpad.xpm $(ICONDIR)

uninstall:
	rm $(BIN)/xpad
	rm $(MANDIR)/xpad.1
	rm $(ICONDIR)/xpad.xpm

clean:
	-rm -rf xpad *.o
