#include "Server.hpp"
#include "RequestHandler.hpp"

Server::Server()
{
}

Server::Server(ConfigParser &config, std::string serverId)
{
	sockaddr_in sockaddr;
	int			gotit = 0;
	std::string	_name;
	const char *	c_name;

	this->_config = &config;
	this->_handler = new RequestHandler;
	this->_uid = serverId;
	//define ipv4
	sockaddr.sin_family = AF_INET;

	// get server name + root
	if (config.hasServerKey(serverId, "host"))
	{
		_name = config.getServerValue(serverId, "host");
		_IdList[_name] = _uid;
	}
	else
		throw servException(serverId + ": no host");

	// check if host is a valid ip
	c_name = _name.c_str();
	if (inet_pton(AF_INET, c_name, &(sockaddr.sin_addr)))
		gotit = 1;
	
	// try getting ip address with host as alias
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *r;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	int status = getaddrinfo(c_name, 0, &hints, &res);
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
		throw servException(serverId + "invalid host");

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
		_handler->setMaxBodySize(config.getServerValue(serverId, "client_max_body_size"));
}

int	Server::addVirtualHost(ConfigParser &config, std::string serverId)
{
	std::string _name;
	sockaddr_in sockaddr;

	sockaddr.sin_family = AF_INET;
	if (config.hasServerKey(serverId, "host") && config.hasServerKey(serverId, "root"))
	{
		int gotit = 0;

		sockaddr_in serveraddr;
		socklen_t serveraddr_len = sizeof(serveraddr);
		getsockname(_socketfd, (struct sockaddr*)&serveraddr, &serveraddr_len);

		_name = config.getServerValue(serverId, "host");

		// check if host is a valid ip
		const char *c_name = _name.c_str();
		if (inet_pton(AF_INET, c_name, &(sockaddr.sin_addr)))
			gotit = 1;

		// try getting ip address with host as alias
		struct addrinfo hints;
		struct addrinfo *res;
		struct addrinfo *r;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		int status = getaddrinfo(c_name, 0, &hints, &res);
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
			throw servException(serverId + " invalid host");
		std::string ip = inet_ntoa(sockaddr.sin_addr);
		if (ip.compare(inet_ntoa(serveraddr.sin_addr)) == 0)
		{
			_IdList[_name] = serverId;
			return 1;
		}
	
		return 0;
	}
	else
	{
		std::cerr << "invalid conf for: " << serverId << std::endl;
		return (1);
	}
}

int	Server::getSocket() const
{
	return (_socketfd);
}

std::string Server::getUid() const
{
	return _uid;
}

std::string Server::getId(const std::string &name) const
{
	std::map<std::string, std::string>::const_iterator it = _IdList.find(name);
	if (it != _IdList.end())
		return it->second;
	return (_uid);
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
		throw servException("accept error");

	// Log the connection info with server details
	sockaddr_in serveraddr;
	socklen_t serveraddr_len = sizeof(serveraddr);
	getsockname(_socketfd, (struct sockaddr*)&serveraddr, &serveraddr_len);
	std::string connexion = get_connection_info(peeraddr, serveraddr);
	std::string logInfo = connexion + " [Server: " + _uid + ", Root: " + _config->getServerValue(_uid, "root") + "]";
	Logger::access(this->_uid, logInfo);
	std::cout << logInfo << std::endl;

	_clientFds.push_back(cfd);
	return (cfd);
}

void	Server::unsetClient(int position)
{
	_clientFds.erase(_clientFds.begin()+position);
}


void	Server::getRequests(fd_set &readFd, fd_set &fullReadFd, ConfigParser* config)
{
	for (size_t i = 0; i < _clientFds.size(); i++)
	{
		if (FD_ISSET(_clientFds[i], &readFd))
		{
			if (_handler->handleRequest(_clientFds[i], *this, config, _uid) == -1)
			{
				FD_CLR(_clientFds[i], &fullReadFd);
				close(_clientFds[i]);
				unsetClient(i);
				std::cout << "Client disconnected" << std::endl;
			}
		}
	}
}

Server::Server(const Server &other)
{
	if (this != &other)
	{
		this->_IdList = other._IdList;
		this->_socketfd = other._socketfd;
		this->_clientFds = other._clientFds;
		this->_uid = other._uid;  // Lost 2 hours of my life because of this
		this->_config = other._config;
		this->_handler = new RequestHandler();
	}
}

Server::~Server()
{
	delete _handler;
}

Server &Server::operator=(const Server &other)
{
	if (this != &other)
	{
		this->_IdList = other._IdList;
		this->_socketfd = other._socketfd;
		this->_clientFds = other._clientFds;
		this->_uid = other._uid;
		this->_config = other._config;
		if (this->_handler)
			delete this->_handler;
		this->_handler = new RequestHandler();
	}
	return *this;
}
