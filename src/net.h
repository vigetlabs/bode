#ifndef NET_H
#define NET_H

#include <stddef.h>
#include <sys/types.h>

ssize_t send_all(int fd, const char *buf, size_t len);

#endif
