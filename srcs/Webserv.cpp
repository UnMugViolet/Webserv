/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: andrean <andrean@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:28:50 by pjaguin           #+#    #+#             */
/*   Updated: 2025/09/08 12:23:53 by andrean          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"

Webserv::Webserv()
{
}

Webserv::Webserv(ConfigParser &config)
{
	std::string	serverId;
	std::vector<std::string>	serverIds;
	
	serverIds = config.getServerIds();
	for (size_t i = 0; i < serverIds.size(); i++)
	{
		serverId = serverIds[i];
		Server server(config, serverId);
		_servers.push_back(server);
	}
}


void Webserv::serverLoop()
{
	int		fd;
	int		maxFd = 0;
	fd_set fullReadFd;
	fd_set readFd;
	FD_ZERO(&readFd);
	FD_ZERO(&fullReadFd);
	
	//mettre les fd d'ecoute de chaque serveur dans readFd
	for (size_t i = 0; i < _servers.size(); i++)
	{
		fd = _servers[i].getSocket();
		if (fd > maxFd)
			maxFd = fd;
		FD_SET(fd, &fullReadFd);
	}

	//ecoute sur la liste de fd, puis boucle sur chaque fd de chaque serveur pour verifier lesquels sont actifs
	while (true)
	{
		readFd = fullReadFd;
		select(maxFd + 1, &readFd, NULL, NULL, NULL);
		for (size_t i = 0; i < _servers.size(); i++)
		{
			if (FD_ISSET(_servers[i].getSocket(), &readFd))
			{
				FD_SET(_servers[i].setClient(), &fullReadFd);
			}
			_servers[i].getRequests(readFd);
		}
		
	}
}

Webserv::~Webserv()
{
}

// void	Webserv::initWebserv(ConfigParser &config)
// {
// 	serverIds.
// }