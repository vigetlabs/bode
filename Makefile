CFLAGS=-Wall -Ivendor/sds -Isrc

SOURCES=$(wildcard *.c src/**/*.c src/*.c) vendor/sds/sds.c
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

TARGET=bode

all: $(TARGET)

$(TARGET): build $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

build:
	@mkdir -p bin

dev: CFLAGS+=-g -Wextra -DNDEBUG
dev: all

clean:
	rm -rf bode *.dSYM *.o src/*.o vendor/sds/*.o
