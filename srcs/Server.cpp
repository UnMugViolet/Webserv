#include "Server.hpp"
#include "RequestHandler.hpp"
#include <unistd.h>
class ARequest;

Server::Server() {}

Server::Server(const Server &other)
{
	if (this != &other)
	{
		this->_socketfds = other._socketfds;
		this->_clientFds = other._clientFds;
		this->_uid = other._uid; // Lost 2 hours of my life because of this
		this->_config = other._config;
		this->_handler = new RequestHandler();
		this->_env = other._env;
		this->_server_names = other._server_names;
		this->_keepalive = other._keepalive;
		this->_cgi_for_client = other._cgi_for_client;
		this->_pid_for_cgi = other._pid_for_cgi;
		this->_cgi_request = other._cgi_request;

		// Transfer ownership of the socket to avoid double-close

		for (vector<int>::iterator it = const_cast<Server &>(other)._socketfds.begin(); it != const_cast<Server &>(other)._socketfds.end(); it++)
		{
			if (*it > -1)
				*it = -1;
		}
	}
}

Server::Server(ConfigParser &config, string serverUid)
{
	vector<sockaddr_in> sockvector;
	vector<int> portvector;

	this->_config = &config;
	this->_handler = NULL;
	// this->_socketfd = -1;
	this->_uid = serverUid;

	try
	{
		this->_handler = new RequestHandler;

		// check if servername is present;
		if (!config.hasServerKey(serverUid, "server_name"))
		{
			Logger::error(serverUid, "No server_name found in the config file for this server not starting up the services");
			throw ServException(serverUid + " no server name found");
		}

		// fill server_names vector based on config and return vector with all sockaddr
		sockvector = setServerNames(config, serverUid);

		// get a vector containing all port numbers if valid
		portvector = checkPorts(config, serverUid);

		// fill sockFds
		createSockets(serverUid, portvector, sockvector);

		// put max body size in handler
		if (config.hasServerKey(serverUid, "client_max_body_size"))
			_handler->setMaxBodySize(config.getServerValue(serverUid, "client_max_body_size"));

		// Setup env
		initEnv(environ);
	}
	catch (const ServException &e)
	{
		// Clean up allocated resources before re-throwing
		if (_handler)
		{
			delete _handler;
			_handler = NULL;
		}
		if (!_socketfds.empty())
		{
			for (vector<int>::iterator it = _socketfds.begin(); it != _socketfds.end(); it++)
			{
				if (*it > -1)
				{
					close(*it);
					*it = -1;
				}
			}
		}
		throw; // Re-throw the original exception to parent Webserv
	}
}

Server::~Server()
{
	if (_handler)
	{
		delete _handler;
		_handler = NULL;
	}
	if (!_clientFds.empty())
	{
		for (vector<int>::iterator it = _clientFds.begin(); it != _clientFds.end(); it++)
		{
			if (*it > -1)
			{
				close(*it);
				*it = -1;
			}
		}
	}
	if (_socketfds.empty())
	{
		for (vector<int>::iterator it = _socketfds.begin(); it != _socketfds.end(); it++)
		{
			if (*it > -1)
			{
				close(*it);
				*it = -1;
			}
		}
	}
}

Server &Server::operator=(const Server &other)
{
	if (this != &other)
	{
		// Close current socket if we have one
		if (_socketfds.empty())
		{
			for (vector<int>::iterator it = _socketfds.begin(); it != _socketfds.end(); it++)
			{
				if (*it > -1)
				{
					close(*it);
					*it = -1;
				}
			}
		}

		this->_socketfds = other._socketfds;
		this->_clientFds = other._clientFds;
		this->_uid = other._uid;
		this->_config = other._config;
		if (this->_handler)
			delete this->_handler;
		this->_handler = new RequestHandler();

		// Transfer ownership of the socket to avoid double-close
		for (vector<int>::iterator it = const_cast<Server &>(other)._socketfds.begin(); it != const_cast<Server &>(other)._socketfds.end(); it++)
		{
			if (*it > -1)
				*it = -1;
		}
	}
	return (*this);
}

