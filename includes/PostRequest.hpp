#pragma once

#include <string>
#include <iostream>
#include "ARequest.hpp"

class PostRequest: public ARequest
{
private:
	std::string		_Content_type;
	std::string		_body;
public:
	/*constructors and destructor*/
	PostRequest(std::map<std::string, std::string> header);
	PostRequest(PostRequest& src);
	~PostRequest();

	/*member functions*/
	void	UploadFile(std::string body, std::string path);
	void	HandlePost(std::map<std::string, std::string> header, std::string body, std::string fullPath);

	/*operator overloads*/
	PostRequest&	operator=(PostRequest& src);
};

