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
		msg << RED << "You already joined " << name << " channel." << RESET << "\r\n";
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

void Channel::rmClient(Client *other){
	myClients.erase(other->getFd());
	other->rmChannel(this);
	--nClients;
}

void	Channel::sendMsgChannel(std::string msg){
	std::map<int, Client*>::iterator it;
	for (it = myClients.begin(); it != myClients.end(); ++it) {
		Client* client = it->second; 
		if (client)
			send(client->getFd(), msg.c_str(), msg.size(), 0);
	}
}