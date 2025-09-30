#include "Server.hpp"
#include "RequestHandler.hpp"

Server::Server()
{
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
		
		// Transfer ownership of the socket to avoid double-close
		const_cast<Server&>(other)._socketfd = -1;
	}
}

Server::~Server()
{
	if (_handler) {
		delete _handler;
		_handler = NULL;
	}
	if (_socketfd != -1) {
		close(_socketfd);
		_socketfd = -1;
	}
}

Server &Server::operator=(const Server &other)
{
	if (this != &other)
	{
		// Close current socket if we have one
		if (_socketfd != -1) {
			close(_socketfd);
		}
		
		this->_IdList = other._IdList;
		this->_socketfd = other._socketfd;
		this->_clientFds = other._clientFds;
		this->_uid = other._uid;
		this->_config = other._config;
		if (this->_handler)
			delete this->_handler;
		this->_handler = new RequestHandler();
		
		// Transfer ownership of the socket to avoid double-close
		const_cast<Server&>(other)._socketfd = -1;
	}
	return *this;
}

Server::Server(ConfigParser &config, std::string serverUid)
{
	sockaddr_in sockaddr;
	int			gotit = 0;
	std::string	_name;
	const char *	c_name;

	this->_config = &config;
	this->_handler = NULL;
	this->_socketfd = -1;
	this->_uid = serverUid;
	std::ostringstream oss;

	try 
	{
		this->_handler = new RequestHandler;
		
		//define ipv4
		sockaddr.sin_family = AF_INET;

		// get server name + root
		if (config.hasServerKey(serverUid, "server_name"))
		{
			_name = config.getServerValue(serverUid, "server_name");
			_IdList[_name] = _uid;
		}
		else {
			Logger::error(serverUid, "No server_name found in the config file for this server not starting up the services");
			throw ServException(serverUid + " no server name found");
		}

		// check if server_name is a valid ip
		c_name = _name.c_str();
		if (ft_inet_pton4(_name, &(sockaddr.sin_addr)))
			gotit = 1;
	
		// try getting ip address with server_name as alias
		struct addrinfo hints;
		struct addrinfo *res;
		struct addrinfo *r;
		int				portnbr;

		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		ft_memset(&hints, 0, sizeof(hints));
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
			freeaddrinfo(res);
		}
		if (gotit == 0) {
			Logger::error(serverUid, "Invalid server_name in the config file");
			throw ServException(serverUid + " has an invalid server_name");
		}

		if (config.hasServerKey(serverUid,  "listen"))
			portnbr = ft_atoi(config.getServerValue(serverUid, "listen").c_str());
		else {
			Logger::error(serverUid, "No port number found in the config file for this server not starting up the services");
			throw ServException(serverUid + " has no port number");
		}
		if (portnbr <= 0 || portnbr > 65535) {
			Logger::error(serverUid, "The port is out of range in config file (1-65535)");
			throw ServException(serverUid + " has invalid port number");
		}

		//create listening socket with port number
		sockaddr.sin_port = htons(portnbr);
		_socketfd = socket(AF_INET, SOCK_STREAM, 0);
		int opt = 1;
		if (_socketfd == -1)
			throw ServException("socket failed");
		if (setsockopt(_socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		{
			close(_socketfd);
			Logger::error(serverUid, "Unexpected error setsockopt failed");
			throw ServException("setsockopt failed");
		}
		if (bind(_socketfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1)
		{
			close(_socketfd);
			oss << portnbr;
			Logger::error(serverUid, "Bind failed on port " + oss.str() + ", possibly already in use check with 'ss -tuln | grep " + oss.str() + "'");
			throw ServException("bind failed for server: " + _uid + " on port " + oss.str());
		}
		if (listen(_socketfd, 10) == -1)
		{
			close(_socketfd);
			oss.str(""); // Clear the stringstream
			oss << portnbr;
			Logger::error(serverUid, "Listen failed on port " + oss.str());
			throw ServException("listen failed for server: " + _uid + " on port " + oss.str());
		}

		//put max body size in handler
		if (config.hasServerKey(serverUid, "client_max_body_size"))
			_handler->setMaxBodySize(config.getServerValue(serverUid, "client_max_body_size"));

		// Setup env
		initEnv(environ);
	} 
	catch (const ServException &e) 
	{
		// Clean up allocated resources before re-throwing
		if (_handler) {
			delete _handler;
			_handler = NULL;
		}
		if (_socketfd != -1) {
			close(_socketfd);
			_socketfd = -1;
		}
		throw; // Re-throw the original exception to parent Webserv
	}
}

int	Server::addVirtualHost(ConfigParser &config, std::string serverUid)
{
	std::string _name;
	sockaddr_in sockaddr;

	sockaddr.sin_family = AF_INET;
	if (config.hasServerKey(serverUid, "server_name") && config.hasServerKey(serverUid, "root"))
	{
		int gotit = 0;

		sockaddr_in serveraddr;
		socklen_t serveraddr_len = sizeof(serveraddr);
		getsockname(_socketfd, (struct sockaddr*)&serveraddr, &serveraddr_len);

		_name = config.getServerValue(serverUid, "server_name");

		// check if server_name is a valid ip
		const char *c_name = _name.c_str();
		if (inet_pton(AF_INET, c_name, &(sockaddr.sin_addr))) // TODO - Not allowed function
			gotit = 1;

		// try getting ip address with server_name as alias
		struct addrinfo hints;
		struct addrinfo *res;
		struct addrinfo *r;
		ft_memset(&hints, 0, sizeof(hints));
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
			freeaddrinfo(res);
		}
		if (gotit == 0) {
			Logger::error(serverUid, "Invalid server_name in the config file for this virtual host not adding it");
			throw ServException(serverUid + " invalid server_name");
		}
		std::string ip = inet_ntoa(sockaddr.sin_addr);
		if (ip.compare(inet_ntoa(serveraddr.sin_addr)) == 0)
		{
			_IdList[_name] = serverUid;
			return 1;
		}
	
		return 0;
	}
	else
	{
		std::cerr << "invalid conf for: " << serverUid << std::endl;
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

    oss << YELLOW 
		<< BOLD
		<< "Connection:"
		<< NEUTRAL
		<< " client "
        << inet_ntoa(client.sin_addr) << ":"  // TODO - Not allowed function
        << ntohs(client.sin_port)
        << " -> server "
        << inet_ntoa(server.sin_addr) << ":"  // TODO - Not allowed function
        << ntohs(server.sin_port);

    return oss.str();
}

int	Server::setClient()
{
	sockaddr_in	peeraddr;
	socklen_t	peer_addr_size = sizeof(peeraddr);

	int cfd = accept(_socketfd, (struct sockaddr *)&peeraddr, &peer_addr_size);
	if (cfd == -1)
		throw ServException("accept error");

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
	for (size_t i = 0; i < _clientFds.size();)
	{
		// Check if the file descriptor is valid
		if (_clientFds[i] < 0)
		{
			// Invalid file descriptor, remove it
			unsetClient(i);
			continue;
		}
		
		if (FD_ISSET(_clientFds[i], &readFd))
		{
			if (_handler->handleRequest(_clientFds[i], *this, config) == -1)
			{
				FD_CLR(_clientFds[i], &fullReadFd);
				close(_clientFds[i]);
				unsetClient(i);
				std::cout << "Client disconnected" << std::endl;
				continue;
			}
		}
		i++;
	}
}

/* 
 * ENV handling for each instance of Server 
*/
void	Server::initEnv(char **env)
{
	if (!env[0])
	{
		Logger::error(_uid, "No environment variables found shutting down server");
		throw ServException("No environment variables found");
		return ;
	}
	for (int i = 0; env[i]; i++)
	{
		std::string var(env[i]);
		size_t pos = var.find('=');
		if (pos != std::string::npos)
		{
			std::string key = var.substr(0, pos);
			std::string value = var.substr(pos + 1);
			_env[key] = value;
		}
	}
	// Set global CGI variables for server instance
	setEnvValue("SERVER_SOFTWARE", "Webserv/1.0");
	setEnvValue("REDIRECT_STATUS", "200");
	setEnvValue("SERVER_PORT", _config->getServerValue(_uid, "listen"));
	setEnvValue("SERVER_ROOT", _config->getServerValue(_uid, "root"));
	setEnvValue("SERVER_PROTOCOL", "HTTP/1.1");
	setEnvValue("GATEWAY_INTERFACE", "CGI/1.1");

	setEnvValue("SERVER_NAME", _config->getServerValue(_uid, "server_name"));
}

std::string	Server::getEnvValue(std::string const &key)
{
	std::map<std::string, std::string>::const_iterator it = _env.find(key);
	if (it != _env.end())
		return it->second;
	else
		return "";
}

void Server::setEnvValue(std::string const &key, std::string const &value)
{
	std::map<std::string, std::string>::iterator it = _env.find(key);
	if (it != _env.end()) 
		it->second = value;
	else if (!key.empty())
		_env[key] = value;
}

char	**Server::getEnvAsArray() const {
	char	**env = new char*[_env.size() + 1];
	int	j = 0;
	for (std::map<std::string, std::string>::const_iterator i = _env.begin(); i != _env.end(); i++) {
		std::string	element = i->first + "=" + i->second;
		env[j] = new char[element.size() + 1];
		env[j] = strcpy(env[j], (const char*)element.c_str());
		j++;
	}
	env[j] = NULL;
	return env;
}

void	Server::printEnv() const
{
	std::cout << YELLOW 
	<< BOLD << YELLOW
	<< "=== Environment Variables ==="
	<< NEUTRAL << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = _env.begin(); it != _env.end(); ++it)
	{
		std::cout << it->first << "=" << it->second << std::endl;
	}
}
