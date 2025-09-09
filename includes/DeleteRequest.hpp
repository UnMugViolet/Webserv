#pragma once

#include <string>
#include <iostream>
#include "ARequest.hpp"

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

	/*operator overloads*/
	DeleteRequest&	operator=(DeleteRequest& src);
};

