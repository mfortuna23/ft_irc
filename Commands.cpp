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

// PASS <password> se o pass nao for o primeiro error de registo
void Server::cmdPASS(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, pass;
	iss >> cmd >> pass;

	if (cli->get_is_registered()) {
		std::string msg = ":server 462 : You may not reregister\r\n";
		sendMsg(cli->getFd(), msg.c_str(), msg.size());
		return;}
	if (pass.empty()) {
		std::string msg = ":server 461 :No password given\r\n";
		sendMsg(cli->getFd(), msg.c_str(), msg.size());
		return;}
	if (pass != this->get_pass()) {
		std::string msg = ":server 464 :Password incorrect\r\n";
		sendMsg(cli->getFd(), msg.c_str(), msg.size());
		cli->set_want_close(true); // fecha assim que a fila esvaziar
		enableWriteEvent(cli->getFd());
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
		std::string msg = ":server 431 :No nickname given\r\n";
		sendMsg(cli->getFd(), msg.c_str(), msg.size());
		return;
	}
	if (cli->get_regist_steps() == 3)
		return ERR_PASSWDMISMATCH(cli);

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

	tryFinishRegistration(cli);
}

// USER <username> 0 * :realname
void Server::cmdUSER(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, user, unused, asterisk, realname;

	if (cli->get_is_registered()) {
		std::string msg = ":server 462 " + (cli->get_nick().empty() ? "*" : cli->get_nick()) + " :You may not reregister\r\n";
		sendMsg(cli->getFd(), msg.c_str(), msg.size());
		return;
	}
	if (cli->get_regist_steps() == 3)
		return ERR_PASSWDMISMATCH(cli);

	iss >> cmd >> user >> unused >> asterisk;
	std::getline(iss, realname); //getline serve para capturar tudo que vem depois dos 4 primeiros campos (tudo que vem apos asterisk), mesmo que contenha espaços.

	while (!realname.empty() && (realname[0] == ' ' || realname[0] == '\t')) // remover espaço inicial
		realname.erase(0, 1);
	if (!realname.empty() && realname[0] == ':') // remover o ':' inicial
		realname.erase(0, 1);
	// removo qualquer espaço apos ':' para garantir que nao haja realnames apenas feitos de espaços.
	while (!realname.empty() && (realname[0] == ' ' || realname[0] == '\t'))
		realname.erase(0, 1);
	// depois dessas verificacoes agora sim temos o realname.
	// como nao vamos nos aprofundar muito, nao precisamos salvar nada alem do user. 
	if (user.empty() || unused.empty() || asterisk.empty() || realname.empty()) //é tudo ignorado, mas tem que existir ...
		return ERR_NEEDMOREPARAMS(cli, "USER");
	cli->set_username(user);
	std::string msg = "Your username now is " + cli->get_user() + "\r\n"; 
	sendMsg(cli->getFd(), msg.c_str(), msg.size());

	tryFinishRegistration(cli);
}

Channel* Server::getChannelByName(std::string name)
{
	for (size_t i = 0; i < channels.size(); ++i){
		if (toUpper(channels[i]->getName()) == toUpper(name)) // channels are not case sensitive
			return channels[i];
	}
	return NULL;
}


void	Server::cmdJOIN(Client *cli, std::string line){
	std::stringstream msg;
	std::istringstream iss(line);
	std::string cmd, channel, keys;
	iss >> cmd >> channel;
	if (channel.empty())
		return ERR_NEEDMOREPARAMS(cli, "JOIN");
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
		if (checkChannelName(channel)) {
			Channel *c = getChannelByName(channel);
			if (!c){
				channels.push_back(new Channel(channel, cli)); // keys can only be added after the channel is created
				c = channels.back();
				c->sendNamesTo(cli);
				++i;
			} else {
				if (c->getPwd().empty())
					c->addClient(cli);
				else
					c->addClient(cli, allKeys[i++]);
			}
		} else if (channel == "0"){
			std::map<std::string, Channel*> tmp = cli->getChannels();
			for (std::map<std::string, Channel*>::iterator it = tmp.begin(); it != tmp.end(); ++it)
				cmdPART(cli, "PART " + it->first + " :Left all channels");
		}
		else
			ERR_BADCHANMASK(cli, channel);
	}
}

