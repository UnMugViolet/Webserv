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
	if (gotit == 0)
		throw servException(serverId + "invalid server_name");

	//get port number
	int			portnbr;

	if (config.hasServerKey(serverId,  "listen"))
		portnbr = atoi(config.getServerValue(serverId, "listen").c_str());
	else
		throw servException("no port number");
	if (portnbr <= 0 || portnbr > 65535)
		throw servException("invalid port number");


	//create listening socket with port number
	sockaddr.sin_port = htons(portnbr);
	_socketfd = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;
	if (_socketfd == -1)
		throw servException("socket failed");
	if (setsockopt(_socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw servException("setsockopt failed");
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

	//put max body size in handler
	if (config.hasServerKey(serverId, "client_max_body_size"))
		_handler.setMaxBodySize(config.getServerValue(serverId, "client_max_body_size"));
}

int	Server::getSocket() const
{
	return (_socketfd);
}

static std::string get_connection_info(const sockaddr_in& client, const sockaddr_in& server) {
    std::ostringstream oss;

    oss << "Connection: client "
        << inet_ntoa(client.sin_addr) << ":"
        << ntohs(client.sin_port)
        << " -> server "
        << inet_ntoa(server.sin_addr) << ":"
        << ntohs(server.sin_port);

    return oss.str();
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
	//coucou
	char buf[91] = "HTTP/1.1 200 OK\r\nContent-Length: 23\r\nContent-Type: text/plain\r\n\r\nAlan c'est le plus beau";
	if (send(cfd, buf, strlen(buf), 0) == -1)
	{
		std::cerr << "send error : " << strerror(errno) << std::endl;
		return 1;
	}
	else
	{
		sockaddr_in serveraddr;
		socklen_t serveraddr_len = sizeof(serveraddr);
		getsockname(_socketfd, (struct sockaddr*)&serveraddr, &serveraddr_len);
		std::string connexion = get_connection_info(peeraddr, serveraddr);
		Logger::access(this->_name, connexion);
		std::cout << connexion << std::endl;
	}
	_clientFds.push_back(cfd);
	_handler.printRequest(cfd);
	return (cfd);
}

void	Server::unsetClient(int position)
{
	_clientFds.erase(_clientFds.begin()+position);
}

void	Server::getRequests(fd_set &readFd, fd_set &fullReadFd)
{
	for (size_t i = 0; i < _clientFds.size(); i++)
	{
		if (FD_ISSET(_clientFds[i], &readFd))
		{
			if (_handler.printRequest(_clientFds[i]) == -1)
			{
				FD_CLR(_clientFds[i], &fullReadFd);
				close(_clientFds[i]);
				unsetClient(i);
			}
		}
	}
}

Server::Server(const Server &other)
{
	if (this != &other)
	{
		this->_name = other._name;
		// this->_RequestMaxSize = other._RequestMaxSize;
		this->_socketfd = other._socketfd;
		this->_clientFds = other._clientFds;
	}
}

Server::~Server()
{
}

Server &Server::operator=(const Server &other)
{
	if (this != &other)
	{
		this->_name = other._name;
		// this->_RequestMaxSize = other._RequestMaxSize;
		this->_socketfd = other._socketfd;
		this->_clientFds = other._clientFds;
	}
	return *this;
}
