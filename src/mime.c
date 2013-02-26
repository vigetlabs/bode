#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <mime.h>
#include <util.h>

MimeTypes *
mime_types_create(int initial_allocation)
{
    // TODO: check for alloc errors
    MimeTypes *mime_types = calloc(1, sizeof(MimeTypes));

    // TODO: check for alloc errors
    mime_types->types     = calloc(initial_allocation, sizeof(MimeType));
    mime_types->allocated = initial_allocation;
    mime_types->count     = 0;

    return mime_types;
}

MimeType *
mime_type_create(char *type, char *ext)
{
    // TODO: check for alloc errors
    MimeType *mime_type = calloc(1, sizeof(MimeType));

    mime_type->content_type = strdup(type);
    mime_type->extension    = strdup(ext);

    return mime_type;
}

void
mime_types_push(MimeTypes *mime_types, char *type, char *ext)
{
    int total_allocation,
        index = mime_types->count;

    if (mime_types->count >= mime_types->allocated) {
        total_allocation = mime_types->allocated * 2;

        // TODO: check for alloc errors
        mime_types->types = realloc(mime_types->types, total_allocation * sizeof(MimeType *));
        mime_types->allocated = total_allocation;
    }

    mime_types->types[index] = mime_type_create(type, ext);
    mime_types->count++;
}

MimeTypes *
mime_types_load(Config *config)
{
    char *filename;

    asprintf(&filename, "%s/%s", config->config_dir, MIME_TYPES_CONFIG_FILENAME);

    FILE *fp = fopen(filename, "r"); // TODO: check for failure
    const char *d = " \t\n";

    char line[BUF_SIZE],
         *token  = NULL,
         *type   = NULL,
         *source = NULL;

    MimeTypes *mime_types = mime_types_create(MIME_TYPES_ALLOC_SIZE);

    while (fgets(line, BUF_SIZE, fp) != NULL) {
        if (line[0] == '#') { continue; } // Skip comments

        type   = NULL;
        source = line;

        while ((token = strsep(&source, d)) != NULL) {
            if (strlen(token) > 0) {
                if (type) {
                    mime_types_push(mime_types, type, token);
                } else {
                    type = token;
                }
            }
        }
    }

    free(filename);
    fclose(fp);

    return mime_types;
}

char *
mime_types_find_content_type(MimeTypes *mime_types, char *ext)
{
    int i = 0;

    for (i = 0; i < mime_types->count; i++) {
        if (strcmp(ext, mime_types->types[i]->extension) == 0) {
            return mime_types->types[i]->content_type;
        }
    }

    return MIME_TYPES_DEFAULT_TYPE;
}

void
mime_type_free(MimeType *mime_type)
{
    if (mime_type->content_type) { free(mime_type->content_type); }
    if (mime_type->extension)    { free(mime_type->extension); }

    free(mime_type);
}

void
mime_types_free(MimeTypes *mime_types)
{
    int i = 0;

    for (i = 0; i < mime_types->count; i++) {
        mime_type_free(mime_types->types[i]);
    }

    free(mime_types->types);
    free(mime_types);
}