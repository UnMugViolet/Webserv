#include "PostRequest.hpp"

PostRequest::PostRequest(std::map<std::string, std::string> header)
{
	_path = header["path"];
	_method = POST;
	_keep_alive = true;

	if (header["Connection"] == "close")
		_keep_alive = false;
	_client = header["User-agent"];
	
	_host = header["Host"];

	return ;
}

PostRequest::PostRequest(PostRequest& src)
{
	/*copy what needs to be here*/
	return ;
}

PostRequest::~PostRequest()
{
	return ;
}

PostRequest&	PostRequest::operator=(PostRequest& src)
{
	/*copy what needs to be here*/
	return (*this);
}

