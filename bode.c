/*
    Creation of socket-based connections requires several operations.  First, a
    socket is created with socket(2).  Next, a willingness to accept incoming
    connections and a queue limit for incoming connections are specified with
    listen().  Finally, the connections are accepted with accept(2).
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <arpa/inet.h>

#include <buffer.h>

#include <header.h>
#include <response.h>
#include <mime.h>
#include <util.h>
#include <config.h>

#define LISTENQ 1024

char *fetch_request_path(int connection, char *document_root);
Config *initialize_configuration(int argc, char *argv[]);

int
main(int argc, char *argv[])
{
    int     listener, conn, result;
    struct  sockaddr_in servaddr;
    char    *output;
    pid_t   pid;

    Config    *config     = initialize_configuration(argc, argv);

    if (!config) {
        fprintf(stderr, "Usage: bode --doc-root=<path> --port=<port> --conf-dir=<path>\n");
        return 1;
    }

    MimeTypes *mime_types = mime_types_load(config);

    listener = socket(PF_INET, SOCK_STREAM, 0);
    check(listener >= 0, "Couldn't create socket.\n");

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family      = PF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(config->port);

    result = bind(listener, (struct sockaddr *) &servaddr, sizeof(servaddr));
    check(result == 0, "Couldn't bind listening socket.\n");

    result = listen(listener, LISTENQ);
    check(result == 0, "Call to listen failed.\n");

    fprintf(stderr, "My bode is redy on port %d.\n", config->port);

    while (1) {
        conn = accept(listener, NULL, NULL);
        check(conn >= 0, "Error calling accept.\n");

        pid = fork();
        if (pid == 0) {
            char *request_path = fetch_request_path(conn, config->document_root);

            // Child process, handle connection
            check(close(listener) == 0, "Error closing listening socket.\n");

            Response *response = response_create(request_path, mime_types);

            output = response_output(response);

            send(conn, output, response_length(response), 0);

            check(close(conn) == 0, "Error closing connection socket.\n");

            free(request_path);
            response_free(response);
            free(output);

            exit(0);
        }

        check(close(conn) == 0, "Error closing connection socket.\n");

        waitpid(-1, NULL, WNOHANG);
    }

    config_free(config);
    mime_types_free(mime_types);
    return 1;
}

char *
fetch_request_path(int connection, char *document_root)
{
    Buffer  *request_buffer = buffer_alloc(BUF_SIZE);
    char    *tmp            = calloc(BUF_SIZE, sizeof(char));
    char    *request_path, *relative_path;
    ssize_t bytes_received = 0;

    while (1) {
        bytes_received = recv(connection, tmp, BUF_SIZE, 0);
        buffer_append(request_buffer, tmp, bytes_received);

        if (bytes_received < BUF_SIZE) {
            break;
        }
    }

    memset(tmp, 0, BUF_SIZE);

    request_path = buffer_to_s(request_buffer);
    buffer_free(request_buffer);

    sscanf(request_path, "GET %s", tmp);
    asprintf(&relative_path, "%s%s", document_root, tmp);

    free(request_path);
    free(tmp);

    return relative_path;
}

Config *
initialize_configuration(int argc, char *argv[])
{
    char   ch      = '\0';
    Config *config = config_create();
    int    port    = 0;

    static struct option options[] = {
        {"doc-root", required_argument, NULL, 'd'},
        {"port",     required_argument, NULL, 'p'},
        {"conf-dir", required_argument, NULL, 'c'},
        {NULL,       0,                 NULL, 0}
    };

    while ((ch = getopt_long(argc, argv, "d:p:c:", options, NULL)) != -1) {
        switch (ch) {
            case 'd':
                config_set(config, CONFIG_DOCUMENT_ROOT, optarg);
                break;
            case 'c':
                config_set(config, CONFIG_CONFIG_DIR, optarg);
                break;
            case 'p':
                port = atoi(optarg);
                config_set(config, CONFIG_PORT, &port);
                break;
            case ':':
            case '?':
            default:
                return NULL;
                break;
        }
    }

    debug("document_root set to '%s'", config->document_root);
    debug("config_path set to '%s'", config->config_dir);

    return config;
}