PREFIX ?= /usr/local
INSTALL ?= install
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man
CC ?= gcc -Wall -pedantic -Wno-long-long -Wno-deprecated -ansi

all: objdir austerus-panel austerus-send austerus-verge austerus-core

objdir:
	mkdir -p build

austerus-panel: nbgetline.o popen2.o serial.o austerus-panel.o
	$(CC) -o austerus-panel build/popen2.o build/nbgetline.o build/serial.o \
		build/austerus-panel.o -lncurses -lform

austerus-send: nbgetline.o popen2.o stats.o serial.o austerus-send.o
	$(CC) -o austerus-send build/nbgetline.o build/popen2.o build/stats.o \
		build/serial.o build/austerus-send.o -lm

austerus-verge: stats.o austerus-verge.o
	$(CC) -o austerus-verge build/stats.o build/austerus-verge.o -lm

austerus-core: serial.o austerus-core.o
	$(CC) -o austerus-core build/serial.o build/austerus-core.o

austerus-panel.o: src/austerus-panel.c
	$(CC) -c -o build/austerus-panel.o src/austerus-panel.c

austerus-send.o: src/austerus-send.c
	$(CC) -c -o build/austerus-send.o src/austerus-send.c

austerus-verge.o: src/austerus-verge.c
	$(CC) -c -o build/austerus-verge.o src/austerus-verge.c

austerus-core.o: src/austerus-core.c
	$(CC) -c -o build/austerus-core.o src/austerus-core.c

serial.o: src/serial.c
	$(CC) -c -o build/serial.o src/serial.c

stats.o: src/stats.c
	$(CC) -c -o build/stats.o src/stats.c

popen2.o: src/popen2.c
	$(CC) -c -o build/popen2.o src/popen2.c

nbgetline.o: src/nbgetline.c
	$(CC) -c -o build/nbgetline.o src/nbgetline.c

install:
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -m 0755 austerus-core $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 austerus-send $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 austerus-panel $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0644 docs/austerus-core.1 $(DESTDIR)$(MANDIR)/man1

clean:
	rm austerus-panel austerus-send austerus-core austerus-verge build/*

