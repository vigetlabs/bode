#include "minunit.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <config.h>
#include <mime.h>

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
    return NULL;
}

RUN_TESTS(all_tests);
