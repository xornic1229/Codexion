/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cycle.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaialons <jaialons@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 16:20:54 by jaialons          #+#    #+#             */
/*   Updated: 2026/07/27 16:20:55 by jaialons         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	set_dongle_order(t_coder *coder, t_dongle **first,
		t_dongle **second)
{
	if (coder->id % 2 == 0)
	{
		*first = coder->right_dongle;
		*second = coder->left_dongle;
	}
	else
	{
		*first = coder->left_dongle;
		*second = coder->right_dongle;
	}
}

static int	take_single_dongle(t_coder *coder)
{
	if (take_dongle(coder, coder->left_dongle))
		return (1);
	while (!is_finished(coder->sim))
		precise_sleep(coder->sim, 1);
	release_dongle(coder->left_dongle);
	return (1);
}

int	take_two_dongles(t_coder *coder)
{
	t_dongle	*first;
	t_dongle	*second;

	if (!coder || !coder->left_dongle)
		return (1);
	if (!coder->right_dongle)
		return (take_single_dongle(coder));
	set_dongle_order(coder, &first, &second);
	if (take_dongle(coder, first))
		return (1);
	if (take_dongle(coder, second))
	{
		release_dongle(first);
		return (1);
	}
	return (0);
}

void	release_two_dongles(t_coder *coder)
{
	if (!coder)
		return ;
	release_dongle(coder->left_dongle);
	release_dongle(coder->right_dongle);
}
