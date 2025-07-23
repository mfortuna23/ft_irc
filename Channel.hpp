#ifndef CHANNEL_HPP
# define CHANNEL_HPP
# pragma once

# include "irc.hpp"
class Client;
class Server;

class Channel{
	private :
		std::string name;
		std::string passW;
		Client		*host;
		int			type;
		std::map<std::string, Client *> myClients;
	public :
		Channel(){name = "default";};
		Channel(std::string other){name = other;};
		std::string getName(void) const {return name;};
		void setName(std::string other) {name = other;};
		std::map<std::string, Client *> getClients(void) const {return myClients;};
		Channel &operator=(const Channel &other);
		void addClient(Client *other);
		void rmClient(Client *other);
		~Channel(){};
} ;

struct joinChannel{
	std::string name;
	std::string passW;
	int			type; // 2 general 1 private
} ;

std::ostream &operator<<(std::ostream &out, const Channel& other);

#endif