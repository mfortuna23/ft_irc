#include "Channel.hpp"

std::ostream &operator<<(std::ostream &out, const Channel& other){
	out << other.getName();
	return out;
}

Channel &Channel::operator=(const Channel &other){
	name = other.getName();
	myClients = other.getClients();
	return *this;
}

void Channel::addClient(Client *other){
	myClients.insert(std::make_pair(other->get_nick(), other));
}

void Channel::rmClient(Client *other){
	myClients.erase(other->get_nick());
}