#include <sys/socket.h>

#include <net.h>

/*
    send() may transmit fewer bytes than requested, so a single call can
    truncate a large response. Loop until everything is sent. Returns the
    number of bytes sent, or -1 if the peer went away / send failed.
*/
ssize_t
send_all(int fd, const char *buf, size_t len)
{
    size_t total = 0;

    while (total < len) {
        ssize_t sent = send(fd, buf + total, len - total, 0);
        if (sent <= 0) {
            return -1;
        }
        total += sent;
    }

    return (ssize_t) total;
}
