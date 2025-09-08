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
		throw CGIException("Webserver does not interpret file: " + path, false, 415);
	switch (_checkAccess(path, type))
	{
		case -1:
			throw CGIException("file " + path + " does not exist", false, 404);
		case 0:
			throw CGIException("Do not have permission to access :" + path + " on this server", false, 403);
		case 1:
			break;
	}

	if (type == HTML || type == CSS)
	{
		int fd = open(path.c_str(), O_RDONLY);
		if (fd == -1)
			throw CGIException("webserver cannot open file: " + path, false, 500);
		return (fd);
	}
	
	int	fd[2];
	if (pipe(fd) == -1)
		throw CGIException("Internal error: pipe failed", false, 500);
	pid_t	pid;
	pid = fork();
	if (pid == -1)
		throw CGIException("Internal error: fork failed", false, 500);
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
				throw CGIException("Internal error: execve failed", true, 500);
			
		}
		const char *arg[3] = {interpreter.c_str(), cpath, NULL};
		execve(interpreter.c_str(), (char *const *)arg, environ);
        throw CGIException("Internal error: execve failed", true, 500);
	}
	close(fd[1]);
	int status;
    if (waitpid(pid, &status, 0) == -1)
       throw CGIException("Internal error: waitpid failed", true, 500);
    if (WIFEXITED(status))
	{
		if (WEXITSTATUS(status) == 0)
        	return fd[0];
		else
		{
			std::stringstream ss;
			ss << WEXITSTATUS(status);
			throw CGIException("interpreter exited with status " + ss.str() + " please make sure your script if correct", false, 500);
		}
	}
    else
        throw CGIException("Internal error: exit failed", true, 500);
}
