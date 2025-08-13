#include "irc.hpp"

bool Server::isThisCmd(const std::string& line, std::string cmd){
	std::istringstream iss(line);
	std::string firstWord;
	if (iss >> firstWord)
		return (toUpper(firstWord) == cmd);
	return false;
}

// CAP <capabilities>
void Server::cmdCAP(Client *cli, std::string line) {
	std::istringstream iss(line);
    std::string cmd, sub; 
    iss >> cmd >> sub;

	if (toUpper(sub) == "LS") {
		std::string reply = "CAP * LS :\r\n";
		sendMsg(cli->getFd(), reply.c_str(), reply.size());
	}
		return ;
}

// PASS <password>
void Server::cmdPASS(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, pass;
	iss >> cmd >> pass;

	if (cli->get_is_registered()) {
		sendMsg(cli->getFd(), ":server 462 :You are already registered\r\n", 42);
		return;}
	if (pass.empty()) {
		sendMsg(cli->getFd(), ":server 461 :No password given\r\n", 33);
		return;}
	if (pass != this->get_pass()) {
		sendMsg(cli->getFd(), ":server 464 :Password incorrect\r\n", 34);
		close(cli->getFd());
		clearClients(cli->getFd());
		return;}
	if (cli->get_regist_steps() == 3)
		cli->confirm_regist_step(this);
}

// NICK <nickname>
void Server::cmdNICK(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, nick;

	iss >> cmd >> nick;

	if (nick.empty()) {
		sendMsg(cli->getFd(), ":server 431 :No nickname given\r\n", 33);
		return;
	}

	// bloqueio de nicks duplicados (433)
	Client* holder = getClientByNick(nick);
	if (holder && holder != cli) {
		std::string err;
		// formato comum: ":server 433 <seuNickOu*> <nick> :Nickname is already in use"
		err = ":server 433 " + (cli->get_nick().empty() ? "*" : cli->get_nick()) + " " + nick + " :Nickname is already in use\r\n";
		sendMsg(cli->getFd(), err.c_str(), err.size());
		return; // NÃO avançar passo de registro
	}
	std::string old_nick = cli->get_nick();
	if (!old_nick.empty() && old_nick != nick){
		std::string msg = ":" + old_nick + " NICK :" + nick + "\r\n";
		sendMsg(cli->getFd(), msg.c_str(), msg.length());
	}
	cli->set_nickname(nick);

	std::string msg = "You're now known as " + cli->get_nick() + "\r\n";
	sendMsg(cli->getFd(), msg.c_str(), msg.size());
	// if (cli->get_regist_steps() == 2)
	// 	cli->confirm_regist_step(this);
	tryFinishRegistration(cli);
}

