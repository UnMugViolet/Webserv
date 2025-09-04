/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: andrean <andrean@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:28:50 by pjaguin           #+#    #+#             */
/*   Updated: 2025/09/04 15:05:00 by andrean          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"

Webserv::Webserv()
{
}

// Webserv::Webserv(ConfigParser &config)
// {
	// int			portnb;
	// sockaddr_in sockaddr;
	// std::string	serverId;
	
	// sockaddr.sin_family = AF_INET;

	// serverIds = config.getServerIds();
	// for (size_t i = 0; i < serverIds.size(); i++)
	// {	
	// 	sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	// 	serverId = serverIds[i];
	// 	portnb = std::stoi(config.getServerValue(serverId, "listen"));
	// 	if (portnb <= 0 || portnb > 65535)
	// 		throw WebservException("invalid port number");
	// 	sockaddr.sin_port = htons(portnb);
	// 	int fd = socket(AF_INET, SOCK_STREAM, 0);
	// 	if (fd == -1)
	// 		throw WebservException("socket failed");
	// 	if (bind(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1)
	// 	{
	// 		close(fd);
	// 		throw WebservException("bind failed");
	// 	}
	// 	if (listen(fd, 10) == -1)
	// 	{
	// 		close(fd);
	// 		throw WebservException("listen failed");
	// 	}
	// 	sockfds.push_back(fd);
	// }
// }

Webserv::~Webserv()
{
}

// void	Webserv::initWebserv(ConfigParser &config)
// {
// 	serverIds.
// }