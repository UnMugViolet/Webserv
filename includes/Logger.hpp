/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yguinio <yguinio@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 11:43:52 by yguinio           #+#    #+#             */
/*   Updated: 2025/09/09 14:14:02 by yguinio          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include "dict.hpp"

class Logger {
	private:
		static std::ofstream accessLog;
		static std::ofstream errorLog;
		static std::string timeStamp();

	public:
		Logger();
		~Logger();
		
		static void init();
		static void access(const std::string &serverId, const std::string &msg);
		static void error(const std::string &serverId, const std::string &msg);
} ;
