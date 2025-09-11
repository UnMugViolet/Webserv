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
class Server;

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
	static std::string					_getExtension(const std::string &path);
	static int							_checkAccess(const std::string &path, int type);
	std::string							getIndex(const std::string &indexes, const std::string &root) const;
	int									handleRequest(int fd, Server const &server, ConfigParser *config, std::string const &serverUid);
	void								setMaxBodySize(std::string size);
	std::string							trim(const std::string &str) const;
	std::map<std::string, std::string>	parseHeader(std::string header) const;

};

