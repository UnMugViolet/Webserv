#include <string.h>
#include <sstream>
#include <vector>

#include "ARequest.hpp"
#include "ConfigParser.hpp"
#include "RequestHandler.hpp"

ARequest::ARequest()
{
	_method = 0;
	_path = "";
	_host = "";
	_keep_alive = true;
	_client = "";
	_accepted_mime = "*/*";
	_tmp_file = "";
}

ARequest::ARequest(const ARequest &src)
{
	if (this != &src)
		*this = src;
}

ARequest::~ARequest() {}

int ARequest::isKeepalive() const
{
	return (_keep_alive);
}

ARequest &ARequest::operator=(const ARequest &src)
{
	if (this != &src)
	{
		this->_method = src._method;
		this->_path = src._path;
		this->_host = src._host;
		this->_keep_alive = src._keep_alive;
		this->_client = src._client;
		this->_accepted_mime = src._accepted_mime;
		this->_tmp_file = src._tmp_file;
	}
	return (*this);
}

string generateDirectoryListing(string const &dirPath, string const &requestPath)
{
	DIR *dir = opendir(dirPath.c_str());
	if (!dir)
		return ("<h1>Cannot open directory</h1>");

	ostringstream html;

	html << "<html><head><title>Index of " << requestPath << "</title></head><body>";
	html << "<h1>Index of " << requestPath << "</h1><ul>";

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		string name = entry->d_name;

		if (name == ".")
			continue;
		html << "<li><a href=\"" << requestPath
			 << ((requestPath.length() > 0 && requestPath[requestPath.length() - 1] == '/') ? "" : "/")
			 << name << "\">" << name << "</a></li>";
	}
	closedir(dir);
	html << "</ul></body></html>";
	return (html.str());
}

/**
 * Load and send an error page to the client.
 * @param fd The file descriptor of the client socket.
 * @param errorCode The HTTP error code to send.
 * @param server The server instance handling the request.
 * @param config The configuration parser instance.
 * @param errorMessage Optional error message for logging.
 * @param keepAlive Whether to keep the connection alive after sending the error page (default: true).
 */
void ARequest::fetchErrorPageWithCode(int fd, int errorCode, Server &server, ConfigParser const *config, string errorMessage, bool keepAlive)
{
	GetRequest requestObject;

	if (!errorMessage.empty())
	{
		cout << RED << BOLD << "[Loading Error Page]: " << errorCode << " " << NEUTRAL << RED << errorMessage << NEUTRAL << endl;
		Logger::error(server.getUid(), "[Loading Error Page]: " + ft_itos(errorCode) + " " + errorMessage);
	}

	string errorPage = config->getErrorPageContent(const_cast<ConfigParser &>(*config), server.getUid(), errorCode);
	string response = requestObject.writeHTTPResponse(server, errorCode, errorPage, "text/html");
	server.keepaliveDefine(fd, keepAlive);
	server.fillClientBuffer(fd, response);
}


/**
 * Writes an HTTP response string based on the provided status code and body.
 * @param server The server instance handling the request.
 * @param statusCode The HTTP status code to include in the response.
 * @param body The body content of the response.
 * @param contentType The MIME type of the response content.
 * @return A formatted HTTP response string.
 */
string ARequest::writeHTTPResponse(const Server &server, int statusCode, const string &body, const string &contentType)
{
	ostringstream response;
	string statusText;

	// Set status text based on code
	switch (statusCode)
	{
	case 200:
		statusText = "OK";
		break;
	case 204:
		statusText = "No content";
		break;
	case 403:
		statusText = "Forbidden";
		break;
	case 404:
		statusText = "Not Found";
		break;
	case 413:
		statusText = "Body too large";
		break;
	case 415:
		statusText = "Unsupported Media Type";
		break;
	case 500:
		statusText = "Internal Server Error";
		break;
	case 503:
		statusText = "Gateway timeout";
		break;
	default:
		statusText = "Unknown";
		break;
	}

	// Build HTTP response
	response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
	if (statusCode != 204)
	{
		response << "Content-Type: " << contentType << "\r\n";
		response << "Content-Length: " << body.length() << "\r\n";
	}
	if (!_keep_alive)
	{
		response << "Connection: close\r\n";
		// Additional headers to prevent browser caching and retries on timeout
		if (statusCode == 504)
		{
			response << "Cache-Control: no-cache, no-store, must-revalidate\r\n";
			response << "Pragma: no-cache\r\n";
			response << "Expires: 0\r\n";
			response << "Retry-After: 60\r\n"; // Tell browser to wait 60 seconds before retry
		}
	}

	// Add cookies if any
	if (!server.getCookieHeader().empty())
		response << server.getCookieHeader();

	response << "\r\n";
	response << body;

	return (response.str());
}

