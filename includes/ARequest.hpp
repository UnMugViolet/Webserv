#pragma once

#include <string>
#include <iostream>
#include <map>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include <fstream>
#include "dict.hpp"
#include "Server.hpp"

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
	string	_path;
	string	_host;
	bool		_keep_alive;
	string	_client;
public:
	/*constructors and destructor*/
	ARequest();
	ARequest(ARequest& src);
	~ARequest();

	/*member functions*/
	int			isKeepalive() const;
	int			sendCGIResponse(int clientFd, const string &scriptPath, const ConfigParser *config, const Server &Server);
	int			sendHTTPResponse(int clientFd, int statusCode, const string &body, const string &contentType = "text/html");
	string loadErrorPage(int statusCode, const ConfigParser *config, const string &serverUid) const;
	string getContentType(const string &filePath) const;

	/*operator overloads*/
	ARequest&	operator=(ARequest& src);
};

map<string, string> 	parseQuery(const string &query);
string 						generateDirectoryListing(string const &dirPath, string const &requestPath);
