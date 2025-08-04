#include "Server.hpp"

bool Server::signal = false;
bool Server::running = true;
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
	while (running){
		if (poll(&fds.at(0), fds.size(), -1) < 0)
			throw (std::runtime_error("Poll() failed"));
		for (size_t i = 0; i < fds.size(); i++){
			if ((fds[i].revents & POLLIN) == true){
				if (fds[i].fd == ServSockFd)
					acceptNewClient();
				else
					recvNewData(fds[i].fd); // comd recv
			}
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

void Server::acceptNewClient(){
	struct sockaddr_in client_addr; // 
	socklen_t addr_len = sizeof(client_addr);

	int client_fd = accept(ServSockFd, (struct sockaddr*)&client_addr, &addr_len);// aceite essa nova conexão e me diga: quem está conectando?
																				// accept preenche client_addr.sin_addr → IP do cliente
																				// e preenche client_addr.sin_port → porta usada pelo cliente
	if (client_fd < 0)
		throw(std::runtime_error("failed to accept client"));
	set_nonblocking(client_fd);

	// adiciona fd ao poll
	struct pollfd client_pfd; // troquei de client_fd para client_pfd para ficar mais claro que se refere ao poll de fds.
	client_pfd.fd = client_fd;
	client_pfd.events = POLLIN;
	client_pfd.revents = 0;
	fds.push_back(client_pfd);

	// cria e armazena o client
	std::string ip = inet_ntoa(client_addr.sin_addr); // salvamos o ip que accept preencheu
	Client new_client(client_fd, ip); // crio um objeto client e salvo seu fd e seu ip;
	clients.push_back(new_client); // coloco ele no vetor de clients.

	std::cout << GREEN << "New client connected: fd " << client_fd << " | IP: " << ip << RESET << std::endl;
}

Client* Server::getClientByFd(int fd) // nova funcao para selecionar o client que queremos.
{
	for (size_t i = 0; i < clients.size(); ++i)
	{
		if (clients[i].getFd() == fd)
			return &clients[i];
	}
	return NULL;
}

void Server::recvNewData(int fd)
{
	Client* cli = getClientByFd(fd);
	if (!cli)
		return;
	char tmp_buffer[BUFFER_SIZE];
	ssize_t bytes = recv(fd, tmp_buffer, BUFFER_SIZE, 0); // n bytes lidos
	if (bytes <= 0) {
		std::cout << "Client disconnected: fd " << fd << std::endl;
		close(fd); // eh melhor fechar o fd primeiro para torna-lo invalido imediatamente.
		clearClients(fd); // apesar de fechado o fd continua com o mesmo numero, so nao eh mais valido no kernel
		return;
	}
	cli->get_buffer().append(tmp_buffer, bytes);

	// verifica se o buffer ultrapassou 512 sem um '\n' → desconecta
	if (cli->get_buffer().size() > 512) {
		std::string err = "ERROR :Line too long\r\n";
		send(fd, err.c_str(), err.size(), 0);
		std::cout << RED << "Line too long from fd " << fd << ". YOU ARE NOT following IRC protocol. Disconnecting." << RESET << std::endl;
		close(fd);
		clearClients(fd);
		return;
	}

	while (true){
		size_t pos = cli->get_buffer().find("\r\n");
		if (pos != std::string::npos) // se encontro \r\n
		{
			std::string line = cli->get_buffer().substr(0, pos); // cria uma substring da posicao 0 até \r\n (sem inclui-lo)
			cli->get_buffer().erase(0, pos + 2); // remove os caracteres da string de 0 ate o \r\n (incluidos)
			std::cout << "[fd " << fd << "] " << line << std::endl; //TODO apenas para debug
			handleCommand(cli, line);
		}
		else
			break;
	}
}

void Server::handleCommand(Client *a, std::string line){
	std::string cmds[15] = {"PASS", "NICK", "USER", "CAP", "PING", "PONG", "QUIT", "JOIN", "KICK",
	"INVITE", "TOPIC", "MODE", "PRIVMSG", "NOTICE", "PART"};
	void (Server::*fCmds[15])(Client *, std::string) = {&Server::cmdPASS, &Server::cmdNICK, &Server::cmdUSER, 
		&Server::cmdCAP, &Server::cmdPING, &Server::voidCmd, &Server::cmdQUIT, &Server::cmdJOIN, 
		&Server::voidCmd, &Server::voidCmd, &Server::voidCmd, &Server::voidCmd, &Server::cmdPRIVMSG, &Server::cmdNOTICE, &Server::cmdPART};
	for (size_t i = 0; i < 15; i++){
		if (isThisCmd(line, cmds[i])){
			std::cout << "ive recived " << cmds[i] << std::endl;
			(this->*fCmds[i])(a, line);
			return ;
		}
	}
	std::string response = "ERROR :Unknown command\r\n";
	sendMsg(a->getFd(), response.c_str(), response.size());
}


//TODO add channel
void Server::sendMsgAll(int fd_client, const char *buffer, size_t len){
	for (size_t i = 0; i < fds.size(); i++){
		if (fds[i].fd != fd_client )
			//if (getClientByFd(fd_client)->get_is_registered()) //TODO REGISTERED client
				sendMsg(fds[i].fd, buffer, len);
	}
}

void Server::signalHandler (int signum){
	(void)signum; // any signal ...
	std::cout << std::endl << RED << "Signal recived!" << RESET << std::endl;
	Server::signal = true;
	running = false;
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
		if(fds.at(i).fd == fd){
			fds.erase(fds.begin() + i);
			break ;
		}
	}
	for (size_t i = 0; i < clients.size(); i++){
		if(clients.at(i).getFd() == fd){
			clients.erase(clients.begin() + i);
			break ;
		}
	}
}

std::string Server::get_pass(){ return password;}

void Server::voidCmd(Client *a, std::string line){(void)a; (void)line;}

void Server::checkRegistration(Client *cli)
{
	if (cli->get_is_registered() && !cli->get_nick().empty()) {
		std::string msg = ":server 001 " + cli->get_nick() + " :Welcome to the IRC server\r\n";
		sendMsg(cli->getFd(), msg.c_str(), msg.size());
	}
}

Client* Server::getClientByNick(const std::string& nick) {
	for (size_t i = 0; i < clients.size(); ++i) {
		if (clients[i].get_nick() == nick)
			return &clients[i];
	}
	return NULL;
}


Server::~Server(){
	for (size_t i = 0; i < channels.size(); ++i) // libera os ponteiros no destrutor de Serve
		delete channels[i];
	channels.clear();
}
