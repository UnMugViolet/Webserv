/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: andrean <andrean@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:28:50 by pjaguin           #+#    #+#             */
/*   Updated: 2025/09/08 11:18:40 by andrean          ###   ########.fr       */
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


void Webserv::serverLoop(std::vector<Server> servers)
{
	int		fd;
	int		maxFd = 0;
	fd_set fullReadFd;
	fd_set readFd;
	FD_ZERO(&readFd);
	FD_ZERO(&fullReadFd);
	
	for (int i = 0; i < servers.size(); i++)
	{
		fd = servers[i].getSocket();
		if (fd > maxFd)
			maxFd = fd;
		FD_SET(fd, &fullReadFd);
	}
	while (true)
	{
		select()
	}
}

Webserv::~Webserv()
{
}

// void	Webserv::initWebserv(ConfigParser &config)
// {
// 	serverIds.
// }