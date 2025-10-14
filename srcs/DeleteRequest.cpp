#include "DeleteRequest.hpp"

DeleteRequest::DeleteRequest()
{
}

DeleteRequest::DeleteRequest(std::map<std::string, std::string> header)
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
	std::string			root = serv.getEnvValue("SERVER_ROOT");
	std::map<std::string, std::string>	queryMap = parseQuery(serv.getEnvValue("QUERY_STRING"));
	std::string			fileName = queryMap["file"];
	std::string			uploadPath = queryMap["upload"];

	if (root.rfind('/') == root.size() - 1 && fileName[0] == '/')
		fileName.erase(0, 1);
	std::string			filePath = root + fileName;

	if (!uploadPath.empty())
		if (std::remove(uploadPath.c_str()) != 0)
			std::cout << "wtf\n";
	if (std::remove(filePath.c_str()) == 0)
	{
		return (sendHTTPResponse(fd, 204, "", ""));
	}
	else
		return (-1);
}


int DeleteRequest::handleDelete(int fd, const Server &server, const ConfigParser *config, const std::string &path)
{


	if (access(path.c_str(), F_OK))
		delete_file(fd, server);
	else
	{
		std::string errorPage = loadErrorPage(404, config, server.getUid());
		if (sendHTTPResponse(fd, 404, errorPage, "text/html") == -1)
			std::cerr << "Failed to send 404 response" << std::endl;
	}
	if (!isKeepalive())
	{
		std::cout << "there?\n";
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

