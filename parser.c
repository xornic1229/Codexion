#include "codexion.h"

int	parse_args(int argc, char **argv, t_config *config)
{
	int	i;

	(void)config;
	if (argc != 9)
		return (1);
	i = 1;
	while (i <= 7)
	{
		if (!only_digits(argv[i]))
			return (1);
		i++;
	}
	if (strcmp(argv[8], "fifo") != 0 && strcmp(argv[8], "edf") != 0)
		return (1);
	return (0);
}

void parse_config(char **argv, t_config *config)
{
    config->number_of_coders = ft_atoi(argv[1]);
    config->time_to_burnout = ft_atoi(argv[2]);
    config->time_to_compile = ft_atoi(argv[3]);
    config->time_to_debug = ft_atoi(argv[4]);
    config->time_to_refactor = ft_atoi(argv[5]);
    config->number_of_compiles_required = ft_atoi(argv[6]);
    config->dongle_cooldown = ft_atoi(argv[7]);
    if (strcmp(argv[8], "fifo") == 0)
        config->scheduler = SCHEDULER_FIFO;
    else
        config->scheduler = SCHEDULER_EDF;
}

int valid_parameters(t_config *config)
{
    if (config->number_of_coders < 1)
        return 0;
    if (config->time_to_burnout < 1)
        return 0;
    if (config->time_to_compile < 0)
        return 0;
    if (config->time_to_debug < 0)
        return 0;
    if (config->time_to_refactor < 0)
        return 0;
    if (config->number_of_compiles_required < 1)
        return 0;
    if (config->dongle_cooldown < 0)
        return 0;
    return 1;
}
