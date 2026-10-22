#include "minunit.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <config.h>
#include <mime.h>
#include <response.h>
#include <util.h>

/* Not in the public headers; exercised directly so we can force huge sizes. */
int write_value(char **target, const char *value, int buf_length);
MimeTypes *mime_types_create(int initial_allocation);
void mime_types_push(MimeTypes *mime_types, const char *type, const char *ext);

/* Make oversized allocations fail with NULL (rather than ASan aborting), so
   these tests see what the code does with a failed allocation. handle_segv=0
   lets a NULL dereference die by SIGSEGV instead of ASan's exit(1), so it
   can't be mistaken for a clean out-of-memory exit. */
const char *
__asan_default_options(void)
{
    return "allocator_may_return_null=1:max_allocation_size_mb=64:handle_segv=0";
}

#define TOO_BIG (1 << 28)

/* Run fn in a child; true if it exited with EXIT_FAILURE (the out-of-memory
   path), false if it returned normally or crashed. */
int
exits_with_failure(void (*fn)(void))
{
    int   status = 0;
    pid_t pid    = fork();

    if (pid == 0) {
        fn();
        _exit(0);
    }

    waitpid(pid, &status, 0);

    return WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE;
}

void
write_value_oversized()
{
    char *target = NULL;

    write_value(&target, "value", TOO_BIG);
}

void
mime_types_create_oversized()
{
    mime_types_create(TOO_BIG);
}

void
mime_types_push_grow_fails()
{
    MimeTypes *mime_types = mime_types_create(1);

    // Pretend the array is full at a size whose doubling can't be allocated.
    mime_types->count     = TOO_BIG;
    mime_types->allocated = TOO_BIG;

    mime_types_push(mime_types, "text/plain", "txt");
}

/* Write end of the pipe the out-of-memory handler reports through; stands in
   for the client connection the server's handler writes to. */
static int handler_fd = -1;

void
send_out_of_memory_response()
{
    write(handler_fd, RESPONSE_OUT_OF_MEMORY, strlen(RESPONSE_OUT_OF_MEMORY));
}

char *
test_out_of_memory_handler_runs_before_exit()
{
    /* A worker installs a handler so a failed allocation still answers the
       client with a 500 before the process exits. */
    char    reply[256] = {0};
    int     fds[2], status = 0;
    ssize_t received = 0;

    mu_assert(pipe(fds) == 0, "pipe failed");

    pid_t pid = fork();
    if (pid == 0) {
        close(fds[0]);
        handler_fd = fds[1];
        set_out_of_memory_handler(send_out_of_memory_response);
        write_value_oversized();
        _exit(0);
    }

    close(fds[1]);
    received = read(fds[0], reply, sizeof(reply) - 1);
    close(fds[0]);
    waitpid(pid, &status, 0);

    mu_assert(received > 0 && strncmp(reply, "HTTP/1.1 500 ", 13) == 0,
              "the handler should have sent a 500 before exiting");
    mu_assert(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE,
              "the process should still exit with EXIT_FAILURE");
    return NULL;
}

char *
test_write_value_exits_when_allocation_fails()
{
    mu_assert(exits_with_failure(write_value_oversized),
              "write_value should exit cleanly when realloc fails");
    return NULL;
}

char *
test_mime_types_create_exits_when_allocation_fails()
{
    mu_assert(exits_with_failure(mime_types_create_oversized),
              "mime_types_create should exit cleanly when calloc fails");
    return NULL;
}

char *
test_mime_types_push_exits_when_grow_fails()
{
    mu_assert(exits_with_failure(mime_types_push_grow_fails),
              "mime_types_push should exit cleanly when realloc fails");
    return NULL;
}

char *
all_tests()
{
    mu_suite_start();
    mu_run_test(test_write_value_exits_when_allocation_fails);
    mu_run_test(test_mime_types_create_exits_when_allocation_fails);
    mu_run_test(test_mime_types_push_exits_when_grow_fails);
    mu_run_test(test_out_of_memory_handler_runs_before_exit);
    return NULL;
}

RUN_TESTS(all_tests);
