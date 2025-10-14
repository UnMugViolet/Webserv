#pragma once

#include <string>
#include <iostream>
#include <sstream>

#include "ARequest.hpp"
#include "Logger.hpp"

class PostRequest: public ARequest
{
private:
	string		_Content_type;
	string		_body;
public:
	/*constructors and destructor*/
	PostRequest();
	PostRequest(map<string, string> header);
	PostRequest(PostRequest& src);
	~PostRequest();

	/*member functions*/
	int	UploadFile(string body, string path);
	int	UploadContent(map<string, string> content, string path);
	int	createPost(int fd, string body, string postpath, string uploadpath);
	int	handlePost(int fd, const Server &server, const string &body, const ConfigParser *config);

	/*operator overloads*/
	PostRequest&	operator=(PostRequest& src);
};

