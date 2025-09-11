/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pjaguin <pjaguin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 11:49:10 by yguinio           #+#    #+#             */
/*   Updated: 2025/09/11 14:25:27 by pjaguin          ###   ########.fr       */
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

void	Logger::access(const std::string &serverUid, const std::string &msg)
{
	if (!_accessLogStream.is_open())
        std::cerr << "Logger: _accessLogStream not open!" << std::endl;
	else {
		_accessLogStream << "[" << serverUid << "]\n" << msg << std::endl;
	}
}

void	Logger::error(const std::string &serverUid, const std::string &msg)
{
	if (!_errorLogStream.is_open())
        std::cerr << "Logger: _errorLogStream not open!" << std::endl;
	else {
		_errorLogStream << "[" << serverUid << "]\n" << msg << std::endl;
	}
}
