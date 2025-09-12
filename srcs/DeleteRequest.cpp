#include "DeleteRequest.hpp"

DeleteRequest::DeleteRequest(std::map<std::string, std::string> header)
{
	_path = header["path"];
	_method = DELETE;
	_keep_alive = true;

	if (header.find("Connection") != header.end())
		if (header["Connection"] == "close")
			_keep_alive = false;
	_client = header["User-agent"];
	_host = header["Host"];
	return ;
}

DeleteRequest::DeleteRequest(DeleteRequest &src) : ARequest(src)
{
	return ;
}

void	DeleteRequest::delete_file(int fd, const char *path) const
{
	std::ostringstream response;

	
	response << "\r\n";
	if (std::remove(path) == 0)
	{
		response << "HTTP/1.1 204 No Content\r\n\r\n";
		std::string responseStr = response.str();
		if (send(fd, responseStr.c_str(), responseStr.length(), 0) == -1)
		{
			std::cerr << "Failed to send HTTP response" << std::endl;
			return;
		}
	}
	else
		; //why?
}

DeleteRequest::~DeleteRequest()
{
	return ;
}

DeleteRequest	&DeleteRequest::operator=(DeleteRequest &src)
{
	if (this != &src)
		ARequest::operator=(src);
	return (*this);
}

