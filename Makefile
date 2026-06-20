CFLAGS=-Wall -Ivendor/sds -Isrc

SOURCES=$(wildcard *.c src/**/*.c src/*.c) vendor/sds/sds.c
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

# Our own sources only; vendored sds is excluded from lint/analysis (3rd party).
LINT_SOURCES=bode.c $(wildcard src/*.c)

# Library sources for tests: everything but bode.c (which owns main()), plus
# vendored sds. Test binaries supply their own main() via RUN_TESTS.
LIB_SOURCES=$(wildcard src/*.c) vendor/sds/sds.c

TEST_SRC=$(wildcard tests/*_tests.c)
TESTS=$(patsubst %.c,%,$(TEST_SRC))

# Sanitizers live on the test build only.
TEST_CFLAGS=-Wall -g -Isrc -Ivendor/sds -Itests -fsanitize=address,undefined

TARGET=bode

.PHONY: all dev build clean lint analyze test

all: $(TARGET)

$(TARGET): build $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

build:
	@mkdir -p bin

dev: CFLAGS+=-g -Wextra -DNDEBUG
dev: all

clean:
	rm -rf bode *.dSYM *.o src/*.o vendor/sds/*.o $(TESTS) tests/*.dSYM tests/tests.log

# Build and run the unit tests under AddressSanitizer + UBSan. Each test is
# compiled fresh with the lib sources (no main()) so the sanitizers cover
# everything. Honors $(VALGRIND); see tests/runtests.sh.
tests/%_tests: tests/%_tests.c $(LIB_SOURCES)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(LIB_SOURCES)

test: $(TESTS)
	@sh ./tests/runtests.sh

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
		-Isrc -Ivendor/sds $(LINT_SOURCES)
