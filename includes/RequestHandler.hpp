#pragma once

#include <map>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include "Logger.hpp"
#include <unistd.h>
#include <string.h>
#include "PostRequest.hpp"
#include "DeleteRequest.hpp"
#include "GetRequest.hpp"
#include <sstream>
#include "dict.hpp"
#include <fstream>

// Forward declaration to avoid circular dependency
class ConfigParser;
class Server;

class RequestHandler
{
	private:
		int _maxBodySize;
	public:
		RequestHandler();
		RequestHandler(RequestHandler& src);
		RequestHandler&	operator=(RequestHandler& src);
		~RequestHandler();

		/*member functions*/
		static string					getExtension(const string &path);
		static int							_checkAccess(const string &path);
		string							getIndex(const string &indexes, const string &root) const;
		int									handleRequest(int fd, Server &server, ConfigParser *config);
		void								setMaxBodySize(string size);
		map<string, string>	parseHeader(string header) const;
		string							handleChunckedRequest(int fd, const string &body);
};

string	trim(const string &str);
string urlDecode(const string &src);
