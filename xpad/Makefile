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

xpad-target: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o xpad

# must be root for this
install:	xpad-target
		install -d $(BIN)
		install ./xpad $(BIN)

remove:
	rm /usr/bin/xpad

uninstall:
	rm /usr/bin/xpad

clean:
	-rm -rf xpad core *.o tags







