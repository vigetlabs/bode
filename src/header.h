#ifndef HEADER_H
#define HEADER_H

typedef struct Header {
    char *key;
    char *value;
} Header;

Header *header_create(const char *key, char *value);
void header_free(Header *header);

#endif