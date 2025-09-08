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
	while (received > 0)
	{
		request.append(buff, received);
		received = recv(fd, buff, 4096, 0);
	}
	std::cout << "http request : " << request << std::endl;
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

