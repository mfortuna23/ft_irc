#include "irc.hpp"
#include "Server.hpp"

bool Server::isThisCmd(const std::string& line, std::string cmd){
	std::istringstream iss(line);
	std::string firstWord;
	if (iss >> firstWord)
		return (toUpper(firstWord) == cmd);
	return false;
}

// CAP <capabilities>
void Server::cmdCAP(Client *cli, std::string line) {
	(void)line;
	std::string reply = "CAP * LS :\r\n";
	sendMsg(cli->getFd(), reply.c_str(), reply.size());
}

// PASS <password>
void Server::cmdPASS(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, pass;
	iss >> cmd >> pass;

	if (cli->get_is_registered()) {
		sendMsg(cli->getFd(), "ERROR :You are already registered\r\n", 36);
		return;}
	if (pass.empty()) {
		sendMsg(cli->getFd(), "ERROR :No password given\r\n", 27);
		return;}
	if (pass != this->get_pass()) {
		sendMsg(cli->getFd(), "ERROR :Password incorrect\r\n", 28);
		close(cli->getFd());
		return;}
	if (cli->get_regist_steps() == 3)
		cli->confirm_regist_step(this);
}

// NICK <nickname>
void Server::cmdNICK(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, nick;

	if (cli->get_regist_steps() > 2)
	{	
		sendMsg(cli->getFd(), "ERROR :Enter PASS first\r\n", 26);
		return ;
	}

	iss >> cmd >> nick;

	if (nick.empty()) {
		sendMsg(cli->getFd(), "ERROR :No nickname given\r\n", 27);
		return;
	}

	std::string old_nick = cli->get_nick();
	std::string msg = ":" + old_nick + " NICK :" + nick + "\r\n";
	sendMsg(cli->getFd(), msg.c_str(), msg.length());
	cli->set_nickname(nick);

	//std::string msg = "You're now known as " + cli->get_nick() + "\r\n";
	//sendMsg(cli->getFd(), msg.c_str(), msg.size());
	if (cli->get_regist_steps() == 2)
		cli->confirm_regist_step(this);
}

// USER <username> 0 * :realname
void Server::cmdUSER(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, user, unused, asterisk, realname;

	if (cli->get_regist_steps() > 2)
	{	
		sendMsg(cli->getFd(), "ERROR :Enter PASS first\r\n", 26);
		return ;
	}
	
	iss >> cmd >> user >> unused >> asterisk;
	std::getline(iss, realname); //getline serve para capturar tudo que vem depois dos 4 primeiros campos, mesmo que contenha espaços.
	// como nao vamos nos aprofundar muito, nao precisamos salvar nada alem do user.
	if (user.empty()) {
		sendMsg(cli->getFd(), "ERROR :No username given\r\n", 27);
		return;
	}
	cli->set_username(user);
	std::string msg = "Your username now is " + cli->get_user() + "\r\n";
	sendMsg(cli->getFd(), msg.c_str(), msg.size());
	
	if (cli->get_regist_steps() == 1)
		cli->confirm_regist_step(this);
}

Channel* Server::getChannelByName(std::string name)
{
	for (size_t i = 0; i < channels.size(); ++i){
		if (channels[i]->getName() == name)
			return channels[i];
	}
	return NULL;
}


