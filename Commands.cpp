#include "irc.hpp"
#include "Server.hpp"

bool Server::isThisCmd(const std::string& line, std::string cmd){
	std::istringstream iss(line);
	std::string firstWord;
	if (iss >> firstWord)
		return (toUpper(firstWord) == cmd);
	return false;
}

// CAP <capabilities>
void Server::cmdCAP(Client *cli, std::string line) {
	(void)line;
	std::string reply = "CAP * LS :\r\n";
	sendMsg(cli->getFd(), reply.c_str(), reply.size());
}

// PASS <password>
void Server::cmdPASS(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, pass;
	iss >> cmd >> pass;

	if (cli->get_is_registered()) {
		sendMsg(cli->getFd(), "ERROR :You are already registered\r\n", 36);
		return;}
	if (pass.empty()) {
		sendMsg(cli->getFd(), "ERROR :No password given\r\n", 27);
		return;}
	if (pass != this->get_pass()) {
		sendMsg(cli->getFd(), "ERROR :Password incorrect\r\n", 28);
		close(cli->getFd());
		return;}
	if (cli->get_regist_steps() == 3)
		cli->confirm_regist_step(this);
}

// NICK <nickname>
void Server::cmdNICK(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, nick;
	if (cli->get_regist_steps() > 2)
		return ;
	
	iss >> cmd >> nick;

	if (nick.empty()) {
		sendMsg(cli->getFd(), "ERROR :No nickname given\r\n", 27);
		return;
	}
	cli->set_nickname(nick);
	std::string msg = "You're now known as " + cli->get_nick() + "\r\n";
	sendMsg(cli->getFd(), msg.c_str(), msg.size());
	if (cli->get_regist_steps() == 2)
		cli->confirm_regist_step(this);
}

// USER <username> 0 * :realname
void Server::cmdUSER(Client *cli, std::string line) {
	std::istringstream iss(line);
	std::string cmd, user, unused, asterisk, realname;

	if (cli->get_regist_steps() > 2)
		return ;
	
	iss >> cmd >> user >> unused >> asterisk;
	std::getline(iss, realname); //getline serve para capturar tudo que vem depois dos 4 primeiros campos, mesmo que contenha espaços.
	// como nao vamos nos aprofundar muito, nao precisamos salvar nada alem do user.
	if (user.empty()) {
		sendMsg(cli->getFd(), "ERROR :No username given\r\n", 27);
		return;
	}
	cli->set_username(user);
	std::string msg = "Your username now is " + cli->get_user() + "\r\n";
	sendMsg(cli->getFd(), msg.c_str(), msg.size());
	
	if (cli->get_regist_steps() == 1)
		cli->confirm_regist_step(this);
}


// void	Server::joinCmd(Client *a, std::string line){
// 	if (channels.empty())
		
// }