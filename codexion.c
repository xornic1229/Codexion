/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaialons <jaialons@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 16:22:14 by jaialons          #+#    #+#             */
/*   Updated: 2026/07/27 16:22:15 by jaialons         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	run_simulation(t_config *config)
{
	t_simulation	sim;

	if (init_sim(&sim, config) != 0)
	{
		print_error();
		return (1);
	}
	if (start_simulation(&sim) != 0)
	{
		print_error();
		free_sim(&sim);
		return (1);
	}
	join_threads(&sim);
	free_sim(&sim);
	return (0);
}

int	main(int argc, char **argv)
{
	t_config	config;

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
	return (run_simulation(&config));
}
