/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yguinio <yguinio@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:28:31 by pjaguin           #+#    #+#             */
/*   Updated: 2025/09/27 16:52:48 by yguinio          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "ConfigParser.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <csignal>
#include <errno.h>
#include <sys/time.h>
#include "Server.hpp"

class Webserv
{
	private:
		std::vector<Server> _servers;
		ConfigParser 		*_config;
		static bool			_shutdown;
	public:
		Webserv();
		Webserv(ConfigParser &config);
		~Webserv();

		void 		serverLoop();
		void 		stopServer();
		static void signalHandler(int signal);
		
		class WebservException : public std::exception
		{
			private:
				std::string _message;
			public:
				WebservException(std::string message) throw() {
					_message = "Webserv error: " + message;
				}
				virtual const char* what() const throw() {
					return (_message.c_str());
				}
				virtual ~WebservException() throw() {}
		};		
};
