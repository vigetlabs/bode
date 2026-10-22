#include "minunit.h"

#include <string.h>

#include <config.h>

char *
test_config_create_sets_defaults()
{
    Config *config = config_create();

    mu_assert(strcmp(config->document_root, CONFIG_DEFAULT_DOCUMENT_ROOT) == 0,
              "document_root should default to CONFIG_DEFAULT_DOCUMENT_ROOT");
    mu_assert(strcmp(config->config_dir, CONFIG_DEFAULT_CONFIG_DIR) == 0,
              "config_dir should default to CONFIG_DEFAULT_CONFIG_DIR");
    mu_assert(config->port == CONFIG_DEFAULT_PORT,
              "port should default to CONFIG_DEFAULT_PORT");

    config_free(config);
    return NULL;
}

char *
test_config_set_overwrites_value()
{
    Config *config = config_create();

    config_set(config, CONFIG_DOCUMENT_ROOT, "public");

    mu_assert(strcmp(config->document_root, "public") == 0,
              "document_root should be replaced, not appended to");

    config_free(config);
    return NULL;
}

char *
test_config_set_truncates_long_value()
{
    /* Values are stored in a CONFIG_VALUE_MAX_LENGTH buffer. An over-long
       value must be truncated and still NUL-terminated, never read past the
       end of the buffer. */
    char value[CONFIG_VALUE_MAX_LENGTH * 2];

    memset(value, 'a', sizeof(value) - 1);
    value[sizeof(value) - 1] = '\0';

    Config *config = config_create();
    config_set(config, CONFIG_DOCUMENT_ROOT, value);

    mu_assert(strlen(config->document_root) == CONFIG_VALUE_MAX_LENGTH - 1,
              "an over-long value should be truncated to fit the buffer");

    config_free(config);
    return NULL;
}

char *
all_tests()
{
    mu_suite_start();
    mu_run_test(test_config_create_sets_defaults);
    mu_run_test(test_config_set_overwrites_value);
    mu_run_test(test_config_set_truncates_long_value);
    return NULL;
}

RUN_TESTS(all_tests);
