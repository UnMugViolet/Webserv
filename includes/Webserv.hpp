/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: andrean <andrean@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:28:31 by pjaguin           #+#    #+#             */
/*   Updated: 2025/09/04 15:05:04 by andrean          ###   ########.fr       */
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

class Webserv
{
	private:
		// std::vector<std::string> serverIds;
		// std::vector<int> sockfds;
	public:
		Webserv();
		// Webserv(ConfigParser &config);
		~Webserv();

		class WebservException : public std::exception
		{
			private:
				std::string _message;
			public:
				WebservException(std::string message) throw()
				{
					_message = "Webserv error: " + message;
				}
				virtual const char* what() const throw()
				{
					return (_message.c_str());
				}
				virtual ~WebservException() throw() {}
		};
		// void	initWebserv(ConfigParser &config);
		
};
