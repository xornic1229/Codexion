/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_sift.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaialons <jaialons@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 16:28:58 by jaialons          #+#    #+#             */
/*   Updated: 2026/07/27 16:33:19 by jaialons         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	heap_sift_up(t_dongle *dongle, int index)
{
	int	parent;

	while (index > 0)
	{
		parent = (index - 1) / 2;
		if (!heap_higher(dongle, dongle->heap.items[index],
				dongle->heap.items[parent]))
			break ;
		heap_swap(&dongle->heap.items[index], &dongle->heap.items[parent]);
		index = parent;
	}
}

void	heap_sift_down(t_dongle *dongle, int index)
{
	int	left;
	int	right;
	int	best;

	while (1)
	{
		left = index * 2 + 1;
		right = index * 2 + 2;
		best = index;
		if (left < dongle->heap.size && heap_higher(dongle,
				dongle->heap.items[left], dongle->heap.items[best]))
			best = left;
		if (right < dongle->heap.size && heap_higher(dongle,
				dongle->heap.items[right], dongle->heap.items[best]))
			best = right;
		if (best == index)
			break ;
		heap_swap(&dongle->heap.items[index], &dongle->heap.items[best]);
		index = best;
	}
}
