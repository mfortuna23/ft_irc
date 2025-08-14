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
	limit = 0;          // 0 = sem limite
	nClients = 0;
	inviteOnly = false;
	topicRestrict = false;
}

Channel::Channel(std::string other){
	name = other;
	passW = "";
	host = NULL;
	type = 0;
	nClients = 0;
	limit = 0;
	inviteOnly = false;
	topicRestrict = false;
}

Channel::Channel(std::string oName, Client *cli){
	name = oName;
	host = cli;
	passW = "";
	type = 0;
	nClients = 1;
	limit = 0;
	inviteOnly = false;
	topicRestrict = false;

	cli->newChannel(this);
	myClients.insert(std::make_pair(cli->getFd(), cli));
	makeOperator(cli); // creator é op

	std::stringstream msg;
	msg << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp()
	    << " JOIN :" << name << "\r\n";
	sendMsgChannel(msg.str());
	//sendNamesTo(cli);
}

Channel::Channel(std::string oName, Client *cli, std::string pwd){
	name = oName;
	host = cli;
	passW = pwd;
	type = 0;
	nClients = 1;
	limit = 0;
	inviteOnly = false;
	topicRestrict = false;

	cli->newChannel(this);
	myClients.insert(std::make_pair(cli->getFd(), cli));
	makeOperator(cli); // creator é op

	std::stringstream msg;
	msg << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp()
	    << " JOIN :" << name << "\r\n";
	sendMsgChannel(msg.str());
	//sendNamesTo(cli);
}

Channel &Channel::operator=(const Channel &other){
	name = other.getName();
	myClients = other.getClients();
	return *this;
}

void Channel::addClient(Client *other){
	std::ostringstream msg;

	// já está no canal
	if (myClients.find(other->getFd()) != myClients.end()) {
		msg << ":server 443 " << other->get_nick() << " " << name
		    << " :is already on channel\r\n"; // RPL_USERONCHANNEL (informativo)
		sendMsg(other->getFd(), msg.str().c_str(), msg.str().size());
		return;
	}

	// invite-only
	if (inviteOnly) {
		std::ostringstream e;
		e << ":server 473 " << other->get_nick() << " " << name
		  << " :Cannot join channel (+i)\r\n"; // ERR_INVITEONLYCHAN
		sendMsg(other->getFd(), e.str().c_str(), e.str().size());
		return;
	}

	// key exigida
	if (!passW.empty()) {
		std::ostringstream e;
		e << ":server 475 " << other->get_nick() << " " << name
		  << " :Cannot join channel (+k)\r\n"; // ERR_BADCHANNELKEY
		sendMsg(other->getFd(), e.str().c_str(), e.str().size());
		return;
	}

	// limite ativo
	if (limit > 0 && (size_t)nClients >= limit) {
		std::ostringstream e;
		e << ":server 471 " << other->get_nick() << " " << name
		  << " :Channel is full\r\n"; // ERR_CHANNELISFULL
		sendMsg(other->getFd(), e.str().c_str(), e.str().size());
		return;
	}

	myClients.insert(std::make_pair(other->getFd(), other));
	other->newChannel(this);
	++nClients;

	std::ostringstream j;
	j << ":" << other->get_nick() << "!~" << other->get_user() << "@" << other->getIp()
	  << " JOIN :" << name << "\r\n";
	sendMsgChannel(j.str());
	sendNamesTo(other);
}

