#include "ConfigParser.hpp"

ConfigParser::ConfigParser()
{
	_configMap.clear();
	_serverBlocks.clear();
	_locationBlocks.clear();
}

ConfigParser::ConfigParser(const string &filePath)
{
	parseFile(filePath);
}

ConfigParser::ConfigParser(const ConfigParser &other)
{
	_configMap = other._configMap;
	_serverBlocks = other._serverBlocks;
	_locationBlocks = other._locationBlocks;
}

ConfigParser &ConfigParser::operator=(const ConfigParser &other)
{
	if (this != &other)
	{
		_configMap = other._configMap;
		_serverBlocks = other._serverBlocks;
		_locationBlocks = other._locationBlocks;
	}
	return *this;
}

ConfigParser::~ConfigParser()
{
	_locationBlocks.clear();
	_serverBlocks.clear();
	_configMap.clear();
}

void ConfigParser::parseFile(const string &filePath)
{
	_checkSemicolons(filePath);
	ifstream file(filePath.c_str());
	if (!file.is_open())
	{
		throw ErrorException("Cannot open config file: " + filePath);
	}

	string line;
	int lineNumber = 0;
	
	while (getline(file, line))
	{
		lineNumber++;
		
		// Remove leading and trailing whitespace
		line = _trim(line);
		
		// Skip empty lines and comments
		if (line.empty() || line[0] == '#')
			continue;
		
		// Check if this is a server block
		if (line.find("server") == 0 && line.find("{") != string::npos)
		{
			// Create a default server name
			string serverUid = "server_" + ft_itos(_serverBlocks.size());
			_parseServerBlock(file, serverUid);
		}
		else
		{
			// Parse global configuration (outside server blocks)
			size_t pos = line.find(' ');
			if (pos == string::npos || pos + 1 >= line.length())
				continue;
			
			string key = _trim(line.substr(0, pos));
			if (key == "error_page")
			{
				// Find the error code (skip any extra spaces)
				size_t codeStart = pos + 1;
				while (codeStart < line.length() && line[codeStart] == ' ')
					codeStart++;
				
				size_t codeEnd = line.find(' ', codeStart);
				if (codeEnd != string::npos && codeEnd + 1 < line.length())
				{
					string errorCode = _trim(line.substr(codeStart, codeEnd - codeStart));
					key += " " + errorCode;
					pos = codeEnd;
				}
			}
			string value = _trim(line.substr(pos + 1));
			
			// Remove semicolon if present
			if (!value.empty() && value[value.length() - 1] == ';')
				value = value.substr(0, value.length() - 1);
			
			_configMap[key] = value;
		}
	}
	file.close();
}

void ConfigParser::_parseServerBlock(ifstream &file, const string &serverName)
{
	string line;
	map<string, string> serverConfig;
	bool 		rootSet = false;
	
	while (getline(file, line))
	{
		line = _trim(line);
		
		// Skip empty lines and comments
		if (line.empty() || line[0] == '#')
			continue;
		
		// Check for end of server block
		if (line == "}")
			break;
		
		// Check for location block
		if (line.find("location") == 0 && line.find("{") != string::npos)
		{
			// Extract location path
			size_t start = line.find(' ');
			size_t end = line.find('{');
			if (start != string::npos && end != string::npos)
			{
				string location = _trim(line.substr(start, end - start));
				_parseLocationBlock(file, serverName, location);
			}
			continue;
		}

		// Checks if line is ended by a semicolon, if it does, format the line, if it doesn't, ignores the line
		if (line[line.length() - 1] != ';')
			continue;
		line = _formatLine(line);
		
		// Parse server directive
		size_t pos = line.find(' ');
		if (pos != string::npos && pos + 1 < line.length())
		{
			string key = _trim(line.substr(0, pos));
			string value = _trim(line.substr(pos + 1));
			
			if (key == "root")
				rootSet = true;
			if (key == "error_page")
			{
				size_t secondSpace = value.find(' ');
				if (secondSpace != string::npos && secondSpace + 1 < value.length())
				{
					string errorCode = _trim(value.substr(0, secondSpace));
					key += " " + errorCode;
					value = _trim(value.substr(secondSpace + 1));
				}
			}
			
			serverConfig[key] = value;
		}
	}
	if (!rootSet)
		throw ErrorException("Server block missing 'root' directive for " + serverName);
	_serverBlocks[serverName] = serverConfig;
}

