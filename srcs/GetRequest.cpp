#include "GetRequest.hpp"

GetRequest::GetRequest()
{
	return ;
}

GetRequest::GetRequest(std::map<std::string, std::string> header)
{
	_path = header["path"];
	_method = GET;
	_keep_alive = true;

	if (header.find("Connection") != header.end())
		if (header["Connection"] == "close")
			_keep_alive = false;
	_client = header["User-agent"];
	_host = header["Host"];
	return ;
}

GetRequest::GetRequest(GetRequest &src) : ARequest(src)
{
	return ;
}

int	GetRequest::handleGet(int fd, const Server &server, const ConfigParser *config, const std::string &fullPath)
{
	std::ifstream file(fullPath.c_str());
	DIR *dir = opendir(fullPath.c_str()); 

	if (!file.is_open() && dir == NULL) {
		// File not found - send 404 error
		std::string errorPage = loadErrorPage(404, config, server.getUid());
		if (sendHTTPResponse(fd, 404, errorPage, "text/html") == -1)
			std::cerr << "Failed to send 404 response" << std::endl;
	} else {
		file.close();
		if (dir) closedir(dir);
		
		// Check if it's a CGI script (ends with .php, .py, etc.)
		std::string contentType = getContentType(fullPath);
		std::cout << CYAN << BOLD << "File requested: " << NEUTRAL << CYAN << fullPath << NEUTRAL << std::endl;
		// Handle as CGI
		if (sendCGIResponse(fd, fullPath, config, server) == -1)
			std::cerr << "Failed to send CGI response" << std::endl;
	}
	if (!isKeepalive())
		return (-1);
	return (0);
}

GetRequest&	GetRequest::operator=(GetRequest &src)
{
	if (this != &src)
		ARequest::operator=(src);
	return (*this);
}

GetRequest::~GetRequest()
{
	return ;
}



