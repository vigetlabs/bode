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

// Allocation failure is not recoverable here: at startup there is nothing to
// serve without config/MIME data, and per-request work runs in a forked
// worker, so exiting drops just that one connection. Works for any allocator
// (malloc family, strdup, sds) and for asprintf via check_mem(r != -1).
#define check_mem(A)                                                          \
    do {                                                                      \
        if (!(A)) {                                                           \
            fprintf(stderr, "Out of memory at %s:%d\n", __FILE__, __LINE__);  \
            exit(EXIT_FAILURE);                                               \
        }                                                                     \
    } while (0)

#endif