#ifndef CLIENT_HPP
#define CLIENT_HPP
#pragma once
#include "irc.hpp"
#include "Server.hpp"

class Channel;
class Server;

class Client {
	private:
		int fd;
		std::string IpAdr;
		std::string nickname;
		std::string username;
		bool 		is_registered;
		std::string buffer;// acumula os dados recebidos até um "\r\n"
		int			regist_3steps;// comeca com 3 (3 estapas: nick, user e pass) e reduz ate 0
		Channel		*myChannel;
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
		void confirm_regist_step(Server *srv);
		// getters
		int getFd(){return fd;};
		std::string getIp(){return IpAdr;};
		std::string get_nick(){return nickname;};
		std::string get_user(){return username;};
		bool get_is_registered(){return is_registered;};
		int get_regist_steps(){return regist_3steps;};
		std::string &get_buffer(){return buffer;};
		void set_channel(Channel *c) { myChannel = c; }
		Channel* get_channel() { return myChannel; }
} ;

#endif