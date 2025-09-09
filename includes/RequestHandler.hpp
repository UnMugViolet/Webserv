#pragma once

#include <string>
#include <iostream>
#include <sys/socket.h>
#include "Logger.hpp"

class RequestHandler
{
private:
	/*attributes here*/
	int _maxBodySize;
public:
	/*constructors and destructor*/
	RequestHandler();
	RequestHandler(RequestHandler& src);
	~RequestHandler();

	/*member functions*/
	int	printRequest(int fd) const;
	void	setMaxBodySize(std::string size);

	/*operator overloads*/
	RequestHandler&	operator=(RequestHandler& src);
};

