#ifndef RESPONSE_H
#define RESPONSE_H

#include <stdio.h>

#include <header.h>
#include <mime.h>

typedef struct Status {
    int code;
    char *message;
} Status;

typedef struct Response {
    Status *status;
    Header **headers;
    int headers_count;
    char *bode;
    size_t total_size;
} Response;

Response *response_create(char *filename, MimeTypes *mime_types);
int response_bode_from_string(Response *response, const char *bode);
int response_bode_from_file(Response *response, FILE *source);
char *response_output(Response *response);
size_t response_length(Response *response);
void response_free(Response *response);

#endif