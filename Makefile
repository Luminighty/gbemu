IDIR=./include
TARGET=bin/main

CC=gcc
CFLAGS=-I$(IDIR)
LDFLAGS=-Llibs -lraylib -lpthread -lm -ldl
LDFLAGS_WIN=-lgdi32 -lwinmm
LDFLAGS_UNIX=-lGL -ldl -lX11

ifeq ($(OS),Windows_NT)
	LDFLAGS += $(LDFLAGS_WIN)
else
	LDFLAGS += $(LDFLAGS_UNIX)
endif


DEPS=$(wildcard $(IDIR)/*.h)
SRCS := $(shell find src -name '*.c')
OBJS=$(patsubst src/%.c, build/%.o, $(SRCS))

.PHONY: debug release clean

debug: CFLAGS += -g -O0 -Wall -Wextra -DDEV_MODE
debug: $(TARGET)

release: CFLAGS += -O2 -DNDEBUG
release: $(TARGET)

build/%.o: src/%.c $(DEPS)
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS)


$(TARGET): $(OBJS)
	mkdir -p bin
	cp -r assets bin/
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)


clean:
	rm -rf build bin