void ConfigParser::_parseLocationBlock(ifstream &file, const string &serverName, const string &location)
{
	string line;
	
	while (getline(file, line))
	{
		line = _trim(line);
		
		// Skip empty lines and comments
		if (line.empty() || line[0] == '#')
			continue;
		
		// Check for end of location block
		if (line == "}")
			break;
		
		// Checks if line is ended by a semicolon, if it does, format the line, if it doesn't, ignores the line
		if (line[line.length() - 1] != ';')
			continue;
		line = _formatLine(line);

		// Parse location directive
		size_t pos = line.find(' ');
		if (pos != string::npos && pos + 1 < line.length())
		{
			string key = _trim(line.substr(0, pos));
			string value = _trim(line.substr(pos + 1));
			
			// Store location-specific config
			_locationBlocks[serverName][location][key] = value;
		}
	}
}

string ConfigParser::getValue(const string &key) const
{
	map<string, string>::const_iterator it = _configMap.find(key);
	if (it != _configMap.end())
		return it->second;
	return "";
}

/**
 * Retrieving server-specific configuration values.
 * @param serverName The unique identifier of the server.
 * @param key The configuration parameter to retrieve.
 * @return The value of the specified parameter for the given server,
 *         or an empty string if the server or parameter is not found.
 */
string ConfigParser::getServerValue(const string &serverName, const string &key) const
{
	map<string, map<string, string> >::const_iterator serverIt = _serverBlocks.find(serverName);
	if (serverIt != _serverBlocks.end())
	{
		map<string, string>::const_iterator keyIt = serverIt->second.find(key);
		if (keyIt != serverIt->second.end())
			return keyIt->second;
	}
	return "";
}

/**
 * Retrieving location-specific configuration values for any given key.
 * @param serverName The unique identifier of the server.
 * @param location The path of the location block.
 * @param key The configuration parameter to retrieve.
 * @return The value of the specified parameter for the given location block,
 *         or an empty string if the server, location, or parameter is not found.
 */
string ConfigParser::getLocationValue(const string &serverName, const string &location, const string &key) const
{
	map<string, map<string, map<string, string> > >::const_iterator serverIt = _locationBlocks.find(serverName);
	if (serverIt != _locationBlocks.end())
	{
		map<string, map<string, string> >::const_iterator locationIt = serverIt->second.find(location);
		if (locationIt != serverIt->second.end())
		{
			map<string, string>::const_iterator keyIt = locationIt->second.find(key);
			if (keyIt != locationIt->second.end())
				return keyIt->second;
		}
	}
	return "";
}

/**
 * Check if a specific key exists in a given server block.
 * @param serverName The unique identifier of the server.
 * @param key The configuration parameter to check.
 * @return True if the key exists in the specified server block, false otherwise.
 */
bool ConfigParser::hasServerKey(const string &serverName, const string &key) const
{
	map<string, map<string, string> >::const_iterator serverIt = _serverBlocks.find(serverName);
	if (serverIt != _serverBlocks.end())
	{
		return serverIt->second.find(key) != serverIt->second.end();
	}
	return false;
}

/**
 * Retrieving the uids of a all of the server blocks.
 * @return A vector containing all server unique identifiers.
 */
vector<string> ConfigParser::getServerUids() const
{
	vector<string> names;
	for (map<string, map<string, string> >::const_iterator it = _serverBlocks.begin();
		 it != _serverBlocks.end(); ++it)
	{
		names.push_back(it->first);
	}
	return names;
}

