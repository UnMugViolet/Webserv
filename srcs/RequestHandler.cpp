#include "RequestHandler.hpp"
#include "ConfigParser.hpp"
#include "Server.hpp"

RequestHandler::RequestHandler()
{
	_maxBodySize = 0;
	return;
}

RequestHandler::RequestHandler(RequestHandler &src)
{
	/*copy what needs to be here*/
	this->_maxBodySize = src._maxBodySize;
	return;
}

RequestHandler &RequestHandler::operator=(RequestHandler &src)
{
	this->_maxBodySize = src._maxBodySize;
	return (*this);
}

RequestHandler::~RequestHandler()
{
	return;
}

/**
 * Get the original URL and decode percent-encoded characters.
 * Also, sanitize the path by removing consecutive slashes.
 * @param src The percent-encoded URL string.
 * @return The decoded and sanitized URL string. 
*/
string urlDecode(const string &src)
{
	ostringstream out;

	cout << "Decoding URL: " << src << endl;
	for (size_t i = 0; i < src.length(); ++i) {
		while (src[i] == '/' && src[i + 1] == '/')
			++i;
		if (src[i] == '%' && i + 2 < src.length()) {
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
	return out.str();
}

int RequestHandler::_checkAccess(const string &path)
{
	if (access(path.c_str(), F_OK) == -1)
		return 404;
	if (getExtension(path) == "cgi" && access(path.c_str(), X_OK) == -1)
		return 403;
	if (access(path.c_str(), R_OK) == -1)
		return 403;
	return 200;
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
        return result;

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
        
        if (goodIndex[0] != '/' && goodIndex[0] != '.')
            goodIndex = "/" + goodIndex;
        
        fullPath = root + goodIndex;
        
        size_t status = RequestHandler::_checkAccess(fullPath);
        
        cout << CYAN << BOLD << "Checking index: " << goodIndex << " -> " << fullPath << " (status: " << status << ")" << NEUTRAL << endl;
        
        // If we find a 200 (accessible), return it immediately
        if (status == 200) {
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
    if (!lastIndex.empty()) {
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
				throw runtime_error("Bad Request: Invalid header key");
			string value = trim(line.substr(colon + 1));
			if (value.empty())
				throw runtime_error("Bad Request: Empty header value");
			headers[key] = value;
		}
	}
	return (headers);
}

int RequestHandler::readOnce(int fd, Server &server, ConfigParser *config)
{
	string	serverUid = server.getUid();
	size_t	const BUFFER_SIZE = 1;
	char	buff[BUFFER_SIZE];
	int		received;
	string	body;
	string	header;
	string	savestring;
	size_t	const MAX_HEADER_SIZE = 8192; // 8KB for headers
	size_t	headerlimit;
	map<string, string>	headermap;

	if (server.getClientBuffer(fd) == "")
	{
		received = recv(fd, buff, BUFFER_SIZE, 0);
		if (received <= 0)
			return -1;

		// Quick exit for HTTPS/TLS handshake	
		if ((unsigned char)buff[0] == 0x16)
		{
			Logger::error(serverUid, "Received HTTPS/TLS handshake, closing connection.");
			cerr << "Received HTTPS/TLS handshake, closing connection." << endl;
			return -1;
		}
		savestring.append(buff, received);
		server.fillClientBuffer(fd, savestring);
		return (0);
	}

	savestring.append(server.getClientBuffer(fd));
	headerlimit = savestring.find("\r\n\r\n");
	if (headerlimit == string::npos)
	{
		// Prevent header from being too large
		if (savestring.size() > MAX_HEADER_SIZE)
			return (Logger::error(serverUid, "Header too large"), -1);
		received = recv(fd, buff, BUFFER_SIZE, 0);
		if (received <= 0)
		{
			server.clearClientBuffer(fd);
			return -1;
		}
		
		savestring.append(buff, received);
		server.fillClientBuffer(fd, savestring);
		return (0);
	} else {
		body = savestring.substr(headerlimit + 4, string::npos);
		header = savestring;
		header.erase(headerlimit, string::npos);
		
		headermap = parseHeader(header);

		if (headermap.find("Host") == headermap.end())
		{
			GetRequest requestObject;

			cerr << "No server_name, bad request" << endl;
			string errorPage = config->getErrorPageContent(const_cast<ConfigParser &>(*config), serverUid, 400);
			// if (requestObject.sendHTTPResponse(fd, 400, errorPage, "text/html") == -1)
				// cerr << "Failed to send 400 response" << endl;
			server.clearClientBuffer(fd);
			return -1;
		}
		server.setEnvValue("SERVER_NAME", headermap["Host"].substr(0, headermap["Host"].find(':')));
		if (headermap["Host"].find(':'))
			server.setEnvValue("SERVER_PORT", headermap["Host"].substr(headermap["Host"].find(':') + 1));
		else
			server.setEnvValue("SERVER_PORT", "80");
		string max_body_size = config->getServerValue(serverUid, "client_max_body_size");
		if (max_body_size.empty())
			max_body_size = config->getValue("client_max_body_size");
		setMaxBodySize(max_body_size);
		// Check if we need to read more body data
		// if (headermap.find("Transfer-Encoding") != headermap.end() && headermap["Transfer-Encoding"].find("chuncked"))
		// {
		// 	body = handleChunckedRequest(fd, body);
		// 	if (body.empty()) {
		// 		cout << "empty body\n";
		// 		return (-1);
		// 	}
		// }
		if (headermap.find("Content-Length") != headermap.end())
		{
			istringstream iss(headermap["Content-Length"]);
			size_t contentLength;
			if (!(iss >> contentLength))
			{
				GetRequest requestObject;

				cerr << "Invalid Content-Length header" << endl;
				string errorPage = config->getErrorPageContent(const_cast<ConfigParser &>(*config), serverUid, 400);
				// if (requestObject.sendHTTPResponse(fd, 400, errorPage, "text/html") == -1)
				// 	cerr << "Failed to send 400 response" << endl;
				server.clearClientBuffer(fd);
				return -1;
			}

			// Check against max body size
			if (_maxBodySize > 0 && contentLength > static_cast<size_t>(_maxBodySize))
			{
				GetRequest requestObject;

				cerr << "Request body too large: " << contentLength << " > " << _maxBodySize << endl;
				string errorPage = config->getErrorPageContent(const_cast<ConfigParser &>(*config), serverUid, 413);
				// if (requestObject.sendHTTPResponse(fd, 413, errorPage, "text/html") == -1)
				// 	cerr << "Failed to send 413 response" << endl;
				server.clearClientBuffer(fd);
				return -1;
			}
			// Read remaining body if needed
			// if (readbody == 0)
			// {
			// 	while (true)
			// 	{
			// 		received = recv(fd, buff, BUFFER_SIZE, 0);
			// 		if (received <= 0)
			// 			break; // TODO - Error or connection closed ?
			// 	}
			// 	return (-1);
			// }
			if (body.size() < contentLength)
			{
				memset(buff, 0, BUFFER_SIZE);
				received = recv(fd, buff, min(BUFFER_SIZE, contentLength), 0);
				if (received <= 0)
				{
					server.clearClientBuffer(fd);
					return -1;
				}
				body.append(buff, received);
				savestring.append(body);
				server.fillClientBuffer(fd, savestring);
				return (0);
			}
			return (1);
		}
	}
}

int RequestHandler::handleRequest(int fd, Server &server, ConfigParser *config)
{
	int readbody = 1;
	size_t const MAX_HEADER_SIZE = 8192; // 8KB for headers
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
	
	Logger::access(serverUid, "http request: " + server.getClientBuffer(fd));

	try
	{
		headermap = parseHeader(header);

		server.setEnvValue("SERVER_NAME", headermap["Host"].substr(0, headermap["Host"].find(':')));
		if (headermap["Host"].find(':'))
			server.setEnvValue("SERVER_PORT", headermap["Host"].substr(headermap["Host"].find(':') + 1));
		else
			server.setEnvValue("SERVER_PORT", "80");
		string max_body_size = config->getServerValue(serverUid, "client_max_body_size");
		if (max_body_size.empty())
			max_body_size = config->getValue("client_max_body_size");
		setMaxBodySize(max_body_size);
		serverRoot = config->getServerValue(serverUid, "root");
		// Check if we need to read more body data
		// if (headermap.find("Transfer-Encoding") != headermap.end() && headermap["Transfer-Encoding"].find("chuncked"))
		// {
		// 	body = handleChunckedRequest(fd, body);
		// 	if (body.empty()) {
		// 		cout << "empty body\n";
		// 		return (-1);
		// 	}
		// }
		
		// get the index full path
		if (serverRoot[serverRoot.length() - 1] == '/')
			serverRoot = serverRoot.substr(0, serverRoot.length() - 1);
		string fullPath = serverRoot + headermap["path"];

		server.setEnvValue("REQUEST_METHOD", headermap["method"]);
		server.setEnvValue("REQUEST_URI", headermap["path"]);
		if (headermap["path"].find('?') != string::npos)
			server.setEnvValue("QUERY_STRING", headermap["path"].substr(headermap["path"].find('?') + 1));
		else
			server.setEnvValue("QUERY_STRING", "");

		// Checks if the path is allowed by the location for the requested method
		string cleanPath = headermap["path"].substr(0, headermap["path"].find('?'));
		string auto_index = config->getLocationValueForPath(cleanPath, server.getUid(), "autoindex");

		try
		{
			int status = 0;
			string allowed_methods = config->getLocationValueForPath(cleanPath, server.getUid(), "allow_methods");
			istringstream iss(allowed_methods);
			string one_method;

			while (iss >> one_method) {
				if (one_method == headermap["method"]) {
					status = 1;
					break;
				}
			}
			// Case no methods allowed for the location
			if (status == 0)
			{
				GetRequest requestObject;

				cout << "EARLY 403" << endl;
				string errorPage = requestObject.loadErrorPage(403, config, serverUid);
				if (requestObject.sendHTTPResponse(fd, 403, errorPage, "text/html") == -1)
					cerr << "Failed to send 403 response" << endl;
				if (!requestObject.isKeepalive())
					return (-1);
				return 0;
			}
		}
		catch (exception const &e)
		{
			GetRequest requestObject;

			string errorPage = requestObject.loadErrorPage(403, config, serverUid);
			if (requestObject.sendHTTPResponse(fd, 403, errorPage, "text/html") == -1)
				cerr << "Failed to send 403 response" << endl;
			if (!requestObject.isKeepalive())
				return (-1);
			return 0;
		}
		string redirect = config->getLocationValueForPath(cleanPath, server.getUid(), "return");
		if (!redirect.empty())
		{
			; // redirect here
		}
		if (headermap["method"] == "GET")
		{
			GetRequest requestObject(headermap);
			// Process the GET request and send response

			return (requestObject.handleGet(fd, server, config, fullPath));
		}
		else if (headermap["method"] == "POST")
		{
			PostRequest requestObject(headermap);
			// Process the POST request

			return (requestObject.handlePost(fd, server, body, config));
		}
		else if (headermap["method"] == "DELETE")
		{
			DeleteRequest requestObject(headermap);
			// Process the DELETE request

			return (requestObject.handleDelete(fd, server, config, fullPath));
		}
		else
		{
			Logger::error(serverUid, "Unknown method in HEADER: " + headermap["method"]);
			return (-1);
		}
	}
	catch (const exception &e)
	{
		cerr << "Error parsing request: " << e.what() << endl;
		return (-1);
	}
	return (0);
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
		// cout << "Body size = " << value << endl; TODO - Convert the figure in MB and set the value to the attribute
	}
	else
		_maxBodySize = MAX_BODY_SIZE; // Default 1MB if invalid
}

string RequestHandler::handleChunckedRequest(int fd, const string &body)
{
	string fullBody;

	string tmp = "";
	char buff[4096];
	int received;
	memset(buff, 0, 4096);

	if (!body.empty())
		tmp.append(body);
	while (true)
	{
		if (tmp.find('\r') != string::npos)
		{
			cout << "tmp : '" << tmp << "'" << endl;
			string chunk;
			size_t chunkSize;

			size_t br = tmp.find('\r');
			if (tmp.size() == br)
			{
				memset(buff, 0, 4096);
				received = recv(fd, buff, 1, 0);
				if (received <= 0)
					return (cout << "error 1\n", ""); // error
				tmp.append(buff);
			}

			istringstream iss(tmp.substr(0, tmp.find("\r\n")));
			if (!(iss >> hex >> chunkSize))
			{
				return (cout << "error 2\n", ""); // error
			}

			chunk.append(tmp.substr(tmp.find("\r\n") + 2));
			cout << "chunksize: " << chunkSize << endl;
			if (chunk.size() > chunkSize)
			{
				tmp = chunk.substr(chunkSize + 2);
				chunk = chunk.substr(0, chunkSize);
			}
			else
			{
				tmp = "";
			}
			if (chunkSize == 0)
			{
				break;
			}
			while (chunk.size() < chunkSize)
			{
				memset(buff, 0, 4096);
				cout << "chunk: '" << chunk << "'" << endl;
				int toRead = chunkSize - chunk.size();
				received = recv(fd, buff, min(4095, toRead), 0);
				if (received <= 0)
					return (cout << "error 3\n", ""); // error
				chunk.append(buff);
				cout << "chunk after: '" << chunk << "'" << endl;
			}
			cout << "chunk: '" << chunk << "'" << endl;
			if (tmp.empty())
			{
				received = recv(fd, buff, 2, 0);
				if (received <= 0)
					return (cout << "error 4\n", ""); // error
				if (buff[0] != '\r' || buff[1] != '\n')
					return (cout << "error 5 : \n"
								 << buff << endl,
							""); // error
				if (chunkSize == 0)
					break;
			}
			fullBody.append(chunk);
		}
		else
		{
			memset(buff, 0, 4096);
			received = recv(fd, buff, 10, 0);
			cout << "received: " << received << endl;
			if (received < 3)
				buff[received] = '\0';
			if (received <= 0)
			{

				return (cout << "error 6\n", ""); // error
			}
			tmp.append(buff);
		}
	}
	cout << "body : " << fullBody << endl;
	return (fullBody);
}
