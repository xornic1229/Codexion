/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_release.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaialons <jaialons@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 16:20:59 by jaialons          #+#    #+#             */
/*   Updated: 2026/07/27 16:21:00 by jaialons         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	release_dongle(t_dongle *dongle)
{
	if (!dongle)
		return ;
	pthread_mutex_lock(&dongle->mutex);
	dongle->available = 1;
	dongle->last_release_time = get_time_ms();
	pthread_mutex_unlock(&dongle->mutex);
}
