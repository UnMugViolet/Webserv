#include "RequestHandler.hpp"
#include "ConfigParser.hpp"
#include "Server.hpp"

RequestHandler::RequestHandler()
{
	_maxBodySize = 0;
	return ;
}

RequestHandler::RequestHandler(RequestHandler& src)
{
	/*copy what needs to be here*/
	this->_maxBodySize = src._maxBodySize;
	return ;
}

RequestHandler&	RequestHandler::operator=(RequestHandler &src)
{
	this->_maxBodySize = src._maxBodySize;
	return (*this);
}

RequestHandler::~RequestHandler()
{
	return ;
}

std::string urlDecode(const std::string &src) {
    std::ostringstream out;
    for (size_t i = 0; i < src.length(); ++i) {
        if (src[i] == '%' && i + 2 < src.length()) {
            std::istringstream iss(src.substr(i + 1, 2));
            int hex = 0;
            if (iss >> std::hex >> hex)
                out << static_cast<char>(hex);
            i += 2;
        } else if (src[i] == '+') {
            out << ' ';
        } else {
            out << src[i];
        }
    }
    return out.str();
}

int RequestHandler::_checkAccess(const std::string &path)
{
	if (access(path.c_str(), F_OK) == -1)
		return (-1);
	if (getExtension(path) == "cgi" && access(path.c_str(), X_OK) == -1)
		return (0);
	if (access(path.c_str(), R_OK) == -1)
		return (0);
	return (1);
	
}

std::string	RequestHandler::getExtension(const std::string &path)
{
	size_t pos = path.rfind('.');
	if (pos == std::string::npos)
		return ("");
	return (path.substr(pos + 1));
}

std::string RequestHandler::getIndex(const std::string &indexes, const std::string &root) const
{
	std::string	fullPath;
	std::string	goodIndex;
	size_t	space1;
	size_t	space2 = 0;

	while (true)
	{
		space1 = indexes.find_first_not_of(" ", space2);
		if (space1 == std::string::npos)
			break ;
		space2 = indexes.find(' ', space1);
		goodIndex = indexes.substr(space1, space2);
		if (goodIndex[0] != '/')
			goodIndex = "/" + goodIndex;
		fullPath = root + goodIndex;
		if (_checkAccess(fullPath) == 1) {
			std::cout << "Index found: " << fullPath << std::endl;
			return (goodIndex);
		}
	}
	return ("");
}

std::string	trim(const std::string &str)
{
	size_t first = str.find_first_not_of(" \r\n\t");
	if (first == std::string::npos)
		return ("");
	size_t last = str.find_last_not_of(" \n\r\t");
	return (str.substr(first, last - first + 1));
}

std::map<std::string, std::string>	RequestHandler::parseHeader(std::string header) const
{
	std::map<std::string, std::string> headers;
	std::istringstream	stream(header);
	std::string			line;
	std::string			method;
	std::string			path;
	size_t	colon;
	
	std::getline(stream, line);
	std::istringstream requestLine(line);
	
	// Parse the request line: METHOD PATH HTTP/VERSION
	requestLine >> method >> path;
	headers["path"] = path;
	headers["method"] = method;
	
	while (std::getline(stream, line) && line != "\r" && !line.empty())
	{
		colon = line.find(':');
		if (colon != std::string::npos)
		{
			std::string key = line.substr(0, colon);
			if (key.find(' ') != std::string::npos)
				throw std::runtime_error("Bad Request: Invalid header key");
			std::string value = trim(line.substr(colon + 1));
			if (value.empty())
				throw std::runtime_error("Bad Request: Empty header value");
			headers[key] = value;
		}
	}
	return (headers);
}

