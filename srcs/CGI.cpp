#include "CGI.hpp"

CGI::CGI()
{
}

CGI::~CGI()
{
}

int CGI::_checkAccess(const std::string &path, int type)
{
	if (access(path.c_str(), F_OK) == -1)
		return (-1);
	if (type == BINARY && access(path.c_str(), X_OK) == -1)
		return (0);
	if (access(path.c_str(), R_OK) == -1)
		return (0);
	return (1);
	
}

std::string	CGI::_getExtension(const std::string &path)
{
	size_t pos = path.rfind('.');
	if (pos == std::string::npos)
		return ("");
	return (path.substr(pos + 1));
}

int	CGI::_getType(std::string ext)
{
	if (ext == "py")
		return (PYTHON);
	else if (ext == "pl")
		return (PERL);
	else if (ext == "php")
		return (PHP);
	else if (ext == "sh")
		return (SHELL);
	else if (ext == "cgi")
		return (BINARY);
	else if (ext == "html")
		return (HTML);
	else if (ext == "css")
		return (CSS);
	else
		return (UNKNOWN);
}

int	CGI::interpret(const std::string &path)
{
	int type = _getType(_getExtension(path));

	if (type == UNKNOWN)
		throw CGIException("Webserver does not interpret file: " + path);
	switch (_checkAccess(path, type))
	{
		case -1:
			throw CGIException("File: " + path + " does not exist");
		case 0:
			throw CGIException(("No permission to access file: " + path));
		case 1:
			break;
	}

	if (type == HTML || type == CSS)
	{
		int fd = open(path.c_str(), O_RDONLY);
		if (fd == -1)
			throw CGIException("webserver cannot open file: " + path);
		return (fd);
	}
	
	int	fd[2];
	if (pipe(fd) == -1)
		throw CGIException("Pipe failed");
	pid_t	pid;
	pid = fork();
	if (pid == -1)
		throw CGIException("fork failed");
	if (pid == 0)
	{
		const char	*cpath = path.c_str();
		std::string	interpreter;
		
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);
		switch (type)
		{
			case PYTHON :
				interpreter = "/usr/bin/python3";
				break;
			case PERL :
				interpreter = "/usr/bin/perl";
				break;
			case PHP :
				interpreter = "/usr/bin/php";
				break;
			case SHELL :
				interpreter = "/usr/bin/sh";
				break;
			case BINARY :
				std::string tmp = "./" + path;
				char *arg[2] = {(char *)tmp.c_str(), NULL};
				execve(tmp.c_str(), arg, environ);
				throw CGIException("execve failed", 1);
			
		}
		const char *arg[3] = {interpreter.c_str(), cpath, NULL};
		execve(interpreter.c_str(), (char *const *)arg, environ);
        throw CGIException("execve failed", 1);
	}
	close(fd[1]);
	int status;
    if (waitpid(pid, &status, 0) == -1)
       throw CGIException("waitpid failed");
    if (WIFEXITED(status))
        return fd[0];
    else
        throw CGIException("exit failed");
}