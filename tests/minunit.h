/*
    minunit — a minimal C unit-test framework (single header, return-string-on-
    failure style). A test is a function returning char*: NULL on success, or a
    message on failure. Macros track a running count and short-circuit a suite
    on the first failure.

    Usage:

        #include "minunit.h"

        char *test_something()
        {
            mu_assert(1 + 1 == 2, "math is broken");
            return NULL;
        }

        char *all_tests()
        {
            mu_suite_start();
            mu_run_test(test_something);
            return NULL;
        }

        RUN_TESTS(all_tests);
*/
#ifndef MINUNIT_H
#define MINUNIT_H

#include <stdio.h>

/* Declares the `message` variable the run/assert macros thread through. */
#define mu_suite_start() char *message = NULL

#define mu_assert(test, msg)                                                  \
    do {                                                                      \
        if (!(test)) {                                                        \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, (msg)); \
            return (msg);                                                     \
        }                                                                     \
    } while (0)

#define mu_run_test(test)                  \
    do {                                   \
        fprintf(stderr, "- %s\n", #test);  \
        message = test();                  \
        tests_run++;                       \
        if (message) { return message; }   \
    } while (0)

#define RUN_TESTS(suite)                                       \
    int tests_run = 0;                                         \
    int main(void)                                             \
    {                                                          \
        printf("RUNNING: %s\n", __FILE__);                     \
        char *result = suite();                                \
        if (result != NULL) {                                  \
            printf("FAILED: %s\n", result);                    \
        } else {                                               \
            printf("ALL TESTS PASSED (%d run)\n", tests_run);  \
        }                                                      \
        return result != NULL;                                 \
    }

extern int tests_run;

#endif
