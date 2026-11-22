CC       ?= cc
CFLAGS   ?= -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Iinclude
LDFLAGS  ?=
LDLIBS   ?= -pthread

TARGET  = syswatch
SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all build run test clean

all: build

build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./tests/test_syswatch.sh

clean:
	rm -f $(TARGET) $(OBJECTS)
