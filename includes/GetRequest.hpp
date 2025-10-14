#pragma once

#include <string>
#include <iostream>

#include "ARequest.hpp"
#include "RequestHandler.hpp"

class GetRequest: public ARequest
{
private:
	/*attributes here*/
public:
	/*constructors and destructor*/
	GetRequest();
	GetRequest(std::map<std::string, std::string> header);
	GetRequest(GetRequest& src);
	~GetRequest();

	/*member functions*/
	int	handleGet(int fd, const Server &server, const ConfigParser *config, const std::string &fullPath);

	/*operator overloads*/
	GetRequest&	operator=(GetRequest& src);
};

