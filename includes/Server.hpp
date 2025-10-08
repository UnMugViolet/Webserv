#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "ConfigParser.hpp"
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "Logger.hpp"

void	*ft_memset(void *s, int c, unsigned long int n);
int 	ft_atoi(const std::string &str);
int 	ft_inet_pton4(std::string &src, struct in_addr *dst);

class RequestHandler;

class Server
{
private:
	/*attributes here*/
	std::string							_uid;
	std::map<std::string, std::string>	_env;
	std::vector<std::string>			_server_names;
	std::vector<int>					_socketfds;
	std::vector<int>					_clientFds;
	RequestHandler						*_handler;
	ConfigParser						*_config;

public:
	/*constructors and destructor*/
	Server();
	Server(const Server &other);
	Server(ConfigParser &config, std::string Name);
	~Server();

	/*member functions*/
	std::vector<sockaddr_in>	setServerNames(const ConfigParser &config, const std::string &serverUid);
	std::vector<int>			checkPorts(const ConfigParser &config, const std::string &serverUid);
	void						CreateSockets(const std::string &serverUid, std::vector<int> &ports, std::vector<sockaddr_in> &sockaddrs);
	// int			addVirtualHost(ConfigParser &config, std::string serverUid);
	std::vector<int>	getSocket() const;
	std::string	getUid() const;
	std::vector<std::string> getServerNames() const;
	const ConfigParser&	getConfig() const;
	int			setClient(int _socketfd);
	void		unsetClient(int position);
	void		getRequests(fd_set &readFd, fd_set &fullReadFd, ConfigParser *config);

	// Env handling methods
	void		initEnv(char **env);
	void		printEnv() const;
	std::string	getEnvValue(const std::string &key) const;
	void		setEnvValue(const std::string &key, const std::string &value);
	char		**getEnvAsArray() const;
	const std::map<std::string, std::string> getEnv() const;

	/*operator overloads*/
	Server		&operator=(const Server &other);
	class ServException : public std::exception
		{
			private:
				std::string _message;
			public:
				ServException(std::string message) throw() {
					_message = std::string(RED) + std::string(BOLD) + "[ERROR] " + std::string(NEUTRAL) + std::string(RED) + "Server: " + message;
				}
				virtual const char *what() const throw() {
					return (_message.c_str());
				}
				virtual ~ServException() throw() {}
		};
};

