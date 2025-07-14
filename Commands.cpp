#include "irc.hpp"

bool Server::isThisCmd(const std::string& line, std::string cmd){
	std::istringstream iss(line);
	std::string firstWord;
	if (iss >> firstWord)
		return firstWord == cmd;
	return false;
}

void	Server::joinCmd(Client *a, std::string line){
	if (channels.empty())
		
}