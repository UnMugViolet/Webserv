#pragma once

#include <string>
#include <iostream>
#include <map>

#define GET 0
#define POST 1
#define DELETE 2

class ARequest
{
protected:
	/*attributes here*/
	int			_method;
	std::string	_path;
	std::string	_host;
	bool		_keep_alive;
	std::string	_client;
public:
	/*constructors and destructor*/
	ARequest();
	ARequest(ARequest& src);
	~ARequest();

	/*member functions*/

	/*operator overloads*/
	ARequest&	operator=(ARequest& src);
};

