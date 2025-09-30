/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unmugviolet <unmugviolet@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 11:43:52 by yguinio           #+#    #+#             */
/*   Updated: 2025/09/30 11:00:05 by unmugviolet      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <vector>
#include "dict.hpp"
#include "ConfigParser.hpp"

class ConfigParser;

class Logger {
	private:
		static std::ofstream _accessLogStream;
		static std::ofstream _errorLogStream;
		static std::string	 _accessFile;
		static std::string	 _errorFile;
		static const int MAX_LOG_LINES = 2000;
		
		static int countLines(const std::string &filename);
		static void rotateLogFile(const std::string &filename, std::ofstream &stream);
	public:
		Logger(ConfigParser &config);
		~Logger();
		
		static void init();
		static void access(const std::string &serverUid, const std::string &msg);
		static void error(const std::string &serverUid, const std::string &msg);
		static void info(const std::string &msg);
} ;
