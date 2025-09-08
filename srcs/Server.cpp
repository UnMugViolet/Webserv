#include "Server.hpp"

Server::Server()
{
}
Server::Server(ConfigParser &config, std::string serverId)
{
	sockaddr_in sockaddr;
	int			gotit = 0;
	

	//define ipv4
	sockaddr.sin_family = AF_INET;

	// get server name
	if (config.hasServerKey(serverId, "server_name"))
		_name = config.getServerValue(serverId, "server_name");
	else
		throw servException(serverId + ": no server_name");

	// check if server_name is a valid ip
	if (inet_pton(AF_INET, _name.c_str(), &(sockaddr.sin_addr)))
		gotit = 1;
	
	// try getting ip address with server_name as alias
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *r;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	int status = getaddrinfo(_name.c_str(), 0, &hints, &res);
	if (status == 0)
	{
		r = res;
		while (r != NULL)
		{
			if (r->ai_family == AF_INET)
			{
				gotit = 1;
				struct sockaddr_in *ipv4 = (struct sockaddr_in *)r->ai_addr;
				sockaddr.sin_addr = ipv4->sin_addr;
			}
			r = r->ai_next;
		}

	}
	if (gotit = 0)
		throw servException(serverId + "invalid server_name");

	//get port number
	int			portnbr;

	if (config.hasServerKey(serverId,  "listen"))
		portnbr = std::stoi(config.getServerValue(serverId, "listen"));
	if (portnbr <= 0 || portnbr > 65535)
		throw servException("invalid port number");


	//create listening socket with port number
	sockaddr.sin_port = htons(portnbr);
	_socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketfd == -1)
		throw servException("socket failed");
	if (bind(_socketfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1)
	{
		close(_socketfd);
		throw servException("bind failed");
	}
	if (listen(_socketfd, 10) == -1)
	{
		close(_socketfd);
		throw servException("listen failed");
	}
}

int	Server::getSocket() const
{
	return (_socketfd);
}

int	Server::setClient()
{
	sockaddr_in	peeraddr;
	socklen_t	peer_addr_size = sizeof(peeraddr);

	int cfd = accept(_socketfd, (struct sockaddr *)&peeraddr, &peer_addr_size);
	if (cfd == -1)
	{
		throw servException("accept error");
	}
	_clientFds.push_back(cfd);
}

void	Server::getRequests(fd_set &readFd) const
{
	for (int i = 0; i < _clientFds.size(); i++)
	{
		if (FD_ISSET(_clientFds[i], &readFd))
		{
			;// get request here
		}
	}
}

Server::Server(Server& src)
{
	/*copy what needs to be here*/
}

Server::~Server()
{
}

Server &Server::operator=(Server &other)
{
	if (this != &other)
		*this = other;
	return *this;
}
