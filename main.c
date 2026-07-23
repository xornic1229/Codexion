#include "codexion.h"

int	main(int argc, char **argv)
{
	t_config		config;
	t_simulation	sim;

	if (parse_args(argc, argv, &config) != 0)
	{
		print_error();
		return (1);
	}
	parse_config(argv, &config);
	if (!valid_parameters(&config))
	{
		print_error();
		return (1);
	}
	if (init_sim(&sim, &config) != 0)
	{
		print_error();
		return (1);
	}
	free_sim(&sim);
	return (0);
}