CFLAGS=-Wall -I./vendor/buffer/src
LDFLAGS=-L./vendor/buffer/build -lbuffer

all: bode

clean:
	rm -rf bode *.dSYM *.o
