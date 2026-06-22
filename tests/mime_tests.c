#include "minunit.h"

#include <string.h>

#include <config.h>
#include <mime.h>

char *
test_load_missing_file_returns_null()
{
    /* A missing or unreadable mime.types must fail gracefully (NULL), not
       dereference a NULL FILE* and segfault on startup. */
    Config *config = config_create();
    config_set(config, CONFIG_CONFIG_DIR, "tests/no-such-dir");

    MimeTypes *types = mime_types_load(config);

    mu_assert(types == NULL,
              "loading a missing mime.types should return NULL, not crash");

    config_free(config);
    return NULL;
}

char *
test_load_reads_configured_types()
{
    /* The default conf-dir is "config"; runtests.sh runs from the repo root,
       so config/mime.types is present. */
    Config *config = config_create();

    MimeTypes *types = mime_types_load(config);

    mu_assert(types != NULL, "loading config/mime.types should succeed");
    mu_assert(strcmp(mime_types_find_content_type(types, "html"),
                     "text/html") == 0,
              "'html' should map to text/html from config/mime.types");

    mime_types_free(types);
    config_free(config);
    return NULL;
}

char *
all_tests()
{
    mu_suite_start();
    mu_run_test(test_load_missing_file_returns_null);
    mu_run_test(test_load_reads_configured_types);
    return NULL;
}

RUN_TESTS(all_tests);
