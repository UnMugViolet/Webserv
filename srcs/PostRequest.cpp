#include "PostRequest.hpp"

PostRequest::PostRequest()
{
}

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
	{//couldn't create file
		return (-1);
	}
}

int	PostRequest::UploadFile(std::string body, std::string path)
{
	std::string base = path;
	std::string filename;
	std::string extension = "";
	if (path.rfind('.') != std::string::npos)
	{
		extension = path.substr(path.rfind('.'));
		base.erase(base.rfind('.'), std::string::npos);
	}
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

int	PostRequest::createPost(int fd, std::string body, std::string path)
{
	std::string	filename;
	std::map<std::string, std::string>	content;

	if (_Content_type.compare("text/plain") == 0)
	{
		path += "/var/posts";
		filename = path + "/post.txt";
		
		if(UploadFile(body, filename) == -1)
			return (-1);
		else
			return (sendHTTPResponse(fd, 204, "", ""));
	}
	if (_Content_type.find("multipart/form-data") != std::string::npos)
	{
		std::string bodypart;
		std::string boundary = _Content_type.substr(_Content_type.find("boundary=") + 9);

		size_t pos = body.find(boundary);
		pos += boundary.size();
		size_t end = body.find(boundary, pos) - 2;
		while (true)
		{
			bodypart = "";

			if (body[pos] != '\r')
			{
				break;
			}
			pos = body.find("name=", pos) + 6;
			std::string fieldname = body.substr(pos, body.find("\"", pos) - pos);
			std::cout << "pouet\n";
			if (body.find("filename=", pos) < end && body.find("filename=", pos) != std::string::npos)
			{
				int res;

				pos = body.find("filename=", pos) + 10;
				filename = body.substr(pos, body.find("\"", pos) - pos);
				if (filename == "")
				{
					pos = body.find(boundary, pos);
					pos += boundary.size();
					end = body.find(boundary, pos) - 2;
					continue;
				}
				filename = path + "/var/uploads/" + filename;
				pos = body.find("\r\n\r\n", pos) + 4;
				bodypart = body.substr(pos, end - pos);
				res = UploadFile(bodypart, filename);
				if(res == -1)
					return (-1);
				if (res > 0)
				{
					std::cout << "file?\n" << filename << std::endl;
					std::string extension = "";
					if (filename.rfind('.') != std::string::npos)
					{
						extension = filename.substr(filename.rfind('.'));
						filename.erase(filename.rfind('.'), std::string::npos);
					}
					filename += "(";
					filename += res + 48;
					filename += ")";
					filename += extension;
					std::cout << "filename: " << filename << std::endl;
				}
				content[fieldname] = filename;
			}
			else
			{
				pos = body.find("\r\n\r\n", pos) + 4;
				bodypart = body.substr(pos, end - pos - 2);
				content[fieldname] = bodypart;
			}
			pos = body.find(boundary, pos);
			pos += boundary.size();
			end = body.find(boundary, pos) - 2;
		}
		if (!content.empty())
		{
			filename = path + "/var/posts/data";
			if (UploadContent(content, filename) == -1)
				return (-1);
			else
				return (sendHTTPResponse(fd, 204, "", ""));
		}
		else
		{
			return (-1);
		}
	}
	std::cerr << RED BOLD << "[ERROR]" << NEUTRAL RED << "unknown content type: " << _Content_type << NEUTRAL << std::endl;
	return (0);
}

int PostRequest::handlePost(int fd, const Server &server, const std::string &body, const ConfigParser *config)
{
	std::string serverRoot = config->getServerValue(server.getUid(), "root");
	std::string path = server.getEnvValue("REQUEST_URI");
	std::string cleanPath = path.substr(0, path.find('?'));
	
	std::string uploadir = config->getLocationValueForPath(cleanPath, server.getUid(), "put_uploads");
	std::string postdir = config->getLocationValueForPath(cleanPath, server.getUid(), "put_posts");
	// define target directory
	path = serverRoot + uploadir;
	DIR* dir = opendir(path.c_str());
	if (dir == NULL)
	{
		std::cerr << "no uploads directory" << std::endl;//what? no appropriate directory or no permission
		std::string errorPage = loadErrorPage(500, config, server.getUid());
		if (sendHTTPResponse(fd, 500, errorPage, "text/html") == -1)
			std::cerr << "Failed to send 500 response" << std::endl;
		if (isKeepalive())
			return (0);
		return (-1);
	}
	closedir(dir);
	path = serverRoot + postdir;
	dir = opendir(path.c_str());
	if (dir == NULL)
	{
		std::cerr << "no posts directory" << std::endl;//what? no appropriate directory or no permission
		std::string errorPage = loadErrorPage(500, config, server.getUid());
		if (sendHTTPResponse(fd, 500, errorPage, "text/html") == -1)
			std::cerr << "Failed to send 500 response" << std::endl;
		if (isKeepalive())
			return (0);
		return (-1);
	}
	closedir(dir);
	int res = createPost(fd, body, serverRoot);
	if (res == -1)
	{
		std::string errorPage = loadErrorPage(500, config, server.getUid());
		if (sendHTTPResponse(fd, 500, errorPage, "text/html") == -1)
			std::cerr << "Failed to send 500 response" << std::endl;
	}
	if (!isKeepalive())
		return (-1);
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

