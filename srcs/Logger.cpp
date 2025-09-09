/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yguinio <yguinio@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 11:49:10 by yguinio           #+#    #+#             */
/*   Updated: 2025/09/09 14:58:46 by yguinio          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Logger.hpp"

std::ofstream Logger::accessLog(ACC_LOG, std::ios::app);
std::ofstream Logger::errorLog(ERR_LOG, std::ios::app);

Logger::Logger()
{
	accessLog.open(ACC_LOG, std::ios::app);
	errorLog.open(ERR_LOG, std::ios::app);
}

Logger::~Logger()
{
	if (accessLog.is_open())
		accessLog.close();
	if (errorLog.is_open())
		errorLog.close();
}

std::string Logger::timeStamp()
{
	time_t now = time(0);
	char buf[80];
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
	return std::string(buf);
}

void	Logger::init()
{
	std::ofstream a(ACC_LOG, std::ios::trunc);
	std::ofstream e(ERR_LOG, std::ios::trunc);
	
	if (accessLog.is_open())
        accessLog.close();
    accessLog.open(ACC_LOG, std::ios::app);

    if (errorLog.is_open())
        errorLog.close();
    errorLog.open(ERR_LOG, std::ios::app);
}

void	Logger::access(const std::string &serverId, const std::string &msg)
{
	if (!accessLog.is_open())
        std::cerr << "Logger: accessLog not open!" << std::endl;
	else {
		if (serverId.empty())
			accessLog << "[" << timeStamp() << "] " << serverId << msg << std::endl;
		else
			accessLog << "[" << timeStamp() << "] SERVER: " << serverId + " | " + msg << std::endl;
	}
}

void	Logger::error(const std::string &serverId, const std::string &msg)
{
	if (!errorLog.is_open())
        std::cerr << "Logger: errorLog not open!" << std::endl;
	else {
		if (serverId.empty())
			errorLog << "[" << timeStamp() << "] " << serverId << msg << std::endl;
		else
			errorLog << "[" << timeStamp() << "] SERVER: " << serverId + " | " + msg << std::endl;
	}
}
