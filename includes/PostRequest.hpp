#pragma once

#include <string>
#include <iostream>
#include <sstream>

#include "ARequest.hpp"
#include "Logger.hpp"

class PostRequest: public ARequest
{
private:
	std::string		_Content_type;
	std::string		_body;
public:
	/*constructors and destructor*/
	PostRequest();
	PostRequest(std::map<std::string, std::string> header);
	PostRequest(PostRequest& src);
	~PostRequest();

	/*member functions*/
	int	UploadFile(std::string body, std::string path);
	int	UploadContent(std::map<std::string, std::string> content, std::string path);
	int	createPost(int fd, std::string body, std::string fullPath);
	int	handlePost(int fd, const Server &server, const std::string &body, const ConfigParser *config);

	/*operator overloads*/
	PostRequest&	operator=(PostRequest& src);
};

