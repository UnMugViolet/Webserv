#pragma once

#include <string>
#include <iostream>
#include "unistd.h"
#include "stdlib.h"
#include "sys/wait.h"
#include "sys/types.h"

#define PYTHON 0
#define PERL 1
#define PHP 2
#define SHELL 3
#define BINARY 4
#define UNKNOWN 5

class CGI
{
private:
	/*attributes here*/
	static int	_getType(std::string ext);
	static std::string	_getExtension(const std::string &path);
public:
	/*constructors and destructor*/
	CGI();
	~CGI();

	/*member functions*/
	static bool must_interpret(const std::string &path);
	
	static int	interpret(const std::string &path);

	/*operator overloads*/

	class CGIException : public std::exception
	{
		private:
			std::string _message;
		public:
			CGIException(std::string message) throw()
			{
				_message = "CGIException error: " + message;
			}
			virtual const char* what() const throw()
			{
				return (_message.c_str());
			}
			virtual ~CGIException() throw() {}
	};
};

