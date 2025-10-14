#include "GetRequest.hpp"

GetRequest::GetRequest()
{
	return ;
}

GetRequest::GetRequest(map<string, string> header)
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

int	GetRequest::handleGet(int fd, const Server &server, const ConfigParser *config, const string &fullPath)
{
	string		decodedUrl = urlDecode(fullPath.c_str());
	ifstream file(decodedUrl.c_str());
	DIR *dir = opendir(decodedUrl.c_str()); 

	if (!file.is_open() && dir == NULL) {
		// File not found - send 404 error
		string errorPage = loadErrorPage(404, config, server.getUid());
		if (sendHTTPResponse(fd, 404, errorPage, "text/html") == -1)
			cerr << "Failed to send 404 response" << endl;
	} else {
		file.close();
		if (dir) closedir(dir);
		
		// Check if it's a CGI script (ends with .php, .py, etc.)
		string contentType = getContentType(decodedUrl);
		cout << CYAN << BOLD << "File requested: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;
		// Handle as CGI
		if (sendCGIResponse(fd, decodedUrl, config, server) == -1)
			cerr << "Failed to send CGI response" << endl;
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



