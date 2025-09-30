/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unmugviolet <unmugviolet@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 11:49:10 by yguinio           #+#    #+#             */
/*   Updated: 2025/09/30 11:46:43 by unmugviolet      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Logger.hpp"

std::ofstream Logger::_accessLogStream;
std::ofstream Logger::_errorLogStream;
std::string Logger::_accessFile;
std::string Logger::_errorFile;

Logger::Logger(ConfigParser &config)
{
	_accessFile = config.getValue("access_log");
	_errorFile = config.getValue("error_log");

	// Creating Access file if nothing provided fallback to default
	if (_accessFile.empty())
		_accessFile = DEFAULT_ACCESS_LOG_FILE;
	
	// Creating Error file same way as Access
	if (_errorFile.empty())
		_errorFile = DEFAULT_ERROR_LOG_FILE;
	
	_accessFile = LOG_FOLDER_PATH + _accessFile;
	_errorFile = LOG_FOLDER_PATH + _errorFile;

	_accessLogStream.open(_accessFile.c_str(), std::ios::app);
	_errorLogStream.open(_errorFile.c_str(), std::ios::app);
}

Logger::~Logger()
{
	if (_accessLogStream.is_open())
		_accessLogStream.close();
	if (_errorLogStream.is_open())
		_errorLogStream.close();
}

void	Logger::init()
{
	std::ofstream a(_accessFile.c_str(), std::ios::trunc);
	std::ofstream e(_errorFile.c_str(), std::ios::trunc);
	
	if (_accessLogStream.is_open())
        _accessLogStream.close();
    _accessLogStream.open(_accessFile.c_str(), std::ios::app);

    if (_errorLogStream.is_open())
        _errorLogStream.close();
    _errorLogStream.open(_errorFile.c_str(), std::ios::app);
}

void	Logger::info(const std::string &msg)
{
	if (msg.empty())
		return ;
	std::cout << BOLD << "[INFO] " << NEUTRAL << msg  << std::endl;
}

void	Logger::access(const std::string &serverUid, const std::string &msg)
{
	if (!_accessLogStream.is_open())
        std::cerr << "Logger: _accessLogStream not open!" << std::endl;
	else {
		// Check if rotation is needed before writing
		if (countLines(_accessFile) >= MAX_LOG_LINES) {
			rotateLogFile(_accessFile, _accessLogStream);
		}
		_accessLogStream << "[" << serverUid << "]\n" << msg << std::endl << std::endl;
		_accessLogStream.flush();
	}
}

void	Logger::error(const std::string &serverUid, const std::string &msg)
{
	if (!_errorLogStream.is_open())
        std::cerr << "Logger: _errorLogStream not open!" << std::endl << std::endl;
	else {
		// Check if rotation is needed before writing
		if (countLines(_errorFile) >= MAX_LOG_LINES) {
			rotateLogFile(_errorFile, _errorLogStream);
		}
		_errorLogStream << "[" << serverUid << "]\n" << "ERROR: " << msg << std::endl;
		_errorLogStream.flush();
	}
}

// Count the number of lines in a file
int Logger::countLines(const std::string &filename)
{
	std::ifstream file(filename.c_str());
	if (!file.is_open())
		return 0;
		
	int lineCount = 0;
	std::string line;
	while (std::getline(file, line))
		lineCount++;
		
	file.close();
	return lineCount;
}

// Rotate log file by keeping only the most recent lines
void Logger::rotateLogFile(const std::string &filename, std::ofstream &stream)
{
	const int linesToKeep = MAX_LOG_LINES / 2; // Keep half the max amount of lines
	
	// Read all lines
	std::ifstream file(filename.c_str());
	if (!file.is_open())
		return;
		
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(file, line))
		lines.push_back(line);
	file.close();
	
	// Close the current stream
	if (stream.is_open())
		stream.close();
		
	// Rewrite file with only the most recent lines
	stream.open(filename.c_str(), std::ios::trunc);
	if (stream.is_open()) {
		int startIndex = (lines.size() > linesToKeep) ? lines.size() - linesToKeep : 0;
		for (int i = startIndex; i < (int)lines.size(); i++) {
			stream << lines[i] << std::endl;
		}
		stream.flush();
	}
}
