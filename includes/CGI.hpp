#pragma once

#include <string>
#include <iostream>
#include "unistd.h"
#include "fcntl.h"
#include "stdlib.h"
#include "sys/wait.h"
#include "sys/types.h"

#define PYTHON 0
#define PERL 1
#define PHP 2
#define SHELL 3
#define BINARY 4
#define UNKNOWN 5
#define HTML 6
#define CSS 7

class CGI
{
private:
	/*attributes here*/
	static int	_getType(std::string ext);
	static std::string	_getExtension(const std::string &path);
	static int	_checkAccess(const std::string &path, int type);
public:
	/*constructors and destructor*/
	CGI();
	~CGI();

	/*member functions*/
	
	static int	interpret(const std::string &path);

	/*operator overloads*/

	class CGIException : public std::exception
	{
		private:
			std::string _message;
			int			_exit;
		public:
			CGIException(std::string message) throw()
			{
				_message = "CGIException error: " + message;
				_exit = 0;
			}
			CGIException(std::string message, int exit) throw()
			{
				_message = "CGIException error: " + message;
				_exit = exit;
			}
			virtual const char* what() const throw()
			{
				return (_message.c_str());
			}
			virtual int getExit() const throw()
			{
				return (_exit);
			}
			virtual ~CGIException() throw() {}
	};
};

