#ifndef MIME_H
#define MIME_H

#define MIME_TYPES_ALLOC_SIZE  32
#define MIME_TYPES_CONFIG_FILE "config/mime.types"
#define MIME_TYPES_DEFAULT_TYPE "application/octet-stream"

typedef struct MimeType {
    char *content_type;
    char *extension;
} MimeType;

typedef struct MimeTypes {
    MimeType **types;
    int count;
    int allocated;
} MimeTypes;

MimeTypes *mime_types_load_from_file(char *filename);
char *mime_types_find_content_type(MimeTypes *mime_types, char *ext);
void mime_types_free(MimeTypes *mime_types);

#endif