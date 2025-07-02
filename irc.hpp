#pragma once
#ifndef IRC_HPP
# define IRC_HPP

# include <iostream>
# include <string>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <unistd.h>
# include <fcntl.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/un.h>
# include <poll.h>
# include <signal.h>
# include <cstring>
# include <cerrno>
# include <vector>

# include "Client.hpp"
# include "Server.hpp"

# define BLUE "\033[34m"
# define RED "\033[31m"
# define GREEN "\033[32m"
# define YELLOW "\033[33m"
# define MAGENTA "\033[35m"
# define CYAN "\033[36m"
# define RESET "\033[0m"
# define BUFFER_SIZE 512


/*###### UTILS #####*/
void set_nonblocking(int fd);

#endif
