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

bool	PostRequest::UploadFileSuccessfully(std::string body, std::string path)
{
	std::ofstream file(path.c_str());
	if (file.is_open())
	{
		file << body;
		return true;
	}
	else
	{
		throw std::runtime_error("Failed to open file for writing: " + path);
		return false;
	}
}

void	PostRequest::HandlePost(std::string body, std::string path)
{
	std::string 						base;
	std::string 						filename;


	if (_Content_type.compare("text/plain") == 0)
	{
		base = path + "/post_";
		int i = 1;
		while (true)
		{
			std::ostringstream oss;
			oss << base << i << ".txt";
			filename = oss.str();
			if (access(filename.c_str(), F_OK) != 0)
				break;
		} 
		if(UploadFileSuccessfully(body, filename) == false)
			; // Handle error 
		else
			Logger::info("File uploaded successfully to " + filename);
	}
	if (_Content_type.find("multipart/form-data") != std::string::npos)
	{
		std::map<std::string, std::string>	header;
		std::string boundary = _Content_type.substr(_Content_type.find("boundary=") + 1);

		size_t pos = body.find(boundary);
		pos = body.find("\r\n", pos) + 2;
	}
	// manage multipart

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

