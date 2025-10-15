#include "GetRequest.hpp"

GetRequest::GetRequest()
{
	return;
}

GetRequest::GetRequest(map<string, string> header)
{
	_path = header["path"];
	_method = GET;
	_keep_alive = true;

	if (header.find("Connection") != header.end())
		if (header["Connection"] == "close")
			_keep_alive = false;
	_client = header["User-agent"];
	_host = header["Host"];
	return;
}

GetRequest::GetRequest(GetRequest &src) : ARequest(src) {return ;}

GetRequest &GetRequest::operator=(GetRequest &src)
{
	if (this != &src)
		ARequest::operator=(src);
	return (*this);
}

GetRequest::~GetRequest() {return ;}


int GetRequest::handleGet(int fd, Server &server, ConfigParser const *config, string const &fullPath)
{
	string decodedUrl = urlDecode(fullPath.c_str());
	PathType pathType = getPathType(decodedUrl);

	if (pathType == PATH_NOT_EXISTS)
		return sendErrorResponse(fd, 404, config, server);

	if (pathType == PATH_DIRECTORY)
		return handleDirectory(fd, server, config, decodedUrl);

	if (pathType == PATH_FILE)
		return handleFile(fd, server, config, decodedUrl);

	return sendErrorResponse(fd, 500, config, server);
}

GetRequest::PathType GetRequest::getPathType(const string &path)
{
	struct stat pathStat;
	if (stat(path.c_str(), &pathStat) != 0)
		return PATH_NOT_EXISTS;

	if (S_ISDIR(pathStat.st_mode))
		return PATH_DIRECTORY;

	if (S_ISREG(pathStat.st_mode))
		return PATH_FILE;

	return PATH_NOT_EXISTS;
}

int GetRequest::handleDirectory(int fd, Server &server, ConfigParser const *config, string const &decodedUrl)
{
	string pathForConfig = getPathForConfig(decodedUrl);
	string indexPages = config->getLocationValueForPath(pathForConfig, server.getUid(), "index");

	// Try to serve index file first
	if (!indexPages.empty())
	{
		int result = tryServeIndexFile(fd, server, config, decodedUrl, indexPages);
		if (result != -2) // -2 means no index file found, continue with directory listing
			return 1;
	}

	// No index file, try directory listing
	return handleDirectoryListing(fd, server, config, decodedUrl, pathForConfig);
}

int GetRequest::handleFile(int fd, Server &server, ConfigParser const *config, string const &decodedUrl)
{
	if (access(decodedUrl.c_str(), R_OK) != 0)
		return sendErrorResponse(fd, 403, config, server);

	cout << CYAN << BOLD << "File requested: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;
	string response = sendCGIResponse(decodedUrl, config, server);

	server.fillClientBuffer(fd, response);	
	server.keepaliveDefine(fd, isKeepalive());

	return 1;
}

int GetRequest::tryServeIndexFile(int fd, Server &server, ConfigParser const *config, string const &decodedUrl, string const &indexPages)
{
	map<string, size_t> indexFile = getIndex(indexPages, decodedUrl);

	if (indexFile.empty())
		return -2; // No index file found

	if (indexFile.begin()->second == 200)
		return serveIndexFile(fd, server, config, decodedUrl, indexFile.begin()->first);

	return sendErrorResponse(fd, indexFile.begin()->second, config, server);
}

int GetRequest::serveIndexFile(int fd, Server &server, ConfigParser const *config, string const &decodedUrl, string const &indexFileName)
{
	cout << GREEN << BOLD << "Index files found: " << indexFileName << NEUTRAL << endl;
	string indexFullPath = decodedUrl + indexFileName;
	cout << CYAN << BOLD << "Serving index file: " << NEUTRAL << CYAN << indexFullPath << NEUTRAL << endl;

	string response = sendCGIResponse(indexFullPath, config, server);

	server.fillClientBuffer(fd, response);
	server.keepaliveDefine(fd, isKeepalive());

	return 1;
}

int GetRequest::handleDirectoryListing(int fd, Server &server, ConfigParser const *config, string const &decodedUrl, string const &pathForConfig)
{
	string autoindex = config->getLocationValueForPath(pathForConfig, server.getUid(), "autoindex");
	if (autoindex.empty())
		autoindex = "on";

	cout << CYAN << BOLD << "Directory requested: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;

	if (autoindex == "on") {
		string listing = generateDirectoryListing(decodedUrl, _path);
		string response = writeHTTPResponse(200, listing, "text/html");
		server.fillClientBuffer(fd, response);
	}
	else
		return sendErrorResponse(fd, 403, config, server);
	server.keepaliveDefine(fd, isKeepalive());
	return 1;
}

string GetRequest::getPathForConfig(const string &decodedUrl)
{
	string pathForConfig = _path;
	if (getPathType(decodedUrl) == PATH_DIRECTORY &&
		!_path.empty() &&
		_path[_path.length() - 1] != '/')
	{
		pathForConfig += "/";
	}
	return pathForConfig;
}

int GetRequest::sendErrorResponse(int fd, int errorCode, ConfigParser const *config, Server &server)
{
	string errorPage = loadErrorPage(errorCode, config, server.getUid());
	string response = writeHTTPResponse(errorCode, errorPage, "text/html");
	server.fillClientBuffer(fd, response);
	server.keepaliveDefine(fd, isKeepalive());
	return 1;
}

