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
		return ;
	
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
		return ;
	
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
		if (channels[i].getName() == name)
			return &channels[i];
	}
	return NULL;
}
//fills conteiner of struct join channel with names and passwords
void parsingJoin(std::vector <struct joinChannel> &tv, std::string line){
	// std::istringstream iss(line);
	// std::string cmd, channels, keys;
	// iss >> cmd >> channels;
	// if (iss.peek() == ':') {
	// 	iss.ignore();
	// 	std::getline(iss, keys);
	// }
	// std::istringstream chanStream(channels);
	// std::string channel;
	// while (std::getline(chanStream, channel, ',')) {
	// 	channel.erase(0, channel.find_first_not_of(" \t"));
	// 	channel.erase(channel.find_last_not_of(" \t") + 1);
	// 	if (!channel.empty() && (channel[0] == '#' || channel[0] == '&')) {
	// 		joinChannel join;
	// 		join.type = (channel[0] == '&') ? 1 : 2;
	// 		join.name = channel.substr(1);
	// 		join.passW = "";
	// 		tv.push_back(join);
	// 	}
	// 	else{
	// 		if (channel.empty())
	// 	}
	// }
	// if (!keys.empty()) {
	// 	std::istringstream keyStream(keys);
	// 	std::string key;
	// 	for (size_t i = 0; std::getline(keyStream, key, ',') && i < tv.size(); ++i) {
	// 		key.erase(0, key.find_first_not_of(" \t"));
	// 		key.erase(key.find_last_not_of(" \t") + 1);
	// 		tv[i].passW = key; // Assign to corresponding channel
	// 	}
	// }
	(void)tv;
	(void)line;
}

void	Server::cmdJOIN(Client *a, std::string line){

	std::istringstream iss(line);
	std::string cmd, name, dots;
	std::ostringstream msg;
	iss >> cmd >> name; // TODO parsing join
	if (name.empty()){
		msg << RED << "Missing arguments for JOIN cmd" << RESET << "\r\n";
		sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	Channel *mTv;
	Channel c;
	mTv = &c;
	if (channels.empty() || getChannelByName(name) == NULL) {
		mTv->setName(name);
		channels.push_back(*mTv);
	}
	else
		mTv = getChannelByName(name);
	mTv->addClient(a);
	a->newChannel(mTv);
	msg << GREEN << "You joined " << *mTv << " channel!" << RESET << "\r\n";
	sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
}


void Server::cmdQUIT(Client *a, std::string line){
	(void)line;
	(void)a;
	
	//Channel *tv;
	std::stringstream msg;
	// tv = a->getChannel();
	// if (tv){
	// 	a->newChannel(NULL);
	// 	tv->rmClient(a);
	// 	msg << GREEN << "You quit " << *tv << " channel!" << RESET << "\r\n";
	// 	sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
	// 	return ;
	// }
	msg << GREEN << "You quit IRC" << RESET << "\r\n";
	sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
	for (size_t i = 0; i < clients.size(); i++){
		if (clients[i].getFd() == a->getFd()){
			clients.erase(clients.begin() + i);
			close(a->getFd());
		}
	}
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

	if (message.empty()) {
		sendMsg(cli->getFd(), "ERROR :No text to send\r\n", 25);
		return;
	}

	if (target[0] == '#') {
		Channel* chan = getChannelByName(target);
		if (!chan) {
			sendMsg(cli->getFd(), "ERROR :No such channel\r\n", 26);
			return;
		}
		std::map<std::string, Client*> clients = chan->getClients();
		for (std::map<std::string, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
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
