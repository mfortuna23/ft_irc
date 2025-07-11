
#pragma once
#ifndef SERVER_HPP
# define SERVER_HPP
# include "irc.hpp"

class Client;

class Server {
	private :
		int port;
		std::string password;
		int ServSockFd;
		static bool signal;
		static bool	running;
		std::vector<Client> clients;
		std::vector<struct pollfd> fds;
	public :
		Server();
		void serverInit(int newPort, std::string newPassword);
		void servSock();
		void acceptNewClient();
		void recvNewData(int fd);
		void handleCommand(Client *a, std::string line);
		static void signalHandler (int signum);
		void sendMsgAll(int fd_client, const char *buffer, size_t len);
		void closeFds();
		void clearClients(int fd);
		Client* getClientByFd(int fd);
		~Server();
} ;

#endif