/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   routine.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaialons <jaialons@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 16:21:57 by jaialons          #+#    #+#             */
/*   Updated: 2026/07/27 16:21:58 by jaialons         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	run_compile(t_coder *coder)
{
	if (take_two_dongles(coder))
		return (1);
	update_compile_start(coder);
	safe_log(coder, MSG_COMPILING);
	precise_sleep(coder->sim, coder->sim->config.time_to_compile);
	release_two_dongles(coder);
	if (is_finished(coder->sim))
		return (1);
	increase_compile_count(coder);
	return (0);
}

static int	run_debug(t_coder *coder)
{
	if (is_finished(coder->sim))
		return (1);
	safe_log(coder, MSG_DEBUGGING);
	precise_sleep(coder->sim, coder->sim->config.time_to_debug);
	return (is_finished(coder->sim));
}

static int	run_refactor(t_coder *coder)
{
	if (is_finished(coder->sim))
		return (1);
	safe_log(coder, MSG_REFACTORING);
	precise_sleep(coder->sim, coder->sim->config.time_to_refactor);
	return (is_finished(coder->sim));
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;

	coder = (t_coder *)arg;
	if (!coder)
		return (NULL);
	if (coder->id % 2 == 0)
		precise_sleep(coder->sim, 1);
	while (!is_finished(coder->sim) && !coder_has_required_compiles(coder))
	{
		if (run_compile(coder))
			break ;
		if (coder_has_required_compiles(coder))
			break ;
		if (run_debug(coder))
			break ;
		if (run_refactor(coder))
			break ;
	}
	return (NULL);
}
