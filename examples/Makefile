export DEBUG ?= 0
CC = gcc
CFLAGS += -std=c11 -g -Wall -D_XOPEN_SOURCE=600 -DDEBUG=$(DEBUG)
CFLAGS += -I../src -I../src/asn1

LDFLAGS += -L../src
LDLIBS = -lacars
BIN = decpdlc
DEPS = decpdlc.o

.PHONY: all clean

all: $(BIN)

$(BIN): $(DEPS)

decode_arinc: decode_arinc.o

clean:
	rm -f *.o $(BIN)

decpdlc.o: ../src/libacars.h ../src/vstring.h ../src/util.h ../src/cpdlc.h

decode_arinc.o: ../src/libacars.h
