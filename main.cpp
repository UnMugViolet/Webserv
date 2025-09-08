/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pjaguin <pjaguin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:27:21 by pjaguin           #+#    #+#             */
/*   Updated: 2025/09/08 15:46:00 by pjaguin          ###   ########.fr       */
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
		ConfigParser parser(av[1]);
		parser.printConfig();

		try {
			std::cout << "\n"
					  << YELLOW BOLD << "Testing Cgi:" << NEUTRAL << std::endl;
			int fd = CGI::interpret("www/dynamic_website/index.php");
			// Read and write to stdout the result of the CGI
			if (fd != -1) {
				char buffer[1024];
				ssize_t bytesRead;
				while ((bytesRead = read(fd, buffer, sizeof(buffer) - 1)) > 0)
				{
					buffer[bytesRead] = '\0';
					std::cout << buffer;
				}
				close(fd);
				std::cout << std::endl;
			} else {
				std::cerr << RED BOLD << "Failed to interpret CGI script." << NEUTRAL << std::endl;
			}
		} catch (const CGI::CGIException &e) {
			unsigned int error_code = e.getHttpStatus();
			std::ostringstream oss;
			oss << error_code << ".html";
			std::string error_page = oss.str();
			std::string error_content = parser.getErrorPageContent(parser, error_page);

			// For demonstration, print the error page content to standard output
			std::cout << error_content << std::endl;

			std::cerr << e.what() << '\n';

			if (e.getExit() == 1)
				exit(1);
		}
		try {			
			Webserv webserv(parser);
			webserv.serverLoop();
		} catch (const Webserv::WebservException &e) {
			std::cerr << e.what() << '\n';
		}

	}
	catch (const ConfigParser::ErrorException &e)
	{
		std::cerr << RED BOLD << e.what() << NEUTRAL << std::endl;
		return 1;
	}

	return 0;
}
