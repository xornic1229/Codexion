/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   threads.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaialons <jaialons@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 16:22:01 by jaialons          #+#    #+#             */
/*   Updated: 2026/07/27 16:22:02 by jaialons         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	join_created_coders(t_simulation *sim, int created)
{
	int	i;

	i = 0;
	while (i < created)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
}

int	start_simulation(t_simulation *sim)
{
	int	i;

	if (!sim)
		return (1);
	i = 0;
	while (i < sim->config.number_of_coders)
	{
		if (pthread_create(&sim->coders[i].thread, NULL, coder_routine,
				&sim->coders[i]) != 0)
		{
			set_finished(sim);
			join_created_coders(sim, i);
			return (1);
		}
		i++;
	}
	if (pthread_create(&sim->monitor_thread, NULL, monitor_routine, sim) != 0)
	{
		set_finished(sim);
		join_created_coders(sim, i);
		return (1);
	}
	return (0);
}

void	join_threads(t_simulation *sim)
{
	int	i;

	if (!sim)
		return ;
	i = 0;
	while (i < sim->config.number_of_coders)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
	set_finished(sim);
	pthread_join(sim->monitor_thread, NULL);
}
