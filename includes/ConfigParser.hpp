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
#include <vector>
#include <map>
#include <unistd.h>

#include "utils.hpp"
#include "dict.hpp"

class ConfigParser {
	private:
		map<string, string> 							_configMap;
		map<string, map<string, string> > 				_serverBlocks;
		map<string, map<string, map<string, string> > > _locationBlocks;

		void	_checkSemicolons(const string &file) const;
		string	_trim(const string &str) const;
		string	_formatLine(const string &str) const;
		void	_parseServerBlock(ifstream &file, string const &serverName);
		void	_parseLocationBlock(ifstream &file, string const &serverName, string const &location);

	public:
		ConfigParser();
		ConfigParser(const string &filePath);
		ConfigParser(const ConfigParser &other);
		ConfigParser &operator=(const ConfigParser &other);
		~ConfigParser();

		void			parseFile(const string &filePath);
		string			getErrorPageContent(ConfigParser &parser, const string &serverUid, unsigned int error_code) const;
		string			getValue(const string &key) const;
		string			getServerValue(const string &serverName, const string &key) const;
		string			getLocationValue(const string &serverName, const string &location, const string &key) const;
		bool			hasServerKey(const string &serverName, const string &key) const;
		vector<string>	getServerUids() const;
		vector<string>	getLocationPaths(const string &serverUid) const;
		string			getLocationValueForPath(const string &path, const string &serverUid, const string &parameter, bool must_check_server) const;
		vector<string>	getLocationVectorforPath(const string &path, const string &serverUid, const string &parameter) const;


		void printConfig() const;
		
		class ErrorException : public exception {
			private:
				string _message;
				
			public:
				ErrorException(string message) throw() {
					_message = "ConfigParser error: " + message;
				}
				virtual const char *what() const throw() {
					return (_message.c_str());
				}
				virtual ~ErrorException() throw() {}
		};

};
