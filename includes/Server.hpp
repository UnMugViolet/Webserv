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

class Server
{
private:
	/*attributes here*/
	std::string _name;
	int _socketfd;
	int _RequestMaxSize;
	std::vector<int> _clientFds;
	
public:
	/*constructors and destructor*/
	Server();
	Server(Server& src);
	Server(ConfigParser &config, std::string Name);
	~Server();

	/*member functions*/

	/*operator overloads*/
	Server&	operator=(Server& src);
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

