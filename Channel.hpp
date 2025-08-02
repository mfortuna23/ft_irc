#ifndef CHANNEL_HPP
# define CHANNEL_HPP
# pragma once

# include "irc.hpp"
class Client;
class Server;

class Channel {
	private :
		std::string name;
		std::string passW;
		Client		*host;
		int			type;
		int			limit;
		int			nClients;
		std::map<int, Client *> myClients;
	public :
		Channel();
		Channel(std::string other);
		Channel(std::string oName, Client *a);
		Channel(std::string oName, Client *a, std::string pwd);
		int			getLimit(void){return limit;};
		int			getNClients(void){return nClients;};
		std::string getName(void) const {return name;};
		std::string getPwd(void) const {return passW;};
		void		setName(std::string other) {name = other;};
		std::map<int, Client *> getClients(void) const {return myClients;};
		Channel 	&operator=(const Channel &other);
		void		addClient(Client *other);
		void		addClient(Client *other, std::string pwd);
		bool		rmClient(Client *other);
		void		sendMsgChannel(std::string msg);
		~Channel(){};
} ;

struct joinChannel{
	std::string name;
	std::string passW;
	int			type; // 2 general 1 private
} ;

std::ostream &operator<<(std::ostream &out, const Channel& other);

#endif