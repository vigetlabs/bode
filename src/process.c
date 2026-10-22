#include <errno.h>
#include <signal.h>
#include <string.h>

#include <sys/wait.h>

#include "process.h"

/*
    Reap every child that has already exited. A single waitpid() only
    collects one, so a burst of short requests would otherwise leave
    <defunct> processes behind. Returns the number reaped.
*/
int
reap_children()
{
    int reaped = 0;

    while (waitpid(-1, NULL, WNOHANG) > 0) {
        reaped++;
    }

    return reaped;
}

static void
handle_sigchld(int signo)
{
    // waitpid() can overwrite errno for the code this signal interrupted.
    int saved_errno = errno;

    (void) signo;
    reap_children();

    errno = saved_errno;
}

/*
    Reap children as they exit instead of only when the next connection
    arrives. SA_RESTART keeps accept() from failing with EINTR each time a
    worker finishes. Returns 0 on success, -1 on failure.
*/
int
install_child_reaper()
{
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_sigchld;
    action.sa_flags   = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&action.sa_mask);

    return sigaction(SIGCHLD, &action, NULL);
}
