/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unmugviolet <unmugviolet@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 18:44:09 by unmugviolet       #+#    #+#             */
/*   Updated: 2025/08/28 18:44:49 by unmugviolet      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "dict.hpp"

ConfigParser::ConfigParser()
{
}

ConfigParser::ConfigParser(const std::string &filePath)
{
	parseFile(filePath);
}

ConfigParser::~ConfigParser()
{
}

void ConfigParser::parseFile(const std::string &filePath)
{
	std::ifstream file(filePath.c_str());
	if (!file.is_open())
	{
		throw ErrorException("Cannot open config file: " + filePath);
	}

	std::string line;
	int lineNumber = 0;
	
	while (std::getline(file, line))
	{
		lineNumber++;
		
		// Remove leading and trailing whitespace
		line = _trim(line);
		
		// Skip empty lines and comments
		if (line.empty() || line[0] == '#')
			continue;
		
		// Check if this is a server block
		if (line.find("server") == 0 && line.find("{") != std::string::npos)
		{
			// Create a default server name
			std::string serverUid = "server_" + _intToString(_serverBlocks.size());
			_parseServerBlock(file, serverUid);
		}
		else
		{
			// Parse global configuration (outside server blocks)
			size_t pos = line.find(' ');
			if (pos == std::string::npos)
				continue;
			
			std::string key = _trim(line.substr(0, pos));
			if (key == "error_page")
			{
				// Find the error code (skip any extra spaces)
				size_t codeStart = pos + 1;
				while (codeStart < line.length() && line[codeStart] == ' ')
					codeStart++;
				
				size_t codeEnd = line.find(' ', codeStart);
				if (codeEnd != std::string::npos)
				{
					std::string errorCode = _trim(line.substr(codeStart, codeEnd - codeStart));
					key += " " + errorCode;
					pos = codeEnd;
				}
			}
			std::string value = _trim(line.substr(pos + 1));
			
			// Remove semicolon if present
			if (!value.empty() && value[value.length() - 1] == ';')
				value = value.substr(0, value.length() - 1);
			
			_configMap[key] = value;
		}
	}
	file.close();
}

void ConfigParser::_parseServerBlock(std::ifstream &file, const std::string &serverName)
{
	std::string line;
	std::map<std::string, std::string> serverConfig;
	
	while (std::getline(file, line))
	{
		line = _trim(line);
		
		// Skip empty lines and comments
		if (line.empty() || line[0] == '#')
			continue;
		
		// Check for end of server block
		if (line == "}")
			break;
		
		// Check for location block
		if (line.find("location") == 0 && line.find("{") != std::string::npos)
		{
			// Extract location path
			size_t start = line.find(' ');
			size_t end = line.find('{');
			if (start != std::string::npos && end != std::string::npos)
			{
				std::string location = _trim(line.substr(start, end - start));
				_parseLocationBlock(file, serverName, location);
			}
			continue;
		}
		
		// Parse server directive
		size_t pos = line.find(' ');
		if (pos != std::string::npos)
		{
			std::string key = _trim(line.substr(0, pos));
			std::string value = _trim(line.substr(pos + 1));
			
			if (key == "error_page")
			{
				size_t secondSpace = value.find(' ');
				if (secondSpace != std::string::npos)
				{
					std::string errorCode = _trim(value.substr(0, secondSpace));
					key += " " + errorCode;
					value = _trim(value.substr(secondSpace + 1));
				}
			}
			
			// Remove semicolon if present
			if (!value.empty() && value[value.length() - 1] == ';')
				value = value.substr(0, value.length() - 1);
			
			serverConfig[key] = value;
		}
	}
	
	_serverBlocks[serverName] = serverConfig;
}

void ConfigParser::_parseLocationBlock(std::ifstream &file, const std::string &serverName, const std::string &location)
{
	std::string line;
	
	while (std::getline(file, line))
	{
		line = _trim(line);
		
		// Skip empty lines and comments
		if (line.empty() || line[0] == '#')
			continue;
		
		// Check for end of location block
		if (line == "}")
			break;
		
		// Parse location directive
		size_t pos = line.find(' ');
		if (pos != std::string::npos)
		{
			std::string key = _trim(line.substr(0, pos));
			std::string value = _trim(line.substr(pos + 1));
			
			// Remove semicolon if present
			if (!value.empty() && value[value.length() - 1] == ';')
				value = value.substr(0, value.length() - 1);
			
			// Store location-specific config with prefix
			std::string locationKey = "location_" + location + "_" + key;
			_serverBlocks[serverName][locationKey] = value;
		}
	}
}

std::string ConfigParser::getValue(const std::string &key) const
{
	std::map<std::string, std::string>::const_iterator it = _configMap.find(key);
	if (it != _configMap.end())
		return it->second;
	return "";
}

std::string ConfigParser::getServerValue(const std::string &serverName, const std::string &key) const
{
	std::map<std::string, std::map<std::string, std::string> >::const_iterator serverIt = _serverBlocks.find(serverName);
	if (serverIt != _serverBlocks.end())
	{
		std::map<std::string, std::string>::const_iterator keyIt = serverIt->second.find(key);
		if (keyIt != serverIt->second.end())
			return keyIt->second;
	}
	return "";
}

bool ConfigParser::hasKey(const std::string &key) const
{
	return _configMap.find(key) != _configMap.end();
}

bool ConfigParser::hasServerKey(const std::string &serverName, const std::string &key) const
{
	std::map<std::string, std::map<std::string, std::string> >::const_iterator serverIt = _serverBlocks.find(serverName);
	if (serverIt != _serverBlocks.end())
	{
		return serverIt->second.find(key) != serverIt->second.end();
	}
	return false;
}

std::vector<std::string> ConfigParser::getServerIds() const
{
	std::vector<std::string> names;
	for (std::map<std::string, std::map<std::string, std::string> >::const_iterator it = _serverBlocks.begin();
		 it != _serverBlocks.end(); ++it)
	{
		names.push_back(it->first);
	}
	return names;
}

void ConfigParser::printConfig() const
{
	std::cout << YELLOW BOLD << "=== Global Configuration ===" << NEUTRAL << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = _configMap.begin();
		 it != _configMap.end(); ++it)
	{
		std::cout << GREEN << it->first << NEUTRAL << " = " 
				  << CYAN << it->second << NEUTRAL << std::endl;
	}
	
	std::cout << YELLOW BOLD << "\n=== Server Blocks ===" << NEUTRAL << std::endl;
	for (std::map<std::string, std::map<std::string, std::string> >::const_iterator serverIt = _serverBlocks.begin();
		 serverIt != _serverBlocks.end(); ++serverIt)
	{
		std::cout << RED BOLD << "\n[" << serverIt->first << "]" << NEUTRAL << std::endl;
		for (std::map<std::string, std::string>::const_iterator keyIt = serverIt->second.begin();
			 keyIt != serverIt->second.end(); ++keyIt)
		{
			std::cout << "  " << GREEN << keyIt->first << NEUTRAL << " = " 
					  << CYAN << keyIt->second << NEUTRAL << std::endl;
		}
	}
	std::cout << YELLOW BOLD << "===================" << NEUTRAL << std::endl;
}

std::string ConfigParser::_trim(const std::string &str) const
{
	size_t start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	
	size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

std::string ConfigParser::_intToString(int num) const
{
	std::ostringstream oss;
	oss << num;
	return oss.str();
}

std::string ConfigParser::getErrorPageContent(ConfigParser &parser, const std::string &serverId, unsigned int error_code) const
{
	std::ifstream file;
	std::ostringstream oss;
	oss << error_code;
	std::string error_code_str = oss.str();

	// Priority 1: Check specific server error page first
	if (parser.hasServerKey(serverId, "error_page " + error_code_str))
	{
		std::string serverErrorPage = parser.getServerValue(serverId, "error_page " + error_code_str);

		std::cout << "serverErrorPage: " << serverErrorPage << std::endl;

		file.open(serverErrorPage.c_str());
		if (file.is_open())
		{
			std::stringstream buffer;
			buffer << file.rdbuf();
			file.close();
			return buffer.str();
		}
	}

	// Priority 2: Check global configuration error pages
	std::map<std::string, std::string> globalConf = parser._configMap;
	std::string globalErrorKey = "error_page " + error_code_str;
	std::map<std::string, std::string>::const_iterator it = globalConf.find(globalErrorKey);
	if (it != globalConf.end() && !it->second.empty())
	{
		file.open(it->second.c_str());
		if (file.is_open())
		{
			std::stringstream buffer;
			buffer << file.rdbuf();
			file.close();
			return buffer.str();
		}
	}

	// Priority 3: Use default error pages
	std::string defaultPath = DEFAULT_ERROR_PAGES_PATH + error_code_str + ".html";
	file.open(defaultPath.c_str());
	if (file.is_open())
	{
		std::stringstream buffer;
		buffer << file.rdbuf();
		file.close();
		return buffer.str();
	}

	// Final fallback: return basic HTML error message
	std::cout << "No error page found, using fallback HTML for the code: " << error_code_str << std::endl;
	return "<html><body><h1>Error " + error_code_str + "</h1><p>An error occurred.</p></body></html>";
}
