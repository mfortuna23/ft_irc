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
		std::string getName(void) const {return name;};
		void addClient(Client *other){(void)other;};
		void rmClient(Client *other){(void)other;};
		~Channel(){};
} ;

inline std::ostream &operator<<(std::ostream &out, const Channel& other){
	out << other.getName();
	return out;
}

#endif