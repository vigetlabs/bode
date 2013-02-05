/*
    Creation of socket-based connections requires several operations.  First, a
    socket is created with socket(2).  Next, a willingness to accept incoming
    connections and a queue limit for incoming connections are specified with
    listen().  Finally, the connections are accepted with accept(2).
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <buffer.h>

#define SERVER_PORT (8080)
#define LISTENQ     (1024)
#define BUF_SIZE    (1024)

#define check(A, M) if (!(A)) { printf(M); return 1; }

char *build_response(char *status, char *bode);
char *file_to_string(char *filename);

int
main(int argc, char *argv[])
{
    int     listener, conn, result;
    struct  sockaddr_in servaddr;
    char    *bode, *response;

    listener = socket(PF_INET, SOCK_STREAM, 0);
    check(listener >= 0, "Couldn't create socket.\n");

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family      = PF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(SERVER_PORT);

    result = bind(listener, (struct sockaddr *) &servaddr, sizeof(servaddr));
    check(result == 0, "Couldn't bind listening socket.\n");

    result = listen(listener, LISTENQ);
    check(result == 0, "Call to listen failed.\n")

    fprintf(stderr, "My bode is redy on port %d.\n", SERVER_PORT);

    while (1) {
        conn = accept(listener, NULL, NULL);
        check(conn >= 0, "Error calling accept.\n");

        bode = file_to_string("index.html");

        if (bode) {
            response = build_response("HTTP/1.1 200 OK", bode);
            free(bode);
        } else {
            response = build_response("HTTP/1.1 404 NOT FOUND", "Y U DO DIS\n");
        }

        send(conn, response, strlen(response), 0);

        result = close(conn);
        check(result == 0, "Error closing connection socket.\n");
    }

    free(response);

    return 0;
}

char*
build_response(char *status, char *bode)
{
    char   *response;
    Buffer *response_buffer = buffer_alloc(BUF_SIZE);

    buffer_appendf(response_buffer, "%s\r\n\r\n%s", status, bode);
    response = buffer_to_s(response_buffer);

    buffer_free(response_buffer);

    return response;
}

char*
file_to_string (char *filename)
{
    FILE *filed;
    Buffer *file_buffer = buffer_alloc(BUF_SIZE + 1);
    char *buf = calloc(1, BUF_SIZE * sizeof(char));
    char *contents;

    filed = fopen(filename, "r");
    if (filed == NULL) {
        fprintf(stderr, "Could not open '%s': %s\n", filename, strerror(errno));
        goto error;
    }

    while(1) {
        if (feof(filed)) {
            break;
        } else if (ferror(filed)) {
            fprintf(stderr, "Error reading file.\n");
            goto error;
        }

        fgets(buf, BUF_SIZE, filed);
        buffer_append(file_buffer, buf, BUF_SIZE);
    }

    free(buf);

    contents = buffer_to_s(file_buffer);
    buffer_free(file_buffer);

    return contents;
error:
    if (buf) { free(buf); }
    buffer_free(file_buffer);
    return NULL;
}