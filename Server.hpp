
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
		std::vector<Channel> channels;
	public :
		Server();
		void serverInit(int newPort, std::string newPassword);
		void servSock();
		void acceptNewClient();
		void recvNewData(int fd);
		static void signalHandler (int signum);
		void sendMsgAll(int fd_client, const char *buffer, size_t len);
		Client* getClientByFd(int fd);
		//cmds
		void handleCommand(Client *a, std::string line);
		bool isThisCmd(const std::string& line, std::string cmd);
		void voidCmd(Client *a, std::string line){(void)a; (void)line;};
		//channel cmds
		void joinCmd(Client *a, std::string line);
		//clean
		void closeFds();
		void clearClients(int fd);
		~Server();
} ;

#endif