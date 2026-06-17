#include <string.h>
#include <stdlib.h>

#include <sds.h>

#include <response.h>
#include <header.h>
#include <mime.h>
#include <file.h>
#include <util.h>

void response_add_status(Response *response, int status_code);
void response_add_header_int(Response *response, const char *key, int value);
void response_add_header_str(Response *response, const char *key, char *value);

Response *
response_create(char *filename, MimeTypes *mime_types)
{
    FILE *source = NULL;
    Response *response  = malloc(sizeof(Response));
    int      bytes_read = 0;
    char     *full_file_path = file_path_for(filename);

    response->bode          = NULL;
    response->headers       = NULL;
    response->headers_count = 0;

    if (full_file_path) {
        source = fopen(full_file_path, "r");
    }

    if (source == NULL) {
        // 404
        response_add_status(response, 404);
        response_add_header_str(response, "Content-Type", "text/plain");

        bytes_read = response_bode_from_string(response, "Y U DO DIS\n");
    } else {
        // 200 -- file read could fail in bode_from_file?
        response_add_status(response, 200);
        response_add_header_str(response, "Content-Type", file_content_type_for(mime_types, full_file_path));

        bytes_read = response_bode_from_file(response, source);

        fclose(source);
    }

    response_add_header_int(response, "Content-Length", bytes_read);
    response_add_header_str(response, "Connection", "close");

    if (full_file_path) {
        free(full_file_path);
    }

    return response;
}

void
response_add_status(Response *response, int status_code)
{
    int  max_message_length = 22;
    char *message           = calloc(max_message_length, sizeof(char));

    switch(status_code) {
        case 200:
            strncpy(message, "OK", 3);
            break;
        case 404:
            strncpy(message, "NOT FOUND", 15);
            break;
        default:
            status_code = 500;
            strncpy(message, "INTERNAL SERVER ERROR", 22);
            break;
    }

    Status *status = malloc(sizeof(Status));

    status->code = status_code;
    status->message = calloc(max_message_length, sizeof(char));
    strncpy(status->message, message, max_message_length);

    response->status = status;
    free(message);
}

int
response_bode_from_file(Response *response, FILE *source)
{
    sds    body       = sdsempty();
    char   buf[BUF_SIZE];
    size_t bytes_read = 0;

    // fread copies raw bytes (unlike fgets, which stops at newlines and NUL),
    // so binary files such as images survive intact.
    while ((bytes_read = fread(buf, 1, BUF_SIZE, source)) > 0) {
        body = sdscatlen(body, buf, bytes_read);
    }

    if (ferror(source)) {
        fprintf(stderr, "Error reading from stream.\n");
        sdsfree(body);
        response->bode = sdsempty();
        return -1;
    }

    response->bode = body;

    return sdslen(body);
}

int
response_bode_from_string(Response *response, const char *bode)
{
    response->bode = sdsnew(bode);

    return sdslen(response->bode);
}

char *
response_output(Response *response)
{
    sds output = sdsempty();
    int i      = 0;

    output = sdscatprintf(output, "HTTP/1.1 %d %s\r\n", response->status->code, response->status->message);

    for(i = 0; i < response->headers_count; i++) {
        output = sdscatprintf(output, "%s: %s\r\n", response->headers[i]->key, response->headers[i]->value);
    }

    output = sdscatlen(output, "\r\n", 2);

    // sdscatlen with the body's true length (not strlen) so a body containing
    // NUL bytes is sent in full.
    output = sdscatlen(output, response->bode, sdslen(response->bode));

    response->total_size = sdslen(output);

    return output;
}

size_t
response_length(Response *response)
{
    return response->total_size;
}

void status_free(Status *status)
{
    if (status->message) { free(status->message); }

    free(status);
}

void
response_free(Response *response)
{
    int i = 0;

    if (response->status) { status_free(response->status); }
    if (response->bode)   { sdsfree(response->bode); }

    if (response->headers) {
        for(i = 0; i < response->headers_count; i++) {
            header_free(response->headers[i]);
        }
        free(response->headers);
    }

    free(response);
}

void
response_add_header_str(Response *response, const char *key, char *value)
{
    response->headers = realloc(response->headers, (response->headers_count + 1) * sizeof(Header *));

    response->headers[response->headers_count] = header_create(key, value);
    response->headers_count++;
}

void
response_add_header_int(Response *response, const char *key, int value)
{
    char *str_value;

    asprintf(&str_value, "%d", value);

    response_add_header_str(response, key, str_value);

    free(str_value);
}