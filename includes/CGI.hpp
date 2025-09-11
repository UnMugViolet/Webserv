#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include "unistd.h"
#include "fcntl.h"
#include "stdlib.h"
#include "sys/wait.h"
#include "sys/types.h"
#include "dict.hpp"
#include "Logger.hpp"
#include <sys/socket.h>

#define PYTHON 0
#define PERL 1
#define PHP 2
#define SHELL 3
#define BINARY 4
#define UNKNOWN 5
#define HTML 6
#define CSS 7
#define PNG 8
#define JPG 9
#define JPEG 10
#define GIF 11

class CGI
{
private:
	/*attributes here*/
	static int	_getType(std::string ext);
	static std::string	_getExtension(const std::string &path);
	static int	_checkAccess(const std::string &path, int type);
	std::string	http_status_to_error_page(unsigned int http_status, std::string &error_code);
public:
	/*constructors and destructor*/
	CGI();
	~CGI();

	/*member functions*/
	
	static int	interpret(const std::string &path, std::string const serverUid);

	/*operator overloads*/

	class CGIException : public std::exception
	{
		private:
			std::string 	_message;
			int				_exit;
			unsigned int	_http_status;
			std::string		_serverUid;
		public:
			CGIException(std::string message, bool must_exit_prog, unsigned int http_status, std::string const serverUid) throw()
			{
				_message = "CGIException error: " + message;
				Logger::error(serverUid, _message);
				_message = std::string(RED) + _message + std::string(NEUTRAL);
				if (must_exit_prog)
					_exit = must_exit_prog;
				if (http_status != 0)
					_http_status = http_status;
			}
			virtual const char* what() const throw()
			{
				return (_message.c_str());
			}
			virtual int getExit() const throw()
			{
				return (_exit);
			}
			virtual unsigned int getHttpStatus() const throw()
			{
				return (_http_status);
			}
			virtual ~CGIException() throw() {}
	};
};

