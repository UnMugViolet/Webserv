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
// class Server;
#define GET 0
#define POST 1
#define DELETE 2

class ARequest
{
protected:
	/*attributes here*/
	int _method;
	string _path;
	string _host;
	bool _keep_alive;
	string _client;
	string _accepted_mime;

public:
	ARequest();
	ARequest(const ARequest &src);
	ARequest &operator=(const ARequest &src);
	virtual ~ARequest();

	int isKeepalive() const;
	int sendCGIResponse(int fd, const string &scriptPath, const ConfigParser *config, Server &Server);
	string writeHTTPResponse(const Server &server, int statusCode, const string &body, const string &contentType = "text/html");
	string loadErrorPage(int statusCode, const ConfigParser *config, const string &serverUid) const;
	string getContentType() const;
	string checkContentType(string &contentType);
	int getMethod() const;
	void fetchErrorPageWithCode(int fd, int errorCode, Server &server, ConfigParser const *config, string errorMessage = "", bool keepAlive = true);

	virtual ARequest *clone() const = 0;

};

map<string, string> parseQuery(const string &query);
string generateDirectoryListing(string const &dirPath, string const &requestPath);
