# Set env specific things here
CC=gcc
LD=gcc
CFLAGS= -Wall -std=c99 -g -O3 -pedantic -static
LDFLAGS= -lm

# Put program name here
OUT=mimatool

HEADERS=$(wildcard *.h)
CFILES=$(wildcard *.c)

OBJS=$(CFILES:.c=.o)

.PHONY: all clean

all: $(OUT)

#Since recent gcc versions, ldflags are ordered, so they are repeated twice to avoid ld complain.
$(OUT): $(OBJS)
	$(LD) $^ $(LDFLAGS) -o $@ $(LDFLAGS)

makefile.dep: $(CFILES) $(HEADERS)
	$(CC) -MM $(CFILES) > $@

clean:
	$(RM) $(OBJS)
	$(RM) $(OUT)
