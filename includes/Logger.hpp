/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unmugviolet <unmugviolet@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 11:43:52 by yguinio           #+#    #+#             */
/*   Updated: 2025/10/14 16:20:20 by unmugviolet      ###   ########.fr       */
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
		static ofstream _accessLogStream;
		static ofstream _errorLogStream;
		static string	 _accessFile;
		static string	 _errorFile;
		static const int MAX_LOG_LINES = 2000;
		
		static int countLines(const string &filename);
		static void rotateLogFile(const string &filename, ofstream &stream);
	public:
		Logger(ConfigParser &config);
		~Logger();
		
		static void init();
		static void access(const string &serverUid, const string &msg);
		static void error(const string &serverUid, const string &msg);
		static void info(const string &msg);
} ;
