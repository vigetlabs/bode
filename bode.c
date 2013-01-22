/*
    Creation of socket-based connections requires several operations.  First, a
    socket is created with socket(2).  Next, a willingness to accept incoming
    connections and a queue limit for incoming connections are specified with
    listen().  Finally, the connections are accepted with accept(2).  The
    listen() call applies only to sockets of type SOCK_STREAM or
    SOCK_SEQPACKET.
*/

#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_PORT (8080)
#define LISTENQ     (1024)

int
main(int argc, char *argv[])
{
    int listener, conn;
    pid_t pid;
    struct sockaddr_in servaddr;
    char *response;

    listener = socket(PF_INET, SOCK_STREAM, 0);

    if (listener < 0) {
        printf("Couldn't create socket.");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family      = PF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(SERVER_PORT);

    if (bind(listener, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        printf("Couldn't bind listening socket.");
        return 1;
    }

    if (listen(listener, LISTENQ) < 0) {
        printf("Call to listen failed.");
        return 1;
    }

    printf("My bode is redy on port %d.\n", SERVER_PORT);  

    while (1) {
        conn = accept(listener, NULL, NULL);

        if (conn < 0) {
            printf("Error calling accept()");
            return 1;
        }

        printf("y u do dis\n");

        response = "HTTP/1.1 200 OK\n\ny u do dis\n";

        send(conn, response, strlen(response), 0);

        if (close(conn) < 0) {
            printf("Error closing connection socket.");
            return 1;
        }
    }

    return 0;
}
