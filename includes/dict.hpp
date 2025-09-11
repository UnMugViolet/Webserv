
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dict.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pjaguin <pjaguin@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/20 17:11:35 by pjaguin           #+#    #+#             */
/*   Updated: 2025/05/23 10:40:08 by pjaguin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#ifndef DICT_HPP
# define DICT_HPP

#include <iostream>
#include <limits>

#define CYAN "\033[0;36m"
#define GREEN "\033[0;92m"
#define NEUTRAL "\033[0m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"

#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"
#define ITALIC "\033[3m"

#define MAX_INT std::numeric_limits<int>::max()
#define MIN_INT std::numeric_limits<int>::min()
#define MAX_UINT std::numeric_limits<unsigned int>::max()
#define MIN_UINT -std::numeric_limits<unsigned int>::min()
#define MAX_FLOAT std::numeric_limits<float>::max()
#define MIN_FLOAT -std::numeric_limits<float>::max()
#define MAX_DOUBLE std::numeric_limits<double>::max()
#define MIN_DOUBLE -std::numeric_limits<double>::max()

#define DEFAULT_ERROR_PAGES_PATH "error_pages/default/"
#define LOG_FOLDER_PATH "logs/"

#define DEFAULT_ERROR_LOG_FILE "error.log"
#define DEFAULT_ACCESS_LOG_FILE "access.log"

// Languages
#define PYTHON 0
#define PERL 1
#define PHP 2
#define SHELL 3
#define BINARY 4
#define UNKNOWN 5
#define HTML 6
#define CSS 7
#define PNG 8
#define JPG 9
#define JPEG 10
#define GIF 11
#define ICO 12
#define JS 13
#define MP3 14

#endif
