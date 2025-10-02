#include "PostRequest.hpp"

PostRequest::PostRequest(std::map<std::string, std::string> header)
{
	_path = header["path"];
	_method = POST;
	_keep_alive = true;

	if (header.find("Connection") != header.end())
		if (header["Connection"] == "close")
			_keep_alive = false;
	_client = header["User-Agent"];
	_Content_type = header["Content-Type"];
	std::cout << "path : " << _path << std::endl;
	_host = header["Host"];

	return ;
}

PostRequest::PostRequest(PostRequest& src) : ARequest(src)
{
	this->_Content_type = src._Content_type;
	this->_body = src._body;
	return ;
}

int	PostRequest::UploadContent(std::map<std::string, std::string> content, std::string path)
{
	std::string filename;

	int i = 0;
	while (true)
	{
		std::ostringstream oss;
		if (i > 0)
			oss << path << "(" << i << ")" << ".txt";
		else
			oss << path << ".txt";
		filename = oss.str();
		if (access(filename.c_str(), F_OK) != 0)
			break;
		i++;
	}
	std::ofstream file(filename.c_str());
	if (file.is_open())
	{
		for (std::map<std::string, std::string>::reverse_iterator it = content.rbegin(); it != content.rend(); it++)
		{
			file << it->first << "=" << it->second << std::endl;
		}
		return (i);
	}
	else
	{
		;//couldn't create file
		return (-1);
	}
}

int	PostRequest::UploadFile(std::string body, std::string path)
{
	std::string base = path;
	std::string filename;
	std::string extension = path.substr(path.rfind('.'));

	base.erase(base.rfind('.'), std::string::npos);
	int i = 0;
	while (true)
	{
		std::ostringstream oss;
		if (i > 0)
			oss << base << "(" << i << ")" << extension;
		else
			oss << base << extension;
		filename = oss.str();
		if (access(filename.c_str(), F_OK) != 0)
			break;
		i++;
	} 
	std::ofstream file(filename.c_str());
	if (file.is_open())
	{
		file << body;
		return (i);
	}
	else
	{
		std::cerr << "couldn't create file for upload" << std::endl;//couldn't create file
		return (-1);
	}
}

int	PostRequest::HandlePost(std::string body, std::string path)
{
	std::string	filename;


	if (_Content_type.compare("text/plain") == 0)
	{
		filename = path + "/post.txt";
		
		if(UploadFile(body, filename) == -1)
			return (-1);
		else
			return (1);
	}
	if (_Content_type.find("multipart/form-data") != std::string::npos)
	{
		std::string bodypart;
		std::map<std::string, std::string>	content;
		std::string boundary = _Content_type.substr(_Content_type.find("boundary=") + 9);

		size_t pos = body.find(boundary);
		pos += boundary.size();
		size_t end = body.find(boundary, pos) - 2;
		std::cout << "body : " << body << std::endl;
		while (true)
		{
			if (body[pos] != '\r')
			{
				break;
			}
			pos = body.find("name=", pos) + 6;
			std::string fieldname = body.substr(pos, body.find("\"", pos) - pos);
			if (body.find("filename=", pos) < end && body.find("filename=", pos) != std::string::npos)
			{
				std::cout << "mais\n";
				pos = body.find("filename=", pos) + 10;
				filename = body.substr(pos, body.find("\"", pos) - pos);
				filename = path + "/" + filename;
				pos = body.find("\r\n\r\n", pos) + 4;
				bodypart = bodypart + body.substr(pos, end - pos);
				int res = UploadFile(bodypart, filename);
				if(res == -1)
					return (-1);
				if (res > 0)
				{
					std::string extension = filename.substr(filename.rfind('.'));
					filename.erase(filename.rfind('.'), std::string::npos);
					filename += "(";
					filename += res;
					filename += ")";
					filename += extension;
				}
				content[fieldname] = filename;
			}
			else
			{
				pos = body.find("\r\n\r\n", pos) + 4;
				bodypart = body.substr(pos, body.find("\r\n", pos) - pos);
				std::cout << "fieldname: " << fieldname << ":" << bodypart << std::endl;
				content[fieldname] = bodypart;
			}
			pos = body.find(boundary, pos);
			pos += boundary.size();
			end = body.find(boundary, pos) - 2;
		}
		if (!content.empty())
		{
			filename = path + "/data";
			if (UploadContent(content, filename) == -1)
				return (-1);
			else
				return (2);
		}
		else
		{
			std::cout << "prout\n";
			return (-1);
		}
	}
	std::cerr << RED BOLD << "[ERROR]" << NEUTRAL RED << "unknown content type: " << _Content_type << NEUTRAL << std::endl;
	return (0);
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

