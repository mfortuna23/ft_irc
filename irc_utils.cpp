#include "irc.hpp"

void set_nonblocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

void sendMsg(int fd, const char *buffer, size_t len){
	size_t total_sent = 0;

	while (total_sent < len) {
		int sent = send(fd, buffer + total_sent, len - total_sent, 0);
        if (sent <= 0)
            return ;
        total_sent += sent;
    }
}

std::string startMsg(Client *a){
	std::stringstream msg;
	msg << ":" << a->get_nick() << "!~" << a->get_user() << "@" << a->getIp();
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
