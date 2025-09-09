#pragma once

#include <string>
#include <iostream>
#include "ARequest.hpp"

class PostRequest: public ARequest
{
private:
	size_t		_Content_length;
	int			_Content_type;
	std::string	_body;
public:
	/*constructors and destructor*/
	PostRequest(std::map<std::string, std::string> header);
	PostRequest(PostRequest& src);
	~PostRequest();

	/*member functions*/

	/*operator overloads*/
	PostRequest&	operator=(PostRequest& src);
};

