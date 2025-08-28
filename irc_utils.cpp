#include "irc.hpp"

void set_nonblocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

void sendMsg(int fd, const char *buffer, size_t len){
	// Agora, toda escrita passa pelo poll() do servidor:
	// apenas colocamos na fila de saída e habilitamos POLLOUT.
	Server* s = Server::getInstance(); // para termos acesso ao objeto do tipo server que ja estamos usando.
	if (!s || fd < 0 || !buffer || len == 0)
		return;
	s->enqueueSend(fd, buffer, len);
}

std::string startMsg(Client *cli){
	std::stringstream msg;
	msg << ":" << cli->get_nick() << "!~" << cli->get_user() << "@" << cli->getIp();
	std::cout << msg.str() << std::endl;
	return msg.str();
}

std::string toUpper(const std::string &str) {
	std::string upper = str;
	// Aplica toupper() em cada caractere, sobrescrevendo a string upper
	// O uso de :: antes de toupper diz ao compilador -> função global (não está e nem busque em nenhum namespace)
	std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
	return upper;
}

bool	checkNbr(std::string nbr){
	for (int i = 0; nbr[i]; i++){
		if (nbr[i] && (nbr[i] < '0' || nbr[i] > '9'))
			return false;
	}
	return true;
}

bool	checkChannelName(std::string name){
	if (name.empty() || (name[0] != '#' && name[0] != '&') || name[1] == 0) //channel cannot be just a #
		return false ;
	for (int i = 1; name[i]; i++){
		if (name[i] < '0' || name[i] == ' ' || name[i] == ':' || name[i] == ';') //invalid channel characters
			return false ;
		if (i == 50) //character limit for a channel name
			return false ;
	}
	return true ;
}
