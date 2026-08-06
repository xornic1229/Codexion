/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_state.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaialons <jaialons@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 16:27:17 by jaialons          #+#    #+#             */
/*   Updated: 2026/07/27 16:27:18 by jaialons         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long long	coder_last_compile(t_coder *coder)
{
	long long	last_compile_time;

	if (!coder || !coder->sim)
		return (0);
	pthread_mutex_lock(&coder->sim->state_mutex);
	last_compile_time = coder->last_compile_time;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (last_compile_time);
}

int	coder_compiles(t_coder *coder)
{
	int	compiles_done;

	if (!coder || !coder->sim)
		return (0);
	pthread_mutex_lock(&coder->sim->state_mutex);
	compiles_done = coder->compiles_done;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (compiles_done);
}

void	update_compile_start(t_coder *coder)
{
	if (!coder || !coder->sim)
		return ;
	pthread_mutex_lock(&coder->sim->state_mutex);
	coder->last_compile_time = get_time_ms();
	pthread_mutex_unlock(&coder->sim->state_mutex);
}

void	increase_compile_count(t_coder *coder)
{
	if (!coder || !coder->sim)
		return ;
	pthread_mutex_lock(&coder->sim->state_mutex);
	coder->compiles_done++;
	pthread_mutex_unlock(&coder->sim->state_mutex);
}

int	coder_has_required_compiles(t_coder *coder)
{
	int	compiles_done;
	int	required;

	if (!coder || !coder->sim)
		return (1);
	pthread_mutex_lock(&coder->sim->state_mutex);
	compiles_done = coder->compiles_done;
	required = coder->sim->config.number_of_compiles_required;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (compiles_done >= required);
}
