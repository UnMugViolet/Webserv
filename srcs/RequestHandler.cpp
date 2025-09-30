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
		if (_checkAccess(fullPath) == 1)
			return (goodIndex);
	}
	return ("");
}

std::string	RequestHandler::trim(const std::string &str) const
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

int	RequestHandler::handleRequest(int fd, Server const &server, ConfigParser *config, const std::string &serverUid)
{
	std::string serverRoot;
	const size_t BUFFER_SIZE = 4096;
	const size_t MAX_HEADER_SIZE = 8192; // 8KB for headers
	size_t headerlimit;
	char buff[BUFFER_SIZE];
	std::string header;
	std::string body;
	int			received;
	std::map<std::string, std::string> headermap;

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
	header.erase(headerlimit, std::string::npos);
	
	Logger::access(serverUid, "http request: " + header);
	
	try {
		headermap = parseHeader(header);
		
		// get virtual server root
		if (headermap.find("server_name") == headermap.end())
		{
			std::cerr << "No server_name, bad request" << std::endl;
			return -1;
		}
		std::string server_name = headermap["server_name"];
		std::string serverUid = server.getId(server_name);
		std::string max_body_size = config->getServerValue(serverUid, "client_max_body_size");
		if (max_body_size.empty())
			max_body_size = config->getValue("client_max_body_size");
		setMaxBodySize(max_body_size);
		serverRoot = config->getServerValue(serverUid, "root");
		// Check if we need to read more body data
		if (headermap.find("Content-Length") != headermap.end())
		{
			std::istringstream iss(headermap["Content-Length"]);
			size_t contentLength;
			if (!(iss >> contentLength))
			{
				std::cerr << "Invalid Content-Length header" << std::endl;
				return -1;
			}
			
			// Check against max body size
			if (_maxBodySize > 0 && contentLength > static_cast<size_t>(_maxBodySize))
			{
				std::cerr << "Request body too large: " << contentLength << " > " << _maxBodySize << std::endl;
				return -1;
			}
			
			// Read remaining body if needed
			while (body.size() < contentLength)
			{
				received = recv(fd, buff, BUFFER_SIZE, 0);
				if (received <= 0)
					break;
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

		if (headermap["method"] == "GET")
		{
			GetRequest requestObject(headermap);
			// Process the GET request and send response
			
			
			// Check if file exists
			std::ifstream file(fullPath.c_str());
			if (!file.is_open()) {
				// File not found - send 404 error
				std::string errorPage = requestObject.loadErrorPage(404, config, serverUid);
				if (requestObject.sendHTTPResponse(fd, 404, errorPage, "text/html") == -1)
					std::cerr << "Failed to send 404 response" << std::endl;
			} else {
				file.close();
				
				// Check if it's a CGI script (ends with .php, .py, etc.)
				std::string contentType = requestObject.getContentType(fullPath);
				std::cout << CYAN << BOLD << "File requested: " << NEUTRAL << CYAN << fullPath << NEUTRAL << std::endl;
				// Handle as CGI
				if (requestObject.sendCGIResponse(fd, fullPath, config, serverUid) == -1)
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
			if (headermap["path"] == "/pages/upload.php")
				path += "uploads";
			if (headermap["path"] == "/pages/post.php")
				path += "posts";
			if (access(path.c_str(), X_OK) == -1)
				path = "/tmp";
			std::cout << "Upload path: " << path << std::endl;
			requestObject.HandlePost(body, path);
			if (requestObject.sendCGIResponse(fd, fullPath, config, serverUid) == -1)
				std::cerr << "Failed to send POST response" << std::endl;
			if (!requestObject.isKeepalive())
				return (-1);
		}
		else if (headermap["method"] == "DELETE")
		{
			DeleteRequest requestObject(headermap);
			// Process the DELETE request

			if (access(fullPath.c_str(), F_OK))
				requestObject.delete_file(fd, fullPath.c_str());
			else
			{
				std::string errorPage = requestObject.loadErrorPage(404, config, serverUid);
				if (requestObject.sendHTTPResponse(fd, 404, errorPage, "text/html") == -1)
					std::cerr << "Failed to send 404 response" << std::endl;
			}
			if (!requestObject.isKeepalive())
				return (-1);
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
	if (iss >> value && value >= 0) {
		_maxBodySize = value;
		// std::cout << "Body size = " << value << std::endl; TODO - Convert the figure in MB and set the value to the attribute
	}
	else
		_maxBodySize = MAX_BODY_SIZE; // Default 1MB if invalid
}

