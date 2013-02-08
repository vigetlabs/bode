/*
    Creation of socket-based connections requires several operations.  First, a
    socket is created with socket(2).  Next, a willingness to accept incoming
    connections and a queue limit for incoming connections are specified with
    listen().  Finally, the connections are accepted with accept(2).
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <header.h>
#include <response.h>

#define SERVER_PORT (8080)
#define LISTENQ     (1024)

#define check(A, M) if (!(A)) { printf(M); return 1; }

int
main(void)
{
    int     listener, conn, result;
    struct  sockaddr_in servaddr;
    char    *output;
    pid_t   pid;

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

        pid = fork();
        if (pid == 0) {
            // Child process, handle connection
            check(close(listener) == 0, "Error closing listening socket.\n");

            Response *response = response_create("index.html");

            output = response_output(response);

            send(conn, output, response_length(response), 0);

            check(close(conn) == 0, "Error closing connection socket.\n");

            response_free(response);
            free(output);

            exit(0);
        }

        check(close(conn) == 0, "Error closing connection socket.\n");

        waitpid(-1, NULL, WNOHANG);
    }

    return 1;
}