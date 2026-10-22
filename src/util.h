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

// Fatal assertion: exits rather than returning, so it behaves the same in a
// forked child or a helper function as it does in main(). M is printed as
// data, never used as a format string.
#define check(A, M)                     \
    do {                                \
        if (!(A)) {                     \
            fprintf(stderr, "%s", (M)); \
            exit(EXIT_FAILURE);         \
        }                               \
    } while (0)

#endif