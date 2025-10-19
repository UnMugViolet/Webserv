#include "PostRequest.hpp"

PostRequest::PostRequest() {}

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
	while (true) {
		ostringstream oss;
		if (i > 0)
			oss << path << "(" << i << ")" << ".txt";
		else
			oss << path << ".txt";
		filename = oss.str();
		if (access(filename.c_str(), F_OK) != 0)
			break ;
		i++;
	}
	ofstream file(filename.c_str());
	if (file.is_open()) {
		for (map<string, string>::reverse_iterator it = content.rbegin(); it != content.rend(); it++)
		{
			file << it->first << "=" << it->second << endl;
		}
		return (i);
	}
	else
		//couldn't create file
		return (-1);
}

int	PostRequest::UploadFile(string body, string path)
{
	string base = path;
	string filename;
	string extension = "";

	if (path.rfind('.') != string::npos) {
		extension = path.substr(path.rfind('.'));
		size_t pos = base.rfind('.');
		if (pos != string::npos)
			base.erase(pos, string::npos);
	}
	int i = 0;
	while (true) {
		ostringstream oss;
		if (i > 0)
			oss << base << "(" << i << ")" << extension;
		else
			oss << base << extension;
		filename = oss.str();
		if (access(filename.c_str(), F_OK) != 0)
			break ;
		i++;
	}
	ofstream file(filename.c_str());
	if (file.is_open()) {
		file << body;
		return (i);
	} else {
		cerr << "couldn't create file for upload" << endl;
		return (-1);
	}
}

int	PostRequest::createPost(string body, string postpath, string uploadpath)
{
	string	filename;
	map<string, string>	content;

	if (_Content_type.compare("text/plain") == 0) {
		filename = postpath + "post.txt";
		
		if(UploadFile(body, filename) == -1)
			return (-1);
		else
			return (0);
	}
	if (_Content_type.find("multipart/form-data") != string::npos) {
		string bodypart;
		string boundary = _Content_type.substr(_Content_type.find("boundary=") + 9);

		size_t pos = body.find(boundary);
		pos += boundary.size();
		size_t end = body.find(boundary, pos) - 2;
		while (true) {
			bodypart = "";

			if (body[pos] != '\r')
				break ;
			
			pos = body.find("name=", pos) + 6;
			string fieldname = body.substr(pos, body.find("\"", pos) - pos);
			if (body.find("filename=", pos) < end && body.find("filename=", pos) != string::npos) {
				int res;

				pos = body.find("filename=", pos) + 10;
				filename = body.substr(pos, body.find("\"", pos) - pos);
		
				if (filename == "") {
					pos = body.find(boundary, pos);
					pos += boundary.size();
					end = body.find(boundary, pos) - 2;
					continue ;
				}
				pos = body.find("\r\n\r\n", pos) + 4;
				bodypart = body.substr(pos, end - pos);
				res = UploadFile(bodypart, uploadpath + filename);
				if(res == -1)
					return (-1);
				if (res > 0) {
					string extension = "";
					if (filename.rfind('.') != string::npos) {
						size_t pos = filename.rfind('.');
						extension = filename.substr(pos);
						if (pos != string::npos)
							filename.erase(pos, string::npos);
					}
					filename += "(";
					filename += res + 48;
					filename += ")";
					filename += extension;
				}
				content[fieldname] = filename;
			} else {
				pos = body.find("\r\n\r\n", pos) + 4;
				bodypart = body.substr(pos, end - pos - 2);
				content[fieldname] = bodypart;
			}
			pos = body.find(boundary, end);
			pos += boundary.size();
			end = body.find(boundary, pos) - 2;
		}

		if (!content.empty()) {
			filename = postpath + "/data";
			if (UploadContent(content, filename) == -1)
				return (-1);
			else
				return (0);
		} else
			return (-1);
	}
	cerr << RED BOLD << "[ERROR]" << NEUTRAL RED << "unknown content type: " << _Content_type << NEUTRAL << endl;
	return (-1);
}

int PostRequest::handlePost(int fd, Server &server, const string &body, const ConfigParser *config)
{
	string serverRoot = config->getServerValue(server.getUid(), "root");
	string path = server.getEnvValue("REQUEST_URI");
	string cleanPath = path.substr(0, path.find('?'));
	
	string uploadir = config->getLocationValueForPath(cleanPath, server.getUid(), "put_uploads", true);
	string postdir = config->getLocationValueForPath(cleanPath, server.getUid(), "put_posts", true);
	// define target directory
	if (uploadir.find('/') == 0 && serverRoot.rfind('/') == serverRoot.size() - 1)
		uploadir.erase(0, 1);
	if (uploadir.rfind('/') != uploadir.size() - 1)
		uploadir += '/';
	uploadir = serverRoot + uploadir;
	DIR* dir = opendir(uploadir.c_str());
	if (dir == NULL) {
		cerr << "no uploads directory" << endl;//what? no appropriate directory or no permission
		string errorPage = loadErrorPage(500, config, server.getUid());
		string response = writeHTTPResponse(server, 500, errorPage, "text/html");
		server.keepaliveDefine(fd, isKeepalive());
		server.fillClientBuffer(fd, response);
		return (1);
	}
	closedir(dir);
	if (postdir.find('/') == 0 && serverRoot.rfind('/') == serverRoot.size() - 1)
		postdir.erase(0, 1);
	if (postdir.rfind('/') != postdir.size() - 1)
		postdir += '/';
	postdir = serverRoot + postdir;
	dir = opendir(postdir.c_str());
	if (dir == NULL) {
		cerr << "no posts directory" << endl;//what? no appropriate directory or no permission
		string errorPage = loadErrorPage(500, config, server.getUid());
		string response = writeHTTPResponse(server, 500, errorPage, "text/html");
		server.keepaliveDefine(fd, isKeepalive());
		server.fillClientBuffer(fd, response);
		return (1);
	}
	closedir(dir);
	int res = createPost(body, postdir, uploadir);
	if (res == -1) {
		string errorPage = loadErrorPage(500, config, server.getUid());
		string response = writeHTTPResponse(server, 500, errorPage, "text/html");
		server.keepaliveDefine(fd, isKeepalive());
		server.fillClientBuffer(fd, response);
		return (1);
	}
	string response = writeHTTPResponse(server, 204, "", "");
	server.fillClientBuffer(fd, response);
	server.keepaliveDefine(fd, isKeepalive());
	return (1);
}

PostRequest::~PostRequest() {}

PostRequest&	PostRequest::operator=(PostRequest& src)
{
	if (this != &src) {
		ARequest::operator=(src);
		this->_Content_type = src._Content_type;
		this->_body = src._body;
	}
	return (*this);
}

