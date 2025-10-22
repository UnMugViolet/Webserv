#include "RequestHandler.hpp"
#include "ConfigParser.hpp"
#include "Server.hpp"

RequestHandler::RequestHandler()
{
	_maxBodySize = 0;
}

RequestHandler::RequestHandler(RequestHandler &src)
{
	/*copy what needs to be here*/
	this->_maxBodySize = src._maxBodySize;
}

RequestHandler &RequestHandler::operator=(RequestHandler &src)
{
	this->_maxBodySize = src._maxBodySize;
	return (*this);
}

RequestHandler::~RequestHandler() {}

/**
 * Get the original URL and decode percent-encoded characters.
 * Also, sanitize the path by removing consecutive slashes.
 * @param src The percent-encoded URL string.
 * @return The decoded and sanitized URL string.
 */
string urlDecode(const string &src)
{
	ostringstream out;

	for (size_t i = 0; i < src.length(); ++i)
	{
		while (src[i] == '/' && src[i + 1] == '/')
			++i;
		if (src[i] == '%' && i + 2 < src.length())
		{
			int hex = 0;
			istringstream iss(src.substr(i + 1, 2));
			if (iss >> std::hex >> hex)
				out << static_cast<char>(hex);
			i += 2;
		}
		else if (src[i] == '+')
			out << ' ';
		else
			out << src[i];
	}
	return (out.str());
}

int RequestHandler::_checkAccess(const string &path)
{
	if (access(path.c_str(), F_OK) == -1) {
		return (404);
	}

	if (getExtension(path) == "cgi" && access(path.c_str(), X_OK) == -1) {
		return (403);
	}

	if (access(path.c_str(), R_OK) == -1) {
		return (403);
	}
	return (200);
}

string RequestHandler::getExtension(const string &path)
{
	size_t pos = path.rfind('.');
	if (pos == string::npos)
		return ("");

	return (path.substr(pos + 1));
}

map<string, size_t> getIndex(string const &indexes, string const &root)
{
	map<string, size_t> result;
	string fullPath;
	string goodIndex;
	string lastIndex;
	size_t lastStatus = 404;
	size_t space1;
	size_t space2 = 0;

	if (indexes.empty())
		return (result);

	while (true)
	{
		space1 = indexes.find_first_not_of(" ", space2);
		if (space1 == string::npos)
			break;
		space2 = indexes.find(' ', space1);
		if (space2 == string::npos)
			goodIndex = indexes.substr(space1);
		else
			goodIndex = indexes.substr(space1, space2 - space1);

		fullPath = root + goodIndex;

		size_t status = RequestHandler::_checkAccess(fullPath);

		cout << CYAN << BOLD << "Checking index: " << goodIndex << " -> " << fullPath << " (status: " << status << ")" << NEUTRAL << endl;

		// If we find a 200 (accessible), return it immediately
		if (status == 200)
		{
			result[goodIndex] = status;
			return result;
		}

		// Keep track of the last index and its status
		lastIndex = goodIndex;
		lastStatus = status;

		if (space2 == string::npos)
			break;
	}

	// No 200 found, return the last index with its status
	if (!lastIndex.empty())
	{
		result[lastIndex] = lastStatus;
		cout << CYAN << BOLD << "No accessible index found, returning last: " << lastIndex << " (status: " << lastStatus << ")" << NEUTRAL << endl;
	}

	return result;
}

