#include "DeleteRequest.hpp"

DeleteRequest::DeleteRequest()
{
}

DeleteRequest::DeleteRequest(map<string, string> header)
{
	_path = header["path"];
	_method = DELETE;
	_keep_alive = true;

	if (header.find("Connection") != header.end())
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

int	DeleteRequest::delete_file(int fd, const Server &serv)
{
	string			root = serv.getEnvValue("SERVER_ROOT");
	string				query = urlDecode(serv.getEnvValue("QUERY_STRING"));
	map<string, string>	queryMap = parseQuery(query);
	string			fileName = queryMap["file"];
	string			uploadName = queryMap["upload"];

	if (root.rfind('/') == root.size() - 1 && fileName[0] == '/')
		fileName.erase(0, 1);
	string			filePath = root + fileName;

	if (root.rfind('/') == root.size() - 1 && uploadName[0] == '/')
		uploadName.erase(0, 1);
	string			uploadPath = root + uploadName;
	if (!uploadPath.empty())
		if (remove(uploadPath.c_str()) != 0)
			cout << "wtf\n";
	if (remove(filePath.c_str()) == 0)
	{
		return (sendHTTPResponse(fd, 204, "", ""));
	}
	else
		return (-1);
}


int DeleteRequest::handleDelete(int fd, const Server &server, const ConfigParser *config, const string &path)
{


	if (access(path.c_str(), F_OK))
		delete_file(fd, server);
	else
	{
		string errorPage = loadErrorPage(404, config, server.getUid());
		if (sendHTTPResponse(fd, 404, errorPage, "text/html") == -1)
			cerr << "Failed to send 404 response" << endl;
	}
	if (!isKeepalive())
	{
		cout << "there?\n";
		return (-1);
	}
	return (0);
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

