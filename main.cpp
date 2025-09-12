/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: andrean <andrean@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:27:21 by pjaguin           #+#    #+#             */
/*   Updated: 2025/09/12 09:47:27 by andrean          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "ConfigParser.hpp"
#include "CGI.hpp"
#include "dict.hpp"

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
		std::cout << "Using config file: " << BOLD << av[1] << NEUTRAL << std::endl << std::endl;

		// Test the ConfigParser
		ConfigParser config(av[1]);
		config.printConfig();

		// Init Logger and passing conf in order to create the log files
		Logger logger(config);
		logger.init();

		try {			
			Webserv webserv(config);
			webserv.serverLoop();
		} catch (const Webserv::WebservException &e) {
			std::cerr << e.what() << '\n';
		}

	}
	catch (const ConfigParser::ErrorException &e) {
		std::cerr << RED BOLD << e.what() << NEUTRAL << std::endl;
		return 1;
	}

	return 0;
}
