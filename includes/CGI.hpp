#pragma once

#include <string>
#include <iostream>
#include "unistd.h"

enum class ScriptType{
	PYTHON,
	PERL,
	PHP,
	SHELL,
	BINARY,
	UNKNOWN
};

class CGI
{
private:
	/*attributes here*/
public:
	/*constructors and destructor*/
	CGI();
	~CGI();

	/*member functions*/
	static bool must_interpret(const std::string &path);
	ScriptType	_getType(std::string ext);
	std::string	_getExtension(const std::string &path);
	int	interpret(const std::string &path);

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

