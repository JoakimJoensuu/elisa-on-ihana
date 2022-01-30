SHELL = /bin/sh

CC=gcc
CFLAGS=-Wall
CFLAGS=-Wextra
CFLAGS+=-O0
CFLAGS+=-g
CFLAGS+=-fdata-sections
CFLAGS+=-ffunction-sections
CFLAGS+=-Wl,--gc-sections

OBJECTS := asd cli counter

.PHONY: all clean

all: clean build

clean:
	$(RM) $(OBJECTS)

build: $(OBJECTS)

$(OBJECTS): %: %.c
	$(CC) $(CFLAGS) $^ emoji_data_structure.c -o $@

run:
	sudo ./asd
valgrind:
	sudo valgrind -s --leak-check=full --track-origins=yes --trace-children=yes ./asd
