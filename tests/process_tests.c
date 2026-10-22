#include "minunit.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/wait.h>

#include <process.h>

#define CHILD_COUNT 20

/* Sleep the full duration even when SIGCHLD interrupts nanosleep(). */
static void
sleep_ms(long ms)
{
    struct timespec remaining = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };

    while (nanosleep(&remaining, &remaining) == -1 && errno == EINTR) {
    }
}

/*
    Fork `count` children that exit immediately, then give them a moment to
    die so they sit as zombies until someone reaps them.
*/
static int
spawn_exiting_children(int count)
{
    int i;

    for (i = 0; i < count; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            return -1;
        }
        if (pid == 0) {
            _exit(0);
        }
    }

    sleep_ms(200);
    return 0;
}

/* True once every child of this process has been reaped. */
static int
no_children_left()
{
    return waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD;
}

char *
test_reap_children_reaps_every_exited_child()
{
    /* A single waitpid(WNOHANG) per accept() only reaps one child, so bursts
       of short requests accumulate zombies. reap_children must drain all. */
    mu_assert(spawn_exiting_children(CHILD_COUNT) == 0, "fork failed");

    int reaped = reap_children();

    mu_assert(reaped == CHILD_COUNT, "should reap every exited child");
    mu_assert(no_children_left(), "no zombies should remain");
    return NULL;
}

char *
test_reap_children_with_no_children_is_harmless()
{
    mu_assert(reap_children() == 0, "nothing to reap");
    return NULL;
}

char *
test_child_reaper_reaps_without_polling()
{
    /* Children that exit while the server sits idle in accept() must be
       reaped by the SIGCHLD handler, not left until the next connection. */
    mu_assert(install_child_reaper() == 0, "installing the handler failed");
    mu_assert(spawn_exiting_children(CHILD_COUNT) == 0, "fork failed");

    mu_assert(no_children_left(), "SIGCHLD handler should reap all children");

    signal(SIGCHLD, SIG_DFL);
    return NULL;
}

char *
test_child_reaper_preserves_errno()
{
    /* The handler can interrupt code that is about to inspect errno. */
    mu_assert(install_child_reaper() == 0, "installing the handler failed");

    errno = EAGAIN;
    raise(SIGCHLD);
    mu_assert(errno == EAGAIN, "handler must not clobber errno");

    signal(SIGCHLD, SIG_DFL);
    return NULL;
}

char *
all_tests()
{
    mu_suite_start();

    mu_run_test(test_reap_children_reaps_every_exited_child);
    mu_run_test(test_reap_children_with_no_children_is_harmless);
    mu_run_test(test_child_reaper_reaps_without_polling);
    mu_run_test(test_child_reaper_preserves_errno);

    return NULL;
}

RUN_TESTS(all_tests);
