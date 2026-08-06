/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_order.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaialons <jaialons@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 16:21:15 by jaialons          #+#    #+#             */
/*   Updated: 2026/07/27 16:30:19 by jaialons         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	fifo_higher(t_request *a, t_request *b)
{
	if (a->request_time < b->request_time)
		return (1);
	if (a->request_time > b->request_time)
		return (0);
	return (a->coder->id < b->coder->id);
}

static int	edf_higher(t_request *a, t_request *b)
{
	if (a->deadline < b->deadline)
		return (1);
	if (a->deadline > b->deadline)
		return (0);
	return (a->coder->id > b->coder->id);
}

int	heap_higher(t_dongle *dongle, t_request *a, t_request *b)
{
	if (!dongle || !a || !b)
		return (0);
	if (a->coder->sim->config.scheduler == SCHEDULER_EDF)
		return (edf_higher(a, b));
	return (fifo_higher(a, b));
}

void	heap_swap(t_request **a, t_request **b)
{
	t_request	*tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}
