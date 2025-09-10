#include "DeleteRequest.hpp"

DeleteRequest::DeleteRequest(std::map<std::string, std::string> header)
{
	_path = header["path"];
	_method = DELETE;
	_keep_alive = true;

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

