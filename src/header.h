#ifndef HEADER_H
#define HEADER_H

typedef struct Header {
    char *key;
    char *value;
} Header;

Header *header_create_int(const char *key, int value);
void header_free(Header *header);

#endif