void Server::createSockets(const string &serverUid, vector<int> &ports, vector<sockaddr_in> &sockaddrs)
{
	ostringstream oss;

	for (vector<sockaddr_in>::iterator it = sockaddrs.begin(); it != sockaddrs.end(); it++)
	{
		sockaddr_in sockaddr = *it;

		for (vector<int>::iterator port_it = ports.begin(); port_it != ports.end(); port_it++)
		{
			int portnbr = *port_it;

			// create listening socket with port number
			sockaddr.sin_port = htons(portnbr);
			int _socketfd = socket(AF_INET, SOCK_STREAM, 0);
			int opt = 1;
			if (_socketfd == -1)
				throw(ServException("socket failed"));
			if (setsockopt(_socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
			{
				close(_socketfd);
				Logger::error(serverUid, "Unexpected error setsockopt failed");
				throw(ServException("setsockopt failed"));
			}
			if (bind(_socketfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1)
			{
				close(_socketfd);
				oss.str("");
				oss << portnbr;
				Logger::error(serverUid, "Bind failed on port " + oss.str() + ", possibly already in use check with 'ss -tuln | grep " + oss.str() + "'");
				throw(ServException("bind failed for server: " + _uid + " on port " + oss.str()));
			}
			if (listen(_socketfd, 10) == -1)
			{
				close(_socketfd);
				oss.str(""); // Clear the stringstream
				oss << portnbr;
				Logger::error(serverUid, "Listen failed on port " + oss.str());
				throw(ServException("listen failed for server: " + _uid + " on port " + oss.str()));
			}
			_socketfds.push_back(_socketfd);
		}
	}
}

vector<sockaddr_in> Server::setServerNames(const ConfigParser &config, const string &serverUid)
{
	vector<sockaddr_in> sockvector;
	string _names = config.getServerValue(serverUid, "server_name");

	while (true)
	{
		sockaddr_in sockaddr;
		sockaddr.sin_family = AF_INET;
		int gotit = 0;
		string _one_name = _names.substr(0, _names.find(' '));

		// check if server_name is a valid ip
		const char *c_name = _one_name.c_str();
		if (ft_inet_pton4(_one_name, &(sockaddr.sin_addr)))
			gotit = 1;

		// try getting ip address with server_name as alias
		struct addrinfo hints;
		struct addrinfo *res;
		struct addrinfo *r;

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
		if (gotit == 0)
		{
			Logger::error(serverUid, "Invalid server_name in the config file");
			throw(ServException(serverUid + " has an invalid server_name"));
		}
		sockvector.push_back(sockaddr);
		_server_names.push_back(_one_name);
		if (_names.find(' ') == string::npos)
			break;
		_names = _names.substr(_names.find(' ') + 1);
	}
	return (sockvector);
}

vector<int> Server::checkPorts(const ConfigParser &config, const string &serverUid)
{
	if (!config.hasServerKey(serverUid, "listen"))
	{
		Logger::error(serverUid, "No port number found in the config file for this server not starting up the services");
		throw(ServException(serverUid + " has no port number"));
	}
	string ports = config.getServerValue(serverUid, "listen");
	string _one_port;
	int portnbr;
	vector<int> portvector;

	while (true)
	{
		_one_port = ports.substr(0, ports.find(' '));

		for (string::iterator pos = _one_port.begin(); pos != _one_port.end(); pos++)
		{
			if (*pos > '9' || *pos < '0')
			{
				Logger::error(serverUid, "The port value is invalid");
				throw(ServException(serverUid + " has invalid port number"));
			}
		}
		portnbr = ft_atoi(_one_port.c_str());
		if (portnbr <= 0 || portnbr > 65535)
		{
			Logger::error(serverUid, "The port is out of range in config file (1-65535)");
			throw(ServException(serverUid + " has invalid port number"));
		}
		portvector.push_back(portnbr);
		if (ports.find(' ') == string::npos)
			break;
		ports = ports.substr(ports.find(' ') + 1);
	}
	return (portvector);
}

const ConfigParser &Server::getConfig() const
{
	return (*_config);
}

vector<int> Server::getSocket() const
{
	return (_socketfds);
}

string Server::getUid() const
{
	return (_uid);
}

static string get_connection_info(const sockaddr_in &client, const sockaddr_in &server)
{
	ostringstream oss;

	oss << YELLOW
		<< BOLD
		<< "Connection:"
		<< NEUTRAL
		<< " client "
		<< inet_ntoa(client.sin_addr) << ":"
		<< ntohs(client.sin_port)
		<< " -> server "
		<< inet_ntoa(server.sin_addr) << ":"
		<< ntohs(server.sin_port);

	return (oss.str());
}

int Server::setClient(int _socketfd)
{
	sockaddr_in peeraddr;
	socklen_t peer_addr_size = sizeof(peeraddr);

	int cfd = accept(_socketfd, (struct sockaddr *)&peeraddr, &peer_addr_size);
	if (cfd == -1)
		throw(ServException("accept error"));

	// Log the connection info with server details
	sockaddr_in serveraddr;
	socklen_t serveraddr_len = sizeof(serveraddr);

	getsockname(_socketfd, (struct sockaddr *)&serveraddr, &serveraddr_len);

	string connexion = get_connection_info(peeraddr, serveraddr);
	string logInfo = connexion + " [Server: " + _uid + ", Root: " + _config->getServerValue(_uid, "root") + "]";

	Logger::access(this->_uid, logInfo);

	_clientFds.push_back(cfd);
	return (cfd);
}

void Server::unsetClient(int position)
{
	close(_clientFds[position]);
	_clientFds.erase(_clientFds.begin() + position);
}

void Server::getRequests(fd_set &readFd, fd_set &fullReadFd, ConfigParser *config, fd_set &fullWriteFd)
{
	for (size_t i = 0; i < _clientFds.size(); i++)
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
			if (hasCgiforClient(_clientFds[i]))
			{
				clearClientBuffer(_clientFds[i]);
				FD_CLR(getCgiforClient(_clientFds[i]), &fullReadFd);
				FD_CLR(getCgiforClient(_clientFds[i]), &readFd);
				eraseCgiFd(_clientFds[i], getCgiforClient(_clientFds[i]));
			}
			int res = _handler->handleRequest(_clientFds[i], *this, config);

			if (res == -1)
			{
				FD_CLR(_clientFds[i], &fullReadFd);
				unsetClient(i);
				cout << "Client disconnected" << endl;
				continue;
			}

			if (res == 1)
			{
				if (hasCgiforClient(_clientFds[i]))
				{
					FD_SET(getCgiforClient(_clientFds[i]), &fullReadFd);
				}
				else
				{
					FD_SET(_clientFds[i], &fullWriteFd);
				}
				FD_CLR(_clientFds[i], &fullReadFd);
			}
		}
	}

	// Check for CGI timeouts before processing CGI outputs 5 is default
	string timeout_str = config->getServerValue(_uid, "cgi_timeout");
	size_t timeout_seconds = timeout_str.empty() ? 5 : ft_atoi(timeout_str); // Default timeout 5 seconds if not set
	if (timeout_seconds == 0 || timeout_seconds > 5)
		timeout_seconds = 5;
	checkCgiTimeouts(timeout_seconds, config, fullReadFd, fullWriteFd, readFd);

	map<int, int>::iterator it = _cgi_for_client.begin();
	for (; it != _cgi_for_client.end(); it++)
	{
		if (it->first == -1 || it->second == -1)
		{
			eraseCgiFd(it->first, it->second);
			if (_cgi_for_client.empty() || it == _cgi_for_client.end())
				break;
			continue;
		}
		if (FD_ISSET(it->second, &readFd))
		{
			int res = storeCgiReturn(it->second);
			FD_CLR(it->second, &readFd);
			if (res == -1)
			{
				FD_CLR(it->second, &fullReadFd);
				eraseCgiFd(it->first, it->second);
				it = _cgi_for_client.begin();
				if (_cgi_for_client.empty() || it == _cgi_for_client.end())
					break;
			}
			else if (res == 0)
			{
				string body = getClientBuffer(it->second);
				if (!body.empty())
				{
					ARequest *requestObject = _cgi_request[it->second];

					if (requestObject->getMethod() == GET)
					{
						string contentType = requestObject->getContentType();
						contentType = requestObject->checkContentType(contentType);

						string response = requestObject->writeHTTPResponse(*this, 200, body, contentType);

						keepaliveDefine(it->first, true);
						fillClientBuffer(it->first, response);
						clearClientBuffer(it->second);
						FD_SET(it->first, &fullWriteFd);
					}
					if (requestObject->getMethod() == POST)
					{
						string serverRoot = config->getServerValue(getUid(), "root");
						string path = "post.txt";
						string postdir = config->getLocationValueForPath(requestObject->getPath(), getUid(), "put_posts", true);

						if (serverRoot.rfind('/') == serverRoot.size() - 1 && postdir.find('/') == 0)
							postdir.erase(0, 1);
						if (postdir.rfind('/') != postdir.size())
							postdir += '/';
						path = serverRoot + postdir + path;

						requestObject->UploadFile(body, path);
						unlink(requestObject->getTmpFile().c_str());
						string response = requestObject->writeHTTPResponse(*this, 204, "", "");

						keepaliveDefine(it->first, true);
						fillClientBuffer(it->first, response);
						clearClientBuffer(it->second);
						FD_SET(it->first, &fullWriteFd);
					}
				}

				pid_t pid = getPidForCgi(it->second);
				if (pid > 0)
				{
					int status;
					pid_t result = waitpid(pid, &status, WNOHANG);
					if (result == -1)
					{
						GetRequest requestObject;

						string errorPage = config->getErrorPageContent(const_cast<ConfigParser &>(*config), getUid(), 500);
						string response = requestObject.writeHTTPResponse(*this, 500, errorPage, "text/html");
						keepaliveDefine(it->first, false);
						fillClientBuffer(it->first, response);
					}
				}
				FD_CLR(it->second, &fullReadFd);
				eraseCgiFd(it->first, it->second);
				it = _cgi_for_client.begin();
				if (_cgi_for_client.empty() || it == _cgi_for_client.end())
					break;
			}
		}
		else
		{
			string body = getClientBuffer(it->second);
			if (!body.empty())
			{
				ARequest *requestObject = _cgi_request[it->second];

				if (requestObject->getMethod() == GET)
				{
					string contentType = requestObject->getContentType();
					contentType = requestObject->checkContentType(contentType);

					string response = requestObject->writeHTTPResponse(*this, 200, body, contentType);

					keepaliveDefine(it->first, true);
					fillClientBuffer(it->first, response);
					clearClientBuffer(it->second);
					FD_SET(it->first, &fullWriteFd);
				}
				if (requestObject->getMethod() == POST)
				{
					string serverRoot = config->getServerValue(getUid(), "root");
					string path = "post.txt";
					string postdir = config->getLocationValueForPath(requestObject->getPath(), getUid(), "put_posts", true);

					if (serverRoot.rfind('/') == serverRoot.size() && postdir.find('/') == 0)
						postdir.erase(0, 1);
					if (postdir.rfind('/') != postdir.size())
						postdir += '/';
					path = serverRoot + postdir + path;

					requestObject->UploadFile(body, path);
					unlink(requestObject->getTmpFile().c_str());
					string response = requestObject->writeHTTPResponse(*this, 204, "", "");

					keepaliveDefine(it->first, true);
					fillClientBuffer(it->first, response);
					clearClientBuffer(it->second);
					FD_SET(it->first, &fullWriteFd);
				}
			}

			pid_t pid = getPidForCgi(it->second);
			if (pid <= 0)
				continue;
			int status;
			pid_t result = waitpid(pid, &status, WNOHANG);

			if (result == -1)
			{
				GetRequest requestObject;

				string errorPage = config->getErrorPageContent(const_cast<ConfigParser &>(*config), getUid(), 500);
				string response = requestObject.writeHTTPResponse(*this, 500, errorPage, "text/html");
				keepaliveDefine(it->first, false);
				fillClientBuffer(it->first, response);
				FD_CLR(it->second, &fullReadFd);
				eraseCgiFd(it->first, it->second);
				it = _cgi_for_client.begin();
				if (_cgi_for_client.empty() || it == _cgi_for_client.end())
					break;
			}
			else if (result > 0)
			{
				// cgi process finished
				FD_CLR(it->second, &fullReadFd);
				eraseCgiFd(it->first, it->second);
				it = _cgi_for_client.begin();
				if (_cgi_for_client.empty() || it == _cgi_for_client.end())
					break;
			}
			else
			{
				FD_SET(it->second, &fullReadFd);
			}
		}
	}
}

bool Server::keepaliveStatus(int fd) const
{
	map<int, bool>::const_iterator it = _keepalive.find(fd);

	if (it != _keepalive.end())
		return (it->second);

	return (1);
}

void Server::keepaliveDefine(int fd, bool status)
{
	if (fd > 0)
		_keepalive[fd] = status;
}

void Server::sendResponse(fd_set &writeFd, fd_set &fullWriteFd, fd_set &fullReadFd)
{
	for (size_t i = 0; i < _clientFds.size(); i++)
	{
		int fd = _clientFds[i];
		// Check if the file descriptor is valid
		if (fd < 0)
		{
			// Invalid file descriptor, remove it
			unsetClient(i);
			continue;
		}
		if (FD_ISSET(fd, &writeFd))
		{
			string response = getClientBuffer(fd);

			clearClientBuffer(fd);
			if (response != "")
			{
				int res = send(fd, response.c_str(), response.length(), 0);
				if (res == -1)
				{
					perror("");
					FD_CLR(fd, &fullWriteFd);
					cerr << "failed to send http response" << endl;
					unsetClient(i);
					cout << "Client disconnected" << endl;
					continue;
				}

				FD_CLR(fd, &fullWriteFd);
				if (!keepaliveStatus(fd))
					unsetClient(i);
				else
				{
					FD_SET(fd, &fullReadFd);
				}
			}
		}
	}
}

void Server::fillClientBuffer(int clientFd, const string &buff)
{
	if (!buff.empty() && clientFd >= 0)
		_clientBuffer[clientFd] = buff;
}

string Server::getClientBuffer(int clientFd) const
{
	if (clientFd >= 0)
	{
		map<int, string>::const_iterator it = _clientBuffer.begin();

		for (; it != _clientBuffer.end(); it++)
		{
			if (it->first == clientFd)
				return (it->second);
		}
	}
	return ("");
}

void Server::clearClientBuffer(int clientFd)
{
	if (clientFd >= 0)
	{
		map<int, string>::iterator it = _clientBuffer.find(clientFd);
		if (it != _clientBuffer.end())
			_clientBuffer.erase(it);
	}
}

/*
 * ENV handling for each instance of Server
 */
void Server::initEnv(char **env)
{
	if (!env[0])
	{
		Logger::error(_uid, "No environment variables found shutting down server");
		throw(ServException("No environment variables found"));
		return;
	}
	for (int i = 0; env[i]; i++)
	{
		string var(env[i]);
		size_t pos = var.find('=');

		if (pos != string::npos)
		{
			string key = var.substr(0, pos);
			string value = var.substr(pos + 1);

			_env[key] = value;
		}
	}
	// Set global CGI variables for server instance
	setEnvValue("SERVER_SOFTWARE", "Webserv/1.0");
	setEnvValue("REDIRECT_STATUS", "200");
	setEnvValue("SERVER_PORT", _config->getServerValue(_uid, "listen")); // re set after request
	setEnvValue("SERVER_ROOT", _config->getServerValue(_uid, "root"));

	setEnvValue("SERVER_PROTOCOL", "HTTP/1.1");
	setEnvValue("GATEWAY_INTERFACE", "CGI/1.1");

	setEnvValue("SERVER_NAME", _config->getServerValue(_uid, "server_name")); // re set after request
}

string Server::getEnvValue(string const &key) const
{
	map<string, string>::const_iterator it = _env.find(key);

	if (it != _env.end())
		return (it->second);
	else
		return ("");
}

void Server::setEnvValue(string const &key, string const &value)
{
	map<string, string>::iterator it = _env.find(key);

	if (it != _env.end())
		it->second = value;
	else if (!key.empty())
		_env[key] = value;
}

char **Server::getEnvAsArray() const
{
	char **env = new char *[_env.size() + 1];
	int j = 0;

	for (map<string, string>::const_iterator i = _env.begin(); i != _env.end(); i++)
	{
		string element = i->first + "=" + i->second;

		env[j] = new char[element.size() + 1];
		env[j] = strcpy(env[j], (const char *)element.c_str());
		j++;
	}
	env[j] = NULL;
	return (env);
}

const map<string, string> Server::getEnv() const
{
	return (_env);
}

void Server::printEnv() const
{
	cout << YELLOW
		 << BOLD << YELLOW
		 << "=== Environment Variables ==="
		 << NEUTRAL << endl;
	for (map<string, string>::const_iterator it = _env.begin(); it != _env.end(); ++it)
		cout << it->first << "=" << it->second << endl;
}

vector<string> Server::getServerNames() const
{
	return (_server_names);
}

void Server::setCgiFdforClient(int clientFd, int cgiFd)
{
	if (clientFd != -1 && cgiFd != -1)
	{
		_cgi_for_client[clientFd] = cgiFd;
	}
}

void Server::eraseCgiFd(int clientFd, int cgiFd)
{
	map<int, int>::iterator it = _cgi_for_client.find(clientFd);
	if (it != _cgi_for_client.end() && it->second == cgiFd)
	{
		clearClientBuffer(cgiFd);
		close(cgiFd);
		_cgi_for_client.erase(it);

		// Clean up CGI request if it exists
		map<int, ARequest *>::iterator req_it = _cgi_request.find(cgiFd);
		if (req_it != _cgi_request.end())
		{
			delete req_it->second;
			_cgi_request.erase(req_it);
		}

		// Clean up PID tracking
		map<int, pid_t>::iterator pid_it = _pid_for_cgi.find(cgiFd);
		if (pid_it != _pid_for_cgi.end())
			_pid_for_cgi.erase(pid_it);

		// Clean up start time tracking
		map<int, time_t>::iterator time_it = _cgi_start_time.find(cgiFd);
		if (time_it != _cgi_start_time.end())
			_cgi_start_time.erase(time_it);
	}
	else
	{
		Logger::error(getUid(), "wrong cgi fd");
	}
}

int Server::hasCgiforClient(int clientFd) const
{
	map<int, int>::const_iterator it = _cgi_for_client.find(clientFd);
	if (it != _cgi_for_client.end())
		return (1);
	return (0);
}

int Server::getCgiforClient(int clientFd) const
{
	map<int, int>::const_iterator it = _cgi_for_client.find(clientFd);
	return (it->second);
}

int Server::getClientforCgi(int cgiFd) const
{
	for (map<int, int>::const_iterator it = _cgi_for_client.begin(); it != _cgi_for_client.end(); it++)
	{
		if (it->second == cgiFd)
			return (it->first);
	}
	return (-1);
}

int Server::storeCgiReturn(int cgiFd)
{
	int received;
	char buff[BUFFER_SIZE];

	memset(buff, 0, BUFFER_SIZE);

	received = read(cgiFd, buff, BUFFER_SIZE - 1);
	if (received <= 0)
		return (received);
	string body = getClientBuffer(cgiFd);
	body.append(buff, received);
	fillClientBuffer(cgiFd, body);
	return (received);
}

void Server::setCgiRequest(int cgiFd, ARequest &request)
{
	_cgi_request[cgiFd] = request.clone();
}

void Server::setPidforCgi(int cgiFd, pid_t pid)
{
	if (cgiFd != -1)
	{
		_pid_for_cgi[cgiFd] = pid;
		if (pid > 0) // Only track start time for actual CGI processes (not regular files)
			_cgi_start_time[cgiFd] = time(NULL);
	}
}
pid_t Server::getPidForCgi(int cgiFd) const
{
	map<int, pid_t>::const_iterator it = _pid_for_cgi.find(cgiFd);
	if (it != _pid_for_cgi.end())
		return (it->second);
	return (0);
}
string Server::getCookieValue(const string &session_id, const string &key) const
{
	map<string, map<string, string> >::const_iterator session_it = _cookies.find(session_id);

	if (session_it != _cookies.end())
	{
		map<string, string>::const_iterator key_it = session_it->second.find(key);

		if (key_it != session_it->second.end())
			return (key_it->second);
	}
	return ("");
}

void Server::setCookie(const string &session_id, const string &key, const string &value)
{
	// Ensure internal cookie map is updated
	_cookies[session_id][key] = value;

	// Build the Set-Cookie header line to add
	string cookieLine = "Set-Cookie: " + key + "=" + value + "; Path=/\r\n";

	// Remove any existing Set-Cookie lines for the same cookie name to avoid duplicates
	string prefix = "Set-Cookie: " + key + "=";
	size_t pos = 0;
	while ((pos = _cookie_header.find(prefix, pos)) != string::npos)
	{
		size_t line_end = _cookie_header.find("\r\n", pos);
		if (line_end == string::npos)
		{
			// malformed header; clear and break
			_cookie_header.clear();
			break;
		}
		// erase the existing header line (including CRLF)
		_cookie_header.erase(pos, line_end + 2 - pos);
	}

	// Append the new cookie line
	_cookie_header += cookieLine;
}

string Server::getCookieHeader() const
{
	if (_cookie_header.empty())
		return ("");
	return (_cookie_header);
}

void Server::parseCookie(const string &sessionId, const string &cookies)
{
	(void)sessionId;
	if (cookies.empty())
		return;

	istringstream iss(cookies);
	string cookie;

	while (iss >> cookie)
	{
		string key = cookie.substr(0, cookie.find("="));
		string value = cookie.substr(cookie.find("=") + 1);
		_cookies[sessionId][key] = value;
	}
}

string Server::generateSessionId() const
{
	const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	const size_t max_index = (sizeof(charset) - 1);
	ostringstream oss;
	srand(time(0) + rand());

	for (size_t i = 0; i < 16; i++)
		oss << charset[rand() % max_index];
	cout << YELLOW << BOLD << "Generated session ID: " << NEUTRAL << oss.str() << endl;
	return (oss.str());
}

void Server::clearCookies()
{
	_cookies.clear();
	_cookie_header.clear();
}

void Server::clearCookieSession(string const &session_id)
{
	map<string, map<string, string> >::iterator it = _cookies.find(session_id);

	if (it != _cookies.end())
		_cookies.erase(it);
}

void Server::clearCookieHeader()
{
	_cookie_header.clear();
}

int Server::checkCgiTimeouts(size_t timeout_seconds, ConfigParser *config, fd_set &fullReadFd, fd_set &fullWriteFd, fd_set &readFd)
{
	time_t current_time = time(NULL);
	vector<int> timed_out_cgis;

	// Find all timed-out CGI processes
	for (map<int, time_t>::iterator it = _cgi_start_time.begin(); it != _cgi_start_time.end(); ++it)
	{
		int cgiFd = it->first;
		time_t start_time = it->second;

		if (static_cast<size_t>(current_time - start_time) > timeout_seconds)
		{
			timed_out_cgis.push_back(cgiFd);
		}
	}

	// Kill timed-out processes
	for (vector<int>::iterator it = timed_out_cgis.begin(); it != timed_out_cgis.end(); ++it)
	{
		GetRequest requestObject;
		int ClientFd = getClientforCgi(*it);
		killTimedOutCgi(*it, timeout_seconds);

		string errorPage = config->getErrorPageContent(const_cast<ConfigParser &>(*config), getUid(), 504);
		string response = requestObject.writeHTTPResponse(*this, 504, errorPage, "text/html");
		keepaliveDefine(ClientFd, false);
		fillClientBuffer(ClientFd, response);
		FD_SET(ClientFd, &fullWriteFd);
		FD_CLR(*it, &fullReadFd);
		FD_CLR(*it, &readFd);
		eraseCgiFd(ClientFd, *it);
	}
	return (0);
}

int Server::killTimedOutCgi(int cgiFd, size_t timeout_seconds)
{
	pid_t pid = getPidForCgi(cgiFd);
	if (pid == 0)
		return -1;

	cout << RED << BOLD << "CGI script timeout detected after " << timeout_seconds << " seconds - killing process " << pid << " (fd: " << cgiFd << ")" << NEUTRAL << endl;

	// Try graceful termination first
	if (kill(pid, SIGTERM) == 0)
	{
		usleep(500000); // Wait 500ms

		// Check if process is still running
		int status;
		pid_t result = waitpid(pid, &status, WNOHANG);
		if (result == 0)
		{
			// Process still running, force kill
			kill(pid, SIGKILL);
			waitpid(pid, &status, 0); // Clean up zombie
		}
	}

	return 504;
}
