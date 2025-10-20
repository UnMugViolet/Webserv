#pragma once

#include <map>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <string.h>
#include <sstream>

#include "PostRequest.hpp"
#include "DeleteRequest.hpp"
#include "GetRequest.hpp"
#include "dict.hpp"
#include "Logger.hpp"

// Forward declaration to avoid circular dependency
class ConfigParser;
class Server;

class RequestHandler {
	private:
		int _maxBodySize;

	public:
		RequestHandler();
		RequestHandler(RequestHandler& src);
		RequestHandler&	operator=(RequestHandler& src);
		~RequestHandler();

		/*member functions*/
		static string					getExtension(const string &path);
		static int						_checkAccess(const string &path);
		int								handleRequest(int fd, Server &server, ConfigParser *config);
		int								handleRedirect(int fd, Server &server, const string &redirect, map<string, string> &headermap);
		void							setMaxBodySize(string size);
		int								readOnce(int fd, Server &server, ConfigParser *config);
		int								checkHeader(int fd, Server &server, ConfigParser *config, map<string, string> &headermap, string &body, string &savestring);
		map<string, string>				parseHeader(string header) const;
		int								handleChunkedRequest(int fd, string &savestring, string &body, Server &server, ConfigParser *config, int can_read);
		void							loadErrorPage(int fd, int errorCode, Server &server, ConfigParser *config, string errorMessage="", bool keepAlive=true);
};

string				trim(const string &str);
string				urlDecode(const string &src);
map<string, size_t>	getIndex(const string &indexes, const string &root);

