#pragma once

#include <map>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include "Logger.hpp"
#include <unistd.h>
#include <string.h>
#include "PostRequest.hpp"
#include "DeleteRequest.hpp"
#include "GetRequest.hpp"
#include <sstream>

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
	int	handleRequest(int fd) const;
	void	setMaxBodySize(std::string size);
	std::string	RequestHandler::trim(const std::string &str) const;
	std::map<std::string, std::string>	RequestHandler::parseHeader(std::string header) const;

	/*operator overloads*/
	RequestHandler&	operator=(RequestHandler& src);
};

