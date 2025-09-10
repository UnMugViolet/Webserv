#pragma once

#include <string>
#include <iostream>
#include <map>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>

// Forward declaration
class ConfigParser;

#define GET 0
#define POST 1
#define DELETE 2

class ARequest
{
protected:
	/*attributes here*/
	int			_method;
	std::string	_path;
	std::string	_host;
	bool		_keep_alive;
	std::string	_client;
public:
	/*constructors and destructor*/
	ARequest();
	ARequest(ARequest& src);
	~ARequest();

	/*member functions*/
	int			sendCGIResponse(int clientFd, const std::string &scriptPath, ConfigParser *config, const std::string &serverId);
	int			sendHTTPResponse(int clientFd, int statusCode, const std::string &body, const std::string &contentType = "text/html");
	std::string loadErrorPage(int statusCode, const ConfigParser *config, const std::string &serverId) const;
	std::string getContentType(const std::string &filePath) const;

	/*operator overloads*/
	ARequest&	operator=(ARequest& src);
};

