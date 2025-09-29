/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unmugviolet <unmugviolet@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:27:21 by pjaguin           #+#    #+#             */
/*   Updated: 2025/09/18 10:41:37 by unmugviolet      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "ConfigParser.hpp"
#include "Server.hpp"
#include "CGI.hpp"
#include "dict.hpp"
#include <signal.h>

int main(int ac, char **av)
{
	if (ac != 2)
	{
		if (ac < 2)
			std::cerr << RED BOLD << "Error: No conf file given as parameters" << NEUTRAL << std::endl;
		if (ac > 2)
			std::cerr << RED BOLD << "Error: Too many arguments" << NEUTRAL << std::endl;
		return 1;
	}

	try {
		// Set up signal handlers for graceful shutdown
		signal(SIGINT, Webserv::signalHandler);
		signal(SIGTERM, Webserv::signalHandler); 
		signal(SIGQUIT, Webserv::signalHandler);

		std::cout << "Using config file: " << BOLD << av[1] << NEUTRAL << std::endl << std::endl;

		ConfigParser config(av[1]);
		config.printConfig();

		// Init Logger and passing conf in order to create the log files
		Logger logger(config);
		logger.init();

		try {	
			Webserv webserv(config);
			webserv.serverLoop();
		} catch (const Webserv::WebservException &e) {
			std::cerr << RED BOLD << "Webserv Error: " << e.what() << NEUTRAL << std::endl;
			return 1;
		} catch (const Server::servException &e) {
			std::cerr << RED BOLD << "Server Error: " << e.what() << NEUTRAL << std::endl;
			return 1;
		} catch (const std::exception &e) {
			std::cerr << RED BOLD << "Unexpected Error: " << e.what() << NEUTRAL << std::endl;
			return 1;
		} catch (...) {
			std::cerr << RED BOLD << "Unknown Error occurred" << NEUTRAL << std::endl;
			return 1;
		}

	}
	catch (const ConfigParser::ErrorException &e) {
		std::cerr << RED BOLD << e.what() << NEUTRAL << std::endl;
		return 1;
	}

	return 0;
}
