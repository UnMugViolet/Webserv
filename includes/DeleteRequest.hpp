#pragma once

#include <string>
#include <iostream>
#include <sstream>

#include "ARequest.hpp"
#include "RequestHandler.hpp"

class DeleteRequest: public ARequest
{
	private:
		/*attributes here*/
	public:
		/*constructors and destructor*/
		DeleteRequest();
		DeleteRequest(std::map<std::string, std::string> header);
		DeleteRequest(DeleteRequest &src);
		~DeleteRequest();

		/*member functions*/
		int	delete_file(int fd, const Server &serv);
		int	handleDelete(int fd, const Server &server, const ConfigParser *config, const std::string &path);

		/*operator overloads*/
		DeleteRequest &operator=(DeleteRequest &src);
};