int	RequestHandler::handleRequest(int fd, Server &server, ConfigParser *config)
{
	int									readbody = 1;
	int									received;
	size_t const 						BUFFER_SIZE = 1;
	size_t const 						MAX_HEADER_SIZE = 8192; // 8KB for headers
	size_t 								headerlimit;
	char 								buff[BUFFER_SIZE];
	std::string							serverRoot;
	std::string 						serverUid = server.getUid();
	std::string 						body;
	std::string 						header;
	std::vector<std::string> 			serverNames = server.getServerNames();
	std::map<std::string, std::string> 	headermap;

	// Read initial chunk
	received = recv(fd, buff, BUFFER_SIZE, 0);
	if (received <= 0)
		return -1;
	
	// Read the complete header
	while (received > 0)
	{
		header.append(buff, received);
		headerlimit = header.find("\r\n\r\n");
		if (headerlimit != std::string::npos)
			break ;
		
		// Prevent header from being too large
		if (header.size() > MAX_HEADER_SIZE)
		{
			Logger::error(serverUid, "Header too large");
			return -1;
		}
		
		received = recv(fd, buff, BUFFER_SIZE, 0);
		if (received <= 0)
			break;
	}
	
	// Extract body if present
	if (headerlimit == std::string::npos)
	{
		std::cout << header << std::endl;
	}
	body = header.substr(headerlimit + 4, std::string::npos);
	if (headerlimit != std::string::npos)
		header.erase(headerlimit, std::string::npos);
	
	Logger::access(serverUid, "http request: " + header);
	
	try {
		headermap = parseHeader(header);
		
		// get virtual server root
		if (headermap.find("Host") == headermap.end())
		{
			GetRequest requestObject;

			std::cerr << "No server_name, bad request" << std::endl;
			std::string errorPage = config->getErrorPageContent(const_cast<ConfigParser&>(*config), serverUid, 400);
			if (requestObject.sendHTTPResponse(fd, 400, errorPage, "text/html") == -1)
				std::cerr << "Failed to send 400 response" << std::endl;
			readbody = 0;
		}
		server.setEnvValue("SERVER_NAME", headermap["Host"].substr(0, headermap["Host"].find(':')));
		if (headermap["Host"].find(':'))
			server.setEnvValue("SERVER_PORT", headermap["Host"].substr(headermap["Host"].find(':') + 1));
		else
			server.setEnvValue("SERVER_PORT", "80");
		std::string max_body_size = config->getServerValue(serverUid, "client_max_body_size");
		if (max_body_size.empty())
			max_body_size = config->getValue("client_max_body_size");
		setMaxBodySize(max_body_size);
		serverRoot = config->getServerValue(serverUid, "root");
		// Check if we need to read more body data
		if (headermap.find("Transfer-Encoding") != headermap.end() && headermap["Transfer-Encoding"].find("chuncked"))
		{
			body = handleChunckedRequest(fd, body);
			if (body.empty()) {
				std::cout << "empty body\n";
				return (-1);
			}
		}
		if (headermap.find("Content-Length") != headermap.end())
		{
			std::istringstream iss(headermap["Content-Length"]);
			size_t contentLength;
			if (!(iss >> contentLength)) {
				GetRequest requestObject;

				std::cerr << "Invalid Content-Length header" << std::endl;
				std::string errorPage = config->getErrorPageContent(const_cast<ConfigParser&>(*config), serverUid, 400);
				if (requestObject.sendHTTPResponse(fd, 400, errorPage, "text/html") == -1)
					std::cerr << "Failed to send 400 response" << std::endl;
				readbody = 0;
			}
			
			// Check against max body size
			if (readbody && _maxBodySize > 0 && contentLength > static_cast<size_t>(_maxBodySize))
			{
				GetRequest requestObject;

				std::cerr << "Request body too large: " << contentLength << " > " << _maxBodySize << std::endl;
				std::string errorPage = config->getErrorPageContent(const_cast<ConfigParser&>(*config), serverUid, 413);
				if (requestObject.sendHTTPResponse(fd, 413, errorPage, "text/html") == -1)
					std::cerr << "Failed to send 413 response" << std::endl;
				readbody = 0;
			}
			// Read remaining body if needed
			if (readbody == 0)
			{
				while (true)
				{
					received = recv(fd, buff, BUFFER_SIZE, 0);
					if (received <= 0)
					{
						std::cout << " miaou\n";
						break;
					}
				}
				return (-1);
			}
			while (body.size() < contentLength)
			{
				received = recv(fd, buff, BUFFER_SIZE, 0);
				if (received <= 0)
				{
					std::cout << " miaou\n";
					break;
				}
				body.append(buff, received);
			}
			
		}

		// get the index full path
		if (serverRoot[serverRoot.length() - 1] == '/')
			serverRoot = serverRoot.substr(0, serverRoot.length() -1);
		std::string fullPath = serverRoot + headermap["path"];
		std::string indexFile = getIndex(config->getServerValue(serverUid, "index"), serverRoot);

		if (headermap["path"] == "/")
			fullPath = serverRoot + indexFile;

		server.setEnvValue("REQUEST_METHOD", headermap["method"]);
		server.setEnvValue("REQUEST_URI", headermap["path"]);
		if (headermap["path"].find('?') != std::string::npos)
			server.setEnvValue("QUERY_STRING", headermap["path"].substr(headermap["path"].find('?') + 1));
		else
			server.setEnvValue("QUERY_STRING", "");
		
		// Checks if the path is allowed by the location for the requested method
		std::string currentLocation = "";
		std::string cleanPath = headermap["path"].substr(0, headermap["path"].find('?'));
		std::vector<std::string> temp = config->getLocationPaths(server.getUid());
		int status = 0;
		std::vector<std::string>::iterator it = temp.begin();
		for (; it != temp.end(); it++) {
			if (cleanPath.find((*it)) != std::string::npos) {
				status = 1;
				if (currentLocation.size() < (*it).size())
					currentLocation = *it;
			}
		}
		if (status == 0) {
			GetRequest requestObject;

			std::string errorPage = requestObject.loadErrorPage(403, config, serverUid);
			std::cout << "HERE\n";
			if (requestObject.sendHTTPResponse(fd, 403, errorPage, "text/html") == -1)
				std::cerr << "Failed to send 403 response" << std::endl;
			if (!requestObject.isKeepalive())
				return (-1);
			return 0;
		}
		status = 0;
		std::string allowed_methods = config->getLocationValue(serverUid, currentLocation, "allow_methods");
		if (allowed_methods.find(headermap["method"]) != std::string::npos) {
			std::cout << "Location: " << currentLocation << std::endl;
			std::cout << "Value: " << allowed_methods << std::endl;
			for(size_t i = allowed_methods.find(' '); i != std::string::npos; i = allowed_methods.find(' ', i))
			{
				std::string one_method = allowed_methods.substr(0, i);
				if (one_method == headermap["method"])
				{
					status = 1;
					break ;
				}
				allowed_methods = allowed_methods.substr(i + 1);
			}
			if (allowed_methods == headermap["method"])
				status = 1;
		}
		if (status == 0) {
			GetRequest requestObject;

			std::string errorPage = requestObject.loadErrorPage(403, config, serverUid);
			if (requestObject.sendHTTPResponse(fd, 403, errorPage, "text/html") == -1)
				std::cerr << "Failed to send 403 response" << std::endl;
			if (!requestObject.isKeepalive())
				return (-1);
			return 0;
		}
		if (headermap["method"] == "GET")
		{
			GetRequest requestObject(headermap);
			// Process the GET request and send response

			/*
			 * 1. Check if the file exists and is accessible
			 * 2. If it's a CGI script, handle it accordingly
			 * 3. Otherwise, serve the static file and fetch taking into account the spaces and special characters
			 * 4. If the file doesn't exist, send a 404 error page 
			*/
			std::string decodedPath = urlDecode(fullPath);
			std::ifstream file(decodedPath.c_str());
			DIR *dir = opendir(fullPath.c_str()); 
				
			if (!file.is_open() && dir == NULL) {
				// File not found - send 404 error
				std::string errorPage = requestObject.loadErrorPage(404, config, serverUid);
				if (requestObject.sendHTTPResponse(fd, 404, errorPage, "text/html") == -1)
					std::cerr << "Failed to send 404 response" << std::endl;
			} else {
				file.close();
				if (dir) closedir(dir);
				
				// Check if it's a CGI script (ends with .php, .py, etc.)
				std::string contentType = requestObject.getContentType(decodedPath);
				std::cout << CYAN << BOLD << "File requested: " << NEUTRAL << CYAN << decodedPath << NEUTRAL << std::endl;
				// Handle as CGI
				if (requestObject.sendCGIResponse(fd, decodedPath, config, server) == -1)
					std::cerr << "Failed to send CGI response" << std::endl;
			}
			if (!requestObject.isKeepalive())
				return (-1);
		}
		else if (headermap["method"] == "POST")
		{
			PostRequest requestObject(headermap);
			// Process the POST request
			std::string path = serverRoot;

			// define target directory
			path += "/var/uploads";
			if (access(path.c_str(), X_OK) == -1)
			{
				std::cerr << "no uploads directory" << std::endl;//what? no appropriate directory or no permission
				std::string errorPage = requestObject.loadErrorPage(500, config, serverUid);
				if (requestObject.sendHTTPResponse(fd, 500, errorPage, "text/html") == -1)
					std::cerr << "Failed to send 500 response" << std::endl;
				return (-1);
			}
			path = serverRoot + "/var/posts";
			if (access(path.c_str(), X_OK) == -1)
			{
				std::cerr << "no posts directory" << std::endl;//what? no appropriate directory or no permission
				std::string errorPage = requestObject.loadErrorPage(500, config, serverUid);
				if (requestObject.sendHTTPResponse(fd, 500, errorPage, "text/html") == -1)
					std::cerr << "Failed to send 500 response" << std::endl;
				return (-1);
			}
			else
			{
				int res = requestObject.HandlePost(fd, body, serverRoot);
				if (res == -1)
				{
					std::string errorPage = requestObject.loadErrorPage(500, config, serverUid);
					if (requestObject.sendHTTPResponse(fd, 500, errorPage, "text/html") == -1)
						std::cerr << "Failed to send 500 response" << std::endl;
				}
			}
			if (!requestObject.isKeepalive())
				return (-1);
		}
		else if (headermap["method"] == "DELETE")
		{
			DeleteRequest requestObject(headermap);
			// Process the DELETE request

			if (access(fullPath.c_str(), F_OK))
				requestObject.delete_file(fd, server);
			else
			{
				std::string errorPage = requestObject.loadErrorPage(404, config, serverUid);
				if (requestObject.sendHTTPResponse(fd, 404, errorPage, "text/html") == -1)
					std::cerr << "Failed to send 404 response" << std::endl;
			}
			if (!requestObject.isKeepalive())
			{
				std::cout << "there?\n";
				return (-1);
			}
		}
		else
		{
			Logger::error(serverUid, "Unknown method in HEADER: " + headermap["method"]); 
			return (-1);
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error parsing request: " << e.what() << std::endl;
		return (-1);
	}
	return (0);
}

void	RequestHandler::setMaxBodySize(std::string size)
{
	std::istringstream iss(size);
	int value;
	std::string unit;

	if (iss >> value && value >= 0) {
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
				std::cerr << "unsupported unit" << std::endl;
				_maxBodySize = 0;
			}
		}
		// std::cout << "Body size = " << value << std::endl; TODO - Convert the figure in MB and set the value to the attribute
	}
	else
		_maxBodySize = MAX_BODY_SIZE; // Default 1MB if invalid
}

