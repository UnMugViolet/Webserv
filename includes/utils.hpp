#pragma once

#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <sstream>
#include <vector>
#include <arpa/inet.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "dict.hpp"

void *ft_memset(void *s, int c, unsigned long int n);
int ft_atoi(const string &str);
int ft_inet_pton4(string &src, struct in_addr *dst);
string ft_itos(int num);

#endif
