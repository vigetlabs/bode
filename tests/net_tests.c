#include "minunit.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include <net.h>

// Far larger than the shrunken send buffer, so send() must return short.
#define PAYLOAD_SIZE (4 * 1024 * 1024)

typedef struct Reader {
    int fd;
    char *buf;
    size_t len;
    size_t received;
} Reader;

static void *
read_all(void *arg)
{
    Reader *reader = arg;

    while (reader->received < reader->len) {
        ssize_t n = recv(reader->fd, reader->buf + reader->received,
                         reader->len - reader->received, 0);
        if (n <= 0) {
            break;
        }
        reader->received += n;
    }

    return NULL;
}

char *
test_send_all_delivers_every_byte_across_short_writes()
{
    int fds[2];
    mu_assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0, "socketpair failed");

    int sndbuf = 4096;
    setsockopt(fds[0], SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof sndbuf);

    char *payload = malloc(PAYLOAD_SIZE);
    char *received = malloc(PAYLOAD_SIZE);
    mu_assert(payload && received, "malloc failed");

    // A non-repeating-ish pattern so a dropped or duplicated chunk shows up.
    for (size_t i = 0; i < PAYLOAD_SIZE; i++) {
        payload[i] = (char) ((i * 31 + (i >> 12)) & 0xff);
    }

    Reader reader = { fds[1], received, PAYLOAD_SIZE, 0 };
    pthread_t thread;
    mu_assert(pthread_create(&thread, NULL, read_all, &reader) == 0,
              "pthread_create failed");

    ssize_t sent = send_all(fds[0], payload, PAYLOAD_SIZE);
    pthread_join(thread, NULL);

    mu_assert(sent == PAYLOAD_SIZE, "send_all should report the full length");
    mu_assert(reader.received == PAYLOAD_SIZE, "peer should receive every byte");
    mu_assert(memcmp(payload, received, PAYLOAD_SIZE) == 0,
              "received bytes should match what was sent");

    free(payload);
    free(received);
    close(fds[0]);
    close(fds[1]);
    return NULL;
}

char *
test_send_all_returns_error_when_peer_closed()
{
    int fds[2];
    mu_assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0, "socketpair failed");
    close(fds[1]);

    // Without this the write to a closed peer kills the test with SIGPIPE.
    signal(SIGPIPE, SIG_IGN);

    const char msg[] = "hello";
    mu_assert(send_all(fds[0], msg, sizeof msg - 1) == -1,
              "send_all should return -1 once the peer has gone away");

    signal(SIGPIPE, SIG_DFL);
    close(fds[0]);
    return NULL;
}

char *
all_tests()
{
    mu_suite_start();

    mu_run_test(test_send_all_delivers_every_byte_across_short_writes);
    mu_run_test(test_send_all_returns_error_when_peer_closed);

    return NULL;
}

RUN_TESTS(all_tests);