int ARequest::sendCGIResponse(int fd, const string &scriptPath, const ConfigParser *config, Server &Server)
{
	int cgiOutputFd = -1;
	string autoindex = config->getLocationValueForPath(scriptPath, Server.getUid(), "autoindex", true);
	struct stat pathStat;

	// For location matching add trailing slash if it's a directory
	string pathForConfig = _path;

	if (stat(scriptPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode))
	{
		if (_path[_path.length() - 1] != '/')
		{
			pathForConfig += "/";
		}
	}

	string auto_index = config->getLocationValueForPath(pathForConfig, Server.getUid(), "autoindex", true);

	if (auto_index.empty())
		auto_index = "on";

	if (stat(scriptPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode))
	{
		if (auto_index == "on")
		{
			string listing = generateDirectoryListing(scriptPath, _path);
			string response = writeHTTPResponse(Server, 200, listing, "text/html");
			Server.fillClientBuffer(fd, response);
			Server.keepaliveDefine(fd, isKeepalive());
		}
		else
		{
			cout << "1" << endl;
			string errorPage = loadErrorPage(403, config, Server.getUid());
			string response = writeHTTPResponse(Server, 403, errorPage, "text/html");
			Server.fillClientBuffer(fd, response);
			Server.keepaliveDefine(fd, isKeepalive());
		}
		return (1);
	}

	// Execute CGI script
	try
	{
		map<string, string> cgi_list = CGI::_getCgiList(Server, config, scriptPath);

		cgiOutputFd = CGI::interpret(scriptPath, Server, cgi_list, "", _tmp_file);

		Server.setCgiFdforClient(fd, cgiOutputFd);
		_path = scriptPath;
		Server.setCgiRequest(cgiOutputFd, *this);

		return (1);
	}
	catch (const CGI::CGIException &e)
	{
		// Close the file descriptor if it was opened
		if (cgiOutputFd != -1)
		{
			close(cgiOutputFd);
		}

		// Handle true CGI execution errors (file not found, permission denied, etc.)
		// These are cases where the script couldn't even run
		string errorPage = loadErrorPage(e.getHttpStatus(), config, Server.getUid());
		string response = writeHTTPResponse(Server, e.getHttpStatus(), errorPage, "text/html");
		Server.fillClientBuffer(fd, response);
		Server.keepaliveDefine(fd, isKeepalive());
		return (1);
	}
	catch (...)
	{
		// Handle any other exceptions
		if (cgiOutputFd != -1)
		{
			close(cgiOutputFd);
		}

		string errorPage = loadErrorPage(500, config, Server.getUid());
		string response = writeHTTPResponse(Server, 500, errorPage, "text/html");
		Server.fillClientBuffer(fd, response);
		Server.keepaliveDefine(fd, isKeepalive());
		return (1);
	}
}

string ARequest::loadErrorPage(int statusCode, const ConfigParser *config, const string &serverUid) const
{
	return (config->getErrorPageContent(const_cast<ConfigParser &>(*config), serverUid, statusCode));
}

string ARequest::checkContentType(string &contentType)
{
	string accepted = _accepted_mime;
	if (accepted.find(contentType) != string::npos)
		return (contentType);
	if (accepted.find("*/*") != string::npos)
		return (contentType);
	string generalType = contentType.substr(0, contentType.find('/') + 1);
	generalType += "*";
	if (accepted.find(generalType) != string::npos)
		return (contentType);
	if (accepted.find("application/octet-stream") != string::npos)
		return ("application/octet-stream");
	if (accepted.find("application/*") != string::npos)
		return ("application/octet-stream");

	throw(exception());
}

string ARequest::getContentType() const
{
	string filePath = _path;
	size_t pos = filePath.rfind('.');
	if (pos == string::npos)
		return ("application/octet-stream");
	// if (filePath.find("/uploads/") != string::npos)
	// 	return "application/octet-stream";
	string ext = filePath.substr(pos + 1);

	if (ext == "html" || ext == "htm" || ext == "php" || ext == "py")
		return ("text/html");
	else if (ext == "css")
		return ("text/css");
	else if (ext == "js")
		return ("application/javascript");
	else if (ext == "png")
		return ("image/png");
	else if (ext == "ico")
		return ("image/x-icon");
	else if (ext == "jpg" || ext == "jpeg")
		return ("image/jpeg");
	else if (ext == "mp3")
		return ("audio/mpeg");
	else if (ext == "wav")
		return ("audio/wav");
	else if (ext == "ogg")
		return ("audio/ogg");
	else if (ext == "gif")
		return ("image/gif");
	else if (ext == "json")
		return ("application/json");
	else if (ext == "txt")
		return ("text/plain");
	else if (ext == "pdf")
		return ("application/pdf");
	else
		return ("application/octet-stream");
}

map<string, string> parseQuery(const string &query)
{
	map<string, string> map;
	string key;
	string value;
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
			break;
		key = query.substr(amperPos + 1, equalPos - amperPos - 1);
		amperPos = query.find('&', equalPos);
		value = query.substr(equalPos + 1, amperPos - equalPos - 1);
	}
	return (map);
}

int ARequest::getMethod() const
{
	return (_method);
}

string ARequest::getPath() const
{
	return (_path);
}

string ARequest::getTmpFile() const
{
	return (_tmp_file);
}