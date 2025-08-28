#include "Server.hpp"

bool	Server::signal = false;
bool	Server::running = true;
Server* Server::instance = NULL;
Server::Server() : ServSockFd (-1) {
	cleaned = false;
}

void Server::serverInit(int newPort, std::string newPassword){
	if (newPort < 6665 || newPort > 6669) //so aceitamos ports recomendados
		throw (std::runtime_error("invalid port"));
	if (newPassword.empty())
		throw (std::runtime_error("invalid password"));
	port = newPort;
	password = newPassword;
	servSock();
	std::cout << GREEN << "Server <" << ServSockFd << "> Connected" << RESET << std::endl;
	std::cout << "Waiting to accept a connection ..." << std::endl;
	// registra instância para sendMsg() enfileirar os dados que irao ser escritos, mantendo acesso a essa instacia Server.
	Server::instance = this;
	while (running){
		if (poll(&fds.at(0), fds.size(), -1) < 0) {
			if (errno == EINTR) break; 
			throw (std::runtime_error("Poll() failed"));
		}
		for (size_t i = 0; i < fds.size(); i++)
		{
			if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
				int fd = fds[i].fd;
				close(fd);
				clearClients(fd);
				continue;
			}
			if (fds[i].revents & POLLIN){
				if (fds[i].fd == ServSockFd)
					acceptNewClient();
				else
					recvNewData(fds[i].fd); // comd recv
			}
			// escreveremos ao fazer flush do outbox quando POLLOUT ficar pronto
			if (fds[i].revents & POLLOUT){
				int fd = fds[i].fd;
				Client* cli = getClientByFd(fd);
				if (!cli) { 
					disableWriteEvent(fd);
					continue;
				}
				bool fatal = false;
				while (cli && !cli->get_outbox().empty()){
					ssize_t n = send(fd, cli->get_outbox().c_str(), cli->get_outbox().size(), 0);
					if (n > 0) {
						cli->get_outbox().erase(0, static_cast<size_t>(n));
					} else {
						if (errno == EAGAIN || errno == EWOULDBLOCK)
							break; // tenta de novo na próxima iteração
						// erro fatal -> encerra
						std::cout << RED << "Send error on fd " << fd << RESET << std::endl;
						cmdQUIT(cli, "quit");
						fatal = true;
						break;
					}
				}
				if (fatal)
					continue; // fds[] mudou, segue o loop externo
				cli = getClientByFd(fd);
				if (cli && cli->get_outbox().empty()){
					disableWriteEvent(fd);
					if (cli->get_want_close()){
						close(fd);
						clearClients(fd);
					}
				}
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

// --- helpers de escrita controlada pelo poll() único ---
void Server::enableWriteEvent(int fd) {
		for (size_t i = 0; i < fds.size(); ++i){
			if (fds[i].fd == fd){
				fds[i].events |= POLLOUT;
				return;
			}
		}
}

void Server::disableWriteEvent(int fd) {
		for (size_t i = 0; i < fds.size(); ++i){
			if (fds[i].fd == fd){
				fds[i].events &= ~POLLOUT; // para de monitorar se o fd está pronto para escrita
				return;
			}
		}
}
void Server::enqueueSend(int fd, const char* buf, size_t len) {
		if (!buf || len == 0)
			return;
		Client* cli = getClientByFd(fd);
		if (!cli)
			return;
		cli->get_outbox().append(buf, len);
		enableWriteEvent(fd);
}
// ---- // ---- //

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
	Client* new_client = new Client(client_fd, ip);
	//Client new_client(client_fd, ip); // crio um objeto client e salvo seu fd e seu ip; 
	clients.push_back(new_client); // coloco ele no vetor de clients.

	std::cout << GREEN << "New client connected: fd " << client_fd << " | IP: " << ip << RESET << std::endl;
}

Client* Server::getClientByFd(int fd) // nova funcao para selecionar o client que queremos.
{
	for (size_t i = 0; i < clients.size(); ++i)
	{
		if (clients[i]->getFd() == fd)
			return clients[i];
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
		cmdQUIT(cli, "quit");
		return;
	}
	cli->get_buffer().append(tmp_buffer, bytes);

	while (true){
		size_t pos = cli->get_buffer().find("\r\n");
		if (pos != std::string::npos) // se encontro \r\n
		{
			if (pos > 510) { // a verificaçao deve ficar dentro do loop para estar sempre checando se a linha passou de 512 bytes ("\r\n" incluidos)
				std::string err = "ERROR :Line too long\r\n";
				sendMsg(fd, err.c_str(), err.size());
				std::cout << RED << "Line too long from fd " << fd << ". NOT following IRC protocol. Disconnecting." << RESET << std::endl;
				cmdQUIT(cli, "quit");
				return;
			}

			std::string line = cli->get_buffer().substr(0, pos); // cria uma substring da posicao 0 até \r\n (sem inclui-lo)
			cli->get_buffer().erase(0, pos + 2); // remove os caracteres da string de 0 ate o \r\n (incluidos)
			std::cout << "[fd " << fd << "] " << line << std::endl; // apenas para debug
			handleCommand(cli, line);
			cli = getClientByFd(fd);
			if (!cli) // verfica se o cliente ja foi desconectado
				return;
		}
		else {
			if (cli->get_buffer().size() > MAX_INPUT_BUFFER) {
				std::string err = "ERROR :Input buffer overflow\r\n";
				sendMsg(fd, err.c_str(), err.size());
				cmdQUIT(cli, "quit");
			}
			break;
		}
	}
}

void Server::handleCommand(Client *cli, std::string line){
	if (line.empty()) // nao precisamos de lidar
		return ;
	std::string cmds[17] = {"PASS", "NICK", "USER", "CAP", "QUIT", "PONG", "PING", "JOIN", "KICK",
	"INVITE", "TOPIC", "MODE", "PRIVMSG", "NOTICE", "PART", "WHO", "WHOIS"};
	void (Server::*fCmds[17])(Client *, std::string) = {&Server::cmdPASS, &Server::cmdNICK, &Server::cmdUSER, 
		&Server::cmdCAP, &Server::cmdQUIT, &Server::voidCmd, &Server::cmdPING, &Server::cmdJOIN, 
		&Server::cmdKICK, &Server::cmdINVITE, &Server::cmdTOPIC, &Server::cmdMODE, &Server::cmdPRIVMSG, 
		&Server::cmdNOTICE, &Server::cmdPART, &Server::voidCmd, &Server::cmdWHOIS};
	for (size_t i = 0; i < 17; i++){
		if (isThisCmd(line, cmds[i])){
			if (i > 5 && !cli->get_is_registered())
				return ERR_NOTREGISTERED(cli); //cliente so pode user pass, nick, user, cap ou quit antes do registo
			std::cout << "ive recived " << cmds[i] << std::endl;
			(this->*fCmds[i])(cli, line);
			return ;
		}
	}
	std::string response = ":server 421 " + cli->get_nick() + (line.empty() ? "" : line) + " :Unknown command\r\n";
	sendMsg(cli->getFd(), response.c_str(), response.size());
}

void Server::signalHandler (int signum){
	(void)signum;
	Server::signal = true;
	running = false;
}

void Server::closeFds(){
	if (cleaned)
		return;
	for (size_t  i = 0; i < clients.size(); i++){
		if (clients[i]){
			std::cout << RED << "Client <" << clients[i]->getFd() << "> Disconnected" << RESET << std::endl;
			close(clients[i]->getFd());
			delete clients[i];
		}
	}
	clients.clear();
	fds.clear();
	// fecha o socket do servidor se ainda aberto
	if (ServSockFd != -1) {
		std::cout << RED << "Server <" << ServSockFd << "> Disconnected" << RESET << std::endl;
		close(ServSockFd);
		ServSockFd = -1;
	}
	cleaned = true;
}

void Server::clearClients(int fd){
	// tira do poll
	for (size_t i = 0; i < fds.size(); i++){
		if(fds.at(i).fd == fd){
			fds.erase(fds.begin() + i);
			break ;
		}
	}
	// acha o Client*
	for (size_t i = 0; i < clients.size(); i++){
		if(clients[i]->getFd() == fd){
			Client* c = clients[i];

			//  retirar de todos os canais
			std::map<std::string, Channel *> chans = c->getChannels();
			for (std::map<std::string, Channel *>::iterator it = chans.begin(); it != chans.end(); ++it) {
				Channel *ch = it->second;
				if (!ch)
					continue;
				ch->rmClient(c);
				if (ch->getClients().empty()) {
					// deletar canal vazio
					for (size_t j = 0; j < channels.size(); ++j) {
						if (channels[j] == ch) {
							delete channels[j];
							channels.erase(channels.begin() + j);
							break;
						}
					}
				}
			}
			// agora é seguro deletar o Client
			std::cout << RED << "[clearClients] Deleting fd " << fd << RESET << std::endl;
			delete c;
			clients.erase(clients.begin() + i);
			break ;
		}
	}
}

std::string Server::get_pass(){ return password;}

void Server::voidCmd(Client *cli, std::string line){(void)cli; (void)line;}

void Server::tryFinishRegistration(Client* cli) {
    if (!cli) return;
    // PASS precisa ter sido aceito antes: no contador isso significa steps <= 2
    if (!cli->get_is_registered() && cli->get_regist_steps() <= 2 && !cli->get_nick().empty() && !cli->get_user().empty())
    {
        cli->set_registration(true);     // marca como registrado
        checkRegistration(cli);
    }
}

void Server::checkRegistration(Client *cli)
{
	if (!cli->get_is_registered() || cli->get_nick().empty() || cli->get_user().empty()) 
		return ;
	// 001 RPL_WELCOME
	std::string msg = ":server 001 " + cli->get_nick() + " :Welcome to the IRC server\r\n";
	sendMsg(cli->getFd(), msg.c_str(), msg.size());
	std::string msg2 = "You're now known as " + cli->get_nick() + "\r\n";
	sendMsg(cli->getFd(), msg2.c_str(), msg2.size());
}

Client* Server::getClientByNick(const std::string& nick) {
	for (size_t i = 0; i < clients.size(); ++i) {
		if (clients[i]->get_nick() == nick)
			return clients[i];
	}
	return NULL;
}

Server::~Server(){
	for (size_t i = 0; i < channels.size(); ++i) // libera os ponteiros no destrutor de Serve
		delete channels[i];
	channels.clear();
	closeFds();
}

