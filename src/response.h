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

// Sent as-is when a worker runs out of memory, so it must not be built at
// runtime.
#define RESPONSE_OUT_OF_MEMORY                \
    "HTTP/1.1 500 INTERNAL SERVER ERROR\r\n"  \
    "Content-Type: text/plain\r\n"            \
    "Content-Length: 22\r\n"                  \
    "Connection: close\r\n"                   \
    "\r\n"                                    \
    "Internal Server Error\n"

Response *response_create(char *filename, MimeTypes *mime_types);
int response_bode_from_string(Response *response, const char *bode);
int response_bode_from_file(Response *response, FILE *source);
char *response_output(Response *response);
size_t response_length(const Response *response);
void response_free(Response *response);

#endif