#include "minunit.h"

#include <string.h>

#include <config.h>
#include <mime.h>

/* Not in the public header; exercised directly to test growth. */
MimeTypes *mime_types_create(int initial_allocation);
void mime_types_push(MimeTypes *mime_types, const char *type, const char *ext);

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
test_push_grows_past_initial_allocation()
{
    /* Start with room for one entry so every push after the first goes
       through the realloc path; all entries must survive the moves. */
    char ext[16];
    int  i = 0;

    MimeTypes *types = mime_types_create(1);

    for (i = 0; i < 100; i++) {
        snprintf(ext, sizeof(ext), "ext%d", i);
        mime_types_push(types, "text/plain", ext);
    }

    mu_assert(types->count == 100, "all pushed types should be counted");
    mu_assert(types->allocated >= types->count,
              "allocation should keep up with the count");

    for (i = 0; i < 100; i++) {
        snprintf(ext, sizeof(ext), "ext%d", i);
        mu_assert(strcmp(mime_types_find_content_type(types, ext),
                         "text/plain") == 0,
                  "every pushed extension should still be found after growing");
    }

    mime_types_free(types);
    return NULL;
}

char *
all_tests()
{
    mu_suite_start();
    mu_run_test(test_load_missing_file_returns_null);
    mu_run_test(test_load_reads_configured_types);
    mu_run_test(test_push_grows_past_initial_allocation);
    return NULL;
}

RUN_TESTS(all_tests);
