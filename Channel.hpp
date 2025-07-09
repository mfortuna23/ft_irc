#ifndef CHANNEL_HPP
# define CHANNEL_HPP
# pragma once

# include "irc.hpp"

class Channel{
	private :
		std::string name;
		std::map<std::string, Client *> myClients;
	public :
		Channel(std::string other){name = other;};
		std::string getName(void){return name;};
		void addClient(Client other);
		void rmClient(Client other);
		~Channel(){};
} ;

#endif