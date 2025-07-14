#include "irc.hpp"

void set_nonblocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

void sendMsg(int fd, const char *buffer, size_t len){
	size_t total_sent = 0;

	while (total_sent < len)
	{
		int sent = send(fd, buffer + total_sent, len - total_sent, 0);
        if (sent <= 0)
            return ;
        total_sent += sent;
    }
}
