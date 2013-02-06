#include <string.h>
#include <stdlib.h>

#include <buffer.h>

#include <response.h>
#include <header.h>

void response_add_status(Response *response, int status_code);
void response_add_header_int(Response *response, const char *key, int value);

Response *
response_create(char *filename)
{
    Response *response = malloc(sizeof(Response));

    response->bode   = NULL;
    response->header = NULL;

    FILE *source = fopen(filename, "r");

    if (source == NULL) {
        // 404
        response_add_status(response, 404);
        response_bode_from_string(response, "Y U DO DIS\n");
    } else {
        // 200 -- file read could fail in bode_from_file?
        response_add_status(response, 200);
        response_bode_from_file(response, source);

        fclose(source);
    }

    return response;
}

void
response_add_status(Response *response, int status_code)
{
    int max_message_length = 22;
    char *message = calloc(max_message_length, sizeof(char));

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

void
response_bode_from_file(Response *response, FILE *source)
{
    Buffer *file_buffer = buffer_alloc(BUF_SIZE + 1);
    char *buf = calloc(BUF_SIZE, sizeof(char));

    while(!feof(source)) {
        if (ferror(source)) {
            fprintf(stderr, "Error reading from stream.\n");
            goto error;
        }

        fgets(buf, BUF_SIZE, source);
        buffer_append(file_buffer, buf, BUF_SIZE);
    }

    free(buf);

    response_add_header_int(response, "Content-Length", buffer_strlen(file_buffer));

    response->bode = buffer_to_s(file_buffer);
    buffer_free(file_buffer);

    return;
error:
    if (buf) { free(buf); }
    buffer_free(file_buffer);

    response->bode = NULL;
}

void
response_bode_from_string(Response *response, const char *bode)
{
    int length = strlen(bode);

    response_add_header_int(response, "Content-Length", length);

    response->bode = calloc(length + 1, sizeof(char));
    strncpy(response->bode, bode, length);
}

char *
response_output(Response *response)
{
    Buffer *output_buffer = buffer_alloc(BUF_SIZE);
    char   *output;

    buffer_appendf(output_buffer, "HTTP/1.1 %d %s\r\n", response->status->code, response->status->message);
    buffer_appendf(output_buffer, "%s: %s\r\n\r\n", response->header->key, response->header->value);

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
    if (response->status) { status_free(response->status); }
    if (response->header) { header_free(response->header); }
    if (response->bode)   { free(response->bode); }

    free(response);
}

void
response_add_header_int(Response *response, const char *key, int value)
{
    Header *header = header_create_int(key, value);
    response->header = header;
}