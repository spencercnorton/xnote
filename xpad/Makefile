CC=gcc
OBJECTS=main.o \
	fio.o \
	color.o \
	font.o \
	pad.o 
CFLAGS=-O3 -Wall `pkg-config --cflags gtk+-2.0`
LDFLAGS=`pkg-config --libs gtk+-2.0`

xpad: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o xpad

install:
	cp xpad /usr/bin/xpad

remove:
	rm /usr/bin/xpad

uninstall:
	rm /usr/bin/xpad

clean:
	-rm -rf xpad core *.o tags







