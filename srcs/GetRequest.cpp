#include "GetRequest.hpp"

GetRequest::GetRequest() {}

GetRequest::GetRequest(map<string, string> header)
{
	_path = header["path"];
	size_t queryPos = header["path"].find('?');
	if (queryPos != string::npos)
		_path = header["path"].substr(0, queryPos);
	_method = GET;
	_keep_alive = true;

	if (header.find("Connection") != header.end())
		if (header["Connection"] == "close")
			_keep_alive = false;
	_client = header["User-agent"];
	_host = header["Host"];
	if (header.find("Accept") != header.end())
		_accepted_mime = header["Accept"];
	return;
}

GetRequest::GetRequest(const GetRequest &src) : ARequest(src) {}

GetRequest &GetRequest::operator=(GetRequest &src)
{
	if (this != &src)
		ARequest::operator=(src);
	return (*this);
}

GetRequest::~GetRequest() {}

/**
 * If the decodedUrl is a directory and doesn't end with a slash,
 * tries to redirect to the same path with a trailing slash.
 * Then tries to serve an index file if configured.
 * If no index file is found, checks if autoindex is enabled to serve a directory listing.
 * If autoindex is off, sends a 403 Forbidden error response.
 * @param fd The file descriptor to send the response to
 * @param server The server object containing configuration and state
 * @param config The configuration parser object
 * @param decodedUrl The decoded URL path from the request
 * @return 0 if keep-alive is enabled, -1 if connection should be closed
 */
int GetRequest::handleGet(int fd, Server &server, ConfigParser const *config, string const &fullPath)
{
	string decodedUrl = urlDecode(fullPath.c_str());
	string cleanPath = decodedUrl;
	size_t queryPos = decodedUrl.find('?');
	if (queryPos != string::npos)
		cleanPath = decodedUrl.substr(0, queryPos);

	PathType pathType = getPathType(cleanPath);

	if (pathType == PATH_NOT_EXISTS)
		return (sendErrorResponse(fd, 404, config, server));

	if (pathType == PATH_DIRECTORY)
		return (handleDirectory(fd, server, config, cleanPath));

	if (pathType == PATH_FILE)
		return (handleFile(fd, server, config, cleanPath));

	return (sendErrorResponse(fd, 500, config, server));
}

/**
 * Determines the type of the given path: non-existent, directory, or regular file.
 * @param path The filesystem path to check
 * @return PATH_NOT_EXISTS if the path does not exist,
 *         PATH_DIRECTORY if it's a directory,
 *         PATH_FILE if it's a regular file
 */
GetRequest::PathType GetRequest::getPathType(const string &path)
{
	struct stat pathStat;
	if (stat(path.c_str(), &pathStat) != 0)
		return (PATH_NOT_EXISTS);

	if (S_ISDIR(pathStat.st_mode))
		return (PATH_DIRECTORY);

	if (S_ISREG(pathStat.st_mode))
		return (PATH_FILE);

	return (PATH_NOT_EXISTS);
}

/**
 * If the decodedUrl is a directory and doesn't end with a slash,
 * tries to redirect to the same path with a trailing slash.
 * Then tries to serve an index file if configured.
 * If no index file is found, checks if autoindex is enabled to serve a directory listing.
 * If autoindex is off, sends a 403 Forbidden error response.
 * @param fd The file descriptor to send the response to
 * @param server The server object containing configuration and state
 * @param config The configuration parser object
 * @param decodedUrl The decoded URL path from the request
 * @return 0 if keep-alive is enabled, -1 if connection should be closed
 */
int GetRequest::handleDirectory(int fd, Server &server, ConfigParser const *config, string const &decodedUrl)
{
	string url = decodedUrl;

	// Ensure directory paths end with '/'
		if (url[url.length() - 1] != '/')
			url += '/';
	string pathForConfig = getPathForConfig(url);
	string indexPages = config->getLocationValueForPath(pathForConfig, server.getUid(), "index", true);

	// Try to serve index file first
	if (!indexPages.empty())
	{
		int result = tryServeIndexFile(fd, server, config, url, indexPages);
		if (result != -2) // -2 means no index file found, continue with directory listing
			return (1);
	}

	// No index file, try directory listing
	return (handleDirectoryListing(fd, server, config, url, pathForConfig));
}

/**
 * Handles serving a regular file.
 * If the file is not readable, sends a 403 Forbidden error response.
 * @param fd The file descriptor to send the response to
 * @param server The server object containing configuration and state
 * @param config The configuration parser object
 * @param decodedUrl The decoded URL path from the request
 * @return 0 if keep-alive is enabled, -1 if connection should be closed
 */
