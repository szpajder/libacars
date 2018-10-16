export DEBUG ?= 0
PREFIX = /usr/local
LIBDIR = $(PREFIX)/lib
INSTALL_USER = root
INSTALL_GROUP = root
CC = gcc
CFLAGS += -std=c11 -g -Wall -fPIC -D_XOPEN_SOURCE=600 -DDEBUG=$(DEBUG)

#LIBACARS_VERSION:=\"$(shell git describe --always --tags --dirty)\"
#ifneq ($(LIBACARS_VERSION), \"\")
#  CFLAGS+=-DLIBACARS_VERSION=$(LIBACARS_VERSION)
#endif

CFLAGS += -Iasn1 -I. $(GLIBCFLAGS)
LDLIBS = -lm
SUBDIRS = asn1
CLEANDIRS = $(SUBDIRS:%=clean-%)
LIB = libacars.so
OBJ =	adsc.o \
	arinc.o \
	asn1-format-common.o \
	asn1-format-cpdlc.o \
	asn1-util.o \
	bitstream.o \
	cpdlc.o \
	libacars.o \
	list.o \
	util.o \
	vstring.o
ASN1 = asn1/asn1.a

DEPS = $(OBJ) $(ASN1)

.PHONY: all clean install $(SUBDIRS) $(CLEANDIRS)

all: $(LIB)

$(LIB): $(DEPS)
	$(CC) $(LDLIBS) -shared -Wl,-soname,libacars.so.1 -o $@ $(DEPS)

$(ASN1): asn1 ;

adsc.o: adsc.h arinc.h bitstream.h libacars.h list.h macros.h vstring.h util.h

arinc.o: arinc.h cpdlc.h macros.h vstring.h util.h

asn1-format-common.o: asn1-util.h macros.h util.h vstring.h

asn1-format-cpdlc.o: asn1-util.h asn1-format-common.h macros.h util.h vstring.h

asn1-util.o: asn1-util.h macros.h vstring.h

bitstream.o: bitstream.h util.h

cpdlc.o: asn1-util.h asn1-format-cpdlc.h cpdlc.h libacars.h util.h macros.h vstring.h

libacars.o: macros.h libacars.h vstring.h util.h

list.o: list.h macros.h util.h

util.o: macros.h util.h

vstring.o: vstring.h util.h

$(SUBDIRS):
	$(MAKE) -C $@

$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

clean: $(CLEANDIRS)
	rm -f *.o $(LIB)

install: $(LIB)
	install -d -o $(INSTALL_USER) -g $(INSTALL_GROUP) $(LIBDIR)
	install -o $(INSTALL_USER) -g $(INSTALL_GROUP) -m 755 $(LIB) $(LIBDIR)

