#include "irc.hpp"

void	ERR_NOSUCHCHANNEL(Client *cli, std::string channel){
	std::stringstream msg;
	msg << ":server 403 " << cli->get_nick() << " " << channel << " :No such channel\r\n";
	sendMsg(cli->getFd(), msg.str().c_str(), msg.str().size()); msg.str(""); msg.clear();
}
void	ERR_NORECIPIENT(Client *cli, std::string cmd){
	std::stringstream msg;
	msg << ":server 411 " << cli->get_nick() << " ::No recipient given (" << cmd <<")\r\n";
	sendMsg(cli->getFd(), msg.str().c_str(), msg.str().size());
}
void	ERR_NOTEXTTOSEND(Client *cli, std::string cmd){
	std::stringstream msg;
	msg << ":server 412 " << cli->get_nick() << " " << cmd << " :No text to send\r\n";
	sendMsg(cli->getFd(), msg.str().c_str(), msg.str().size());
}
void	ERR_NOTONCHANNEL(Client *cli, std::string chan){
	std::stringstream msg;
	msg << ":server 442 " << cli->get_nick() << " " << chan << " :You're not on that channel\r\n";
	sendMsg(cli->getFd(), msg.str().c_str(), msg.str().size()); msg.str(""); msg.clear();
}
void	ERR_NOTREGISTERED(Client *cli){
	std::ostringstream err;
	err << ":server 451 " << (cli->get_nick().empty() ? "*" : cli->get_nick()) << " :You have not registered\r\n";
	sendMsg(cli->getFd(), err.str().c_str(), err.str().size());
	return;
}
void	ERR_NEEDMOREPARAMS(Client *cli, std::string cmd){
	std::stringstream msg;
	msg << ":server 461 " << cli->get_nick() << " " << cmd << " :Not enough parameters\r\n";
	sendMsg(cli->getFd(), msg.str().c_str(), msg.str().size()); msg.str(""); msg.clear();
}
void	ERR_BADCHANMASK(Client *cli, std::string channel){
	std::stringstream msg;
	msg << ":server 476 " << cli->get_nick() << " " << channel << " :Bad Channel Mask\r\n";
	sendMsg(cli->getFd(), msg.str().c_str(), msg.str().size()); msg.str(""); msg.clear();
}
void	ERR_CHANOPRIVSNEEDED(Client *cli, std::string chan){
	std::stringstream msg;
	msg << ":server 482 " << cli->get_nick() << " " << chan << " :You're not channel operator\r\n";
	sendMsg(cli->getFd(), msg.str().c_str(), msg.str().size());
}

void	ERR_PASSWDMISMATCH(Client *cli){
	std::string err;
	err = ":server 464 " + (cli->get_nick().empty() ? "*" : cli->get_nick()) + " :Password must be sent first\r\n";
	sendMsg(cli->getFd(), err.c_str(), err.size());
	return;
}