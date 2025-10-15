/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unmugviolet <unmugviolet@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:28:50 by pjaguin           #+#    #+#             */
/*   Updated: 2025/10/15 09:57:40 by unmugviolet      ###   ########.fr       */
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
	string					serverUid;
	vector<string>			serverUids;

	
	_config = &config;
	serverUids = config.getServerUids();
	for (size_t i = 0; i < serverUids.size(); i++)
	{
		serverUid = serverUids[i];

		try {
			Server server(config, serverUid);
			_servers.push_back(server);
		} catch (const exception &e) {
			// Failed to create server, log error and continue
			cerr << RED << e.what() << NEUTRAL << endl;
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
			validServers++;
	}
	
	if (validServers == 0) {
		cerr << "No valid servers created. Exiting." << endl;
		return;
	}

	cout << GREEN BOLD << validServers << " SERVER INSTANCE RUNNING" << NEUTRAL << endl;
	
	//mettre les fd d'ecoute de chaque serveur dans readFd
	for (size_t i = 0; i < _servers.size(); i++)
	{
		vector<int> serverFds = _servers[i].getSocket();
		for (vector<int>::iterator it = serverFds.begin(); it != serverFds.end(); it++)
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
			cerr << "No valid file descriptors in set, exiting" << endl;
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
			cerr << "Select error: " << strerror(errno) << endl;
			continue;
		}
		else if (selectResult > 0)
		{
			for (size_t i = 0; i < _servers.size(); i++)
			{
				vector<int> serverFds = _servers[i].getSocket();
				for (vector<int>::iterator it = serverFds.begin(); it != serverFds.end(); it++)
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
			cout << endl << YELLOW BOLD << "SIGINT (Ctrl+C) received. Preparing to shut down..." << NEUTRAL << endl;
			_shutdown = true;
			break;
		case SIGTERM:
			cout << endl << YELLOW BOLD << "SIGTERM received. Preparing to shut down..." << NEUTRAL << endl;
			_shutdown = true;
			break;
		case SIGQUIT:
			cout << endl << YELLOW BOLD << "SIGQUIT (Ctrl+\\) received. Preparing to shut down..." << NEUTRAL << endl;
			_shutdown = true;
			break;
		case SIGPIPE:
			cout << endl << YELLOW BOLD << "SIGPIPE received. Preparing to shut down..." << NEUTRAL << endl;
			_shutdown = true;
			break;
		default:
			cout << endl << YELLOW BOLD << "Unknown signal " << signal << " received. Ignoring..." << NEUTRAL << endl;
			break;
	}
}

// Method to gracefully stop the server
void Webserv::stopServer()
{
	size_t server_count = _servers.size();

	if (server_count == 0)
	{
		cout << BLUE BOLD << "No servers to stop." << NEUTRAL << endl;
		return;
	} 
	else if (server_count == 1)
		cout << BLUE BOLD << "Stopping the server..." << NEUTRAL << endl;
	else 
		cout << BLUE BOLD << "Stopping all servers..." << NEUTRAL << endl;

	// Close all server sockets
	for (size_t i = 0; i < _servers.size(); i++)
	{
		vector<int> serverFds = _servers[i].getSocket();
		string serverUid = _servers[i].getUid();
		for (vector<int>::iterator it = serverFds.begin(); it != serverFds.end(); it++)
		{
			int serverSocket = *it;
			if (serverSocket != -1)
			{
				close(serverSocket);
				cout << BLUE << "Closed socket " << serverSocket << " on server " << serverUid << endl;
			}
		}
		cout << BLUE BOLD << "Closed server " << serverUid << endl;
	}
	
	cout << GREEN BOLD << "Server stopped successfully." << NEUTRAL << endl;
}
