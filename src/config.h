#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_VALUE_MAX_LENGTH 1024

#define CONFIG_DEFAULT_DOCUMENT_ROOT "root"
#define CONFIG_DEFAULT_CONFIG_DIR    "config"
#define CONFIG_DEFAULT_PORT          8080

#define CONFIG_DOCUMENT_ROOT 1
#define CONFIG_CONFIG_DIR    2
#define CONFIG_PORT          3

typedef struct Config {
    char *document_root;
    char *config_dir;
    int port;
} Config;

Config *config_create();
void config_set(Config *config, int attr, void *value);
void config_free(Config *config);

#endif