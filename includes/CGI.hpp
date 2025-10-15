#pragma once



#include "unistd.h"
#include "fcntl.h"
#include "stdlib.h"
#include "sys/wait.h"
#include "sys/types.h"
#include "dict.hpp"
#include "Logger.hpp"
#include "Server.hpp"

#include <string>
#include <iostream>
#include <sstream>
#include <dirent.h>
#include <sys/socket.h>

class CGI
{
private:
	/*attributes here*/
	static int	_getType(string ext);
	static string	_getExtension(const string &path);
	static int	_checkAccess(const string &path, int type);
	string	http_status_to_error_page(unsigned int http_status, string &error_code);
public:
	/*constructors and destructor*/
	CGI();
	~CGI();

	/*member functions*/
	
	static int	interpret(const string &path, const Server &Server, map<string, string> &cgi_list);

	/*operator overloads*/

	class CGIException : public exception
	{
		private:
			string 	_message;
			int				_exit;
			unsigned int	_http_status;
			string		_serverUid;
		public:
			CGIException(string message, bool must_exit_prog, unsigned int http_status, string const serverUid) throw()
			{
				_message = "CGIException error: " + message;
				Logger::error(serverUid, _message);
				_message = string(RED) + _message + string(NEUTRAL);
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

