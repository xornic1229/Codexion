#include "codexion.h"

int	main(int argc, char **argv)
{
	t_config	config;

	if (parse_args(argc, argv, &config) != 0)
	{
		print_error();
		return (1);
	}
	parse_config(argv, &config);

	if (!valid_parametrs(&config))
	{
		print_error();
		return (1);
	}
	return (0);
}
