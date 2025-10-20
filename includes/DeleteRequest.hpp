#pragma once

#include <string>
#include <iostream>
#include <sstream>

#include "ARequest.hpp"
#include "RequestHandler.hpp"

class DeleteRequest : public ARequest
{
private:
	/*attributes here*/
public:
	/*constructors and destructor*/
	DeleteRequest();
	DeleteRequest(map<string, string> header);
	DeleteRequest(DeleteRequest &src);
	~DeleteRequest();

	/*member functions*/
	int delete_file(int fd, Server &serv);
	int handleDelete(int fd, Server &server, const ConfigParser *config, const string &path);

	/*operator overloads*/
	DeleteRequest &operator=(DeleteRequest &src);
};
