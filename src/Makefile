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

CFLAGS += -Iasn1 $(GLIBCFLAGS)
SUBDIRS = asn1
CLEANDIRS = $(SUBDIRS:%=clean-%)
LIB = libacars.so
OBJ =	asn1-format-common.o \
	asn1-format-cpdlc.o \
	asn1-util.o \
	libacars.o \
	util.o

ASN1 = asn1/asn1.a

DEPS = $(OBJ) $(ASN1)

.PHONY: all clean install $(SUBDIRS) $(CLEANDIRS)

all: $(LIB)

$(LIB): $(DEPS)
	$(CC) -shared -Wl,-soname,libacars.so.1 -o $@ $(DEPS)

$(ASN1): asn1 ;

asn1-format-common.o: asn1-util.h util.h

asn1-format-cpdlc.o: asn1-util.h asn1-format-common.h util.h

asn1-util.o: util.h asn1-util.h

libacars.o: asn1-util.h asn1-format-cpdlc.h libacars.h

util.o:

$(SUBDIRS):
	$(MAKE) -C $@

$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

clean: $(CLEANDIRS)
	rm -f *.o $(LIB)

install: $(LIB)
	install -d -o $(INSTALL_USER) -g $(INSTALL_GROUP) $(LIBDIR)
	install -o $(INSTALL_USER) -g $(INSTALL_GROUP) -m 755 $(LIB) $(LIBDIR)

