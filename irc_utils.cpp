#include "irc.hpp"

void set_nonblocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}
