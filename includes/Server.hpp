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
#include "RequestHandler.hpp"
#include "Logger.hpp"

class Server
{
private:
	/*attributes here*/
	std::string _name;
	int _socketfd;
	std::vector<int> _clientFds;
	RequestHandler	_handler;

public:
	/*constructors and destructor*/
	Server();
	Server(const Server &other);
	Server(ConfigParser &config, std::string Name);
	~Server();

	/*member functions*/
	int		getSocket() const;
	int		setClient();
	void	unsetClient(int position);
	void	getRequests(fd_set &readFd, fd_set &fullReadFd);

	/*operator overloads*/
	Server&	operator=(const Server &other);
	class servException : public std::exception
		{
			private:
				std::string _message;
			public:
				servException(std::string message) throw()
				{
					_message = "serv error: " + message;
				}
				virtual const char* what() const throw()
				{
					return (_message.c_str());
				}
				virtual ~servException() throw() {}
		};
};

