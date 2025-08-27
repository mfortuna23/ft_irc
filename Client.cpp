/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alex <alex@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/07 16:32:44 by alex              #+#    #+#             */
/*   Updated: 2025/08/26 23:26:52 by alex             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client()
	: fd(-1), IpAdr(""), nickname(""), username(""),
	  is_registered(false), buffer(""), regist_3steps(3), outbox(""), want_close(false) {}

Client::Client(int fd, const std::string &ip)
	: fd(fd), IpAdr(ip), nickname(""), username(""), 
	is_registered(false), buffer(""), regist_3steps(3), outbox(""), want_close(false) {}

void Client::confirm_regist_step(Server *srv)
{
	if (regist_3steps > 0)
		regist_3steps--; 
	if (regist_3steps == 0 && !is_registered)
	{
		is_registered = true;
		srv->checkRegistration(this);
	}
}
void Client::newChannel(Channel *other){myChannels.insert(std::make_pair(other->getName(), other));}
void Client::rmChannel(Channel *other){myChannels.erase(other->getName());}