/**
 * Retrieving the paths of all location blocks for a given server.
 * @param serverUid The unique identifier of the server.
 * @return A vector containing all location paths for the specified server.
*/
vector<string> ConfigParser::getLocationPaths(const string &serverUid) const
{
	vector<string> paths;
	map<string, map<string, map<string, string> > >::const_iterator serverIt = _locationBlocks.find(serverUid);
	if (serverIt != _locationBlocks.end())
	{
		for (map<string, map<string, string> >::const_iterator locationIt = serverIt->second.begin();
			 locationIt != serverIt->second.end(); ++locationIt)
		{
			paths.push_back(locationIt->first);
		}
	}
	return paths;
}

void ConfigParser::printConfig() const
{
	cout << YELLOW BOLD << "=== Global Configuration ===" << NEUTRAL << endl;
	for (map<string, string>::const_iterator it = _configMap.begin();
		 it != _configMap.end(); ++it)
	{
		cout << GREEN << it->first << NEUTRAL << " = " 
				  << CYAN << it->second << NEUTRAL << endl;
	}
	
	cout << YELLOW BOLD << "\n=== Server Blocks ===" << NEUTRAL << endl;
	for (map<string, map<string, string> >::const_iterator serverIt = _serverBlocks.begin();
		 serverIt != _serverBlocks.end(); ++serverIt)
	{
		cout << RED BOLD << "\n[" << serverIt->first << "]" << NEUTRAL << endl;
		for (map<string, string>::const_iterator keyIt = serverIt->second.begin();
			 keyIt != serverIt->second.end(); ++keyIt)
		{
			cout << "  " << GREEN << keyIt->first << NEUTRAL << " = " 
					  << CYAN << keyIt->second << NEUTRAL << endl;
		}
		// Print associated location blocks
		map<string, map<string, map<string, string> > >::const_iterator locIt = _locationBlocks.find(serverIt->first);
		if (locIt != _locationBlocks.end())
		{
			for (map<string, map<string, string> >::const_iterator locationIt = locIt->second.begin();
				 locationIt != locIt->second.end(); ++locationIt)
			{
				cout << BLUE BOLD << "  [location " << locationIt->first << "]" << NEUTRAL << endl;
				for (map<string, string>::const_iterator locKeyIt = locationIt->second.begin();
					 locKeyIt != locationIt->second.end(); ++locKeyIt)
				{
					cout << "    " << GREEN << locKeyIt->first << NEUTRAL << " = " 
							  << CYAN << locKeyIt->second << NEUTRAL << endl;
				}
			}
		}
	}
	cout << YELLOW BOLD << "===================" << NEUTRAL << endl;
}

