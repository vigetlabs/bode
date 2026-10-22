#include "minunit.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#include <util.h>

/*
    check() is fatal: it must terminate the process, not `return 1` from
    whatever function it happens to be used in. Run it in a child and
    collect the exit status and anything written to stderr.
*/
static int
run_failing_check(const char *message, char *stderr_out, size_t len)
{
    int fds[2];
    int status;

    if (pipe(fds) != 0) {
        return -1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(fds[0]);
        dup2(fds[1], STDERR_FILENO);
        check(0, message);
        _exit(0);
    }

    close(fds[1]);
    ssize_t n = read(fds[0], stderr_out, len - 1);
    stderr_out[n > 0 ? n : 0] = '\0';
    close(fds[0]);

    waitpid(pid, &status, 0);
    return status;
}

char *
test_check_failure_exits_the_process()
{
    char out[256];
    int status = run_failing_check("boom\n", out, sizeof(out));

    mu_assert(WIFEXITED(status), "check should exit, not crash");
    mu_assert(WEXITSTATUS(status) == EXIT_FAILURE,
              "check should exit with EXIT_FAILURE");
    return NULL;
}

char *
test_check_failure_reports_on_stderr()
{
    char out[256];
    run_failing_check("boom\n", out, sizeof(out));

    mu_assert(strstr(out, "boom") != NULL,
              "the message should be written to stderr");
    return NULL;
}

char *
test_check_message_is_not_a_format_string()
{
    char out[256];
    run_failing_check("100%s done\n", out, sizeof(out));

    mu_assert(strstr(out, "100%s done") != NULL,
              "conversion specifiers in the message should print literally");
    return NULL;
}

char *
test_check_success_is_a_single_statement()
{
    /* An unbraced `if` macro would steal this `else`. */
    int reached = 0;

    if (reached)
        check(1, "unreachable\n");
    else
        reached = 1;

    mu_assert(reached, "check should expand to a single statement");
    return NULL;
}

char *
all_tests()
{
    mu_suite_start();

    mu_run_test(test_check_failure_exits_the_process);
    mu_run_test(test_check_failure_reports_on_stderr);
    mu_run_test(test_check_message_is_not_a_format_string);
    mu_run_test(test_check_success_is_a_single_statement);

    return NULL;
}

RUN_TESTS(all_tests);
