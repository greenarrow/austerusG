PREFIX ?= /usr/local
INSTALL ?= install
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man

CFLAGS += -Wall -pedantic -Wno-long-long -Wno-deprecated -ansi

LDLIBS += -lncurses -lform -lm

SETUID ?= 0

ifeq ($(SETUID),1)
COREFLAGS = -D SETUID
COREMODE = 6755
else
COREMODE = 0755
endif

all: austerus-panel austerus-send austerus-verge austerus-core \
	austerus-shift

austerus-panel: austerus-panel.o nbgetline.o popen2.o serial.o

austerus-send: nbgetline.o popen2.o stats.o serial.o austerus-send.o

austerus-verge: austerus-verge.o common.o point.o gvm.o stats.o

austerus-shift: stats.o austerus-shift.o

austerus-core: serial.o austerus-core.o

install:
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -m $(COREMODE) austerus-core $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 austerus-send $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 austerus-panel $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 austerus-verge $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 austerus-shift $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0644 docs/austerus-core.1 $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -m 0644 docs/austerus-verge.1 $(DESTDIR)$(MANDIR)/man1

clean:
	rm -f *.o austerus-panel austerus-send austerus-core austerus-verge \
		austerus-shift
