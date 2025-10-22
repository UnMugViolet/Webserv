#pragma once

#include <iostream>
#include <string>
#include <cstdlib>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <csignal>
#include <errno.h>
#include <sys/time.h>

#include "ConfigParser.hpp"
#include "Server.hpp"

class Webserv
{
private:
	vector<Server> _servers;
	ConfigParser *_config;
	static bool _shutdown;
	vector<int> allFds;

public:
	Webserv();
	Webserv(ConfigParser &config);
	~Webserv();

	void serverLoop();
	void stopServer();
	static void signalHandler(int signal);

	class WebservException : public exception
	{

	private:
		string _message;

	public:
		WebservException(string message) throw()
		{
			_message = string(RED) + string(BOLD) + "[ERROR] " + string(NEUTRAL) + string(RED) + "Webserv: " + message;
		}
		virtual const char *what() const throw()
		{
			return (_message.c_str());
		}
		virtual ~WebservException() throw() {}
	};
};
