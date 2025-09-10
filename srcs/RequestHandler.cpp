#include "RequestHandler.hpp"
#include "ConfigParser.hpp"

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

int	RequestHandler::handleRequest(int fd, const std::string& serverRoot, ConfigParser* config, const std::string& serverId) const
{
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
			Logger::error("", "Header too large");
			return -1;
		}
		
		received = recv(fd, buff, BUFFER_SIZE, 0);
		if (received <= 0)
			break;
	}
	
	// Extract body if present
	body = header.substr(headerlimit + 4, std::string::npos);
	header.erase(headerlimit, std::string::npos);
	
	Logger::access("", "http request: " + header);
	
	try {
		headermap = parseHeader(header);
		
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
		
		if (headermap["method"] == "GET")
		{
			GetRequest requestObject(headermap);
			// Process the GET request and send response
			std::string fullPath = serverRoot + headermap["path"];
			std::string indexFile = config->getServerValue(serverId, "index");

			std::cout << "Index file: " << indexFile << std::endl; // TODO - Implement the index searching logic
			if (headermap["path"] == "/")
				fullPath = serverRoot + "/index.php";
			
			if (requestObject.sendCGIResponse(fd, fullPath, config, serverId) == -1)
				std::cerr << "Failed to send GET response" << std::endl;
		}
		else if (headermap["method"] == "POST")
		{
			PostRequest requestObject(headermap);
			// Process the POST request
			std::string fullPath = serverRoot + headermap["path"];
			if (headermap["path"] == "/")
				fullPath = serverRoot + "/index.php";
				
			if (requestObject.sendCGIResponse(fd, fullPath, config, serverId) == -1)
				std::cerr << "Failed to send POST response" << std::endl;
		}
		else if (headermap["method"] == "DELETE")
		{
			DeleteRequest requestObject(headermap);
			// Process the DELETE request
			std::string response = "<html><body><h1>DELETE Method</h1><p>Resource deletion not implemented</p></body></html>";
			if (requestObject.sendHTTPResponse(fd, 200, response) == -1)
				std::cerr << "Failed to send DELETE response" << std::endl;
		}
		else
		{
			Logger::error("", "Unknown method in HEADER: " + headermap["method"]); 
			return (-1);
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error parsing request: " << e.what() << std::endl;
		return (-1);
	}
	
	std::cout << std::string(GREEN) << "Request handled succesfully" << std::string(NEUTRAL) << std::endl;
	return (0);
}

int	RequestHandler::printRequest(int fd) const
{
	char buff[4096];
	std::string request;
	int			received;

	received = recv(fd, buff, 4096, 0);
	if (received <= 0)
	{
		return (-1);
	}
	request = buff;
	std::cout << "http request : " << request << std::endl;
	char buf[78] = "HTTP/1.1 200 OK\r\nContent-Length: 6\r\nContent-Type: text/plain\r\n\r\ncoucou";
	if (send(fd, buf, strlen(buf), 0) == -1)
	{
		std::cerr << "send error : pouet" << std::endl;
		return 1;
	}
	return (0);
}

void	RequestHandler::setMaxBodySize(std::string size)
{
	std::istringstream iss(size);
	int value;
	if (iss >> value && value >= 0)
		_maxBodySize = value;
	else
		_maxBodySize = 1048576; // Default 1MB if invalid 
}

