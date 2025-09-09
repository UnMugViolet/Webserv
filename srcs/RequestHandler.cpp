#include "RequestHandler.hpp"

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

	method = line.substr(0, line.find(' '));
	path = line.substr(line.find(' ') + 1, line.find(' ', line.find(' ') + 1));
	headers["path"] = path;
	headers["method"] = method;
	while (std::getline(stream, line) && line != "\r" && !line.empty())
	{
		colon = line.find(':');
		if (colon != std::string::npos)
		{
			std::string key = line.substr(0, colon);
			if (key.find(' ') != std::string::npos)
				;//error bad request
			std::string value = trim(line.substr(colon + 1));
			if (value.empty())
				;//error bad request
			headers[key] = value;
		}
	}
	return (headers);
}

int	RequestHandler::handleRequest(int fd) const
{
	size_t headerlimit;
	char buff[4096];
	std::string header;
	std::string request;
	int			received;
	std::map<std::string, std::string> headermap;


	received = recv(fd, buff, 4096, 0);
	if (received <= 0)
	{
		return (-1);
	}
	while (received > 0)
	{
		header.append(buff, received);
		headerlimit = header.find("\r\n\r\n");
		if (headerlimit != std::string::npos)
		{
			break ;
		}
		received = recv(fd, buff, 4096, 0);
	}
	Logger::access("", "http request: " + request);
	request = header.substr(headerlimit + 4, std::string::npos);
	header.erase(headerlimit, std::string::npos);
	headermap = parseHeader(header);
	if (headermap["method"] == "GET")
		GetRequest	RequestObject(headermap);
	else if (headermap["method"] == "POST")
		PostRequest	RequestObject(headermap);
	else if (headermap["method"] == "DELETE")
		DeleteRequest	RequestObject(headermap);
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
	(void)size;
}

RequestHandler&	RequestHandler::operator=(RequestHandler& src)
{
	/*copy what needs to be here*/
	this->_maxBodySize = src._maxBodySize;
	return (*this);
}

