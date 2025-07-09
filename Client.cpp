/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alex <alex@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/07 16:32:44 by alex              #+#    #+#             */
/*   Updated: 2025/07/08 11:50:06 by alex             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client()
	: fd(-1), IpAdr(""), nickname(""), username(""),
	  is_registered(false), buffer("") {}

Client::Client(int fd, const std::string &ip)
	: fd(fd), IpAdr(ip), nickname(""), username(""), is_registered(false), buffer("") {}