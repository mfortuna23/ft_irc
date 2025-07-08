#ifndef CLIENT_HPP
#define CLIENT_HPP
#pragma once
#include "irc.hpp"

class Client {
	private:
		int fd;
		std::string username;
		std::string IpAdr;
		bool active;
	public :
		Client(){active = false;};
		int getFd(){return fd;};
		std::string getUsername(void){return username;};
		std::string getIp(){return IpAdr;};
		bool getActive(void){return active;};
		void setFd(int other){fd = other;};
		void setIp(std::string other){IpAdr = other;};
		void setUsername(std::string name){username = name;};
		void setActive(void){active = true;};
		~Client(){};
} ;

#endif