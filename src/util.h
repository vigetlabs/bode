#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 1024

#ifndef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG: " M "\n", ##__VA_ARGS__)
#endif

#define check(A, M) if (!(A)) { printf(M); return 1; }

// Allocation failure is not recoverable mid-operation: the state being built
// is incomplete. At startup there is nothing to serve without config/MIME
// data, so we just exit. A forked worker installs a handler first so its
// client still gets a 500 before the worker exits. Works for any allocator
// (malloc family, strdup, sds) and for asprintf via check_mem(r != -1).
#define check_mem(A)                                                          \
    do {                                                                      \
        if (!(A)) { out_of_memory(__FILE__, __LINE__); }                      \
    } while (0)

void set_out_of_memory_handler(void (*handler)(void));
void out_of_memory(const char *file, int line) __attribute__((noreturn));

#endif