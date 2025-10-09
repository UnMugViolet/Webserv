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

