#include "minunit.h"

#include <string.h>

#include <header.h>

char *
test_header_create_sets_key_and_value()
{
    Header *header = header_create("Content-Type", "text/plain");

    mu_assert(strcmp(header->key, "Content-Type") == 0,
              "key should match the supplied string");
    mu_assert(strcmp(header->value, "text/plain") == 0,
              "value should match the supplied string");

    header_free(header);
    return NULL;
}

char *
test_header_copies_its_inputs()
{
    /* header_create must own copies, not alias the caller's buffers. Mutating
       the originals after construction should not change the stored header. */
    char key[]   = "X-Test";
    char value[] = "one";

    Header *header = header_create(key, value);

    key[0]   = 'Z';
    value[0] = 'Z';

    mu_assert(strcmp(header->key, "X-Test") == 0,
              "key should be an independent copy");
    mu_assert(strcmp(header->value, "one") == 0,
              "value should be an independent copy");

    header_free(header);
    return NULL;
}

char *
all_tests()
{
    mu_suite_start();
    mu_run_test(test_header_create_sets_key_and_value);
    mu_run_test(test_header_copies_its_inputs);
    return NULL;
}

RUN_TESTS(all_tests);
