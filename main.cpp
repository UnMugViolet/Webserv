/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pjaguin <pjaguin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:27:21 by pjaguin           #+#    #+#             */
/*   Updated: 2025/08/28 17:08:55 by pjaguin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
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
	
	std::cout << "Using config file: " << av[1] << std::endl;
	
	return 0;
}
