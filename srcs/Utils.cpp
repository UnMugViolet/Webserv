#include <iostream>
#include <sstream>
#include <vector>
#include <arpa/inet.h>
#include <stdlib.h>
#include <limits.h>

void	*ft_memset(void *s, int c, unsigned long int n)
{
	unsigned long int	i;

	i = 0;
	while (i < n)
	{
		((unsigned char *)s)[i] = c;
		i++;
	}
	return (s);
}

int ft_atoi(const std::string &str) {
    int i = 0;
    int n = str.size();

    while (i < n && (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r' || str[i] == '\f' || str[i] == '\v'))
        i++;

    int sign = 1;
    if (i < n && (str[i] == '+' || str[i] == '-')) {
        if (str[i] == '-')
			sign = -1;
        i++;
    }

    long result = 0;
    while (i < n && (str[i] >= '0' && str[i] <= '9')) {
        int digit = str[i] - '0';
        if (result > (LONG_MAX - digit) / 10)
            return (sign == 1) ? INT_MAX : INT_MIN;
        result = result * 10 + digit;
        i++;
    }

    result *= sign;

    if (result > INT_MAX) return INT_MAX;
    if (result < INT_MIN) return INT_MIN;

    return static_cast<int>(result);
}

int ft_inet_pton4(std::string &src, struct in_addr *dst) {
    std::istringstream iss(src);
    std::string token;
    unsigned char bytes[4];
    int i = 0;

    if (src == "localhost")
        src = "127.0.0.1";

    while (std::getline(iss, token, '.')) {
        if (i >= 4)
            return 0;
        int val = ft_atoi(token);
        if (val < 0 || val > 255)
            return 0;
        bytes[i++] = static_cast<unsigned char>(val);
    }

    if (i != 4)
        return 0;

    dst->s_addr = htonl((bytes[0] << 24) | (bytes[1] << 16) |
                        (bytes[2] << 8) | bytes[3]);

    return 1;
}