#pragma once

#include <string>
#include <iostream>
#include "ARequest.hpp"
#include <sstream>

class DeleteRequest: public ARequest
{
private:
	/*attributes here*/
public:
	/*constructors and destructor*/
	DeleteRequest(std::map<std::string, std::string> header);
	DeleteRequest(DeleteRequest& src);
	~DeleteRequest();

	/*member functions*/
	void	delete_file(int fd, const char *path) const;

	/*operator overloads*/
	DeleteRequest&	operator=(DeleteRequest& src);
};

