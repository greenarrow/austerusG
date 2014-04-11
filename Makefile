PREFIX ?= /usr/local
INSTALL ?= install
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man

CFLAGS += -Wall -pedantic -Wno-long-long -Wno-deprecated -ansi
CFLAGS += -O2
LDLIBS += -lm

REG_VERGE_TESTS = tests/verge/tests/default-simple \
	tests/verge/tests/deposition-physical-simple \
	tests/verge/tests/deposition-shifted \
	tests/verge/tests/deposition-simple \
	tests/verge/tests/physical-deposition-end-home \
	tests/verge/tests/physical-shifted \
	tests/verge/tests/regression-G0 \
	tests/verge/tests/zmin-ignore \
	tests/verge/tests/zmin-shifted \
	tests/verge/tests/zmin-simple

SETUID ?= 0

ifeq ($(SETUID),1)
COREFLAGS = -D SETUID
COREMODE = 6755
else
COREMODE = 0755
endif

default: all test

all: austerus-panel austerus-send austerus-verge austerus-core \
	austerus-shift

austerus-panel: austerus-panel.o nbgetline.o popen2.o serial.o
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -lncurses -lform -lm -o $@

austerus-send: common.o point.o gvm.o stats.o nbgetline.o popen2.o serial.o

austerus-verge: common.o point.o gvm.o stats.o

austerus-shift: common.o point.o gvm.o stats.o

austerus-core: serial.o austerus-core.o

test:	$(addsuffix .reg.verge,$(REG_VERGE_TESTS))

%.reg.verge:	%
		tests/verge/run.sh $<

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
