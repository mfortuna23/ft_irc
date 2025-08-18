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
		size_t		limit;
		int			nClients;
		std::map<int, Client *> myClients;
		std::vector <Client *> operators;
		bool inviteOnly;            // +i
		bool topicRestrict;         // +t
		std::vector<std::string> invited; // vetor de strings com os nicks de quem foi invited pro canal.
		std::string topic;
	public :
		Channel();
		Channel(std::string other);
		Channel(std::string oName, Client *cli);
		Channel(std::string oName, Client *cli, std::string pwd);
		int			getLimit(void){return limit;};
		int			getNClients(void){return nClients;};
		std::string getName(void) const {return name;};
		std::string getPwd(void) const {return passW;};
		void		setName(std::string other) {name = other;};
		std::map<int, Client *> getClients(void) const {return myClients;};
		std::string getTopic(void) const {return topic;};		std::map<int, Client *> getClients(void) const {return myClients;};
		void		setTopic(std::string other) {topic = other;};
		Channel 	&operator=(const Channel &other);
		void		addClient(Client *other);
		void		addClient(Client *other, std::string pwd);
		bool		rmClient(Client *other);
		void		sendMsgChannel(std::string msg);
		void		modePNA(Client *cli, char mode); //mode positive no arguments
		void		modePWA(Client *cli, char mode, std::string args); //mode positive with arguments
		void		modeNNA(Client *cli, char mode); //mode negative no arguments
		void		modeNWA(Client *cli, char mode, std::string args); //mode negative with arguments
		// operator handling //
		bool		isOperator(Client* c) const; // check if client is operator
		void		makeOperator(Client* c); // the one who creates or mode +o
		bool		removeOperator(Client* c); // -o
		bool		isMember(Client* c) const;
		Client*		getMemberByNick(const std::string& nick) const; // same as getClientByNick() but just for a channel
		// modes //
		bool        getInviteOnly() const { return inviteOnly; }
		bool        getTopicRestrict() const { return topicRestrict; }
		bool        hasKey() const { return !passW.empty(); }
		void        setInviteOnly(bool v) { inviteOnly = v; }
		void        setTopicRestrict(bool v) { topicRestrict = v; }
		void        setKey(const std::string& k) { passW = k; }
		void        clearKey() { passW.clear(); }
		void        setLimitCount(size_t v) { limit = v; }

		// operators have @ infront of their nicks
		void sendNamesTo(Client* requester) const;
		void sendNamesToAll() const;

		// invite only channels
		bool		isInvited(const std::string& nick) const;
		void		addInvite(const std::string& nick);
		void		removeInvite(const std::string& nick);

		~Channel(){};
} ;

std::ostream &operator<<(std::ostream &out, const Channel& other);

#endif