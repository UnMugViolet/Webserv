#include "CGI.hpp"

CGI::CGI() {}

CGI::~CGI() {}

int CGI::_checkAccess(const string &path, int type)
{
	DIR *dir = opendir(path.c_str());

	if (type == BINARY && access(path.c_str(), X_OK) == -1)
		return (0);
	if (access(path.c_str(), R_OK) == -1)
		return (0);
	if (dir != NULL)
		return (closedir(dir), 0);
	if (access(path.c_str(), F_OK) == -1)
		return (-1);
	return (2);
}

string CGI::_getExtension(const string &path)
{
	size_t pos = path.rfind('.');
	if (pos == string::npos)
		return ("");
	return (path.substr(pos));
}

int CGI::_getType(string ext)
{
	if (ext == ".py")
		return (PYTHON);
	else if (ext == ".pl")
		return (PERL);
	else if (ext == ".php")
		return (PHP);
	else if (ext == ".sh")
		return (SHELL);
	else if (ext == ".js")
		return (JS);
	else if (ext == ".cgi")
		return (BINARY);
	else if (ext == ".html")
		return (HTML);
	else if (ext == ".css")
		return (CSS);
	else if (ext == ".mp3")
		return (MP3);
	else if (ext == ".png")
		return (PNG);
	else if (ext == ".jpg")
		return (JPG);
	else if (ext == ".jpeg")
		return (JPEG);
	else if (ext == ".gif")
		return (GIF);
	else if (ext == ".ico")
		return (ICO);
	else
		return (UNKNOWN);
}

int CGI::interpret(const string &path, Server &Server, map<string, string> &cgi_list)
{
	string extension = _getExtension(path);
	int type = _getType(extension);

	switch (_checkAccess(path, type)) {
		case -1:
			throw(CGIException("file " + path + " does not exist", false, 404, Server.getUid()));
		case 0:
			throw(CGIException("Do not have permission to access :" + path + " on this server", false, 403, Server.getUid()));
		case 1:
			break;
		case 2:
			break;
	}

	if (cgi_list.find(extension) == cgi_list.end())
	{
		int fd = open(path.c_str(), O_RDONLY);
		if (fd == -1)
			throw CGIException("webserver cannot open file: " + path, false, 500, Server.getUid());
		Server.setPidforCgi(fd, 0);
		return (fd);
	}

	int fd[2];
	if (pipe(fd) == -1)
		throw(CGIException("Internal error: pipe failed", false, 500, Server.getUid()));
	pid_t pid;
	pid = fork();
	if (pid == -1)
		throw(CGIException("Internal error: fork failed", false, 500, Server.getUid()));
	if (pid == 0)
	{
		string basename = path;
		if (path.rfind('/') != string::npos)
		{
			basename = path.substr(path.rfind('/') + 1);
			string directory = path.substr(0, path.rfind('/'));

			if (chdir(directory.c_str()) == -1)
				throw(CGIException("Error: access denied", true, 403, Server.getUid()));
		}
		const char *cpath = basename.c_str();
		string interpreter = cgi_list.find(extension)->second;

		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		dup2(fd[1], STDERR_FILENO); // Redirect stderr to the pipe as well to get the error output on the client side
		close(fd[1]);
		if (type == BINARY)
		{
			string tmp = "./" + basename;
			char *arg[2] = {(char *)tmp.c_str(), NULL};
			execve(tmp.c_str(), arg, Server.getEnvAsArray());
			throw(CGIException("Internal error: execve failed", true, 500, Server.getUid()));
		}

		const char *arg[3] = {interpreter.c_str(), cpath, NULL};

		execve(interpreter.c_str(), (char *const *)arg, Server.getEnvAsArray());
		throw(CGIException("Internal error: execve failed", true, 500, Server.getUid()));
	}
	Server.setPidforCgi(fd[0], pid);
	close(fd[1]);
	return (fd[0]);
}
