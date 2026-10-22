#include <stdlib.h>
#include <string.h>

#include <util.h>
#include <config.h>

int write_value(char **target, const char *value, int buf_length);

Config *
config_create()
{
    Config *config = malloc(sizeof(Config));
    check_mem(config);

    config->document_root = NULL;
    config->config_dir = NULL;

    write_value(&config->document_root, CONFIG_DEFAULT_DOCUMENT_ROOT, CONFIG_VALUE_MAX_LENGTH);
    write_value(&config->config_dir, CONFIG_DEFAULT_CONFIG_DIR, CONFIG_VALUE_MAX_LENGTH);

    config->port = CONFIG_DEFAULT_PORT;

    return config;
}

void
config_set(Config *config, int attr, void *value)
{
    switch (attr) {
        case CONFIG_DOCUMENT_ROOT:
            write_value(&config->document_root, (char *)value, CONFIG_VALUE_MAX_LENGTH);
            break;
        case CONFIG_CONFIG_DIR:
            write_value(&config->config_dir, (char *)value, CONFIG_VALUE_MAX_LENGTH);
            break;
        case CONFIG_PORT:
            config->port = *(int *)value;
            break;
        default:
            // TODO: error
            break;
    }
}

void
config_free(Config *config)
{
    if (config->document_root) { free(config->document_root); }
    if (config->config_dir)    { free(config->config_dir); }

    free(config);
}

int
write_value(char **target, const char *value, int buf_length)
{
    // TODO: Conditionally allocate?
    char *resized = realloc(*target, buf_length * sizeof(char));
    check_mem(resized);
    *target = resized;

    // strncpy doesn't terminate when value fills the buffer.
    strncpy(*target, value, buf_length - 1);
    (*target)[buf_length - 1] = '\0';

    return 0;
}