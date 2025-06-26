
#pragma once
#ifndef SERVER_HPP
# define SERVER_HPP
# include "irc.hpp"

class Server {
	private :
		int port;
		int ServSockFd;
		static bool signal;
		std::vector<Client> clients;
		std::vector<struct pollfd> fds;
	public :
		Server();
		void serverInit();
		void servSock();
		void acceptNewClient();
		void recvNewData(int fd); //from registered client
		static void signalHandler (int signum);
		void closeFds();
		void clearClients(int fd);
		~Server();
} ;

#endif