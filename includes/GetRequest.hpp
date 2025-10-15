#pragma once

#include <string>
#include <iostream>

#include "ARequest.hpp"
#include "RequestHandler.hpp"

class GetRequest: public ARequest
{
public:
	enum PathType {
		PATH_NOT_EXISTS,
		PATH_DIRECTORY,
		PATH_FILE
	};

private:
	/*attributes here*/
	
	/*private helper methods*/
	PathType getPathType(string const &path);
	int handleDirectory(int fd, Server const &server, ConfigParser const *config, string const &decodedUrl);
	int handleFile(int fd, Server const &server, ConfigParser const *config, string const &decodedUrl);
	int tryServeIndexFile(int fd, Server const &server, ConfigParser const *config, string const &decodedUrl, string const &indexPages);
	int serveIndexFile(int fd, Server const &server, ConfigParser const *config, string const &decodedUrl, string const &indexFileName);
	int handleDirectoryListing(int fd, Server const &server, ConfigParser const *config, string const &decodedUrl, string const &pathForConfig);
	string getPathForConfig(string const &decodedUrl);
	int sendErrorResponse(int fd, int errorCode, ConfigParser const *config, string const &serverUid);
	int checkKeepAlive();

public:
	/*constructors and destructor*/
	GetRequest();
	GetRequest(map<string, string> header);
	GetRequest(GetRequest& src);
	~GetRequest();

	/*member functions*/
	int	handleGet(int fd, Server const &server, ConfigParser const *config, string const &fullPath);

	/*operator overloads*/
	GetRequest &operator=(GetRequest& src);
};

