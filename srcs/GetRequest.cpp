#include "GetRequest.hpp"

GetRequest::GetRequest()
{
	return ;
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
	return ;
}

GetRequest&	GetRequest::operator=(GetRequest &src)
{
	if (this != &src)
		ARequest::operator=(src);
	return (*this);
}

GetRequest::~GetRequest()
{
	return ;
}

GetRequest::GetRequest(GetRequest &src) : ARequest(src)
{
	return ;
}
int	GetRequest::handleGet(int fd, const Server &server, const ConfigParser *config, const string &fullPath)
{
    string decodedUrl = urlDecode(fullPath.c_str());
    PathType pathType = getPathType(decodedUrl);
    
    if (pathType == PATH_NOT_EXISTS) {
        return sendErrorResponse(fd, 404, config, server.getUid());
    }
    
    if (pathType == PATH_DIRECTORY) {
        return handleDirectory(fd, server, config, decodedUrl);
    }
    
    if (pathType == PATH_FILE) {
        return handleFile(fd, server, config, decodedUrl);
    }
    
    return sendErrorResponse(fd, 500, config, server.getUid());
}

GetRequest::PathType GetRequest::getPathType(const string &path)
{
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) != 0) {
        return PATH_NOT_EXISTS;
    }
    
    if (S_ISDIR(pathStat.st_mode)) {
        return PATH_DIRECTORY;
    }
    
    if (S_ISREG(pathStat.st_mode)) {
        return PATH_FILE;
    }
    
    return PATH_NOT_EXISTS;
}

int GetRequest::handleDirectory(int fd, const Server &server, const ConfigParser *config, const string &decodedUrl)
{
    string pathForConfig = getPathForConfig(decodedUrl);
    string indexPages = config->getLocationValueForPath(pathForConfig, server.getUid(), "index");
    
    // Try to serve index file first
    if (!indexPages.empty()) {
        int result = tryServeIndexFile(fd, server, config, decodedUrl, indexPages);
        if (result != -2) { // -2 means no index file found, continue with directory listing
            return result;
        }
    }
    
    // No index file, try directory listing
    return handleDirectoryListing(fd, server, config, decodedUrl, pathForConfig);
}

int GetRequest::handleFile(int fd, const Server &server, const ConfigParser *config, const string &decodedUrl)
{
    if (access(decodedUrl.c_str(), R_OK) != 0) {
        return sendErrorResponse(fd, 403, config, server.getUid());
    }
    
    cout << CYAN << BOLD << "File requested: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;
    if (sendCGIResponse(fd, decodedUrl, config, server) == -1) {
        cerr << "Failed to send CGI response" << endl;
    }
    
    return checkKeepAlive();
}

int GetRequest::tryServeIndexFile(int fd, const Server &server, const ConfigParser *config, 
                                  const string &decodedUrl, const string &indexPages)
{
    map<string, size_t> indexFile = getIndex(indexPages, decodedUrl);
    
    if (indexFile.empty()) {
        return -2; // No index file found
    }
    
    if (indexFile.begin()->second == 200) {
        return serveIndexFile(fd, server, config, decodedUrl, indexFile.begin()->first);
    }
    
    return sendErrorResponse(fd, indexFile.begin()->second, config, server.getUid());
}

int GetRequest::serveIndexFile(int fd, const Server &server, const ConfigParser *config,
                               const string &decodedUrl, const string &indexFileName)
{
    cout << GREEN << BOLD << "Index files found: " << indexFileName << NEUTRAL << endl;
    string indexFullPath = decodedUrl + indexFileName;
    cout << CYAN << BOLD << "Serving index file: " << NEUTRAL << CYAN << indexFullPath << NEUTRAL << endl;
    
    if (sendCGIResponse(fd, indexFullPath, config, server) == -1) {
        cerr << "Failed to send CGI response" << endl;
    }
    
    return checkKeepAlive();
}

int GetRequest::handleDirectoryListing(int fd, const Server &server, const ConfigParser *config,
                                       const string &decodedUrl, const string &pathForConfig)
{
    string autoindex = config->getLocationValueForPath(pathForConfig, server.getUid(), "autoindex");
    if (autoindex.empty()) {
        autoindex = "on";
    }
    
    cout << CYAN << BOLD << "Directory requested: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;
    
    if (autoindex == "on") {
        string listing = generateDirectoryListing(decodedUrl, _path);
        if (sendHTTPResponse(fd, 200, listing, "text/html") == -1) {
            cerr << "Failed to send directory listing" << endl;
        }
    } else {
        return sendErrorResponse(fd, 403, config, server.getUid());
    }
    
    return checkKeepAlive();
}

string GetRequest::getPathForConfig(const string &decodedUrl)
{
    string pathForConfig = _path;
    if (getPathType(decodedUrl) == PATH_DIRECTORY && 
        !_path.empty() && 
        _path[_path.length() - 1] != '/') {
        pathForConfig += "/";
    }
    return pathForConfig;
}

int GetRequest::sendErrorResponse(int fd, int errorCode, const ConfigParser *config, const string &serverUid)
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
        return (-1);
    }
    return (0);
}

