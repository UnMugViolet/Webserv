/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unmugviolet <unmugviolet@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 11:43:52 by yguinio           #+#    #+#             */
/*   Updated: 2025/09/17 13:11:14 by unmugviolet      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include "dict.hpp"
#include "ConfigParser.hpp"

class ConfigParser;

class Logger {
	private:
		static std::ofstream _accessLogStream;
		static std::ofstream _errorLogStream;
		static std::string	 _accessFile;
		static std::string	 _errorFile;
	public:
		Logger(ConfigParser &config);
		~Logger();
		
		static void init();
		static void access(const std::string &serverUid, const std::string &msg);
		static void error(const std::string &serverUid, const std::string &msg);
} ;
