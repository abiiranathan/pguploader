CC=clang
TARGET=bin/pguploader
CFLAGS=-Wall -Werror -pedantic -O -std=c11 -I. -I./flags -I./csvparser -I./ -L./libs
LINKER_FLAGS=-Wl,-rpath,'$$ORIGIN/libs'
LIBS=-lpq -lreadline
LINKER_DEPS=linker_deps.txt

SRCS=pguploader.c csvparser/csvparser.c db.c inventory.c 
HEADERS=flag/*.h csvparser/*.h db.h inventory.h

all: setup build copylibs

build: $(HEADERS) $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LINKER_FLAGS) $(LIBS) 

setup:
	mkdir -p bin

copylibs: build
	./copylibs.sh $(TARGET)

# Valgrind will fail with -g/ggdb due to too many debug symbols.
memcheck: build
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(TARGET) --help

clean:
	rm -rf bin