void	Server::cmdJOIN(Client *a, std::string line){
	std::stringstream msg;
	if (a->get_regist_steps() != 0){
		msg << RED << "Error" << RESET << "\r\n";
		sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	std::istringstream iss(line);
	std::string cmd, channel, keys;
	iss >> cmd >> channel;
	// if (iss.peek() == ':')
	// 	iss.ignore();
	// std::getline(iss, keys);
	keys.erase(0, keys.find_first_not_of(" \t\n\r:"));
	std::istringstream chanStream(channel);
	std::istringstream keyStream(keys);
	std::vector<std::string> allKeys;
	int i = 0;
	while (std::getline(keyStream, keys, ','))
		allKeys.push_back(keys);
	for (size_t x = 0; x < 100; x++)
		allKeys.push_back("");
	while (std::getline(chanStream, channel, ',')) {
		channel.erase(0, channel.find_first_not_of(" \t"));
		channel.erase(channel.find_last_not_of(" \t") + 1);
		if (!channel.empty() /* && (channel[0] == '#' || channel[0] == '&') */) {
			Channel *c = getChannelByName(channel);
			if (!c){
				channels.push_back(new Channel(channel, a, allKeys[i]));
				c = channels.back();
				++i;
			}
			else{
				if (c->getPwd().empty())
					c->addClient(a);
				else
					c->addClient(a, allKeys[i++]);
			}
		}
		else{
			msg << RED << "Error" << RESET << "\r\n"; //PROTOCOL 
			sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		}
	}
}

void Server::cmdQUIT(Client *a, std::string line){
	(void)line;

	std::stringstream msg;
	msg << startMsg(a) << " QUIT :Leaving\r\n";
	std::string quit_msg = msg.str();

	// copia o map para uma variável local, oque evita modificar ou iterar diretamente sobre o map original 
	std::map<std::string, Channel*> chansMap = a->getChannels();

	// constrói um vetor de ponteiros apenas para Channel, assim temos de forma segura nesse vetor todos os canais aos quais o cliente pertence no momento do QUIT
	std::vector<Channel*> chans;
	for (std::map<std::string, Channel*>::iterator it = chansMap.begin(); it != chansMap.end(); ++it)
		chans.push_back(it->second);

	std::vector<std::string> toRemove;
	for (size_t i = 0; i < chans.size(); ++i) {
		Channel *ch = chans[i];
		ch->rmClient(a); // Não queremos que este membro seja notificado
		// notifica os outros membros
		ch->sendMsgChannel(quit_msg);
		// std::map<int, Client*> members = ch->getClients();
		// for (std::map<int, Client*>::iterator cit = members.begin(); cit != members.end(); ++cit) {
		// 	if (cit->second->getFd() != a->getFd()) // envia para todos menos para quem saiu
		// 		sendMsg(cit->second->getFd(), quit_msg.c_str(), quit_msg.size());
		// }

		//ch->rmClient(a);// podemos alterar (remover) o cliente com seguranca, pois ja temos a lista dos canais que ele pertence

		if (ch->getClients().empty()) // verifica se o canal nao tem mais nenhum cliente nele
			toRemove.push_back(ch->getName());
	}

	// remove os canais vazios
	for (size_t i = 0; i < toRemove.size(); ++i) {
		for (size_t j = 0; j < channels.size(); ++j) {
			if (channels[j]->getName() == toRemove[i]) {
				channels.erase(channels.begin() + j);
				break;
			}
		}
	}

	sendMsg(a->getFd(), quit_msg.c_str(), quit_msg.size());
	close(a->getFd());
	clearClients(a->getFd());
}

void Server::cmdPRIVMSG(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, target, message;

	iss >> cmd >> target;

	if (target.empty()) {
		sendMsg(cli->getFd(), "ERROR :No recipient given (PRIVMSG)\r\n", 36);
		return;
	}

	size_t pos = line.find(" :");
	if (pos != std::string::npos)
		message = line.substr(pos + 2); // ignora o " :"
	else {
		sendMsg(cli->getFd(), "ERROR :No text to send\r\n", 25);
		return;
	}

	if (target[0] == '#') {
		Channel* chan = getChannelByName(target);
		if (!chan) {
			sendMsg(cli->getFd(), "ERROR :No such channel\r\n", 26);
			return;
		}
		std::map<int, Client*> clients = chan->getClients();
		if (clients.count(cli->getFd()) == 0) // if the client isnt in the channel, he cant send the msg.
		{
			std::ostringstream err;
			err << ":server 404 " << cli->get_nick() << " " << target << " :Cannot send to channel\r\n";
			sendMsg(cli->getFd(), err.str().c_str(), err.str().size());
			return;
		}
		for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
			if (it->second->getFd() != cli->getFd()) {
				std::ostringstream msg;
				msg << ":" << cli->get_nick() << " PRIVMSG " << target << " :" << message << "\r\n";
				sendMsg(it->second->getFd(), msg.str().c_str(), msg.str().size());
			}
		}
	} else {
		Client* dest = getClientByNick(target);
		if (!dest) {
			sendMsg(cli->getFd(), "ERROR :No such nick/channel\r\n", 30);
			return;
		}
		std::ostringstream msg;
		msg << ":" << cli->get_nick() << " PRIVMSG " << target << " :" << message << "\r\n";
		sendMsg(dest->getFd(), msg.str().c_str(), msg.str().size());
	}
}

