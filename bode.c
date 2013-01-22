/*
    Creation of socket-based connections requires several operations.  First, a
    socket is created with socket(2).  Next, a willingness to accept incoming
    connections and a queue limit for incoming connections are specified with
    listen().  Finally, the connections are accepted with accept(2).
*/

#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_PORT (8080)
#define LISTENQ     (1024)

#define check(A, M) if (!(A)) { printf(M); return 1; }

int
main(int argc, char *argv[])
{
    int listener, conn, result;
    pid_t pid;
    struct sockaddr_in servaddr;

    char *response = "HTTP/1.1 200 OK\n\ny u do dis\n";

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

        printf("y u do dis\n");

        send(conn, response, strlen(response), 0);

        result = close(conn);
        check(result == 0, "Error closing connection socket.\n");
    }

    return 0;
}
