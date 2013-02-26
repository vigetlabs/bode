#include <stdlib.h>
#include <string.h>

#include <util.h>
#include <config.h>

int write_value(char **target, char *value, int buf_length);

Config *
config_create()
{
    Config *config = malloc(sizeof(Config));

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
write_value(char **target, char *value, int buf_length)
{
    // TODO: Conditionally allocate?
    *target = realloc(*target, buf_length * sizeof(char *));

    strncpy(*target, value, buf_length - 1);

    return 0;
}