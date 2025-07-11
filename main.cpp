#include "irc.hpp"

int main (int argc, char **argv){
	if (argc != 3){
		std::cout << RED << "Invalid Arguments" << RESET << std::endl;
		return (0);
	}
	std::cout << GREEN << "Building server ..." << RESET << std::endl;
	Server myServer;
	try{
		signal(SIGINT, Server::signalHandler);
		signal(SIGQUIT, Server::signalHandler);
		signal(SIGPIPE, SIG_IGN);
		myServer.serverInit(std::atoi(argv[1]), argv[2]);
	}
	catch (const std::exception& e){
		myServer.closeFds();
		std::cerr << e.what() << std::endl;
	}
	std::cout << "the server is closed!" << std::endl;
	return (0);
}