/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unmugviolet <unmugviolet@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 18:44:21 by unmugviolet       #+#    #+#             */
/*   Updated: 2025/08/28 18:45:48 by unmugviolet      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <fstream>
#include <sstream>
#include <vector>
#include <map>

class ConfigParser {
	private:
		std::map<std::string, std::string> _configMap;
		std::map<std::string, std::map<std::string, std::string> > _serverBlocks;
		
		std::string _trim(const std::string &str) const;
		std::string _intToString(int num) const;
		void _parseServerBlock(std::ifstream &file, const std::string &serverName);
		void _parseLocationBlock(std::ifstream &file, const std::string &serverName, const std::string &location);

	public:
		ConfigParser();
		ConfigParser(const std::string &filePath);
		~ConfigParser();

		void parseFile(const std::string &filePath);
		std::string getValue(const std::string &key) const;
		std::string getServerValue(const std::string &serverName, const std::string &key) const;
		bool hasKey(const std::string &key) const;
		bool hasServerKey(const std::string &serverName, const std::string &key) const;
		std::vector<std::string> getServerIds() const;
		void printConfig() const;
		
		class ErrorException : public std::exception
		{
			private:
				std::string _message;
			public:
				ErrorException(std::string message) throw()
				{
					_message = "ConfigParser error: " + message;
				}
				virtual const char* what() const throw()
				{
					return (_message.c_str());
				}
				virtual ~ErrorException() throw() {}
		};

};
