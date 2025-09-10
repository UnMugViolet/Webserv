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
#include "dict.hpp"
#include <fstream>

// Forward declaration to avoid circular dependency
class ConfigParser;

class RequestHandler
{
private:
	int _maxBodySize;
public:
	RequestHandler();
	RequestHandler(RequestHandler& src);
	RequestHandler&	operator=(RequestHandler& src);
	~RequestHandler();

	/*member functions*/
	int									printRequest(int fd) const;
	int									handleRequest(int fd, const std::string &serverRoot, ConfigParser *config = NULL, const std::string &serverId = "") const;
	void								setMaxBodySize(std::string size);
	std::string							trim(const std::string &str) const;
	std::map<std::string, std::string>	parseHeader(std::string header) const;

};

