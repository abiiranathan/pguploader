CC=clang
TARGET=bin/pguploader

CFLAGS_OVERRIDES=-DMAX_GLOBAL_FLAGS=20 -DMAX_SUBCOMMANDS=3
SEARCHPATHS=-I. -I./flags -I./csvparser -I./
CFLAGS=-Wall -Werror -pedantic $(CFLAGS_OVERRIDES) -O2 -std=c11 $(SEARCHPATHS) -L./libs

LINKER_FLAGS=-Wl,-rpath,'$$ORIGIN/libs'
LIBS=-lpq -lreadline
LINKER_DEPS=linker_deps.txt

SRCS=pguploader.c flag/flag.c csvparser/csvparser.c db.c inventory.c 
HEADERS=flag/*.h csvparser/*.h db.h inventory.h

all: setup build copylibs

build: $(HEADERS) $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LINKER_FLAGS) $(LIBS) 

strip: build
	strip $(TARGET)

setup:
	mkdir -p bin

copylibs: build
	./copylibs.sh $(TARGET)

# Valgrind will fail with -g/ggdb due to too many debug symbols.
memcheck: build
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(TARGET) --help

clean:
	rm -rf bin