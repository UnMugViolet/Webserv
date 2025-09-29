#include "PostRequest.hpp"

PostRequest::PostRequest(std::map<std::string, std::string> header)
{
	_path = header["path"];
	_method = POST;
	_keep_alive = true;

	if (header.find("Connection") != header.end())
		if (header["Connection"] == "close")
			_keep_alive = false;
	_client = header["User-agent"];
	_Content_type = header["Content-type"];
	_host = header["Host"];

	return ;
}

PostRequest::PostRequest(PostRequest& src) : ARequest(src)
{
	this->_Content_type = src._Content_type;
	this->_body = src._body;
	return ;
}

void	PostRequest::UploadFile(std::string body, std::string path)
{
	
	if (access(path.c_str(), F_OK))
	{
		;//file already exists
	}
	else
	{
		std::ofstream file(path.c_str());
		if (file.is_open())
		{
			;
		}
	}

}

void	PostRequest::HandlePost(std::map<std::string, std::string> header, std::string body, std::string path)
{

	if (_Content_type.compare("text/plain") == 0)
	{
		UploadFile(body, path);
	}
	if (_Content_type.find("multipart/form-data") != std::string::npos)
		;// manage multipart

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
		this->_Content_type = src._Content_type;
		this->_body = src._body;
	}
	return (*this);
}

