
#pragma once
#ifndef SERVER_HPP
# define SERVER_HPP
# include "irc.hpp"

class Client;

class Channel;

class Server {
	private :
		int port;
		std::string password;
		int ServSockFd;
		static bool signal;
		static bool	running;
		bool cleaned;
		std::vector<Client*> clients; // quando Client era armazenado por valor e o vetor realocava, os ponteiros guardados nos channels passavam a apontar para lugares errados.
		std::vector<struct pollfd> fds;
		std::vector<Channel*> channels;
		/*
		Por que Client e Channel devem ser ponteiros?
			Client:  estamos armazenando ponteiros para client (enderecos para os elementos do vetor) fora do vetor,
			especificamente nos canais no container myClients (std::map<int, Client*>). Entao se o vetor armazenasse Clients por valor,
			quando o vetor realocar (conforme o numero de clients), esses ponteiros se tornariam inválidos.
			
			Channel: salvamos Channel* nos Client, então o endereço precisa ser estável. não se pode correr o risco do endereço do objeto mudar, segfaults.
		*/
	public :
		Server();
		void serverInit(int newPort, std::string newPassword);
		void servSock();
		void acceptNewClient();
		void recvNewData(int fd);
		static void signalHandler (int signum);
		void sendMsgAll(int fd_client, const char *buffer, size_t len);
		Client* getClientByFd(int fd);
		std::string get_pass();
		Client* getClientByNick(const std::string& nick);
		//cmds
		void handleCommand(Client *a, std::string line);
		bool isThisCmd(const std::string& line, std::string cmd);
		void voidCmd(Client *a, std::string line);
		void cmdCAP(Client *cli, std::string line);
		void cmdPASS(Client *cli, std::string line);
		void cmdNICK(Client *cli, std::string line);
		void cmdUSER(Client *cli, std::string line);
		void cmdQUIT(Client *a, std::string line);
		void checkRegistration(Client *cli);
		void cmdMODE(Client *a, std::string line);
		void cmdPRIVMSG(Client *cli, std::string line);
		void cmdNOTICE(Client *cli, std::string line);
		void cmdPING(Client *cli, std::string line);
		void cmdPART(Client *cli, std::string line);
		void cmdWHOIS(Client *cli, std::string line);
		//channel cmds
		Channel* getChannelByName(std::string name);
		void cmdJOIN(Client *a, std::string line);
		//clean
		void closeFds();
		void clearClients(int fd);
		// helper
		void tryFinishRegistration(Client* cli);

		~Server();
} ;

#endif