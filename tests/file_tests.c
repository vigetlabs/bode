#include "minunit.h"

#include <string.h>
#include <stdlib.h>

/* These have external linkage in src/file.c but are intentionally not part of
   the public file.h API. Declare them here so the tests can link against them
   without widening the module's interface. */
char *file_get_extension(char *filename);
char *path_append(char *path, const char *filename);
int ends_with(const char *path, char end);

/* ---- ends_with -------------------------------------------------------- */

char *
test_ends_with_detects_trailing_char()
{
    mu_assert(ends_with("root/", '/') == 1,
              "ends_with should detect a trailing '/'");
    return NULL;
}

char *
test_ends_with_false_when_not_trailing()
{
    mu_assert(ends_with("root", '/') == 0,
              "ends_with should be false when the char isn't last");
    return NULL;
}

char *
test_ends_with_empty_string_is_safe()
{
    mu_assert(ends_with("", '/') == 0,
              "ends_with on an empty string should be false, not read past end");
    return NULL;
}

/* ---- file_get_extension ---------------------------------------------- */

char *
test_get_extension_returns_extension()
{
    char name[] = "index.html";
    mu_assert(strcmp(file_get_extension(name), "html") == 0,
              "extension of index.html should be 'html'");
    return NULL;
}

char *
test_get_extension_no_dot_returns_empty()
{
    char name[] = "noext";
    mu_assert(strcmp(file_get_extension(name), "") == 0,
              "a name with no dot should have an empty extension");
    return NULL;
}

char *
test_get_extension_does_not_mutate_input()
{
    char name[] = "a.b.c";
    file_get_extension(name);
    mu_assert(strcmp(name, "a.b.c") == 0,
              "file_get_extension must not modify the caller's string");
    return NULL;
}

/* ---- path_append ------------------------------------------------------ */

char *
test_path_append_inserts_separator()
{
    char *result = path_append("root", "index.html");
    mu_assert(strcmp(result, "root/index.html") == 0,
              "path_append should join with a single '/'");
    free(result);
    return NULL;
}

char *
test_path_append_no_double_slash()
{
    char *result = path_append("root/", "index.html");
    mu_assert(strcmp(result, "root/index.html") == 0,
              "a root ending in '/' should not produce a doubled slash");
    free(result);
    return NULL;
}

char *
all_tests()
{
    mu_suite_start();
    mu_run_test(test_ends_with_detects_trailing_char);
    mu_run_test(test_ends_with_false_when_not_trailing);
    mu_run_test(test_ends_with_empty_string_is_safe);
    mu_run_test(test_get_extension_returns_extension);
    mu_run_test(test_get_extension_no_dot_returns_empty);
    mu_run_test(test_get_extension_does_not_mutate_input);
    mu_run_test(test_path_append_inserts_separator);
    mu_run_test(test_path_append_no_double_slash);
    return NULL;
}

RUN_TESTS(all_tests);
