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
#include <limits.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include <sds.h>

#include <header.h>
#include <response.h>
#include <mime.h>
#include <util.h>
#include <config.h>

#define LISTENQ 1024

// Drop a connection that stalls mid-request (or mid-response) rather than
// letting it tie up a forked worker indefinitely (slowloris).
#define CONNECTION_TIMEOUT_SECONDS 10

// Cap on how much of a request we will buffer, so a client can't exhaust
// memory by streaming an unbounded request.
#define MAX_REQUEST_SIZE (64 * 1024)

char *fetch_request_path(int connection, char *document_root);
char *resolve_within_root(const char *document_root, const char *candidate);
void set_connection_timeouts(int connection);
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

        set_connection_timeouts(conn);

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
    sds     request        = sdsempty();
    char    *tmp            = calloc(BUF_SIZE, sizeof(char));
    char    *relative_path;
    ssize_t bytes_received  = 0;
    size_t  total_received  = 0;

    while (1) {
        bytes_received = recv(connection, tmp, BUF_SIZE, 0);

        // <= 0 means the peer closed, an error occurred, or the receive timed
        // out (SO_RCVTIMEO). Stop reading; never append a negative length.
        if (bytes_received <= 0) {
            break;
        }

        // sds may reallocate on append, so reassign the handle.
        request = sdscatlen(request, tmp, bytes_received);
        total_received += bytes_received;

        // Refuse to buffer an unbounded request.
        if (total_received >= MAX_REQUEST_SIZE) {
            break;
        }

        if (bytes_received < BUF_SIZE) {
            break;
        }
    }

    memset(tmp, 0, BUF_SIZE);

    // Width-limited to BUF_SIZE-1 so a long request target can't overflow tmp.
    // A malformed request line (no match) yields NULL, which the caller serves
    // as a 404.
    if (sscanf(request, "GET %1023s", tmp) != 1) {
        sdsfree(request);
        free(tmp);
        return NULL;
    }

    asprintf(&relative_path, "%s%s", document_root, tmp);

    sdsfree(request);
    free(tmp);

    // Reject any target that escapes the document root (e.g. "/../../etc/passwd").
    char *safe_path = resolve_within_root(document_root, relative_path);
    free(relative_path);

    return safe_path;
}

/*
    Canonicalize `candidate` and confirm it lives inside `document_root`.
    Returns a newly-allocated absolute path on success, or NULL if the path
    escapes the root or cannot be resolved (a non-existent path resolves to
    NULL here and is served as a 404 by the caller).
*/
char *
resolve_within_root(const char *document_root, const char *candidate)
{
    char root_real[PATH_MAX];
    char candidate_real[PATH_MAX];

    if (realpath(document_root, root_real) == NULL) {
        return NULL;
    }

    if (realpath(candidate, candidate_real) == NULL) {
        return NULL;
    }

    size_t root_len = strlen(root_real);

    // Must share the root prefix AND sit on a path boundary, so that a sibling
    // like "/srv/wwwevil" can't masquerade as living under "/srv/www".
    if (strncmp(candidate_real, root_real, root_len) != 0 ||
        (candidate_real[root_len] != '/' && candidate_real[root_len] != '\0')) {
        return NULL;
    }

    return strdup(candidate_real);
}

/*
    Apply receive and send timeouts to an accepted connection so a slow or
    stalled client can't tie up a forked worker indefinitely. Best-effort:
    a failure to set the option is non-fatal.
*/
void
set_connection_timeouts(int connection)
{
    struct timeval timeout = { .tv_sec = CONNECTION_TIMEOUT_SECONDS, .tv_usec = 0 };

    setsockopt(connection, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(connection, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
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