string trim(const string &str)
{
	size_t first = str.find_first_not_of(" \r\n\t");
	if (first == string::npos)
		return ("");

	size_t last = str.find_last_not_of(" \n\r\t");
	return (str.substr(first, last - first + 1));
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
void RequestHandler::fetchErrorPageWithCode(int fd, int errorCode, Server &server, ConfigParser *config, string errorMessage, bool keepAlive)
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

map<string, string> RequestHandler::parseHeader(string header) const
{
	map<string, string> headers;
	istringstream stream(header);
	string line;
	string method;
	string path;
	size_t colon;

	getline(stream, line);
	istringstream requestLine(line);

	// Parse the request line: METHOD PATH HTTP/VERSION
	requestLine >> method >> path;
	headers["path"] = path;
	headers["method"] = method;

	while (getline(stream, line) && line != "\r" && !line.empty())
	{
		colon = line.find(':');
		if (colon != string::npos)
		{
			string key = line.substr(0, colon);

			if (key.find(' ') != string::npos)
				throw(runtime_error("Bad Request: Invalid header key"));

			string value = trim(line.substr(colon + 1));

			if (value.empty())
				throw(runtime_error("Bad Request: Empty header value"));

			headers[key] = value;
		}
	}
	return (headers);
}

int RequestHandler::checkHeader(int fd, Server &server, ConfigParser *config, map<string, string> &headermap, string &body, string &savestring)
{
	string serverUid = server.getUid();
	string header;

	if (headermap.find("Host") == headermap.end())
	{
		return (fetchErrorPageWithCode(fd, 400, server, config, "No server_name found in header", false), 2);
	}
	string max_body_size = config->getLocationValueForPath(headermap["path"], serverUid, "client_max_body_size", true);
	if (max_body_size.empty())
		max_body_size = config->getValue("client_max_body_size");
	setMaxBodySize(max_body_size);

	// Check if request is chunked
	if (headermap.find("Transfer-Encoding") != headermap.end() && headermap["Transfer-Encoding"].find("chunked"))
		return (handleChunkedRequest(fd, savestring, body, server, config, 1));

	if (headermap.find("Content-Length") != headermap.end())
	{
		istringstream iss(headermap["Content-Length"]);
		size_t contentLength;
		if (!(iss >> contentLength))
			return (fetchErrorPageWithCode(fd, 400, server, config, "Invalid Content-Length header", false), 2);

		// Check against max body size
		if (_maxBodySize > 0 && contentLength > static_cast<size_t>(_maxBodySize))
		{
			string errormsg = "Content-Length " + ft_itos(contentLength) + " exceeds max body size of " + ft_itos(_maxBodySize);
			return (fetchErrorPageWithCode(fd, 413, server, config, errormsg, false), 2);
		}
		if (body.size() == contentLength)
		{
			server.fillClientBuffer(fd, savestring);
			return (1);
		}
	}
	server.fillClientBuffer(fd, savestring);
	return (0);
}

int RequestHandler::readOnce(int fd, Server &server, ConfigParser *config)
{
	string serverUid = server.getUid();
	char buff[BUFFER_SIZE];
	int received;
	string body = "";
	string header;
	string savestring = "";
	size_t const MAX_HEADER_SIZE = 8192; // 8KB for headers
	size_t headerlimit;
	map<string, string> headermap;

	if (server.getClientBuffer(fd) == "")
	{
		memset(buff, 0, BUFFER_SIZE);

		received = recv(fd, buff, BUFFER_SIZE - 1, 0);
		if (received <= 0)
		{
			Logger::error(server.getUid(), "recv error");
			return (-1);
		}

		// Quick exit for HTTPS/TLS handshake
		if ((unsigned char)buff[0] == 0x16)
		{
			Logger::error(serverUid, "Received HTTPS/TLS handshake, closing connection.");
			cerr << "Received HTTPS/TLS handshake, closing connection." << endl;
			return (-1);
		}
		savestring.append(buff, received);
		server.fillClientBuffer(fd, savestring);
		if (savestring.find("\r\n\r\n") != string::npos)
		{
			headerlimit = savestring.find("\r\n\r\n");
			body = savestring.substr(headerlimit + 4, string::npos);
			header = savestring;
			header.erase(headerlimit, string::npos);
			if (header.size() > MAX_HEADER_SIZE)
			{
				return (fetchErrorPageWithCode(fd, 413, server, config, "Header too large", false), 2);
			}
			if (savestring.find("Content-Length") == string::npos && savestring.find("Transfer-Encoding: chunked") == string::npos)
				return (1);
			else
			{
				headermap = parseHeader(header);
				return (checkHeader(fd, server, config, headermap, body, savestring));
			}
		}
		return (0);
	}

	savestring.append(server.getClientBuffer(fd));
	headerlimit = savestring.find("\r\n\r\n");
	if (headerlimit == string::npos)
	{

		// Prevent header from being too large
		if (savestring.size() > MAX_HEADER_SIZE)
		{
			return (fetchErrorPageWithCode(fd, 413, server, config, "Header too large", false), 2);
		}
		memset(buff, 0, BUFFER_SIZE);

		received = recv(fd, buff, BUFFER_SIZE - 1, 0);
		if (received <= 0)
		{
			server.clearClientBuffer(fd);
			Logger::error(server.getUid(), "recv error");
			return (-1);
		}

		savestring.append(buff, received);
		server.fillClientBuffer(fd, savestring);
		if (savestring.find("\r\n\r\n") != string::npos)
		{
			if (savestring.find("Content-Length") == string::npos && savestring.find("Transfer-Encoding: chunked") == string::npos)
				return (1);
			else
			{
				headerlimit = savestring.find("\r\n\r\n");
				body = savestring.substr(headerlimit + 4, string::npos);
				header = savestring;
				header.erase(headerlimit, string::npos);

				headermap = parseHeader(header);

				return (checkHeader(fd, server, config, headermap, body, savestring));
			}
		}
		return (0);
	}
	else
	{

		body = savestring.substr(headerlimit + 4, string::npos);
		header = savestring;
		header.erase(headerlimit, string::npos);

		headermap = parseHeader(header);

		if (headermap["Transfer-Encoding"].find("chunked") != string::npos)
		{
			return (handleChunkedRequest(fd, savestring, body, server, config, 1));
		}
		istringstream iss(headermap["Content-Length"]);
		size_t contentLength;
		iss >> contentLength;

		if (body.size() < contentLength)
		{
			memset(buff, 0, BUFFER_SIZE);

			received = recv(fd, buff, min((size_t)BUFFER_SIZE - 1, contentLength - body.size()), 0);
			if (received <= 0)
			{
				server.clearClientBuffer(fd);
				Logger::error(server.getUid(), "recv error");
				return (-1);
			}
			body.append(buff, received);
			savestring.append(buff, received);
			server.fillClientBuffer(fd, savestring);
			if (body.size() == contentLength)
				return (1);

			return (0);
		}
		return (1);
	}
}

int RequestHandler::handleRequest(int fd, Server &server, ConfigParser *config)
{
	size_t headerlimit;
	string serverRoot;
	string serverUid = server.getUid();
	string body;
	string header;
	vector<string> serverNames = server.getServerNames();
	map<string, string> headermap;

	// Read with recv until request is complete
	int res = readOnce(fd, server, config);
	if (res != 1)
		return (res);

	try
	{
		header = server.getClientBuffer(fd);
		if (header.size() <= 0)
		{
			cout << "body empty?" << endl;
			return (-1);
		}

		headerlimit = header.find("\r\n\r\n");
		if (headerlimit != string::npos && headerlimit + 4 <= header.length())
		{
			body = header.substr(headerlimit + 4, string::npos);
			header.erase(headerlimit, string::npos);
		}
		else
		{
			body = "";
		}
		headermap = parseHeader(header);
		string session_id;

		// Handle cookies
		if (headermap.find("Cookie") == headermap.end() || headermap["Cookie"].find("session_id=") == string::npos)
		{
			server.clearCookieHeader();
			session_id = server.generateSessionId();
			if (headermap["Cookie"].find("session_id=") == string::npos)
				server.setCookie(session_id, "session_id", session_id);
		}
		server.parseCookie(session_id, headermap["Cookie"]);

		// Make the raw Cookie header available to CGI via the environment
		if (headermap.find("Cookie") != headermap.end())
			server.setEnvValue("HTTP_COOKIE", headermap["Cookie"]);
		else
			server.setEnvValue("HTTP_COOKIE", "");

		// Extract a 'theme' cookie value (if present) and expose it as THEME env var for server-side usage
		{
			string theme_value = "";
			if (headermap.find("Cookie") != headermap.end())
			{
				size_t pos = headermap["Cookie"].find("theme=");
				if (pos != string::npos)
				{
					size_t start = pos + strlen("theme=");
					size_t end = headermap["Cookie"].find(';', start);
					theme_value = headermap["Cookie"].substr(start, (end == string::npos) ? string::npos : end - start);
					// trim possible whitespace
					size_t first = theme_value.find_first_not_of(' ');
					if (first != string::npos)
						theme_value = theme_value.substr(first);
					server.setEnvValue("THEME", theme_value);
				}
			}
		}

		Logger::access(serverUid, "http request: " + header);

		size_t colonPos = headermap["Host"].find(':');
		if (colonPos != string::npos)
		{
			server.setEnvValue("SERVER_NAME", headermap["Host"].substr(0, colonPos));
			server.setEnvValue("SERVER_PORT", headermap["Host"].substr(colonPos + 1));
		}
		else
		{
			server.setEnvValue("SERVER_NAME", headermap["Host"]);
			server.setEnvValue("SERVER_PORT", "80");
		}
		string max_body_size = config->getServerValue(serverUid, "client_max_body_size");

		if (max_body_size.empty())
			max_body_size = config->getValue("client_max_body_size");
		setMaxBodySize(max_body_size);
		serverRoot = config->getServerValue(serverUid, "root");

		// get the index full path
		if (!serverRoot.empty() && serverRoot[serverRoot.length() - 1] == '/')
			serverRoot = serverRoot.substr(0, serverRoot.length() - 1);

		string headerpath = headermap["path"];
		
		string rootForLocation = config->getLocationValueForPath(headerpath, serverUid, "root", 0);

		if (!rootForLocation.empty())
		{
			string location = config->getLocation(headerpath, serverUid);
			headerpath.replace(headerpath.find(location), location.size(), rootForLocation);
		}
		string fullPath = serverRoot + headerpath;
		cout << "HEadermap: " << headerpath << endl;
		cout << "fullpath: " << fullPath << endl;

		server.setEnvValue("REQUEST_METHOD", headermap["method"]);
		server.setEnvValue("REQUEST_URI", headerpath);
		size_t questionPos = headerpath.find('?');

		if (questionPos != string::npos && questionPos + 1 < headerpath.length())
			server.setEnvValue("QUERY_STRING", headerpath.substr(questionPos + 1));
		else
			server.setEnvValue("QUERY_STRING", "");

		// Checks if the path is allowed by the location for the requested method
		string cleanPath = headerpath;
		size_t queryPos = headerpath.find('?');
		if (queryPos != string::npos)
			cleanPath = headerpath.substr(0, queryPos);

		// Check for redirects before method validation
		string redirect = config->getLocationValueForPath(cleanPath, server.getUid(), "return", false);

		if (!redirect.empty())
			return handleRedirect(fd, server, redirect, headermap);

		// Check for path too long before proceeding
		if (cleanPath.length() >= PATH_MAX)
			return (fetchErrorPageWithCode(fd, 414, server, config, "Request URI too long", false), 1);

		try
		{
			int status = 0;
			string allowed_methods = config->getLocationValueForPath(cleanPath, server.getUid(), "allow_methods", true);
			istringstream iss(allowed_methods);
			string one_method;

			while (iss >> one_method)
			{
				if (one_method == headermap["method"])
				{
					status = 1;
					break;
				}
			}
			// Case no methods allowed for the location
			if (status == 0)
				return (fetchErrorPageWithCode(fd, 403, server, config), 1);
		}
		catch (exception const &e)
		{
			return (fetchErrorPageWithCode(fd, 403, server, config), 1);
		}
		if (headermap["method"] == "GET")
		{
			GetRequest requestObject(headermap);

			return (requestObject.handleGet(fd, server, config, fullPath));
		}
		else if (headermap["method"] == "POST")
		{
			PostRequest requestObject(headermap, session_id);

			return (requestObject.handlePost(fd, server, body, config));
		}
		else if (headermap["method"] == "DELETE")
		{
			DeleteRequest requestObject(headermap);

			return (requestObject.handleDelete(fd, server, config, fullPath));
		}
		else
		{
			fetchErrorPageWithCode(fd, 405, server, config, "Unknown method", false);
			return (Logger::error(serverUid, "Unknown method in HEADER: " + headermap["method"]), 1);
		}
	}
	catch (const exception &e)
	{
		cerr << "Error parsing request: " << e.what() << endl;
		return (-1);
	}
	return (0);
}

/**
 * Handles HTTP redirections (301, 302) based on configuration.
 * @param fd The file descriptor to send the response to
 * @param server The server object containing configuration and state
 * @param redirect The redirect string from config (e.g., "301 https://example.com")
 * @param headermap The parsed request headers
 * @return 1 on success, -1 on error
 */
int RequestHandler::handleRedirect(int fd, Server &server, const string &redirect, map<string, string> &headermap)
{
	istringstream iss(redirect);
	int code;
	string url;

	if (!(iss >> code >> url))
		return -1; // Invalid redirect format

	string statusText;
	switch (code)
	{
	case 301:
		statusText = "Moved Permanently";
		break;
	case 302:
		statusText = "Found";
		break;
	case 307:
		statusText = "Temporary Redirect";
		break;
	case 308:
		statusText = "Permanent Redirect";
		break;
	default:
		return -1; // Unsupported redirect code
	}

	stringstream ss;
	ss << code;
	string response = "HTTP/1.1 " + ss.str() + " " + statusText + "\r\n";

	response += "Location: " + url + "\r\n";
	response += "Content-Length: 0\r\n";

	// Determine keep-alive status from headers
	bool keepAlive = !(headermap.find("Connection") != headermap.end() &&
					   headermap["Connection"] == "close");
	response += "Connection: " + (keepAlive ? string("keep-alive") : string("close")) + "\r\n";
	response += "\r\n";

	server.fillClientBuffer(fd, response);
	server.keepaliveDefine(fd, keepAlive);

	return (1);
}

void RequestHandler::setMaxBodySize(string size)
{
	istringstream iss(size);
	int value;
	string unit;

	if (iss >> value && value >= 0)
	{
		_maxBodySize = value;
		iss >> unit;
		if (!unit.empty())
		{
			if (unit == "K" || unit == "k" || unit == "KB" || unit == "kb")
				_maxBodySize *= 1024;
			else if (unit == "M" || unit == "m" || unit == "MB" || unit == "mb")
				_maxBodySize *= 1024 * 1024;
			else
			{
				cerr << "unsupported unit" << endl;
				_maxBodySize = 0;
			}
		}
	}
	else
		_maxBodySize = MAX_BODY_SIZE; // Default 1MB if invalid
}

int RequestHandler::handleChunkedRequest(int fd, string &savestring, string &body, Server &server, ConfigParser *config, int can_read)
{
	static string fullbody;
	size_t hexlen;
	size_t totallen = 0;
	size_t pos = 0;
	char buff[BUFFER_SIZE];

	while (true)
	{
		size_t rn_pos = body.find("\r\n", pos);
		if (rn_pos != string::npos)
		{
			// We have a full chunk size line
			std::string chunk_size_line = body.substr(pos, rn_pos - pos);
			if (chunk_size_line.empty())
				return (fetchErrorPageWithCode(fd, 400, server, config, "Empty chunk size line", false), -1);
			istringstream iss(chunk_size_line);
			if (!(iss >> std::hex >> hexlen))
			{
				return (fetchErrorPageWithCode(fd, 400, server, config, "Malformed chunk size line", false), -1);
			}
			totallen += hexlen;
			pos = rn_pos + 2;
			if (hexlen == 0)
			{
				if (body.size() < pos + 2)
				{
					if (!can_read)
						return (0);
					int received = recv(fd, buff, pos + 2 - body.size(), 0);
					if (received <= 0)
					{
						server.clearClientBuffer(fd);
						return (-1);
					}
				}
				savestring.erase(savestring.find("\r\n\r\n") + 4);
				savestring.append(fullbody);
				server.fillClientBuffer(fd, savestring);
				return (1);
			}
			if (totallen <= fullbody.size())
			{
				pos += hexlen + 2;
				continue;
			}
			if (body.size() > pos)
				fullbody.append(body, pos, totallen - fullbody.size());
			if (totallen > fullbody.size())
			{
				if (!can_read)
					return (0);
				memset(buff, 0, BUFFER_SIZE);
				int received = recv(fd, buff, min((size_t)BUFFER_SIZE - 1, totallen - fullbody.size()), 0);
				if (received <= 0)
				{
					server.clearClientBuffer(fd);
					return (-1);
				}
				fullbody.append(buff, received);
				savestring.append(buff, received);
				server.fillClientBuffer(fd, savestring);
				return (0);
			}
			pos += hexlen + 2;
		}
		else
		{
			// Not enough data for a full chunk size line, wait for more
			if (!can_read)
				return (0);
			memset(buff, 0, BUFFER_SIZE);
			int received = recv(fd, buff, BUFFER_SIZE - 1, 0);
			if (received <= 0)
			{
				server.clearClientBuffer(fd);
				return (-1);
			}
			savestring.append(buff, received);
			body.append(buff, received);
			server.fillClientBuffer(fd, savestring);
			can_read = 0;
		}
	}
}
