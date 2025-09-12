#include "ARequest.hpp"
#include "CGI.hpp"
#include <string.h>
#include <sstream>
#include <vector>
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

int	ARequest::isKeepalive() const
{
	return (_keep_alive);
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

int ARequest::sendStaticFileResponse(int clientFd, const std::string &filePath)
{
	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file.is_open()) {
		return -1; // File not found
	}

	// Get file size
	file.seekg(0, std::ios::end);
	size_t fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	// Read file content
	std::vector<char> buffer(fileSize);
	file.read(buffer.data(), fileSize);
	file.close();

	// Get content type
	std::string contentType = getContentType(filePath);

	// Send HTTP headers
	std::ostringstream response;
	response << "HTTP/1.1 200 OK\r\n";
	response << "Content-Type: " << contentType << "\r\n";
	response << "Content-Length: " << fileSize << "\r\n";
	if (!_keep_alive)
		response << "Connection: close\r\n";
	response << "\r\n";

	// Send headers first
	std::string headers = response.str();
	if (send(clientFd, headers.c_str(), headers.length(), 0) == -1) {
		std::cerr << "Failed to send HTTP headers" << std::endl;
		return -1;
	}

	// Send file content
	if (send(clientFd, buffer.data(), buffer.size(), 0) == -1) {
		std::cerr << "Failed to send file content" << std::endl;
		return -1;
	}

	return 0;
}

std::string ARequest::loadErrorPage(int statusCode, const ConfigParser *config, const std::string &serverUid) const
{
	return config->getErrorPageContent(const_cast<ConfigParser&>(*config), serverUid, statusCode);
}

std::string ARequest::getContentType(const std::string &filePath) const
{
	size_t pos = filePath.rfind('.');
	if (pos == std::string::npos)
		return "application/octet-stream";
	
	std::string ext = filePath.substr(pos + 1);
	
	if (ext == "html" || ext == "htm" || ext == "php" || ext == "py")
		return "text/html";
	else if (ext == "css")
		return "text/css";
	else if (ext == "js")
		return "application/javascript";
	else if (ext == "png")
		return "image/png";
	else if (ext == "ico")
		return "image/x-icon";
	else if (ext == "jpg" || ext == "jpeg")
		return "image/jpeg";
	else if (ext == "mp3")
		return "audio/mpeg";
	else if (ext == "wav")
		return "audio/wav";
	else if (ext == "ogg")
		return "audio/ogg";
	else if (ext == "gif")
		return "image/gif";
	else if (ext == "json")
		return "application/json";
	else if (ext == "txt")
		return "text/plain";
	else if (ext == "pdf")
		return "application/pdf";
	else
		return "application/octet-stream";
}

