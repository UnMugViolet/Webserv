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

GetRequest::GetRequest(GetRequest &src) : ARequest(src)
{
	return ;
}
/**
 * Logic:
 * 1. If directory:
 *    - Try to find index file first, if found and accessible, serve it.
 *    - If no index file or not accessible:
 *      - If autoindex on, serve directory listing.
 *      - If autoindex off, respond 403.
 * 2. If file:
 *    - If accessible, serve it.
 *    - If not accessible, respond 403.
 * 3. If neither file nor directory exists, respond 404.
 * 4. If not keep-alive, return -1 to close connection.
 */
int	GetRequest::handleGet(int fd, Server &server, const ConfigParser *config, const string &fullPath)
{
	string decodedUrl = urlDecode(fullPath.c_str());
	string response;
	struct stat pathStat;
	bool isDirectory = false;
	bool isFile = false;
	
	// Check if path is a directory or file using stat
	if (stat(decodedUrl.c_str(), &pathStat) == 0) {
		if (S_ISDIR(pathStat.st_mode)) {
			isDirectory = true;
		} else if (S_ISREG(pathStat.st_mode)) {
			isFile = true;
		}
	}
	
	// For location matching, use path with trailing slash if it's a directory
	string pathForConfig = _path;
	if (isDirectory && _path[_path.length() - 1] != '/') {
		pathForConfig += "/";
	}
	
	string autoindex = config->getLocationValueForPath(pathForConfig, server.getUid(), "autoindex");
	string indexPages = config->getLocationValueForPath(pathForConfig, server.getUid(), "index");
	map<string, size_t> indexFile;

	// Default autoindex to "on" if not explicitly set
	if (autoindex.empty())
		autoindex = "on";

	if (isDirectory) {
		cout << CYAN << BOLD << "Directory requested: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;
		cout << YELLOW << BOLD << "Index pages configured: " << NEUTRAL << YELLOW << indexPages << NEUTRAL << endl;
		
		// TO REMOVE 
		if (!indexPages.empty())
			indexFile = getIndex(indexPages, decodedUrl);
		
		cout << RED << BOLD << "Index files found: " << indexFile.begin()->first << NEUTRAL << RED << endl;
		// If index file is found, try to serve it
		if (!indexFile.empty()) {
			string indexPath = fullPath;
			if (indexFile.begin()->first[0] == '/')
				indexPath += indexFile.begin()->first;
			else
				indexPath += "/" + indexFile.begin()->first;
			string decodedIndexPath = urlDecode(indexPath.c_str());
			
			// Check if index file exists and is accessible
			if (access(decodedIndexPath.c_str(), F_OK) == 0) {
				if (access(decodedIndexPath.c_str(), R_OK) == 0) {
					// Index file accessible - serve it
					cout << CYAN << BOLD << "Serving index file: " << NEUTRAL << CYAN << decodedIndexPath << NEUTRAL << endl;
					response = sendCGIResponse(decodedIndexPath, config, server);
				} else {
					string errorPage = loadErrorPage(403, config, server.getUid());
					response = writeHTTPResponse(403, errorPage, "text/html");
				}
			} else {
				string errorPage = loadErrorPage(404, config, server.getUid());
				response = writeHTTPResponse(404, errorPage, "text/html");
			}
		} else {
			// No index file found or none accessible - check autoindex
			if (autoindex == "on") {
				// Serve directory listing
				cout << CYAN << BOLD << "Serving directory listing for: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;
				string response = sendCGIResponse(decodedUrl, config, server);
			} else {
				string errorPage = loadErrorPage(403, config, server.getUid());
				response = writeHTTPResponse(403, errorPage, "text/html");
			}
		}
	}
	else if (isFile) {
		// Check if file is accessible
		if (access(decodedUrl.c_str(), R_OK) == 0) {
			// File accessible - serve it
			cout << CYAN << BOLD << "File requested: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;
			response = sendCGIResponse(decodedUrl, config, server);
		} else {
			string errorPage = loadErrorPage(403, config, server.getUid());
			response = writeHTTPResponse(403, errorPage, "text/html");
		}
	}
	// Neither file nor directory exists
	else {
		string errorPage = loadErrorPage(404, config, server.getUid());
		response = writeHTTPResponse(404, errorPage, "text/html");
	}
	server.fillClientBuffer(fd, response);
	server.keepaliveDefine(fd, isKeepalive());
	return (0);
}