void Channel::addClient(Client *other, std::string pwd){
	std::ostringstream msg;

	if (myClients.find(other->getFd()) != myClients.end()) {
		msg << ":server 443 " << other->get_nick() << " " << name
		    << " :is already on channel\r\n";
		sendMsg(other->getFd(), msg.str().c_str(), msg.str().size());
		return;
	}
	// invite-only
	if (inviteOnly) {
		std::ostringstream e;
		e << ":server 473 " << other->get_nick() << " " << name
		  << " :Cannot join channel (+i)\r\n";
		sendMsg(other->getFd(), e.str().c_str(), e.str().size());
		return;
	}
	// key check
	if (!passW.empty() && passW != pwd) {
		std::ostringstream e;
		e << ":server 475 " << other->get_nick() << " " << name
		  << " :Cannot join channel (+k)\r\n";
		sendMsg(other->getFd(), e.str().c_str(), e.str().size());
		return;
	}
	// limite
	if (limit > 0 && (size_t)nClients >= limit) {
		std::ostringstream e;
		e << ":server 471 " << other->get_nick() << " " << name
		  << " :Channel is full\r\n";
		sendMsg(other->getFd(), e.str().c_str(), e.str().size());
		return;
	}

	myClients.insert(std::make_pair(other->getFd(), other));
	other->newChannel(this);
	++nClients;

	std::ostringstream j;
	j << ":" << other->get_nick() << "!~" << other->get_user() << "@" << other->getIp()
	  << " JOIN :" << name << "\r\n";
	sendMsgChannel(j.str());
	sendNamesTo(other);
}

