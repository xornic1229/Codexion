/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaialons <jaialons@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 16:21:52 by jaialons          #+#    #+#             */
/*   Updated: 2026/07/27 16:21:53 by jaialons         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static long long	request_deadline(t_coder *coder)
{
	return (coder_last_compile(coder) + coder->sim->config.time_to_burnout);
}

void	init_request(t_request *request, t_coder *coder)
{
	request->coder = coder;
	request->request_time = get_time_ms();
	request->deadline = request_deadline(coder);
}
