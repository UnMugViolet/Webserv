#include "GetRequest.hpp"

GetRequest::GetRequest(std::map<std::string, std::string> header)
{
	_path = header["path"];
	_method = GET;
	_keep_alive = true;

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



