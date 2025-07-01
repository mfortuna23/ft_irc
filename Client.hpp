#ifndef CLIENT_HPP
#define CLIENT_HPP
#pragma once
#include "irc.hpp"

class Client {
	private:
		int fd;
		std::string IpAdr;
	public :
		Client(){};
		int getFd(){return fd;};
		std::string getIp(){return IpAdr;};
		void setFd(int other){fd = other;};
		void setIp(std::string other){IpAdr = other;};
		~Client(){};
} ;

#endif