/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   destroy.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaialons <jaialons@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 16:20:56 by jaialons          #+#    #+#             */
/*   Updated: 2026/07/27 16:20:57 by jaialons         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	destroy_one_dongle(t_dongle *dongle)
{
	pthread_mutex_destroy(&dongle->mutex);
	free(dongle->heap.items);
	dongle->heap.items = NULL;
	dongle->heap.size = 0;
	dongle->heap.capacity = 0;
}

void	destroy_dongles(t_simulation *sim, int count)
{
	int	i;

	if (!sim || !sim->dongles)
		return ;
	i = 0;
	while (i < count)
	{
		destroy_one_dongle(&sim->dongles[i]);
		i++;
	}
}