std::string	RequestHandler::handleChunckedRequest(int fd, const std::string &body)
{
	std::string	fullBody;
	
	std::string	tmp = "";
	char		buff[4096];
	int			received;
	memset(buff, 0, 4096);

	if (!body.empty())
		tmp.append(body);
	while (true)
	{
		if (tmp.find('\r') != std::string::npos)
		{
			std::cout << "tmp : '" << tmp << "'" << std::endl;
			std::string	chunk;
			size_t	chunkSize;

			size_t br = tmp.find('\r');
			if (tmp.size() == br)
			{
				memset(buff, 0, 4096);
				received = recv(fd, buff, 1, 0);
				if (received <= 0)
					return (std::cout << "error 1\n", "");//error 
				tmp.append(buff);
			}
			
			std::istringstream iss(tmp.substr(0, tmp.find("\r\n")));
			if (!(iss >> std::hex >> chunkSize))
			{
				return (std::cout << "error 2\n", "");//error 
			}
			
			chunk.append(tmp.substr(tmp.find("\r\n") + 2));
			std::cout << "chunksize: " << chunkSize << std::endl;
			if (chunk.size() > chunkSize)
			{
				tmp = chunk.substr(chunkSize + 2);
				chunk = chunk.substr(0, chunkSize);

			} else {
				tmp = "";
			}
			if (chunkSize == 0)
			{
				break;
			}
			while (chunk.size() < chunkSize)
			{
				memset(buff, 0, 4096);
				std::cout << "chunk: '" << chunk << "'" << std::endl;
				int toRead = chunkSize - chunk.size();
				received = recv(fd, buff, std::min(4095, toRead), 0);
				if (received <= 0)
					return (std::cout << "error 3\n", "");//error 
				chunk.append(buff);
				std::cout << "chunk after: '" << chunk << "'" << std::endl;
			}
			std::cout << "chunk: '" << chunk << "'" << std::endl;
			if (tmp.empty())
			{
				received = recv(fd, buff, 2, 0);
				if (received <= 0)
					return (std::cout << "error 4\n", "");//error 
				if (buff[0] != '\r' || buff[1] != '\n')
					return (std::cout << "error 5 : \n" << buff << std::endl, "");//error 
				if (chunkSize == 0)
					break ;
			}
			fullBody.append(chunk);
		} else {
			memset(buff, 0, 4096);
			received = recv(fd, buff, 10, 0);
			std::cout << "received: " << received << std::endl;
			if (received < 3)
				buff[received] = '\0';
			if (received <= 0)
			{
				
				return (std::cout << "error 6\n", "");//error
			}
			tmp.append(buff);
		}
	}
	std::cout << "body : " << fullBody << std::endl;
	return (fullBody);
}
