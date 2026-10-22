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

/*
    Returns 0 on success, or -1 if the attribute is unknown or the value is
    too long to store (the existing value is then left unchanged).
*/
int
config_set(Config *config, int attr, void *value)
{
    switch (attr) {
        case CONFIG_DOCUMENT_ROOT:
            return write_value(&config->document_root, (char *)value, CONFIG_VALUE_MAX_LENGTH);
        case CONFIG_CONFIG_DIR:
            return write_value(&config->config_dir, (char *)value, CONFIG_VALUE_MAX_LENGTH);
        case CONFIG_PORT:
            config->port = *(int *)value;
            return 0;
        default:
            return -1;
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
    size_t length = strlen(value);

    // Refuse rather than truncate: a silently shortened path would point
    // somewhere the user never asked for.
    if (length >= (size_t) buf_length) {
        return -1;
    }

    // TODO: Conditionally allocate?
    char *resized = realloc(*target, buf_length * sizeof(char));
    check_mem(resized);
    *target = resized;

    memcpy(*target, value, length + 1);

    return 0;
}