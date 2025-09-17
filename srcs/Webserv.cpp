/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unmugviolet <unmugviolet@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:28:50 by pjaguin           #+#    #+#             */
/*   Updated: 2025/09/17 13:27:39 by unmugviolet      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"

Webserv::Webserv()
{
	_config = NULL;
}

Webserv::Webserv(ConfigParser &config)
{
	std::string					serverUid;
	std::vector<std::string>	serverUids;

	
	_config = &config;  // Store the config pointer
	serverUids = config.getServerUids();
	for (size_t i = 0; i < serverUids.size(); i++)
	{
		int	gotServ = 0;
		serverUid = serverUids[i];

		std::string port;
		if (config.hasServerKey(serverUid, "listen"))
		{
			port = config.getServerValue(serverUid, "listen");
			for (std::vector<Server>::iterator it = _servers.begin(); it != _servers.end(); it++)
			{
				if (config.getServerValue(it->getUid(), "listen") == port)
				{
					gotServ = it->addVirtualHost(config, serverUid);;
					break ;
				}
			}
			if (gotServ)
				continue;
		}
		try {
			Server server(config, serverUid);
			_servers.push_back(server);
		} catch (const std::exception &e) {
			// Failed to create server, log error and continue
			std::cerr << RED << e.what() << NEUTRAL << std::endl;
		}
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

	// Check if we have any valid servers
	int validServers = 0;
	for (size_t i = 0; i < _servers.size(); i++)
	{
		if (_servers[i].getSocket() != -1)
			validServers++;
	}
	
	if (validServers == 0) {
		std::cerr << "No valid servers created. Exiting." << std::endl;
		return;
	}

	std::cout << GREEN BOLD << validServers << " SERVER INSTANCE RUNNING" << NEUTRAL << std::endl;
	
	//mettre les fd d'ecoute de chaque serveur dans readFd
	for (size_t i = 0; i < _servers.size(); i++)
	{
		fd = _servers[i].getSocket();
		if (fd != -1) {  // Only add valid file descriptors
			if (fd > maxFd)
				maxFd = fd;
			FD_SET(fd, &fullReadFd);
		}
	}

	//ecoute sur la liste de fd, puis boucle sur chaque fd de chaque serveur pour verifier lesquels sont actifs
	while (true)
	{
		readFd = fullReadFd;
		
		// Recalculate maxFd to ensure it's accurate and all FDs are valid
		maxFd = 0;
		for (int testFd = 0; testFd < FD_SETSIZE; testFd++)
		{
			if (FD_ISSET(testFd, &fullReadFd))
			{
				// Validate the file descriptor before including it
				int flags = fcntl(testFd, F_GETFL);
				if (flags == -1)
				{
					// Invalid file descriptor, remove it
					FD_CLR(testFd, &fullReadFd);
				}
				else
				{
					if (testFd > maxFd)
						maxFd = testFd;
				}
			}
		}
		
		if (maxFd == 0)
		{
			std::cerr << "No valid file descriptors in set, exiting" << std::endl;
			break;
		}
		
		if (select(maxFd + 1, &readFd, NULL, NULL, NULL) < 0)
		{
			std::cerr << "Select error: " << strerror(errno) << std::endl;
			continue; // Continue instead of just printing error
		}
		else
		{
			for (size_t i = 0; i < _servers.size(); i++)
			{
				int serverFd = _servers[i].getSocket();
				if (serverFd != -1 && FD_ISSET(serverFd, &readFd))
				{
					fd = _servers[i].setClient();
					if (fd != -1) {  // Only add valid client file descriptors
						FD_SET(fd, &fullReadFd);
						if (fd > maxFd)
							maxFd = fd;
					}
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
