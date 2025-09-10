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

PostRequest::PostRequest(PostRequest& src) : ARequest(src)
{
	this->_Content_length = src._Content_length;
	this->_Content_type = src._Content_type;
	this->_body = src._body;
	return ;
}

PostRequest::~PostRequest()
{
	return ;
}

PostRequest&	PostRequest::operator=(PostRequest& src)
{
	if (this != &src)
	{
		ARequest::operator=(src);
		this->_Content_length = src._Content_length;
		this->_Content_type = src._Content_type;
		this->_body = src._body;
	}
	return (*this);
}

