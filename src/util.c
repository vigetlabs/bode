#include <util.h>

static void (*out_of_memory_handler)(void) = NULL;

void
set_out_of_memory_handler(void (*handler)(void))
{
    out_of_memory_handler = handler;
}

void
out_of_memory(const char *file, int line)
{
    fprintf(stderr, "Out of memory at %s:%d\n", file, line);

    // The handler must not allocate; it runs precisely because we couldn't.
    if (out_of_memory_handler) { out_of_memory_handler(); }

    exit(EXIT_FAILURE);
}
