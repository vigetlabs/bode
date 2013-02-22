CFLAGS=-Wall -Ivendor/buffer/src -Isrc
LDFLAGS=-Lvendor/buffer/build -lbuffer

SOURCES=$(wildcard *.c src/**/*.c src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

TARGET=bode

all: $(TARGET)

$(TARGET): build $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

build:
	@mkdir -p bin

dev: CFLAGS+=-g -Wextra -DNDEBUG
dev: all

clean:
	rm -rf bode *.dSYM *.o src/*.o
