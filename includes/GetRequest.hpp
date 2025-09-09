#pragma once

#include <string>
#include <iostream>
#include "ARequest.hpp"

class GetRequest: public ARequest
{
private:
	/*attributes here*/
public:
	/*constructors and destructor*/
	GetRequest(std::map<std::string, std::string> header);
	GetRequest(GetRequest& src);
	~GetRequest();

	/*member functions*/

	/*operator overloads*/
	GetRequest&	operator=(GetRequest& src);
};

