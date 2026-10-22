#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <header.h>
#include <util.h>

Header *
header_create(const char *key, const char *value)
{
    Header *header = malloc(sizeof(Header));
    check_mem(header);

    header->key = calloc(strlen(key) + 1, sizeof(char));
    check_mem(header->key);
    strncpy(header->key, key, strlen(key));

    header->value = calloc(strlen(value) + 1, sizeof(char));
    check_mem(header->value);
    strncpy(header->value, value, strlen(value));

    return header;
}

void
header_free(Header *header)
{
    if (header->key)   { free(header->key); }
    if (header->value) { free(header->value); }

    free(header);
}