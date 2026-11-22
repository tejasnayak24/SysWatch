CC       ?= cc
CFLAGS   ?= -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Iinclude
LDFLAGS  ?=
NCURSES_CFLAGS := $(shell pkg-config --cflags ncursesw 2>/dev/null)
NCURSES_LIBS   := $(shell pkg-config --libs ncursesw 2>/dev/null || echo -lncursesw -ltinfo)
LDLIBS   ?= -pthread $(NCURSES_LIBS)

CPPFLAGS += $(NCURSES_CFLAGS)

TARGET  = syswatch
SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all build run dashboard test clean

all: build

build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

dashboard: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./tests/test_syswatch.sh

clean:
	rm -f $(TARGET) $(OBJECTS)
