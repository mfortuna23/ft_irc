
#pragma once
#ifndef SERVER_HPP
# define SERVER_HPP
# include "irc.hpp"

class Server {
	private :
		int port;
		std::string password;
		int ServSockFd;
		static bool signal;
		std::map<int, Client> clients;
		std::vector<struct pollfd> fds;
	public :
		Server();
		void serverInit(int newPort, std::string newPassword);
		void servSock();
		void acceptNewClient();
		void recvData(int fd); //not registered
		void recvNewData(int fd); //from registered client
		static void signalHandler (int signum);
		void sendMsgAll(int fd_client, const char *buffer, size_t len);
		void closeFds();
		void clearClients(int fd);
		~Server();
} ;

#endif