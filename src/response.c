#include <string.h>
#include <stdlib.h>

#include <buffer.h>

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
    Buffer *file_buffer = buffer_alloc(BUF_SIZE + 1);
    char   *buf         = calloc(BUF_SIZE, sizeof(char));
    int    bytes_read   = 0;

    while(!feof(source)) {
        if (ferror(source)) {
            fprintf(stderr, "Error reading from stream.\n");
            goto error;
        }

        if (fgets(buf, BUF_SIZE, source) != NULL) {
            buffer_append(file_buffer, buf, BUF_SIZE);
        }
    }

    free(buf);

    bytes_read = buffer_strlen(file_buffer);

    response->bode = buffer_to_s(file_buffer);
    buffer_free(file_buffer);

    return bytes_read;
error:
    if (buf) { free(buf); }
    buffer_free(file_buffer);

    return -1;
}

int
response_bode_from_string(Response *response, const char *bode)
{
    int length = strlen(bode);

    response->bode = calloc(length + 1, sizeof(char));
    strncpy(response->bode, bode, length);

    return length;
}

char *
response_output(Response *response)
{
    Buffer *output_buffer = buffer_alloc(BUF_SIZE);
    char   *output;
    int    i = 0;

    buffer_appendf(output_buffer, "HTTP/1.1 %d %s\r\n", response->status->code, response->status->message);

    for(i = 0; i < response->headers_count; i++) {
        buffer_appendf(output_buffer, "%s: %s\r\n", response->headers[i]->key, response->headers[i]->value);
    }

    buffer_append(output_buffer, "\r\n", 2);
    buffer_append(output_buffer, response->bode, strlen(response->bode));

    response->total_size = buffer_strlen(output_buffer);

    output = buffer_to_s(output_buffer);
    buffer_free(output_buffer);

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
    if (response->bode)   { free(response->bode); }

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