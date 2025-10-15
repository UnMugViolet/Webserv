#include "ARequest.hpp"
#include "CGI.hpp"
#include <string.h>
#include <sstream>
#include <vector>
#include "ConfigParser.hpp"
#include "RequestHandler.hpp"

ARequest::ARequest()
{
	_method = 0;
	_path = "";
	_host = "";
	_keep_alive = true;
	_client = "";
}

ARequest::ARequest(ARequest &src)
{
	if (this != &src)
		*this = src;
	return ;
}

ARequest::~ARequest()
{
	return ;
}

int	ARequest::isKeepalive() const
{
	return (_keep_alive);
}

ARequest&	ARequest::operator=(ARequest &src)
{
	if (this != &src)
	{
		this->_method = src._method;
		this->_path = src._path;
		this->_host = src._host;
		this->_keep_alive = src._keep_alive;
		this->_client = src._client;
	}
	return (*this);
}

string generateDirectoryListing(string const &dirPath, string const &requestPath) {
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) return "<h1>Cannot open directory</h1>";

    ostringstream html;
    html << "<html><head><title>Index of " << requestPath << "</title></head><body>";
    html << "<h1>Index of " << requestPath << "</h1><ul>";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        string name = entry->d_name;
        if (name == ".") continue;
		html << "<li><a href=\"" << requestPath
			 << ((requestPath.length() > 0 && requestPath[requestPath.length() - 1] == '/') ? "" : "/")
			 << name << "\">" << name << "</a></li>";
    }
    closedir(dir);
    html << "</ul></body></html>";
    return html.str();
}

int ARequest::sendHTTPResponse(int clientFd, int statusCode, const string &body, const string& contentType, const string &cookie)
{
	ostringstream response;
	string statusText;
	
	// Set status text based on code
	switch (statusCode) {
		case 200: statusText = "OK"; break;
		case 204: statusText = "No content"; break;
		case 403: statusText = "Forbidden"; break;
		case 404: statusText = "Not Found"; break;
		case 413: statusText = "Body too large"; break;
		case 415: statusText = "Unsupported Media Type"; break;
		case 500: statusText = "Internal Server Error"; break;
		default: statusText = "Unknown"; break;
	}
	// Build HTTP response
	response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
	if (statusCode != 204)
	{
		response << "Content-Type: " << contentType << "\r\n";
		response << "Content-Length: " << body.length() << "\r\n";
	}
	if (!cookie.empty())
		response << cookie + "\r\n";
	if (!_keep_alive)
		response << "Connection: close\r\n";
	response << "\r\n";
	cout << response.str() << endl;
	response << body;
	
	string responseStr = response.str();
	if (send(clientFd, responseStr.c_str(), responseStr.length(), 0) == -1) {
		cerr << "Failed to send HTTP response" << endl;
		return -1;
	}
	return 0;
}

