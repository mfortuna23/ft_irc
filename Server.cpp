#include "Server.hpp"

bool Server::signal = false;
Server::Server(){
	ServSockFd = -1;
}

void Server::serverInit(){
	this->port = 4444;
	servSock();
	std::cout << GREEN << "Server <" << ServSockFd << "> Connected" << RESET << std::endl;
	std::cout << "Waiting to accept a connection ..." << std::endl;
}

void Server::servSock(){
	struct sockaddr_in add;
	struct pollfd NewPoll;
	add.sin_family = AF_INET; // set address familiy to ipv4
	add .sin_port = htons(this->port); //convert the port to network byte order (big endian)
	add.sin_addr.s_addr = INADDR_ANY; //set the address to any local machine address

	ServSockFd = socket(AF_INET, SOCK_STREAM, 0); // create server socket
	if (ServSockFd == -1)
		throw (std::runtime_error("faild to create socket"));
	int en = 1;
	if (setsockopt(ServSockFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1) //set the socket option (SO_REUSEADDR) to reuse the address
		throw (std::runtime_error("faild to set option (SO_REUSEADDR) on socket"));
	if (fcntl(ServSockFd, F_SETFL, O_NONBLOCK) == -1) //set the socket option (O_NONBLOCK) for non-blocking socket
		throw(std::runtime_error("faild to set option (O_NONBLOCK) on socket"));
	if (bind(ServSockFd, (struct sockaddr *)&add, sizeof(add)) == -1) // bind the socket to the address
		throw(std::runtime_error("faild to bind socket"));
	if (listen(ServSockFd, SOMAXCONN) == -1) // listen for incoming connections and making the socket a passive socket
		throw (std::runtime_error("listen() faild"));
	NewPoll.fd = ServSockFd;
	NewPoll.events = POLLIN;
	NewPoll.revents = 0;
	fds.push_back(NewPoll);
}

void Server::acceptNewClient(){

}

void Server::recvNewData(int fd){
	(void)fd;
}

void Server::signalHandler (int signum){
	(void)signum; // any signal ...
	std::cout << std::endl << RED << "Signal recived!" << RESET << std::endl;
	Server::signal = true;
}

void Server::closeFds(){
	for (size_t  i = 0; i < clients.size(); i++){
		std::cout << RED << "Client <" << clients[i].getFd() << "> Disconnected" << RESET << std::endl;
		close(clients[i].getFd());
	}
	if (ServSockFd == -1)
		return ;
	std::cout << RED << "Server <" << ServSockFd << "> Disconnected" << RESET << std::endl;
	close(ServSockFd);

}

void Server::clearClients(int fd){
	for (size_t i = 0; i < fds.size(); i++){
		if(fds[i]. fd == fd){
			fds.erase(fds.begin()) + i;
			break ;
		}
	}
	for (size_t i = 0; i < clients.size(); i++){
		if (clients[i].getFd() == fd){
			clients.erase(clients.begin() + i);
			break ;
		}
	}
}

Server::~Server(){}