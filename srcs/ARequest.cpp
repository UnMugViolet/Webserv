#include "ARequest.hpp"
#include "CGI.hpp"
#include <string.h>
#include <sstream>
#include "ConfigParser.hpp"

ARequest::ARequest()
{
	return ;
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

int ARequest::sendHTTPResponse(int clientFd, int statusCode, const std::string& body, const std::string& contentType)
{
	std::ostringstream response;
	std::string statusText;
	
	// Set status text based on code
	switch (statusCode) {
		case 200: statusText = "OK"; break;
		case 404: statusText = "Not Found"; break;
		case 500: statusText = "Internal Server Error"; break;
		case 403: statusText = "Forbidden"; break;
		case 415: statusText = "Unsupported Media Type"; break;
		default: statusText = "Unknown"; break;
	}
	
	// Build HTTP response
	response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
	response << "Content-Type: " << contentType << "\r\n";
	response << "Content-Length: " << body.length() << "\r\n";
	if (!_keep_alive)
		response << "Connection: close\r\n";
	response << "\r\n";
	response << body;
	
	std::string responseStr = response.str();
	if (send(clientFd, responseStr.c_str(), responseStr.length(), 0) == -1) {
		std::cerr << "Failed to send HTTP response" << std::endl;
		return -1;
	}
	return 0;
}

int ARequest::sendCGIResponse(int clientFd, const std::string &scriptPath, ConfigParser *config, const std::string &serverUid)
{
	int cgiOutputFd = -1;
	try {
		// Execute CGI script
		cgiOutputFd = CGI::interpret(scriptPath, serverUid);
		
		// Read the CGI output
		std::string cgiOutput;
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
		std::string contentType = getContentType(scriptPath);
		return sendHTTPResponse(clientFd, 200, cgiOutput, contentType);
		
	} catch (const CGI::CGIException &e) {
		// Close the file descriptor if it was opened
		if (cgiOutputFd != -1) {
			close(cgiOutputFd);
		}
		
		// Handle true CGI execution errors (file not found, permission denied, etc.)
		// These are cases where the script couldn't even run
		std::string errorPage = loadErrorPage(e.getHttpStatus(), config, serverUid);
		return sendHTTPResponse(clientFd, e.getHttpStatus(), errorPage, "text/html");
	} catch (...) {
		// Handle any other exceptions
		if (cgiOutputFd != -1) {
			close(cgiOutputFd);
		}
		
		std::string errorPage = loadErrorPage(500, config, serverUid);
		return sendHTTPResponse(clientFd, 500, errorPage, "text/html");
	}
}

std::string ARequest::loadErrorPage(int statusCode, const ConfigParser *config, const std::string &serverUid) const
{
	return config->getErrorPageContent(const_cast<ConfigParser&>(*config), serverUid, statusCode);
}

std::string ARequest::getContentType(const std::string& filePath) const
{
	size_t pos = filePath.rfind('.');
	if (pos == std::string::npos)
		return "text/html";
	
	std::string ext = filePath.substr(pos + 1);
	
	if (ext == "html" || ext == "htm")
		return "text/html";
	else if (ext == "css")
		return "text/css";
	else if (ext == "png")
		return "image/png";
	else if (ext == "jpg" || ext == "jpeg")
		return "image/jpeg";
	else if (ext == "gif")
		return "image/gif";
	else if (ext == "js")
		return "application/javascript";
	else if (ext == "json")
		return "application/json";
	else if (ext == "txt")
		return "text/plain";
	else
		return "text/html";
}