void Server::cmdQUIT(Client *cli, std::string line){
	(void)line;

	std::stringstream msg;
	msg << startMsg(cli) << " QUIT :Leaving\r\n";
	std::string quit_msg = msg.str();

	// copia o map para uma variável local, oque evita modificar ou iterar diretamente sobre o map original 
	std::map<std::string, Channel*> chansMap = cli->getChannels();

	// constrói um vetor de ponteiros apenas para Channel, assim temos de forma segura nesse vetor todos os canais aos quais o cliente pertence no momento do QUIT
	std::vector<Channel*> chans;
	for (std::map<std::string, Channel*>::iterator it = chansMap.begin(); it != chansMap.end(); ++it)
		chans.push_back(it->second);

	std::vector<std::string> toRemove;
	for (size_t i = 0; i < chans.size(); ++i) {
		Channel *ch = chans[i];
		ch->rmClient(cli);
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

	sendMsg(cli->getFd(), quit_msg.c_str(), quit_msg.size());
	cli->set_want_close(true);
	enableWriteEvent(cli->getFd());
}

void Server::cmdPRIVMSG(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, target, message;

	iss >> cmd >> target;

	if (target.empty())
		return ERR_NORECIPIENT(cli, "PRIVMSG");

	size_t pos = line.find(" :");
	if (pos != std::string::npos)
		message = line.substr(pos + 2); // ignora o " :"
	else
		return ERR_NOTEXTTOSEND(cli, "PRIVMSG");

	if (target[0] == '#' || target[0] == '&') {
		if (!checkChannelName(target))
			return ERR_BADCHANMASK(cli, target);
		Channel* chan = getChannelByName(target);
		if (!chan) 
			return ERR_NOSUCHCHANNEL(cli, target);
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
		if (!checkChannelName(target))
			return ERR_BADCHANMASK(cli, target);
		Channel* chan = getChannelByName(target);
		if (!chan)
			return ERR_NOSUCHCHANNEL(cli, target);

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
void Server::cmdPART(Client *cli, std::string line){
	std::stringstream msg;
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
		if (checkChannelName(channel)) {
			tv = getChannelByName(channel);
			if (!tv) { // canal não existe
				ERR_NOSUCHCHANNEL(cli, channel);
				continue;
			}
			if (!tv->isMember(cli)) { // user não está no canal
				ERR_NOTONCHANNEL(cli, channel);
				continue;
			}
			if (tv && tv->rmClient(cli)){
				msg.str(""); msg.clear();
				msg << startMsg(cli) << " PART " << tv->getName() << " " << leave << "\r\n";
				std::string part_msg = msg.str();
				tv->sendMsgChannel(part_msg);
				sendMsg(cli->getFd(), part_msg.c_str(), part_msg.size());
				msg.str("");
				msg.clear();
			}
		} else
			ERR_NOSUCHCHANNEL(cli, channel);
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

void Server::cmdMODE(Client *cli, std::string line) {
	// 1) parse
	std::istringstream iss(line);
	std::string cmd, channel, modes;
	iss >> cmd >> channel;
	if (channel.empty())
		return ERR_NEEDMOREPARAMS(cli, "MODE");
	if (!checkChannelName(channel)){
		if (channel == cli->get_nick()) // quando o irssi envia "mode nick +i"
			return;
		return ERR_BADCHANMASK(cli, channel);
	}
	Channel *tv = getChannelByName(channel);
	if (!tv)
		return ERR_NOSUCHCHANNEL(cli, channel);
	// Se o usuário não está no canal:
	if (!tv->isMember(cli))
		return ERR_NOTONCHANNEL(cli, channel);
	// ler 'modes' (se existir)
	iss >> modes;
	// Caso "consulta": MODE #canal
	if (modes.empty()) {
		std::string flags = "+"; //IRC de referencia mostra + mesmo que nao haja flags
		std::vector<std::string> args;
		if (tv->getInviteOnly()) flags += "i";
		if (tv->getTopicRestrict()) flags += "t";
		if (tv->hasKey()) { flags += "k"; args.push_back(tv->getPwd()); }
		if (tv->getLimit() > 0) { flags += "l"; 
			std::ostringstream o; o << tv->getLimit(); args.push_back(o.str()); }

		std::ostringstream rpl;
		rpl << ":server 324 " << cli->get_nick() << " " << channel << " " << flags;
		for (size_t i = 0; i < args.size(); ++i) rpl << " " << args[i];
		rpl << "\r\n";
		sendMsg(cli->getFd(), rpl.str().c_str(), rpl.str().size());
		return;
	}
	// nao precisamos lidar com nicks banidos.
	if (modes == "b" || modes == "B") {
		std::string rpl_banned_nicks = ":server 368 " + cli->get_nick() + " " + channel + " :End of Channel Ban List\r\n";
    	sendMsg(cli->getFd(), rpl_banned_nicks.c_str(), rpl_banned_nicks.size());
    	return;
	}
	// a partir daqui: precisa ser operador
	if (!tv->isOperator(cli))
		return ERR_CHANOPRIVSNEEDED(cli, channel);
	// coletar args
	std::vector<std::string> args;
	for (std::string tmp; iss >> tmp; ) args.push_back(tmp);

	if (modes[0] != '+' && modes[0] != '-') {
		std::string err = ":server 472 " + cli->get_nick() + " " + modes + " :is unknown mode char to me\r\n";
		sendMsg(cli->getFd(), err.c_str(), err.size());
		return;
	}
	char sign = 0;
	size_t x = 0;
	for (size_t i = 0; i < modes.size(); ++i) {
		char c = modes[i];
		if (c == '+' || c == '-') { sign = c; continue; } // pula o sign

		if (sign != '+' && sign != '-') {
			std::string err = ":server 472 " + cli->get_nick() + " " + c + " :is unknown mode char to me\r\n";
			sendMsg(cli->getFd(), err.c_str(), err.size());
			return;
		}

		switch (c) {
			// sem arg
			case 'i': case 't':
				if (sign == '+')
					tv->modePNA(cli, c);
				else
					tv->modeNNA(cli, c);
				break;
			// com arg quando '+'
			case 'k': case 'l': case 'o':
				if (sign == '+') {
					if (x >= args.size())
						return ERR_NEEDMOREPARAMS(cli, "MODE (+)");
					tv->modePWA(cli, c, args[x++]);
				} else { // '-'
					if (c == 'o') {
						if (x >= args.size())
							return ERR_NEEDMOREPARAMS(cli, "MODE (-)");
						tv->modeNWA(cli, c, args[x++]);
					} else // -k / -l não usam arg
						tv->modeNNA(cli, c);
				}
				break;
			default: {
				std::string err = ":server 472 " + cli->get_nick() + " " + c + " :is unknown mode char to me\r\n";
				sendMsg(cli->getFd(), err.c_str(), err.size());
				return;
			}
		}
	}
}

void Server::cmdWHOIS(Client *cli, std::string line) {
    std::istringstream iss(line);
    std::string cmd, target;
    iss >> cmd >> target;
    if (target.empty()) 
		return ERR_NEEDMOREPARAMS(cli, "WHOIS");
    std::string end;
    end = ":server 318 " + cli->get_nick() + " " + target + " :End of WHOIS list\r\n";
    sendMsg(cli->getFd(), end.c_str(), end.size());
}

void Server::cmdKICK(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, chan, victim;
	iss >> cmd >> chan >> victim;

	if (chan.empty() || victim.empty())
        return ERR_NEEDMOREPARAMS(cli, "KICK");

	if (!checkChannelName(chan))
        return ERR_BADCHANMASK(cli, chan);
	
	std::string reason;
	size_t pos = line.find(" :"); // procura pelo ':' pois a reason vem logo depois.
	if (pos != std::string::npos)
		reason = line.substr(pos + 2); // achamos a reason;
	if (reason.empty())
		reason = cli->get_nick(); // resposta padrao quando nao se tem uma reason;
	
	Channel *ch = getChannelByName(chan);
	if (!ch)
		return ERR_NOSUCHCHANNEL(cli, chan);
	
	if (!ch->isMember(cli))
		return ERR_NOTONCHANNEL(cli, chan);

	if (!ch->isOperator(cli))
		return ERR_CHANOPRIVSNEEDED(cli, chan);

	Client *v = getClientByNick(victim);
    if (!v) {
        std::string err = ":server 401 " + cli->get_nick() + " " + victim + " :No such nick/channel\r\n";
        sendMsg(cli->getFd(), err.c_str(), err.size());
        return;
    }
    if (!ch->isMember(v)) {
        std::string err = ":server 441 " + cli->get_nick() + " " + victim + " " + chan + " :They aren't on that channel\r\n";
        sendMsg(cli->getFd(), err.c_str(), err.size());
        return;
    }

	std::ostringstream kickmsg;
    kickmsg << startMsg(cli) << " KICK " << ch->getName() << " " << v->get_nick() << " :" << reason << "\r\n";
    std::string out = kickmsg.str();

    ch->sendMsgChannel(out);
    ch->rmClient(v);

    // Se o canal ficou vazio, remove da lista do servidor (o kicker pode se kickar)
    if (ch->getClients().empty()) {
        for (size_t i = 0; i < channels.size(); ++i) {
            if (channels[i] == ch) {
                delete channels[i];
                channels.erase(channels.begin() + i);
                break;
            }
        }
    }
}

void Server::cmdINVITE(Client *cli, std::string line) {
    std::istringstream iss(line);
    std::string cmd, nick, chan;
    iss >> cmd >> nick >> chan;

    if (nick.empty() || chan.empty())
        return ERR_NEEDMOREPARAMS(cli, "INVITE");

    if (!checkChannelName(chan))
        return ERR_BADCHANMASK(cli, chan);

    Channel *c = getChannelByName(chan);
    if (!c)
        return ERR_NOSUCHCHANNEL(cli, chan);

    // precisa estar no canal para convidar
    if (!c->isMember(cli))
		return ERR_NOTONCHANNEL(cli, chan);

    if (!c->isOperator(cli))
		return ERR_CHANOPRIVSNEEDED(cli, chan);

    Client *target = getClientByNick(nick);
    if (!target) {
        std::string err = ":server 401 " + cli->get_nick() + " " + nick + " :No such nick/channel\r\n";
        sendMsg(cli->getFd(), err.c_str(), err.size());
        return;
    }

    if (c->isMember(target)) {
        std::string err = ":server 443 " + cli->get_nick() + " " + nick + " " + c->getName() + " :is already on channel\r\n";
        sendMsg(cli->getFd(), err.c_str(), err.size());
        return;
    }

    // registra o convite (assim o convidado nao fica preso na condicao +i na hora do JOIN)
    c->addInvite(nick);

    // 341 RPL_INVITING (para quem convidou)
    {
        std::string rpl = ":server 341 " + cli->get_nick() + " " + nick + " " + c->getName() + "\r\n";
        sendMsg(cli->getFd(), rpl.c_str(), rpl.size());
    }

    // notifica o convidado com prefixo do convidante
    {
        std::string out = startMsg(cli) + " INVITE " + nick + " :" + c->getName() + "\r\n";
        sendMsg(target->getFd(), out.c_str(), out.size());
    }
}

void Server::cmdTOPIC(Client *cli, std::string line){
	std::istringstream iss(line);
	std::string cmd, chan, topic;
	std::stringstream msg;
	iss >> cmd >> chan;
	if (chan.empty())
        return ERR_NEEDMOREPARAMS(cli, "TOPIC");
	Channel *tv = getChannelByName(chan);
	if (!tv)
		return ERR_NOSUCHCHANNEL(cli, chan);
	if (!tv->getMemberByNick(cli->get_nick()))
		return ERR_NOTONCHANNEL(cli, chan);
	std::getline(iss, topic);
	topic.erase(0, topic.find_first_not_of(" \t\n\r")); //TOPIC sozinho devolve o topico
	size_t pos = line.find(" :"); // procura pelo ':' senao existir consideramos empty.
	if (topic.empty() || pos == std::string::npos){
		if (tv->getTopic().empty()){
			msg << ":server 331 " << cli->get_nick() << " " << chan << " :No topic is set.\r\n";
			sendMsg(cli->getFd(), msg.str().c_str(), msg.str().size());
			return ;
		}
		msg << ":server 332 " << cli->get_nick() << " " << chan << " :" << tv->getTopic() <<"\r\n";
		sendMsg(cli->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	topic.erase(0, topic.find_first_not_of(" \t\n\r:")); // : sem topico elimina o topico existente
	if ((tv->getTopicRestrict() && tv->isOperator(cli)) || !tv->getTopicRestrict()){
		msg << startMsg(cli) << " TOPIC " << tv->getName() << " :" << topic << "\r\n";
		tv->sendMsgChannel(msg.str());
		return tv->setTopic(topic);
	}
	return ERR_CHANOPRIVSNEEDED(cli, chan);
}