// USER <username> 0 * :realname
void Server::cmdUSER(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, user, unused, asterisk, realname;

	if (cli->get_is_registered()) {
		sendMsg(cli->getFd(), ":server 462 :You may not reregister\r\n", 38);
		return;
	}
	iss >> cmd >> user >> unused >> asterisk;
	std::getline(iss, realname); //getline serve para capturar tudo que vem depois dos 4 primeiros campos, mesmo que contenha espaços.
	// como nao vamos nos aprofundar muito, nao precisamos salvar nada alem do user.
	if (user.empty()) {
		sendMsg(cli->getFd(), ":server 461 :No username given\r\n", 33);
		return;
	}
	cli->set_username(user);
	std::string msg = "Your username now is " + cli->get_user() + "\r\n";
	sendMsg(cli->getFd(), msg.c_str(), msg.size());
	
	// if (cli->get_regist_steps() == 1)
	// 	cli->confirm_regist_step(this);
	tryFinishRegistration(cli);
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
	if (!a->get_is_registered())
		return sendErrorRegist(a);

	std::istringstream iss(line);
	std::string cmd, channel, keys;
	iss >> cmd >> channel;
	if (channel.empty()){
		msg << ":server 461 " << a->get_nick() << " JOIN :Not enough parameters\r\n";
		sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	std::getline(iss, keys);
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
		if (!channel.empty() && (channel[0] == '#' || channel[0] == '&')) {
			Channel *c = getChannelByName(channel);
			if (!c){
				channels.push_back(new Channel(channel, a)); // keys can only be added after the channel is created
				c = channels.back();
				c->sendNamesTo(a);
				++i;
			} else {
				if (c->getPwd().empty())
					c->addClient(a);
				else
					c->addClient(a, allKeys[i++]);
			}
		} else if (channel[0] != '#' || channel[0] == '&'){
				msg << ":server 476 " << a->get_nick() << " " << channel << " :Bad Channel Mask\r\n";
				sendMsg(a->getFd(), msg.str().c_str(), msg.str().size()); msg.str(""); msg.clear();
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
		ch->rmClient(a);
		ch->sendMsgChannel(quit_msg);
		if (ch->getClients().empty())
			toRemove.push_back(ch->getName());
	}
	// remove os canais vazios
	for (size_t i = 0; i < toRemove.size(); ++i) {
		for (size_t j = 0; j < channels.size(); ++j) {
			if (channels[j]->getName() == toRemove[i]) {
				delete channels[j];
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

	if (!cli->get_is_registered())
		return sendErrorRegist(cli);

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

	if (target[0] == '#' || target[0] == '&') {
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

	if (target[0] == '#' || target[0] == '&') {
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
	if (!a->get_is_registered())
		return sendErrorRegist(a);
	std::istringstream iss(line);
	std::string cmd, channel, leave;
	iss >> cmd >> channel;
	std::getline(iss, leave);

	// normaliza motivo: precisa começar com ':'
    // (irssi só mostra o reason se vier com ':')
    size_t p = leave.find_first_not_of(" \t");
    if (p == std::string::npos)
		leave = ":Leaving";
    else {
        leave = leave.substr(p);
        if (!leave.empty() && leave[0] != ':')
            leave.insert(0, ":");
    }

	std::istringstream chanStream(channel);


	Channel *tv;
	while (std::getline(chanStream, channel, ',')) {
		channel.erase(0, channel.find_first_not_of(" \t"));
		channel.erase(channel.find_last_not_of(" \t") + 1);
		if (!channel.empty()) {
			tv = getChannelByName(channel);
			if (!tv) { // canal não existe
				msg.str(""); msg.clear();
				msg << ":server 403 " << a->get_nick() << " " << channel
					<< " :No such channel\r\n";
				sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
				continue;
			}
			if (!tv->isMember(a)) { // user não está no canal
				msg.str(""); msg.clear();
				msg << ":server 442 " << a->get_nick() << " " << channel
					<< " :You're not on that channel\r\n";
				sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
				continue;
			}
			if (tv && tv->rmClient(a)){
				msg.str(""); msg.clear();
				msg << startMsg(a) << " PART " << tv->getName() << " " << leave << "\r\n";
				std::string part_msg = msg.str();
				tv->sendMsgChannel(part_msg);
				sendMsg(a->getFd(), part_msg.c_str(), part_msg.size());
				msg.str("");
				msg.clear();
			}
		}
	}
	// remover canal
	std::vector<std::string> toRemove; // temp list
	for (size_t i = 0; i < channels.size(); ++i) { //busca canais sem membros
	    if (channels[i]->getClients().empty()) // se não há mais clientes
	        toRemove.push_back(channels[i]->getName());// guardamos o nome do channel
	}
	for (size_t k = 0; k < toRemove.size(); ++k) { // percorre a lista temporaria de nomes a remover
   		for (size_t i = 0; i < channels.size(); ++i) {
    	    if (channels[i]->getName() == toRemove[k]) { // achou o canal pelo nome
    	        delete channels[i]; // libera memória do objeto Channel
    	        channels.erase(channels.begin() + i); // remove do vetor principal
    	        break;
    	    }
    	}
	}

}

void Server::cmdMODE(Client *a, std::string line) {
	// 1) registration
	if (!a->get_is_registered())
		return sendErrorRegist(a);

	// 2) parse
	std::istringstream iss(line);
	std::string cmd, channel, modes;
	iss >> cmd >> channel;
	if (channel.empty()) {
		std::ostringstream err;
		err << ":server 461 " << a->get_nick() << " MODE :Not enough parameters\r\n";
		sendMsg(a->getFd(), err.str().c_str(), err.str().size());
		return;
	}

	bool isChannel = channel[0] == '#' || channel[0] == '&';
	if (!isChannel) {
    	if (channel == a->get_nick())
			return;
	}
	Channel *tv = getChannelByName(channel);
	if (!tv) {
		std::ostringstream err;
		err << ":server 403 " << a->get_nick() << " " << channel << " :No such channel\r\n";
		sendMsg(a->getFd(), err.str().c_str(), err.str().size());
		return;
	}

	// Se o usuário não está no canal: 442
	if (!tv->isMember(a)) {
		std::ostringstream err;
		err << ":server 442 " << a->get_nick() << " " << channel
		    << " :You're not on that channel\r\n";
		sendMsg(a->getFd(), err.str().c_str(), err.str().size());
		return;
	}

	// ler 'modes' (se existir)
	iss >> modes;

	// Caso "consulta": MODE #canal
	if (modes.empty()) {
		std::string flags = "+";
		std::vector<std::string> args;
		if (tv->getInviteOnly()) flags += "i";
		if (tv->getTopicRestrict()) flags += "t";
		if (tv->hasKey()) { flags += "k"; args.push_back(tv->getPwd()); }
		if (tv->getLimit() > 0) { flags += "l"; 
			std::ostringstream o; o << tv->getLimit(); args.push_back(o.str()); }

		std::ostringstream rpl;
		rpl << ":server 324 " << a->get_nick() << " " << channel << " " << flags;
		for (size_t i = 0; i < args.size(); ++i) rpl << " " << args[i];
		rpl << "\r\n";
		sendMsg(a->getFd(), rpl.str().c_str(), rpl.str().size());
		return;
	}
	// nao precisamos lidar com nicks banidos.
	if (modes == "b" || modes == "B") {
		std::ostringstream rpl_banned_nicks;
    	rpl_banned_nicks << ":server 368 " << a->get_nick() << " " << channel << " :End of Channel Ban List\r\n";
    	sendMsg(a->getFd(), rpl_banned_nicks.str().c_str(), rpl_banned_nicks.str().size());
    	return;
	}

	// a partir daqui: precisa ser operador
	if (!tv->isOperator(a)) {
		std::ostringstream err;
		err << ":server 482 " << a->get_nick() << " " << channel
		    << " :You're not channel operator\r\n";
		sendMsg(a->getFd(), err.str().c_str(), err.str().size());
		return;
	}

	// coletar args
	std::vector<std::string> args;
	for (std::string tmp; iss >> tmp; ) args.push_back(tmp);

	if (modes[0] != '+' && modes[0] != '-') {
		std::ostringstream err;
		err << ":server 472 " << a->get_nick() << " " << modes
		    << " :is unknown mode char to me\r\n";
		sendMsg(a->getFd(), err.str().c_str(), err.str().size());
		return;
	}
	char sign = 0;
	size_t x = 0;
	for (size_t i = 0; i < modes.size(); ++i) {
		char c = modes[i];
		if (c == '+' || c == '-') { sign = c; continue; } // pula o sign

		if (sign != '+' && sign != '-') {
			std::ostringstream err;
			err << ":server 472 " << a->get_nick() << " " << c
			    << " :is unknown mode char to me\r\n";
			sendMsg(a->getFd(), err.str().c_str(), err.str().size());
			return;
		}

		switch (c) {
			// sem arg
			case 'i': case 't':
				if (sign == '+')
					tv->modePNA(a, c);
				else
					tv->modeNNA(a, c);
				break;

			// com arg quando '+'
			case 'k': case 'l': case 'o':
				if (sign == '+') {
					if (x >= args.size()) {
						std::ostringstream err;
						err << ":server 461 " << a->get_nick()
						    << " MODE :Not enough parameters\r\n";
						sendMsg(a->getFd(), err.str().c_str(), err.str().size());
						return;
					}
					tv->modePWA(a, c, args[x++]);
				} else { // '-'
					if (c == 'o') {
						if (x >= args.size()) {
							std::ostringstream err;
							err << ":server 461 " << a->get_nick()
							    << " MODE :Not enough parameters\r\n";
							sendMsg(a->getFd(), err.str().c_str(), err.str().size());
							return;
						}
						tv->modeNWA(a, c, args[x++]);
					} else // -k / -l não usam arg
						tv->modeNNA(a, c);
				}
				break;

			default: {
				std::ostringstream err;
				err << ":server 472 " << a->get_nick() << " " << c << " :is unknown mode char to me\r\n";
				sendMsg(a->getFd(), err.str().c_str(), err.str().size());
				return;
			}
		}
	}
}

void Server::cmdWHOIS(Client *cli, std::string line) {
    if (!cli->get_is_registered())
		return sendErrorRegist(cli);

    std::istringstream iss(line);
    std::string cmd, target;
    iss >> cmd >> target;
    if (target.empty()) {
        std::ostringstream e;
        e << ":server 461 " << cli->get_nick() << " WHOIS :Not enough parameters\r\n";
        sendMsg(cli->getFd(), e.str().c_str(), e.str().size());
        return;
    }

    std::string end;
    end = ":server 318 " + cli->get_nick() + " " + target + " :End of WHOIS list\r\n";
    sendMsg(cli->getFd(), end.c_str(), end.size());
}
