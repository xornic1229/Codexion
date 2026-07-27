/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init_objects.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaialons <jaialons@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 16:21:27 by jaialons          #+#    #+#             */
/*   Updated: 2026/07/27 16:21:28 by jaialons         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	init_dongle_heap(t_dongle *dongle, int capacity)
{
	dongle->heap.items = malloc(sizeof(t_request *) * capacity);
	if (!dongle->heap.items)
		return (1);
	dongle->heap.size = 0;
	dongle->heap.capacity = capacity;
	return (0);
}

static int	init_one_dongle(t_simulation *sim, int i)
{
	t_dongle	*dongle;

	dongle = &sim->dongles[i];
	dongle->id = i + 1;
	dongle->available = 1;
	dongle->last_release_time = 0;
	if (init_dongle_heap(dongle, sim->config.number_of_coders) != 0)
		return (1);
	if (pthread_mutex_init(&dongle->mutex, NULL) != 0)
	{
		free(dongle->heap.items);
		dongle->heap.items = NULL;
		return (1);
	}
	return (0);
}

int	init_dongles(t_simulation *sim, int *done)
{
	int	i;

	i = 0;
	*done = 0;
	while (i < sim->config.number_of_coders)
	{
		if (init_one_dongle(sim, i) != 0)
			return (1);
		(*done)++;
		i++;
	}
	return (0);
}

void	init_coders(t_simulation *sim)
{
	int	i;
	int	n;

	i = 0;
	n = sim->config.number_of_coders;
	while (i < n)
	{
		sim->coders[i].id = i + 1;
		sim->coders[i].last_compile_time = sim->start_time;
		sim->coders[i].left_dongle = &sim->dongles[i];
		if (n == 1)
			sim->coders[i].right_dongle = NULL;
		else
			sim->coders[i].right_dongle = &sim->dongles[(i + 1) % n];
		sim->coders[i].sim = sim;
		i++;
	}
}
