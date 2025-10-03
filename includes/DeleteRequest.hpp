#pragma once

#include <string>
#include <iostream>
#include <sstream>

#include "ARequest.hpp"

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
		void	delete_file(int fd, const char *path) const;

		/*operator overloads*/
		DeleteRequest &operator=(DeleteRequest &src);
};

