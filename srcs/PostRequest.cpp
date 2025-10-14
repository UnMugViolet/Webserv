#include "PostRequest.hpp"

PostRequest::PostRequest()
{
}

PostRequest::PostRequest(map<string, string> header)
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

int	PostRequest::UploadContent(map<string, string> content, string path)
{
	string filename;

	int i = 0;
	while (true)
	{
		ostringstream oss;
		if (i > 0)
			oss << path << "(" << i << ")" << ".txt";
		else
			oss << path << ".txt";
		filename = oss.str();
		if (access(filename.c_str(), F_OK) != 0)
			break;
		i++;
	}
	ofstream file(filename.c_str());
	if (file.is_open())
	{
		for (map<string, string>::reverse_iterator it = content.rbegin(); it != content.rend(); it++)
		{
			file << it->first << "=" << it->second << endl;
		}
		return (i);
	}
	else
	{//couldn't create file
		return (-1);
	}
}

int	PostRequest::UploadFile(string body, string path)
{
	string base = path;
	string filename;
	string extension = "";
	if (path.rfind('.') != string::npos)
	{
		extension = path.substr(path.rfind('.'));
		size_t pos = base.rfind('.');
		if (pos != string::npos)
			base.erase(pos, string::npos);
	}
	int i = 0;
	while (true)
	{
		ostringstream oss;
		if (i > 0)
			oss << base << "(" << i << ")" << extension;
		else
			oss << base << extension;
		filename = oss.str();
		if (access(filename.c_str(), F_OK) != 0)
			break;
		i++;
	}
	ofstream file(filename.c_str());
	if (file.is_open())
	{
		file << body;
		return (i);
	}
	else
	{
		cerr << "couldn't create file for upload" << endl;
		return (-1);
	}
}

int	PostRequest::createPost(int fd, string body, string path)
{
	string	filename;
	map<string, string>	content;

	if (_Content_type.compare("text/plain") == 0)
	{
		path += "/var/posts";
		filename = path + "/post.txt";
		
		if(UploadFile(body, filename) == -1)
			return (-1);
		else
			return (sendHTTPResponse(fd, 204, "", ""));
	}
	if (_Content_type.find("multipart/form-data") != string::npos)
	{
		string bodypart;
		string boundary = _Content_type.substr(_Content_type.find("boundary=") + 9);

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
			string fieldname = body.substr(pos, body.find("\"", pos) - pos);
			if (body.find("filename=", pos) < end && body.find("filename=", pos) != string::npos)
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
					cout << "file?\n" << filename << endl;
					string extension = "";
					if (filename.rfind('.') != string::npos)
					{
						size_t pos = filename.rfind('.');
						extension = filename.substr(pos);
						if (pos != string::npos)
							filename.erase(pos, string::npos);
					}
					filename += "(";
					filename += res + 48;
					filename += ")";
					filename += extension;
					cout << "filename: " << filename << endl;
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
	cerr << RED BOLD << "[ERROR]" << NEUTRAL RED << "unknown content type: " << _Content_type << NEUTRAL << endl;
	return (0);
}

int PostRequest::handlePost(int fd, const Server &server, const string &body, const ConfigParser *config)
{
	string serverRoot = config->getServerValue(server.getUid(), "root");
	string path = server.getEnvValue("REQUEST_URI");
	string cleanPath = path.substr(0, path.find('?'));
	
	string uploadir = config->getLocationValueForPath(cleanPath, server.getUid(), "put_uploads");
	string postdir = config->getLocationValueForPath(cleanPath, server.getUid(), "put_posts");
	// define target directory
	path = serverRoot + uploadir;
	DIR* dir = opendir(path.c_str());
	if (dir == NULL)
	{
		cerr << "no uploads directory" << endl;//what? no appropriate directory or no permission
		string errorPage = loadErrorPage(500, config, server.getUid());
		if (sendHTTPResponse(fd, 500, errorPage, "text/html") == -1)
			cerr << "Failed to send 500 response" << endl;
		if (isKeepalive())
			return (0);
		return (-1);
	}
	closedir(dir);
	path = serverRoot + postdir;
	dir = opendir(path.c_str());
	if (dir == NULL)
	{
		cerr << "no posts directory" << endl;//what? no appropriate directory or no permission
		string errorPage = loadErrorPage(500, config, server.getUid());
		if (sendHTTPResponse(fd, 500, errorPage, "text/html") == -1)
			cerr << "Failed to send 500 response" << endl;
		if (isKeepalive())
			return (0);
		return (-1);
	}
	closedir(dir);
	int res = createPost(fd, body, serverRoot);
	if (res == -1)
	{
		string errorPage = loadErrorPage(500, config, server.getUid());
		if (sendHTTPResponse(fd, 500, errorPage, "text/html") == -1)
			cerr << "Failed to send 500 response" << endl;
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

