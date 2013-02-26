#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

#define BUF_SIZE 1024

#ifndef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG: " M "\n", ##__VA_ARGS__)
#endif

#define check(A, M) if (!(A)) { printf(M); return 1; }

#endif