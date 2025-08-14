#include "irc.hpp"

void	ERR_BADCHANMASK(Client *cli, std::string channel){
	std::stringstream msg;
	msg << ":server 476 " << cli->get_nick() << " " << channel << " :Bad Channel Mask\r\n";
	sendMsg(cli->getFd(), msg.str().c_str(), msg.str().size()); msg.str(""); msg.clear();
}
void	ERR_NOSUCHCHANNEL(Client *cli, std::string channel){
	std::stringstream msg;
	msg << ":server 403 " << cli->get_nick() << " " << channel << " :No such channel\r\n";
	sendMsg(cli->getFd(), msg.str().c_str(), msg.str().size()); msg.str(""); msg.clear();
}
void	ERR_NEEDMOREPARAMS(Client *cli, std::string cmd){
	std::stringstream msg;
	msg << ":server 461 " << cli->get_nick() << " " << cmd << " :Not enough parameters\r\n";
	sendMsg(cli->getFd(), msg.str().c_str(), msg.str().size()); msg.str(""); msg.clear();
}