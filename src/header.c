#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <header.h>

Header *header_create_int(const char *key, int value)
{
    Header *header = malloc(sizeof(Header));

    header->key = calloc(strlen(key) + 1, sizeof(char));
    strncpy(header->key, key, strlen(key));

    asprintf(&header->value, "%d", value);

    return header;
}

void
header_free(Header *header)
{
    if (header->key)   { free(header->key); }
    if (header->value) { free(header->value); }

    free(header);
}