CFLAGS=-Wall -Ivendor/sds -Isrc

SOURCES=$(wildcard *.c src/**/*.c src/*.c) vendor/sds/sds.c
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

# Our own sources only; vendored sds is excluded from lint/analysis (3rd party).
LINT_SOURCES=bode.c $(wildcard src/*.c)

TARGET=bode

.PHONY: all dev build clean lint analyze

all: $(TARGET)

$(TARGET): build $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

build:
	@mkdir -p bin

dev: CFLAGS+=-g -Wextra -DNDEBUG
dev: all

clean:
	rm -rf bode *.dSYM *.o src/*.o vendor/sds/*.o

# Clang's built-in static analyzer (no extra install; uses $(CC)).
analyze:
	$(CC) --analyze -Xanalyzer -analyzer-output=text $(CFLAGS) $(LINT_SOURCES)

# cppcheck static analysis. Install with: brew install cppcheck
lint:
	@command -v cppcheck >/dev/null 2>&1 || \
		{ echo "cppcheck not installed; run: brew install cppcheck"; exit 1; }
	cppcheck --enable=warning,performance,portability,style --inconclusive \
		--std=c11 --quiet --error-exitcode=1 \
		--suppress=missingIncludeSystem \
		--suppress='*:vendor/sds/*' \
		-Isrc -Ivendor/sds $(LINT_SOURCES)