string ConfigParser::_trim(const string &str) const
{
	size_t start = str.find_first_not_of(" \t\r\n");
	if (start == string::npos)
		return "";
	
	size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

string ConfigParser::getErrorPageContent(ConfigParser &parser, const string &serverUid, unsigned int error_code) const
{
	ifstream file;
	ostringstream oss;
	oss << error_code;
	string error_code_str = oss.str();
	string context_path = parser.getServerValue(serverUid, "root");

	if (context_path.empty() || context_path[context_path.length() - 1] != '/')
		context_path += '/';

	// Priority 1: Check specific server error page first
	if (parser.hasServerKey(serverUid, "error_page " + error_code_str))
	{
		string serverErrorPage = parser.getServerValue(serverUid, "error_page " + error_code_str);
		if (serverErrorPage[0] == '/')
			serverErrorPage = serverErrorPage.substr(1);
		string relative_path = context_path + serverErrorPage;

		file.open(relative_path.c_str());
		if (!file.is_open())
			file.open(serverErrorPage.c_str());
		if (file.is_open())
		{
			stringstream buffer;
			buffer << file.rdbuf();
			file.close();
			return buffer.str();
		}
	}

	// Priority 2: Check global configuration error pages
	string globalErrorKey = "error_page " + error_code_str;
	map<string, string>::const_iterator it = parser._configMap.find(globalErrorKey);

	if (it != parser._configMap.end() && !it->second.empty())
	{
		file.open(it->second.c_str());
		if (file.is_open())
		{
			stringstream buffer;
			buffer << file.rdbuf();
			file.close();
			return buffer.str();
		}
	}

	// Priority 3: Use default error pages
	string defaultPath = DEFAULT_ERROR_PAGES_PATH + error_code_str + ".html";
	file.open(defaultPath.c_str());
	if (file.is_open())
	{
		stringstream buffer;
		buffer << file.rdbuf();
		file.close();
		return buffer.str();
	}

	// Final fallback: return basic HTML error message
	cout << string(RED) << "No error page found, using fallback HTML for the code: " << error_code_str << string(NEUTRAL) << endl;
	return "<html><body><h1>Error " + error_code_str + "</h1><p>An undefined error occurred.</p></body></html>";
}

/* Format the line suppressing all unnecessaries whitespaces and the last semicolon if found
*  @param string
*  @return string
*/
string ConfigParser::_formatLine(const string &str) const
{
	string line = str;
	if (line[line.length() - 1] == ';')
		line = line.substr(0, line.length() - 1);
	for (size_t i = 0; i < line.length() ; i++)
		if (line[i] == '\t')
			line[i] = ' ';
	for (size_t i = 0; i < line.length(); i++)
		while (i < line.length() - 1 && line[i] == ' ' && line[i + 1] == ' ')
			line = line.erase(i, 1);
	return (line);
}

void	ConfigParser::_checkSemicolons(const string &filePath) const
{
	ifstream file(filePath.c_str());
	if (!file.is_open())
	{
		throw ErrorException("Cannot open config file: " + filePath);
	}
	string line;
	int i = 0;
	while (getline(file, line)) {
		for (size_t j = 0; line[j]; j++) {
			if (line[j] == '{')
				i++;
			if (line[j] == '}')
				i--;
		}
	}
	if (i != 0)
	{
		throw ErrorException("Block error in config file: " + filePath);
	}
}
/**
 * Checks the most specific location block that matches the given path for a server,
 * and retrieves the value of the specified parameter from that location block.
 * @param path The request path to match against location blocks.
 * @param serverUid The unique identifier of the server.
 * @param parameter The configuration parameter to retrieve from the matched location block.
 * @return The value of the specified parameter from the most specific matching location block,
 *         fallback to the server value if nothing is defined in the location. If neither
 *         location nor server define the parameter, returns empty string.
*/
string	ConfigParser::getLocationValueForPath(const string &path, const string &serverUid, const string &parameter) const
{
	string currentLocation = "";
	string values = "";
	vector<string> temp = getLocationPaths(serverUid);
	vector<string>::iterator it = temp.begin();

	for (; it != temp.end(); it++) {
		if (path.find((*it)) != string::npos) {
			if (currentLocation.size() < (*it).size())
				currentLocation = *it;
		}
	}
	values = getLocationValue(serverUid, currentLocation, parameter);
	if (values.empty())
		values = getServerValue(serverUid, parameter);
	if (values.empty())
		cout << YELLOW << "Parameter '" << parameter << "' not found in location '" << currentLocation << "' or server '" << serverUid << "'" << NEUTRAL << endl;
	return (values);
}

vector<string> ConfigParser::getLocationVectorforPath(const string &path, const string &serverUid, const string &parameter) const
{
	vector<string> result;
	string currentLocation = "";
	vector<string> temp = getLocationPaths(serverUid);

	int status = 0;
	vector<string>::iterator it = temp.begin();
	for (; it != temp.end(); it++) {
		if (path.find((*it)) != string::npos) {
			status = 1;
			if (currentLocation.size() < (*it).size())
				currentLocation = *it;
		}
	}
	if (status == 0)
		throw ErrorException("No valid location");
	map<string, map<string, map<string, string> > >::const_iterator serverIt = _locationBlocks.find(serverUid);
	if (serverIt != _locationBlocks.end())
	{
		map<string, map<string, string> >::const_iterator locationIt = serverIt->second.find(currentLocation);
		if (locationIt != serverIt->second.end())
		{
			for (map<string, string>::const_iterator keyIt = locationIt->second.find(parameter); keyIt != locationIt->second.end(); keyIt++)
			{
				if (keyIt->first == parameter)
				{
					result.push_back(keyIt->second);
				}
			}
		}
	}
	return (result);
}