#include "Channel.hpp"

std::ostream &operator<<(std::ostream &out, const Channel& other){
	out << other.getName();
	return out;
}

Channel::Channel(){
	name = "default";
	passW = "";
	host = NULL;
	type = 0;
	limit = 1024;
}
Channel::Channel(std::string other){
	name = other;
	passW = "";
	host = NULL;
	type = 0;
	nClients = 0;
	limit = 1024;
};

Channel::Channel(std::string oName, Client *a){
	name = oName;
	host = a;
	passW = "";
	type = 0;
	nClients = 1;
	limit = 1024;
	a->newChannel(this);
	myClients.insert(std::make_pair(a->getFd(), a));
	std::stringstream msg;
	msg << GREEN << "You've joined " << name << " channel!" << RESET << "\r\n";
	sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
}

Channel::Channel(std::string oName, Client *a, std::string pwd){
	name = oName;
	host = a;
	passW = pwd;
	std::cout << pwd << std::endl;
	type = 0;
	nClients = 1;
	limit = 1024;
	a->newChannel(this);
	myClients.insert(std::make_pair(a->getFd(), a));
	std::stringstream msg;
	msg << GREEN << startMsg(a) << "JOIN :" << name << RESET << "\r\n";
	sendMsgChannel(msg.str());
}

Channel &Channel::operator=(const Channel &other){
	name = other.getName();
	myClients = other.getClients();
	return *this;
}

void Channel::addClient(Client *other){
	std::stringstream msg;
	//TODO check invite only
	if (myClients.find(other->getFd()) != myClients.end()){
		msg << RED << "You already joined " << name << " channel." << RESET << "\r\n";
		sendMsg(other->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	if ((limit - nClients) == 0){
		msg << RED << "no more space in channel " << name << RESET << "\r\n"; //PROTOCOLCHECK
		sendMsg(other->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	myClients.insert(std::make_pair(other->getFd(), other));
	other->newChannel(this);
	++nClients;
	msg << GREEN << startMsg(other) << "JOIN :" << name << RESET << "\r\n";
	sendMsgChannel(msg.str());
}

void Channel::addClient(Client *other, std::string pwd){
	std::stringstream msg;
	//TODO check invite only
	if (myClients.find(other->getFd()) != myClients.end()){
		msg << RED << "You already joined " << name << " channel." << RESET << "\r\n"; //PROTOCOLCHECK
		sendMsg(other->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	if (passW != pwd){
		msg << RED << "Wrong password" << RESET << "\r\n"; //PROTOCOLCHECK
		sendMsg(other->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	if ((limit - nClients) == 0){
		msg << RED << "no more space in channel " << name << RESET << "\r\n"; //PROTOCOLCHECK
		sendMsg(other->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	myClients.insert(std::make_pair(other->getFd(), other));
	other->newChannel(this);
	++nClients;
	msg << GREEN << startMsg(other) << "JOIN :" << name << RESET << "\r\n";
	sendMsgChannel(msg.str());
}

bool Channel::rmClient(Client *other){
	if (myClients.erase(other->getFd()) == 0) { /* not found */
		//error msg
		return false;
	}
	other->rmChannel(this);
	--nClients;
	// caso seja o host que esteja saindo
	if (host == other)
		host = NULL;
	return true ;
}

void	Channel::sendMsgChannel(std::string msg){
	std::map<int, Client*>::iterator it;
	for (it = myClients.begin(); it != myClients.end(); ++it) {
		Client* client = it->second; 
		if (client)
			send(client->getFd(), msg.c_str(), msg.size(), 0);
	}
}
void	Channel::modePNA(Client *a, char mode){
	std::stringstream msg;
	if (mode == 'i'){
		msg << "Setting channel " << name << " to invite only" << "\r\n"; //protocol
		sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	msg << "Channel " << name << " has topic restrictions" << "\r\n"; //protocol
	sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());

}
void	Channel::modePWA(Client *a, char mode, std::string args){
	std::stringstream msg;
	if (mode == 'k'){
		msg << "Setting channel " << name << " with key: " << args << "\r\n"; //protocol
		sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	if (mode == 'o'){
		//TODO check if user exists and give them operator priv
		msg << "Giving user " << args << " operator privileges in channel " << name << "\r\n"; //protocol
		sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	if (!checkNbr(args)){
		msg << "Limit " << args << " is not vallid" << "\r\n";
		sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	msg << "limit of " << args << " users was set in channel " << name << "\r\n";
	sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());


}
void	Channel::modeNNA(Client *a, char mode){
	std::stringstream msg;
	if (mode == 'i'){
		msg << "Remove invite only from channel " << name << "\r\n"; //protocol
		sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	if (mode == 't'){
		msg << "Remove topic restrictions from channel " << name << "\r\n"; //protocol
		sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	if (mode == 'k'){
		msg << "Remove key from channel " << name << "\r\n"; //protocol
		sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
		return ;
	}
	msg << "Remove user limit from channel " << name << "\r\n"; //protocol
	sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
	
}
void	Channel::modeNWA(Client *a, char mode, std::string args){
	std::stringstream msg;
	(void)mode;
	//check if user exists and removes operator priv
	msg << "Remove operator privileges from " << args << " from channel " << name;
	sendMsg(a->getFd(), msg.str().c_str(), msg.str().size());
}