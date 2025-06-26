#include "irc.hpp"

int main (int argc, char **argv){
	(void)argc;
	(void)argv;

	std::cout << GREEN << "Building server ..." << RESET << std::endl;
	Server myServer;
	try{
		signal(SIGINT, Server::signalHandler);
		signal(SIGQUIT, Server::signalHandler);
		myServer.serverInit();
		sleep(300);
	}
	catch (const std::exception& e){
		myServer.closeFds();
		std::cerr << e.what() << std::endl;
	}
	std::cout << "the server is closed!" << std::endl;
	return (0);
}