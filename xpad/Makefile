CC=gcc
OBJECTS=main.o \
	fio.o \
	pref.o \
	pad.o 
CFLAGS=-Wall `pkg-config --cflags gtk+-2.0`
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
		install ./xpad.1.gz $(MANDIR)
		install ./xpad.xpm $(ICONDIR)

uninstall:
	rm $(BIN)/xpad
	rm $(MANDIR)/xpad.1.gz
	rm $(ICONDIR)/xpad.xpm

clean:
	-rm -rf xpad core *.o tags







