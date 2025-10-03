/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: andrean <andrean@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:28:50 by pjaguin           #+#    #+#             */
/*   Updated: 2025/10/03 12:29:10 by andrean          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"

// Initialize static member
bool Webserv::_shutdown = false;

Webserv::Webserv()
{
	_servers.clear();
	_config = NULL;
}

Webserv::Webserv(ConfigParser &config)
{
	std::string					serverUid;
	std::vector<std::string>	serverUids;

	
	_config = &config;
	serverUids = config.getServerUids();
	for (size_t i = 0; i < serverUids.size(); i++)
	{
		serverUid = serverUids[i];

		try {
			Server server(config, serverUid);
			_servers.push_back(server);
		} catch (const std::exception &e) {
			// Failed to create server, log error and continue
			std::cerr << RED << e.what() << NEUTRAL << std::endl;
		}
	}
}

Webserv::~Webserv()
{
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
		if (!_servers[i].getSocket().empty())
		{
			validServers++;
		}
	}
	
	if (validServers == 0) {
		std::cerr << "No valid servers created. Exiting." << std::endl;
		return;
	}

	std::cout << GREEN BOLD << validServers << " SERVER INSTANCE RUNNING" << NEUTRAL << std::endl;
	
	//mettre les fd d'ecoute de chaque serveur dans readFd
	for (size_t i = 0; i < _servers.size(); i++)
	{
		std::vector<int> serverFds = _servers[i].getSocket();
		for (std::vector<int>::iterator it = serverFds.begin(); it != serverFds.end(); it++)
		{
			fd = *it;
			if (fd != -1) {  // Only add valid file descriptors
				if (fd > maxFd)
					maxFd = fd;
				FD_SET(fd, &fullReadFd);
			}
		}
	}

	// ecoute sur la liste de fd, puis boucle sur chaque fd de chaque serveur pour verifier lesquels sont actifs
	while (!_shutdown)
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
		
		// Use a timeout for select to periodically check shutdown flag
		struct timeval timeout;
		timeout.tv_sec = 1;  // 1 second timeout
		timeout.tv_usec = 0;
		
		int selectResult = select(maxFd + 1, &readFd, NULL, NULL, &timeout);
		
		if (selectResult < 0)
		{
			if (errno == EINTR)
			{
				// Interrupted by signal, check shutdown flag
				continue;
			}
			std::cerr << "Select error: " << strerror(errno) << std::endl;
			continue;
		}
		else if (selectResult > 0)
		{
			for (size_t i = 0; i < _servers.size(); i++)
			{
				std::vector<int> serverFds = _servers[i].getSocket();
				for (std::vector<int>::iterator it = serverFds.begin(); it != serverFds.end(); it++)
				{
					if (*it != -1 && FD_ISSET(*it, &readFd))
					{
						fd = _servers[i].setClient(*it);
						if (fd != -1) // Only add valid client file descriptors
						{
							FD_SET(fd, &fullReadFd);
							if (fd > maxFd)
								maxFd = fd;
						}
					}
				}
				_servers[i].getRequests(readFd, fullReadFd, _config);
			}
		}
		// If selectResult == 0, it's a timeout, loop continues to check shutdown flag
	}
	
	// If we exit the loop due to shutdown signal, clean up gracefully
	if (_shutdown)
		stopServer();
}

// Static signal handler
void Webserv::signalHandler(int signal)
{
	switch (signal)
	{
		case SIGINT:
			std::cout << std::endl << YELLOW BOLD << "SIGINT (Ctrl+C) received. Preparing to shut down..." << NEUTRAL << std::endl;
			_shutdown = true;
			break;
		case SIGTERM:
			std::cout << std::endl << YELLOW BOLD << "SIGTERM received. Preparing to shut down..." << NEUTRAL << std::endl;
			_shutdown = true;
			break;
		case SIGQUIT:
			std::cout << std::endl << YELLOW BOLD << "SIGQUIT (Ctrl+\\) received. Preparing to shut down..." << NEUTRAL << std::endl;
			_shutdown = true;
			break;
		case SIGPIPE:
			std::cout << std::endl << YELLOW BOLD << "SIGPIPE received. Preparing to shut down..." << NEUTRAL << std::endl;
			_shutdown = true;
			break;
		default:
			std::cout << std::endl << YELLOW BOLD << "Unknown signal " << signal << " received. Ignoring..." << NEUTRAL << std::endl;
			break;
	}
}

// Method to gracefully stop the server
void Webserv::stopServer()
{
	size_t server_count = _servers.size();

	if (server_count == 0)
	{
		std::cout << BLUE BOLD << "No servers to stop." << NEUTRAL << std::endl;
		return;
	} 
	else if (server_count == 1)
		std::cout << BLUE BOLD << "Stopping the server..." << NEUTRAL << std::endl;
	else 
		std::cout << BLUE BOLD << "Stopping all servers..." << NEUTRAL << std::endl;

	// Close all server sockets
	for (size_t i = 0; i < _servers.size(); i++)
	{
		std::vector<int> serverFds = _servers[i].getSocket();
		std::string serverUid = _servers[i].getUid();
		for (std::vector<int>::iterator it = serverFds.begin(); it != serverFds.end(); it++)
		{
			int serverSocket = *it;
			if (serverSocket != -1)
			{
				close(serverSocket);
				std::cout << BLUE << "Closed socket " << serverSocket << " on server " << serverUid << std::endl;
			}
		}
		std::cout << BLUE BOLD << "Closed server " << serverUid << std::endl;
	}
	
	std::cout << GREEN BOLD << "Server stopped successfully." << NEUTRAL << std::endl;
}
