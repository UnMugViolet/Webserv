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
int	GetRequest::handleGet(int fd, const Server &server, const ConfigParser *config, const string &fullPath)
{
	string decodedUrl = urlDecode(fullPath.c_str());
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
	string indexFile;

	cout << "Index file " << indexFile << endl;

	// Default autoindex to "on" if not explicitly set
	if (autoindex.empty())
		autoindex = "on";

	if (isDirectory) {
		if (!indexPages.empty())
			indexFile = getIndex(indexPages, fullPath);

		// If index file is found, try to serve it
		if (!indexFile.empty()) {
			string indexPath = fullPath;
			if (indexFile[0] == '/')
				indexPath += indexFile;
			else
				indexPath += "/" + indexFile;
			string decodedIndexPath = urlDecode(indexPath.c_str());
			
			// Check if index file exists and is accessible
			if (access(decodedIndexPath.c_str(), F_OK) == 0) {
				if (access(decodedIndexPath.c_str(), R_OK) == 0) {
					// Index file accessible - serve it
					cout << CYAN << BOLD << "Serving index file: " << NEUTRAL << CYAN << decodedIndexPath << NEUTRAL << endl;
					if (sendCGIResponse(fd, decodedIndexPath, config, server) == -1)
						cerr << "Failed to send CGI response" << endl;
				} else {
					string errorPage = loadErrorPage(403, config, server.getUid());
					if (sendHTTPResponse(fd, 403, errorPage, "text/html") == -1)
						cerr << "Failed to send 403 response" << endl;
				}
			} else {
				string errorPage = loadErrorPage(404, config, server.getUid());
				if (sendHTTPResponse(fd, 404, errorPage, "text/html") == -1)
					cerr << "Failed to send 404 response" << endl;
			}
		} else {
			// No index file found or none accessible - check autoindex
			if (autoindex == "on") {
				// Serve directory listing
				cout << CYAN << BOLD << "Serving directory listing for: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;
				if (sendCGIResponse(fd, decodedUrl, config, server) == -1)
					cerr << "Failed to send CGI response" << endl;
			} else {
				string errorPage = loadErrorPage(403, config, server.getUid());
				if (sendHTTPResponse(fd, 403, errorPage, "text/html") == -1)
					cerr << "Failed to send 403 response" << endl;
			}
		}
	}
	else if (isFile) {
		// Check if file is accessible
		if (access(decodedUrl.c_str(), R_OK) == 0) {
			// File accessible - serve it
			cout << CYAN << BOLD << "File requested: " << NEUTRAL << CYAN << decodedUrl << NEUTRAL << endl;
			if (sendCGIResponse(fd, decodedUrl, config, server) == -1)
				cerr << "Failed to send CGI response" << endl;
		} else {
			string errorPage = loadErrorPage(403, config, server.getUid());
			if (sendHTTPResponse(fd, 403, errorPage, "text/html") == -1)
				cerr << "Failed to send 403 response" << endl;
		}
	}
	// Neither file nor directory exists
	else {
		string errorPage = loadErrorPage(404, config, server.getUid());
		if (sendHTTPResponse(fd, 404, errorPage, "text/html") == -1)
			cerr << "Failed to send 404 response" << endl;
	}

	if (!isKeepalive())
		return (-1);
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
	return ;
}