bool Channel::rmClient(Client *other){
	if (myClients.erase(other->getFd()) == 0) { /* not found */
		//error msg
		return false;
	}
	removeOperator(other);
	other->rmChannel(this);
	--nClients;
	// caso seja o host que esteja saindo
	if (host == other)
		host = NULL;
	if (operators.empty() && !myClients.empty()){
		Client* newOp = myClients.begin()->second; // qualquer um; simples e eficaz //pelos testes que fiz. nao parece ser aleatorio. vai para o fd menor
		makeOperator(newOp);
		// avisa todos: agora fulano é o novo op
		std::ostringstream m;
		m << ":server MODE " << name << " +o " << newOp->get_nick() << "\r\n";
		sendMsgChannel(m.str());
		host = newOp;
		//sendNamesToAll();
	}
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

//static std::string itos(size_t n){ std::ostringstream o; o<<n; return o.str(); }

void Channel::modePNA(Client *cli, char mode){
	std::ostringstream b;
	if (mode == 'i'){
		if (inviteOnly) return;
		inviteOnly = true;
		b << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp()
		  << " MODE " << name << " +i\r\n";
		sendMsgChannel(b.str());
		return;
	}
	if (mode == 't'){
		if (topicRestrict) return;
		topicRestrict = true;
		b << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp()
		  << " MODE " << name << " +t\r\n";
		sendMsgChannel(b.str());
		return;
	}
}

void Channel::modeNNA(Client *cli, char mode){
	std::ostringstream b;
	if (mode == 'i'){
		if (!inviteOnly) return;
		inviteOnly = false;
		b << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp()
		  << " MODE " << name << " -i\r\n";
		sendMsgChannel(b.str());
		return;
	}
	if (mode == 't'){
		if (!topicRestrict) return;
		topicRestrict = false;
		b << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp()
		  << " MODE " << name << " -t\r\n";
		sendMsgChannel(b.str());
		return;
	}
	if (mode == 'k'){ // remove key sem arg (comport. comum)
		if (!passW.empty()){
			passW.clear();
			b << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp()
			  << " MODE " << name << " -k\r\n";
			sendMsgChannel(b.str());
		}
		return;
	}
	if (mode == 'l'){ // remove limite
		if (limit != 0){
			limit = 0;
			b << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp()
			  << " MODE " << name << " -l\r\n";
			sendMsgChannel(b.str());
		}
		return;
	}
}

void Channel::modePWA(Client *cli, char mode, std::string args){
	std::ostringstream out;

	if (mode == 'k'){
		if (!passW.empty()){
			std::ostringstream e;
			e << ":server 467 " << cli->get_nick() << " " << name
			  << " :Channel key already set\r\n"; // ERR_KEYSET
			sendMsg(cli->getFd(), e.str().c_str(), e.str().size());
			return;
		}
		if (args.empty())
			return ERR_NEEDMOREPARAMS(cli, "MODE");
		passW = args;
		out << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp()
		    << " MODE " << name << " +k " << args << "\r\n";
		sendMsgChannel(out.str());
		return;
	}

	if (mode == 'l'){
		if (args.empty() || !checkNbr(args))
			return ERR_NEEDMOREPARAMS(cli, "MODE (+l)");
		size_t newLim = std::max<size_t>(1, (size_t)std::atoi(args.c_str()));
		if (newLim == limit) return;
		limit = newLim;
		out << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp()
		    << " MODE " << name << " +l " << args << "\r\n";
		sendMsgChannel(out.str());
		return;
	}

	if (mode == 'o'){
		Client* target = getMemberByNick(args);
		if (!target){
			std::ostringstream e;
			e << ":server 441 " << cli->get_nick() << " " << args << " " << name
			  << " :They aren't on that channel\r\n"; // ERR_USERNOTINCHANNEL
			sendMsg(cli->getFd(), e.str().c_str(), e.str().size());
			return;
		}
		if (isOperator(target)) return; // já é op
		makeOperator(target);
		out << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp()
		    << " MODE " << name << " +o " << args << "\r\n";
		sendMsgChannel(out.str());
		return;
	}
}

void Channel::modeNWA(Client *cli, char mode, std::string args){
	if (mode == 'o'){
		std::ostringstream out;
		Client* target = getMemberByNick(args);
		if (!target){
			std::ostringstream e;
			e << ":server 441 " << cli->get_nick() << " " << args << " " << name
			  << " :They aren't on that channel\r\n";
			sendMsg(cli->getFd(), e.str().c_str(), e.str().size());
			return;
		}
		if (!removeOperator(target)) return; // não era op
		out << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp()
		    << " MODE " << name << " -o " << args << "\r\n";
		sendMsgChannel(out.str());
		return;
	}
}

bool	Channel::isOperator(Client* c) const
{
    if (!c)
		return false;
    for (size_t i = 0; i < operators.size(); ++i) {
        if (operators[i] == c)
			return true;
    }
    return false;
}

bool	Channel::isMember(Client* c) const
{
    if (!c)
		return false;
    if (myClients.find(c->getFd()) == myClients.end()) // .end() "position after last element" (used as a "not found" signal)
		return false;
	return true;
}

void	Channel::makeOperator(Client* c)
{
    if (!isMember(c))
		return ;
    if (isOperator(c))
		return ; // already operator
    operators.push_back(c);
    return ;
}

bool	Channel::removeOperator(Client* c)
{
    for (size_t i = 0; i < operators.size(); ++i) {
        if (operators[i] == c) {
            operators.erase(operators.begin() + i);
            return true;
        }
    }
    return false;
}

Client*	Channel::getMemberByNick(const std::string& nick) const
{
    for (std::map<int, Client*>::const_iterator it = myClients.begin(); it != myClients.end(); ++it){
        if (it->second && it->second->get_nick() == nick)
			return it->second;
    }
    return NULL;
}

void Channel::sendNamesTo(Client* requester) const {
    if (!requester) return;

    // monta lista: @nick para operadores, nick normal caso contrário
    std::ostringstream list;
    for (std::map<int, Client*>::const_iterator it = myClients.begin();
         it != myClients.end(); ++it)
    {
        Client* m = it->second;
        if (!m) continue;
        if (isOperator(m)) list << "@" << m->get_nick();
        else               list << m->get_nick();
        list << " ";
    }
    std::string names = list.str();
    if (!names.empty() && names[names.size()-1] == ' ')
        names.erase(names.size()-1);

    // 353 = RPL_NAMREPLY   formato comum: ":server 353 <nick> = <#canal> :<lista>"
    std::ostringstream rpl353;
    rpl353 << ":server 353 " << requester->get_nick()
           << " = " << name << " :" << names << "\r\n";
    sendMsg(requester->getFd(), rpl353.str().c_str(), rpl353.str().size());

    // 366 = RPL_ENDOFNAMES
    std::ostringstream rpl366;
    rpl366 << ":server 366 " << requester->get_nick()
           << " " << name << " :End of /NAMES list.\r\n";
    sendMsg(requester->getFd(), rpl366.str().c_str(), rpl366.str().size());
}

// Channel.cpp
void Channel::sendNamesToAll() const {
    for (std::map<int, Client*>::const_iterator it = myClients.begin();
         it != myClients.end(); ++it) {
        if (it->second) sendNamesTo(it->second);
    }
}