int GetRequest::handleFile(int fd, Server &server, ConfigParser const *config, string const &decodedUrl)
{
	if (access(decodedUrl.c_str(), R_OK) != 0)
		return (sendErrorResponse(fd, 403, config, server));

	cout << CYAN << BOLD << "File requested: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;
	return (sendCGIResponse(fd, decodedUrl, config, server));
}

/**
 * Tries to find and serve an index file from the list of possible index files.
 * @param fd The file descriptor to send the response to
 * @param server The server object containing configuration and state
 * @param config The configuration parser object
 * @param decodedUrl The decoded URL path from the request
 * @param indexPages A space-separated list of possible index files (e.g. "index.html index.php")
 * @return 0 if keep-alive is enabled, -1 if connection should be closed, -2 if no index file found
 */
int GetRequest::tryServeIndexFile(int fd, Server &server, ConfigParser const *config, string const &decodedUrl, string const &indexPages)
{
	map<string, size_t> indexFile = getIndex(indexPages, decodedUrl);

	if (indexFile.empty())
		return (-2); // No index file found

	if (indexFile.begin()->second == 200)
		return (serveIndexFile(fd, server, config, decodedUrl, indexFile.begin()->first));

	return (sendErrorResponse(fd, indexFile.begin()->second, config, server));
}

/**
 * Serves the specified index file with the CGI.
 * @param fd The file descriptor to send the response to
 * @param server The server object containing configuration and state
 * @param config The configuration parser object
 * @param decodedUrl The decoded URL path from the request
 * @param indexFileName The name of the index file to serve
 * @return 0 if keep-alive is enabled, -1 if connection should be closed
 */
int GetRequest::serveIndexFile(int fd, Server &server, ConfigParser const *config, string const &decodedUrl, string const &indexFileName)
{
	// cout << GREEN << BOLD << "Index files found: " << indexFileName << NEUTRAL << endl;
	string indexFullPath = decodedUrl + indexFileName;
	// cout << CYAN << BOLD << "Serving index file: " << NEUTRAL << CYAN << indexFullPath << NEUTRAL << endl;

	return (sendCGIResponse(fd, indexFullPath, config, server));
}

/**
 * Handles directory listing if autoindex is enabled.
 * If autoindex is off, sends a 403 Forbidden error response.
 * @param fd The file descriptor to send the response to
 * @param server The server object containing configuration and state
 * @param config The configuration parser object
 * @param decodedUrl The decoded URL path from the request
 * @param pathForConfig The path adjusted for configuration lookup
 * @return 0 if keep-alive is enabled, -1 if connection should be closed
 */
int GetRequest::handleDirectoryListing(int fd, Server &server, ConfigParser const *config, string const &decodedUrl, string const &pathForConfig)
{
	string autoindex = config->getLocationValueForPath(pathForConfig, server.getUid(), "autoindex", true);
	if (autoindex.empty())
		autoindex = "on";

	cout << CYAN << BOLD << "Directory requested: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;

	if (autoindex == "on") {
		string listing = generateDirectoryListing(decodedUrl, _path);
		string response = writeHTTPResponse(server, 200, listing, "text/html");
		server.fillClientBuffer(fd, response);
	} 
	else
		return (sendErrorResponse(fd, 403, config, server));
	server.keepaliveDefine(fd, isKeepalive());
	return (1);
}

/**
 * If the decodedUrl is a directory and doesn't end with a slash,
 * we add a trailing slash to the pathForConfig to ensure correct
 * location matching in the configuration.
 * @param decodedUrl The decoded URL path from the request
 * @return The adjusted path for configuration lookup `eg: /path/` instead of `/path`
 */
string GetRequest::getPathForConfig(const string &decodedUrl)
{
	string pathForConfig = _path;
	if (getPathType(decodedUrl) == PATH_DIRECTORY && !_path.empty() && _path[_path.length() - 1] != '/')
		pathForConfig += "/";
	return (pathForConfig);
}

int GetRequest::sendErrorResponse(int fd, int errorCode, ConfigParser const *config, Server &server)
{
	string errorPage = loadErrorPage(errorCode, config, server.getUid());
	string response = writeHTTPResponse(server, errorCode, errorPage, "text/html");
	server.fillClientBuffer(fd, response);
	server.keepaliveDefine(fd, isKeepalive());
	return (1);
}

ARequest*	GetRequest::clone() const
{
	return (new GetRequest(*this));
}

int GetRequest::UploadFile(string body, string path)
{
	(void)body;
	(void)path;
	return (0);
}