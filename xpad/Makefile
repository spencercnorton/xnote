CC=gcc
OBJECTS=main.o \
	fio.o \
	color.o \
	font.o \
	pad.o 
CFLAGS=-O3 -Wall `pkg-config --cflags gtk+-2.0`
LDFLAGS=`pkg-config --libs gtk+-2.0`

#debian-friendly var
DESTDIR=

#where to put binary
BIN=$(DESTDIR)/usr/bin

MANDIR=$(DESTDIR)/usr/share/man/man1

xpad-target: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o xpad

# must be root for this
install:	xpad-target
		install -d $(BIN) $(MANDIR)
		install ./xpad $(BIN)
		install ./xpad.1.gz $(MANDIR)

uninstall:
	rm $(BIN)/xpad
	rm $(MANDIR)/xpad.1.gz

clean:
	-rm -rf xpad core *.o tags







