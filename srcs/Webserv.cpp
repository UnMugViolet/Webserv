/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: andrean <andrean@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:28:50 by pjaguin           #+#    #+#             */
/*   Updated: 2025/09/11 10:59:57 by andrean          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"

Webserv::Webserv()
{
	_config = NULL;
}

Webserv::Webserv(ConfigParser &config)
{
	std::string					serverId;
	std::vector<std::string>	serverIds;

	
	_config = &config;  // Store the config pointer
	serverIds = config.getServerIds();
	for (size_t i = 0; i < serverIds.size(); i++)
	{
		int	gotServ = 0;
		serverId = serverIds[i];

		std::string port;
		if (config.hasServerKey(serverId, "listen"))
		{
			port = config.getServerValue(serverId, "listen");
			for (std::vector<Server>::iterator it = _servers.begin(); it != _servers.end(); it++)
			{
				if (config.getServerValue(it->getUid(), "listen") == port)
				{
					gotServ = it->addVirtualHost(config, serverId);;
					break ;
				}
			}
			if (gotServ)
				continue;
		}
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
		if (select(maxFd + 1, &readFd, NULL, NULL, NULL) < 0)
			std::cerr << strerror(errno) << std::endl;
		else
		{
			for (size_t i = 0; i < _servers.size(); i++)
			{
				if (FD_ISSET(_servers[i].getSocket(), &readFd))
				{
					fd = _servers[i].setClient();
					FD_SET(fd, &fullReadFd);
					if (fd > maxFd)
						maxFd = fd;
				}
				_servers[i].getRequests(readFd, fullReadFd, _config);  // Pass the config
			}
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
