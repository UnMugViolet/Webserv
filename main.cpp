/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: unmugviolet <unmugviolet@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:27:21 by pjaguin           #+#    #+#             */
/*   Updated: 2025/09/04 16:14:01 by unmugviolet      ###   ########.fr       */
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
	
	try
	{
		std::cout << "Using config file: " << av[1] << std::endl;
		
		// Test the ConfigParser
		ConfigParser parser(av[1]);
		parser.printConfig();
		
		// Test accessing server values
		std::vector<std::string> servers = parser.getServerIds();
		if (!servers.empty())
		{
			for (size_t i = 0; i < servers.size(); i++) {
				std::string serverName = servers[i];
				std::cout << "\n" << YELLOW BOLD << "Testing server(s) access:" << NEUTRAL << std::endl;
				std::cout << "Server: " << serverName << std::endl;
				std::cout << "Listen: " << parser.getServerValue(serverName, "listen") << std::endl;
				std::cout << "Server Name: " << parser.getServerValue(serverName, "server_name") << std::endl;
				std::cout << "Root: " << parser.getServerValue(serverName, "root") << std::endl;
			}
		}
	}
	catch (const ConfigParser::ErrorException &e)
	{
		std::cerr << RED BOLD << e.what() << NEUTRAL << std::endl;
		return 1;
	}
	catch (const std::exception &e)
	{
		std::cerr << RED BOLD << "Unexpected error: " << e.what() << NEUTRAL << std::endl;
		return 1;
	}
	try
	{
		std::cout << "\n" << YELLOW BOLD << "Testing Cgi:" << NEUTRAL << std::endl;
		int fd = CGI::interpret("www/dynamic_website/index.php");
		// Read and write to stdout the result of the CGI
		if (fd != -1) {
			char buffer[1024];
			ssize_t bytesRead;
			while ((bytesRead = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
				buffer[bytesRead] = '\0';
				std::cout << buffer;
			}
			close(fd);
			std::cout << std::endl;
		} else {
			std::cerr << "CGI interpretation failed." << std::endl;
		}

	}
	catch(const CGI::CGIException& e)
	{
		std::cerr << e.what() << '\n';
		if (e.getExit() == 1)
			exit(1);
	}
	return 0;
}
