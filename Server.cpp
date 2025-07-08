#include "Server.hpp"

bool Server::signal = false;
Server::Server(){
	ServSockFd = -1;
}

void Server::serverInit(int newPort, std::string newPassword){
	if (newPort > 65535) // TODO parsing
		throw (std::runtime_error("invalid port"));
	if (newPassword.empty()) // TODO parsing
		throw (std::runtime_error("invalid password"));
	port = newPort;
	password = newPassword;
	servSock();
	std::cout << GREEN << "Server <" << ServSockFd << "> Connected" << RESET << std::endl;
	std::cout << "Waiting to accept a connection ..." << std::endl;
	while (true){
		if (poll(&fds.at(0), fds.size(), -1) < 0)
			throw (std::runtime_error("Poll() failed"));
		for (size_t i = 0; i < fds.size(); i++){
			if ((fds[i].revents & POLLIN) == true){
				if (fds[i].fd == ServSockFd)
					acceptNewClient();
				else
					recvNewData(fds[i].fd);
			}
			if ((fds[i].revents & POLLOUT) == true) // clients not active
				
		}
	}
}

void Server::servSock(){
	struct sockaddr_in add;
	struct pollfd NewPoll;
	std::memset(&add, 0, sizeof(add));
	add.sin_family = AF_INET; // set address familiy to ipv4
	add.sin_port = htons(this->port); //convert the port to network byte order (big endian)
	add.sin_addr.s_addr = INADDR_ANY; //set the address to any local machine address

	ServSockFd = socket(PF_INET, SOCK_STREAM, 0); // create server socket
	if (ServSockFd < 0)
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

//TODO provavelmente add username and stuff
void Server::acceptNewClient(){
	std::string msg;
	Client a;
	a.setFd(accept(ServSockFd, NULL, NULL));
	if (a.getFd() < 1)
		throw(std::runtime_error("faild to accept client"));
	set_nonblocking(a.getFd());
	struct pollfd client_fd;
	client_fd.fd = a.getFd();
	client_fd.events = POLLOUT;
	sendMsg(a.getFd(), "Password:", 9);
	// wait for response check password
	// char buffer[BUFFER_SIZE];
	// if (msg != password){
	// 	close(a.getFd());
	// 	return ;}
	// client_fd.events = POLLIN;
	fds.push_back(client_fd);
	clients.map::insert(a.getFd(), a);
	std::cout << GREEN << "New client connected: fd " << a.getFd() << RESET << std::endl;
}

void Server::sendMsgAll(int fd_client, const char *buffer, size_t len){
	for (size_t i = 0; i < fds.size(); i++){
		if (fds[i].fd != fd_client)
			sendMsg(fds[i].fd, buffer, len);
	}
}

void Server::recvData(int fd){
	char buffer[BUFFER_SIZE];
	ssize_t bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0); // n bytes lidos
	if (bytes <= 0){
		std::cout << "Client disconnected: fd " << fd << std::endl;
		for (size_t i = 0; i < fds.size(); i++){
			if (fds[i].fd == fd){
				fds.erase(fds.begin() + i);
				close(fd);
				return ;
			}
		}
	}
	buffer[bytes] = 0;
	std::string msg(buffer); // crio um objeto do tipo string.
	if (password != msg){
		close(fd);
		clients.find(fd);
		return ;
	}


}

void Server::recvNewData(int fd){
	char buffer[BUFFER_SIZE];
	ssize_t bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0); // n bytes lidos
	if (bytes <= 0){
		std::cout << "Client disconnected: fd " << fd << std::endl;
		for (size_t i = 0; i < fds.size(); i++){
			if (fds[i].fd == fd){
				fds.erase(fds.begin() + i);
				close(fd);
				return ;
			}
		}
	}
	buffer[bytes] = 0;
	std::string msg(buffer); // crio um objeto do tipo string.
	std::cout << "[fd " << fd << "] " << msg; // escrevo a mensagem recebida.
	std::string response = ":server PONG :" + msg; // crio um outro objeto tipo string estilo protocolo irc.
	this->sendMsgAll(fd, response.c_str(), response.size());  // o cliente recebe a confirmacao da mensagem que enviou.
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
