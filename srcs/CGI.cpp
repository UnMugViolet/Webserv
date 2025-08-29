#include "CGI.hpp"

CGI::CGI()
{
	return ;
}

CGI::~CGI()
{
	return ;
}

bool	CGI::must_interpret(const std::string &path)
{
	if (path.find("/cgi-bin/") == std::string::npos)
		return (false);
	return (true);
}

std::string	CGI::_getExtension(const std::string &path)
{
	size_t pos = path.rfind('.');
	if (pos == std::string::npos)
		return ("");
	return (path.substr(pos + 1));
}

ScriptType	CGI::_getType(std::string ext)
{
	if (ext == "py")
		return (ScriptType::PYTHON);
	else if (ext == "pl")
		return (ScriptType::PERL);
	else if (ext == "php")
		return (ScriptType::PHP);
	else if (ext == "sh")
		return (ScriptType::SHELL);
	else if (ext == ".cgi")
		return (ScriptType::BINARY);
	else
		return (ScriptType::UNKNOWN);
}

int	CGI::interpret(const std::string &path)
{
	ScriptType type = _getType(_getExtension(path));

	if (type == ScriptType::UNKNOWN)
		throw CGIException("Webserver does not interpret file: " + path);
	int fd[2];
	pid_t	pid;
	pid = fork();
	if (pid == -1)
		;//error
	if (pid == 0)
	{
		std::string interpreter;
		std::string arg[2] = {interpreter, path};
		switch (type)
		{
			case ScriptType::PYTHON :
				execve("py", {"py", path}, NULL);
				break;
			case ScriptType::PERL :
				interpreter = "pl";
				break;
			case ScriptType::PHP :
				interpreter = "php";
				break;
			case ScriptType::SHELL :
				interpreter = "sh";
				break;
			case ScriptType::BINARY :
				interpreter = "./" + path;
				arg[1] = NULL;


		}
				
	}
}