void Server::cmdNOTICE(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, target, message;

	iss >> cmd >> target;

	if (target.empty())
		return; // NOTICE nunca envia mensagem de erro, testado no irc libera chat

	size_t pos = line.find(" :"); // verificamos primeiro a partir do identificador de mensagem
	if (pos != std::string::npos)
		message = line.substr(pos + 2); // removemos o identificador de mensagem
	else
		return; // sem mensagem, nao faz nada

	if (message.empty())
		return;

	if (target[0] == '#') {
		Channel* chan = getChannelByName(target);
		if (!chan)
			return; // canal nao existe

		std::map<int, Client*> clients = chan->getClients(); // busca todos os clientes atualmente conectados no canal
		for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
			if (it->second->getFd() != cli->getFd()) {
				std::ostringstream msg;
				msg << ":" << cli->get_nick() << " NOTICE " << target << " :" << message << "\r\n"; // padrao irc
				sendMsg(it->second->getFd(), msg.str().c_str(), msg.str().size());
			}
		}
	} else {
		Client* dest = getClientByNick(target);
		if (!dest)
			return;
		std::ostringstream msg;
		msg << ":" << cli->get_nick() << " NOTICE " << target << " :" << message << "\r\n"; // padrao irc
		sendMsg(dest->getFd(), msg.str().c_str(), msg.str().size());
	}
}

void Server::cmdPING(Client *cli, std::string line) {
	std::string token;
	std::istringstream iss(line);
	iss >> token; // ignora "PING"

	std::string payload;
	size_t pos = line.find(" :");
	if (pos != std::string::npos)
		payload = line.substr(pos + 2);
	else {
		// fallback: tenta pegar o segundo argumento, mesmo sem :
		iss >> payload;
	}

	if (payload.empty())
		return; // PING sem argumento → ignorar

	std::ostringstream reply;
	reply << "PONG :" << payload << "\r\n";
	sendMsg(cli->getFd(), reply.str().c_str(), reply.str().size());
}

//leave channels
void Server::cmdPART(Client *a, std::string line){
	std::stringstream msg;
	if (a->get_regist_steps() != 0){
		msg << RED << "Error" << RESET << "\r\n";
		sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	std::istringstream iss(line);
	std::string cmd, channel, leave;
	iss >> cmd >> channel;
	std::istringstream chanStream(channel);
	Channel *tv;
	std::getline(iss, leave);
	while (std::getline(chanStream, channel, ',')) {
		channel.erase(0, channel.find_first_not_of(" \t"));
		channel.erase(channel.find_last_not_of(" \t") + 1);
		if (!channel.empty() /* && (channel[0] == '#' || channel[0] == '&') */) {
			tv = getChannelByName(channel);
			if (tv && tv->rmClient(a)){
				msg << startMsg(a) << " PART " << tv->getName() << " " << leave << "\r\n";
				tv->sendMsgChannel(msg.str());
				msg.str("");
				msg.clear();
			}
		}
		else{
			msg << RED << "Error" << RESET << "\r\n"; //PROTOCOL
			sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		}
	}
}