int ARequest::sendCGIResponse(int clientFd, const string &scriptPath, const ConfigParser *config, const Server &Server, const string &cookie)
{
	int 			cgiOutputFd = -1;
	vector<string>	location_cgi = config->getLocationVectorforPath(scriptPath, Server.getUid(), "cgi");
	string			autoindex = config->getLocationValueForPath(scriptPath, Server.getUid(), "autoindex");
    struct stat 	pathStat;

    // For location matching add trailing slash if it's a directory
    string pathForConfig = _path;
    if (stat(scriptPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
        if (_path[_path.length() - 1] != '/') {
            pathForConfig += "/";
        }
    }
    
	string 	auto_index = config->getLocationValueForPath(pathForConfig, Server.getUid(), "autoindex");

	if (auto_index.empty())
		auto_index = "on";

    if (stat(scriptPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
        if (auto_index == "on") {
            string listing = generateDirectoryListing(scriptPath, _path);
            return sendHTTPResponse(clientFd, 200, listing, "text/html", cookie);
        } else {
            string errorPage = loadErrorPage(403, config, Server.getUid());
            return sendHTTPResponse(clientFd, 403, errorPage, "text/html", cookie);
        }
    }

	try {
		// Execute CGI script
		
		map<string, string> cgi_list;
		for (vector<string>::iterator it = location_cgi.begin(); it != location_cgi.end(); it++)
		{
			if (it->find(' ') != string::npos)
			{
				string extension = it->substr(0, it->find(' '));
				string cgi = it->substr(it->find(' ') + 1);

				cgi_list[extension] = cgi;
			}
			else {
				
				if (*it == ".py")
					cgi_list[*it] = "/usr/bin/python3";
				if (*it == ".php")
					cgi_list[*it] = "/usr/bin/php";
				if (*it == ".sh")
					cgi_list[*it] = "/usr/bin/sh";
				if (*it == ".pl")
					cgi_list[*it] = "/usr/bin/perl";
			}
		}

		cgiOutputFd = CGI::interpret(scriptPath, Server, cgi_list);
	
		// Read the CGI output
		string cgiOutput;
		char buffer[4096];
		ssize_t bytesRead;
		
		while ((bytesRead = read(cgiOutputFd, buffer, sizeof(buffer))) > 0) {
			cgiOutput.append(buffer, bytesRead);
		}
		
		if (cgiOutputFd != -1) {
			close(cgiOutputFd);
			cgiOutputFd = -1;
		}
		
		// Send successful response with CGI output
		string contentType = getContentType(scriptPath);
		return sendHTTPResponse(clientFd, 200, cgiOutput, contentType, cookie);
		
	} catch (const CGI::CGIException &e) {
		// Close the file descriptor if it was opened
		if (cgiOutputFd != -1) {
			close(cgiOutputFd);
		}
		
		// Handle true CGI execution errors (file not found, permission denied, etc.)
		// These are cases where the script couldn't even run
		string errorPage = loadErrorPage(e.getHttpStatus(), config, Server.getUid());
		return sendHTTPResponse(clientFd, e.getHttpStatus(), errorPage, "text/html", cookie);
	} catch (...) {
		// Handle any other exceptions
		if (cgiOutputFd != -1) {
			close(cgiOutputFd);
		}
		
		string errorPage = loadErrorPage(500, config, Server.getUid());
		return sendHTTPResponse(clientFd, 500, errorPage, "text/html", cookie);
	}
}

string ARequest::loadErrorPage(int statusCode, const ConfigParser *config, const string &serverUid) const
{
	return config->getErrorPageContent(const_cast<ConfigParser&>(*config), serverUid, statusCode);
}

string ARequest::getContentType(const string &filePath) const
{
	size_t pos = filePath.rfind('.');
	if (pos == string::npos)
		return "application/octet-stream";
	if (filePath.find("/uploads/") != string::npos)
		return "application/octet-stream";
	string ext = filePath.substr(pos + 1);
	
	if (ext == "html" || ext == "htm" || ext == "php" || ext == "py")
		return "text/html";
	else if (ext == "css")
		return "text/css";
	else if (ext == "js")
		return "application/javascript";
	else if (ext == "png")
		return "image/png";
	else if (ext == "ico")
		return "image/x-icon";
	else if (ext == "jpg" || ext == "jpeg")
		return "image/jpeg";
	else if (ext == "mp3")
		return "audio/mpeg";
	else if (ext == "wav")
		return "audio/wav";
	else if (ext == "ogg")
		return "audio/ogg";
	else if (ext == "gif")
		return "image/gif";
	else if (ext == "json")
		return "application/json";
	else if (ext == "txt")
		return "text/plain";
	else if (ext == "pdf")
		return "application/pdf";
	else
		return "application/octet-stream";
}

map<string, string> parseQuery(const string &query)
{
	map<string, string> map;
	string	key;
	string	value;
	size_t amperPos = query.find('&');
	size_t equalPos = query.find('=');

	key = query.substr(0, equalPos);
	value = query.substr(equalPos + 1, amperPos - equalPos - 1);
	while (true)
	{
		if (!key.empty())
			map[key] = value;
		equalPos = query.find('=', amperPos);
		if (equalPos == string::npos)
			break ;
		key = query.substr(amperPos + 1, equalPos - amperPos - 1);
		amperPos = query.find('&', equalPos);
		value = query.substr(equalPos + 1, amperPos - equalPos - 1);
	}
	return (map);
}
