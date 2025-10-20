#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "ConfigParser.hpp"
#include "Logger.hpp"
#include "sys/wait.h"

class ARequest;
class RequestHandler;

class Server
{
private:
	/*attributes here*/
	string _uid;
	map<string, string> _env;
	vector<string> _server_names;
	vector<int> _socketfds;
	vector<int> _clientFds;
	map<int, string> _clientBuffer;
	RequestHandler *_handler;
	ConfigParser *_config;
	map<int, bool> _keepalive;
	map<int, int> _cgi_for_client;
	map<int, pid_t> _pid_for_cgi;
	map<int, ARequest *> _cgi_request;
	string _cookie_header;
	map<string, map<string, string>> _cookies;

public:
	/*constructors and destructor*/
	Server();
	Server(const Server &other);
	Server(ConfigParser &config, string Name);
	~Server();

	/*member functions*/
	vector<sockaddr_in> setServerNames(const ConfigParser &config, const string &serverUid);
	vector<int> checkPorts(const ConfigParser &config, const string &serverUid);
	void CreateSockets(const string &serverUid, vector<int> &ports, vector<sockaddr_in> &sockaddrs);

	// int			addVirtualHost(ConfigParser &config, string serverUid);
	vector<int> getSocket() const;
	string getUid() const;
	vector<string> getServerNames() const;
	const ConfigParser &getConfig() const;
	int setClient(int _socketfd);
	void unsetClient(int position);
	void getRequests(fd_set &readFd, fd_set &fullReadFd, ConfigParser *config, fd_set &fullWriteFd);
	void sendResponse(fd_set &writeFd, fd_set &fullWriteFd, fd_set &fullReadFd);
	void fillClientBuffer(int clientFd, const string &buff);
	string getClientBuffer(int clientFd) const;
	void clearClientBuffer(int clientFd);
	bool keepaliveStatus(int fd) const;
	void keepaliveDefine(int fd, bool status);
	void setCgiFdforClient(int clientFd, int cgiFd);
	int hasCgiforClient(int clientFd) const;
	int getCgiforClient(int clientFd) const;
	void eraseCgiFd(int clientFd, int cgiFd);
	int storeCgiReturn(int clientFd, int cgiFd);
	void setCgiRequest(int cgiFd, ARequest &request);
	void setPidforCgi(int cgiFd, pid_t pid);
	int getPidForCgi(int cgiFd) const;
	
	// Cookie handling methods
	void setCookie(const string &sesion_id, const string &key, const string &value);
	string getCookieHeader() const;
	string getCookieValue(const string &session_id, const string &key) const;
	string generateSessionId() const;
	void clearCookies();
	void clearCookieSession(const string &session_id);
	void clearCookieHeader();

	// Env handling methods
	void initEnv(char **env);
	void printEnv() const;
	string getEnvValue(const string &key) const;
	void setEnvValue(const string &key, const string &value);
	char **getEnvAsArray() const;
	const map<string, string> getEnv() const;

	/*operator overloads*/
	Server &operator=(const Server &other);

	class ServException : public exception
	{
	private:
		string _message;

	public:
		ServException(string message) throw()
		{
			_message = string(RED) + string(BOLD) + "[ERROR] " + string(NEUTRAL) + string(RED) + "Server: " + message;
		}
		virtual const char *what() const throw()
		{
			return (_message.c_str());
		}
		virtual ~ServException() throw() {}
	};
};
