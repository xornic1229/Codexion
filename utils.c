#include "codexion.h"

void	print_error(void)
{
	write(2, "Error\n", 6);
}

int	only_digits(char *str)
{
	int	i;

	if (!str || str[0] == '\0')
		return (0);
	i = 0;
	while (str[i])
	{
		if (str[i] < '0' || str[i] > '9')
			return (0);
		i++;
	}
	return (1);
}

int ft_atoi(char *str)
{
    int result = 0;
    int sign = 1;
    int i = 0;

    // Handle optional sign
    if (str[i] == '-' || str[i] == '+') {
        if (str[i] == '-') {
            sign = -1;
        }
        i++;
    }

    // Convert digits to integer
    while (str[i] >= '0' && str[i] <= '9') {
        if (result > (INT_MAX - (str[i] - '0')) / 10)
        {
            return(-1);
        }
        result = result * 10 + (str[i] - '0');
        i++;
    }

    return result * sign;
}


long long	get_current_time(void)
{
	struct timeval	time;

	if (gettimeofday(&time, NULL) != 0)
		return (0);
	return ((long long)time.tv_sec * 1000 + time.tv_usec / 1000);
}
