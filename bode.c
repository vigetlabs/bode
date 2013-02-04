/*
    Creation of socket-based connections requires several operations.  First, a
    socket is created with socket(2).  Next, a willingness to accept incoming
    connections and a queue limit for incoming connections are specified with
    listen().  Finally, the connections are accepted with accept(2).
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_PORT (8080)
#define LISTENQ     (1024)

#define check(A, M) if (!(A)) { printf(M); return 1; }

char *build_response(char *status, char **headers, char *bode);
char *file_to_string(char *filename);

int
main(int argc, char *argv[])
{
    int     listener, conn, result;
    pid_t   pid;
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

    printf("My bode is redy on port %d.\n", SERVER_PORT);

    while (1) {
        conn = accept(listener, NULL, NULL);
        check(conn >= 0, "Error calling accept.\n");

        bode = file_to_string("index.html");

        if (bode) {
            response = build_response("HTTP/1.1 200 OK", NULL, bode);
        } else {
            response = build_response("HTTP/1.1 404 NOT FOUND", NULL, "Y U DO DIS\n");
        }

        send(conn, response, strlen(response), 0);

        result = close(conn);
        check(result == 0, "Error closing connection socket.\n");
    }

    return 0;
}

char*
build_response(char *status, char **headers, char *bode)
{
    int  status_length,
         bode_length,
         bode_start,
         cur;

    char *response;

    status_length = strlen(status);
    bode_length   = strlen(bode);
    bode_start    = status_length + 4;

    response      = calloc(status_length + bode_length + 5, sizeof(char));

    for (cur = 0; cur < status_length; cur++) {
        response[cur] = status[cur];
    }

    response[cur++] = '\r';
    response[cur++] = '\n';
    response[cur++] = '\r';
    response[cur++] = '\n';

    for (cur = 0; cur < bode_length; cur++) {
        response[bode_start + cur] = bode[cur];
    }

    response[bode_start + cur] = '\0';

    return response;
}

char*
file_to_string (char *filename)
{
    FILE   *file;
    struct stat file_stat;
    int    size;
    char   *contents, *cur, c;

    if (stat(filename, &file_stat) != 0) {
        return NULL;
    }

    file     = fopen(filename, "r");
    size     = file_stat.st_size;
    contents = calloc(size, sizeof(char));
    cur      = contents;

    while ((c = fgetc(file)) != EOF) {
        *(cur++) = c;
    }

    return contents;
}
