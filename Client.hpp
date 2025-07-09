#ifndef CLIENT_HPP
#define CLIENT_HPP
#pragma once
#include "irc.hpp"

class Client {
	private:
		int fd;
		std::string IpAdr;
		std::string nickname;
		std::string username;
		bool 		is_registered;
		std::string buffer;// acumula os dados recebidos até um "\r\n"
	public :
		Client();
		Client(int fd, const std::string &ip);
		~Client(){};
		// setters
		void setFd(int other){fd = other;};
		void setIp(std::string other){IpAdr = other;};
		void set_nickname(std::string nick){nickname = nick;};
		void set_username(std::string user){username = user;};
		void set_registration(bool y){is_registered = y;};
		// getters
		int getFd(){return fd;};
		std::string getIp(){return IpAdr;};
		std::string get_nick(){return nickname;};
		std::string get_user(){return username;};
		bool get_is_registered(){return is_registered;};
		std::string &get_buffer(){return buffer;};
} ;

#endif