GetRequest&	GetRequest::operator=(GetRequest &src)
{
	if (this != &src)
		ARequest::operator=(src);
	return (*this);
}

GetRequest::~GetRequest()
{
	return;
}

GetRequest::GetRequest(GetRequest &src) : ARequest(src)
{
	return;
}

int GetRequest::handleGet(int fd, Server const &server, ConfigParser const *config, string const &fullPath)
{
	string decodedUrl = urlDecode(fullPath.c_str());
	PathType pathType = getPathType(decodedUrl);

	if (pathType == PATH_NOT_EXISTS)
		return sendErrorResponse(fd, 404, config, server.getUid());

	if (pathType == PATH_DIRECTORY)
		return handleDirectory(fd, server, config, decodedUrl);

	if (pathType == PATH_FILE)
		return handleFile(fd, server, config, decodedUrl);

	return sendErrorResponse(fd, 500, config, server.getUid());
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

int GetRequest::handleDirectory(int fd, Server const &server, ConfigParser const *config, string const &decodedUrl)
{
	string pathForConfig = getPathForConfig(decodedUrl);
	string indexPages = config->getLocationValueForPath(pathForConfig, server.getUid(), "index");

	// Try to serve index file first
	if (!indexPages.empty())
	{
		int result = tryServeIndexFile(fd, server, config, decodedUrl, indexPages);
		if (result != -2) // -2 means no index file found, continue with directory listing
			return result;
	}

	// No index file, try directory listing
	return handleDirectoryListing(fd, server, config, decodedUrl, pathForConfig);
}

int GetRequest::handleFile(int fd, Server const &server, ConfigParser const *config, string const &decodedUrl)
{
	if (access(decodedUrl.c_str(), R_OK) != 0)
		return sendErrorResponse(fd, 403, config, server.getUid());

	cout << CYAN << BOLD << "File requested: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;
	if (sendCGIResponse(fd, decodedUrl, config, server) == -1)
		cerr << "Failed to send CGI response" << endl;

	return checkKeepAlive();
}

int GetRequest::tryServeIndexFile(int fd, Server const &server, ConfigParser const *config, string const &decodedUrl, string const &indexPages)
{
	map<string, size_t> indexFile = getIndex(indexPages, decodedUrl);

	if (indexFile.empty())
		return -2; // No index file found

	if (indexFile.begin()->second == 200)
		return serveIndexFile(fd, server, config, decodedUrl, indexFile.begin()->first);

	return sendErrorResponse(fd, indexFile.begin()->second, config, server.getUid());
}

int GetRequest::serveIndexFile(int fd, Server const &server, ConfigParser const *config, string const &decodedUrl, string const &indexFileName)
{
	cout << GREEN << BOLD << "Index files found: " << indexFileName << NEUTRAL << endl;
	string indexFullPath = decodedUrl + indexFileName;
	cout << CYAN << BOLD << "Serving index file: " << NEUTRAL << CYAN << indexFullPath << NEUTRAL << endl;

	if (sendCGIResponse(fd, indexFullPath, config, server) == -1)
		cerr << "Failed to send CGI response" << endl;

	return checkKeepAlive();
}

int GetRequest::handleDirectoryListing(int fd, Server const &server, ConfigParser const *config, string const &decodedUrl, string const &pathForConfig)
{
	string autoindex = config->getLocationValueForPath(pathForConfig, server.getUid(), "autoindex");
	if (autoindex.empty())
		autoindex = "on";

	cout << CYAN << BOLD << "Directory requested: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;

	if (autoindex == "on") {
		string listing = generateDirectoryListing(decodedUrl, _path);
		if (sendHTTPResponse(fd, 200, listing, "text/html") == -1)
			cerr << "Failed to send directory listing" << endl;
	}
	else
		return sendErrorResponse(fd, 403, config, server.getUid());

	return checkKeepAlive();
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

int GetRequest::sendErrorResponse(int fd, int errorCode, ConfigParser const *config, string const &serverUid)
{
	string errorPage = loadErrorPage(errorCode, config, serverUid);

	if (sendHTTPResponse(fd, errorCode, errorPage, "text/html") == -1) {
		cerr << "Failed to send " << errorCode << " response" << endl;
	}

	return checkKeepAlive();
}

int GetRequest::checkKeepAlive()
{
	if (!isKeepalive()) {
		return -1;
	}
	return 0;
}
