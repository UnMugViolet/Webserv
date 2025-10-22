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
			cerr << RED BOLD << "Error: No conf file given as parameters" << NEUTRAL << endl;
		if (ac > 2)
			cerr << RED BOLD << "Error: Too many arguments" << NEUTRAL << endl;
		return 1;
	}

	try {
		// Set up signal handlers for graceful shutdown
		signal(SIGINT, Webserv::signalHandler);
		signal(SIGTERM, Webserv::signalHandler); 
		signal(SIGQUIT, Webserv::signalHandler);
		signal(SIGPIPE, Webserv::signalHandler);

		cout << "Using config file: " << BOLD << av[1] << NEUTRAL << endl << endl;

		ConfigParser config(av[1]);
		config.printConfig();

		// Init Logger and passing conf in order to create the log files
		Logger logger(config);
		logger.init();

		try {	
			Webserv webserv(config);
			webserv.serverLoop();
		} catch (const Webserv::WebservException &e) {
			cerr << e.what() << NEUTRAL << endl;
			return 1;
		} catch (const Server::ServException &e) {
			cerr << RED BOLD << "Server Error: " << e.what() << NEUTRAL << endl;
			return 1;
		} catch (const exception &e) {
			cerr << RED BOLD << "Unexpected Error: " << e.what() << NEUTRAL << endl;
			return 1;
		} catch (...) {
			cerr << RED BOLD << "Unknown Error occurred" << NEUTRAL << endl;
			return 1;
		}

	}
	catch (const ConfigParser::ErrorException &e) {
		cerr << RED BOLD << e.what() << NEUTRAL << endl;
		return 1;
	}

	return 0;
}
