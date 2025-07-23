/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mfortuna <mfortuna@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/07 16:32:44 by alex              #+#    #+#             */
/*   Updated: 2025/07/21 09:50:06 by mfortuna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client()
	: fd(-1), IpAdr(""), nickname(""), username(""),
	  is_registered(false), buffer(""), regist_3steps(3) {}

Client::Client(int fd, const std::string &ip)
	: fd(fd), IpAdr(ip), nickname(""), username(""), 
	is_registered(false), buffer(""), regist_3steps(3) {}

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
