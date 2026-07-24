#include "codexion.h"

static int	request_can_take(t_dongle *dongle, t_request *request,
		long long now)
{
	t_simulation	*sim;

	sim = request->coder->sim;
	if (heap_top(dongle) != request)
		return (0);
	if (!dongle->available)
		return (0);
	if (sim->config.dongle_cooldown == 0)
		return (1);
	return (now - dongle->last_release_time >= sim->config.dongle_cooldown);
}

static int	register_request(t_dongle *dongle, t_request *request)
{
	pthread_mutex_lock(&dongle->mutex);
	if (heap_push(dongle, request) != 0)
	{
		pthread_mutex_unlock(&dongle->mutex);
		return (1);
	}
	return (0);
}

static int	grant_dongle(t_coder *coder, t_dongle *dongle)
{
	heap_pop(dongle);
	dongle->available = 0;
	pthread_mutex_unlock(&dongle->mutex);
	safe_log(coder, MSG_TAKEN);
	return (0);
}

int	take_dongle(t_coder *coder, t_dongle *dongle)
{
	t_request	request;
	long long	now;

	if (!coder || !dongle || !coder->sim)
		return (1);
	init_request(&request, coder);
	if (register_request(dongle, &request) != 0)
		return (1);
	while (!is_finished(coder->sim))
	{
		now = get_time_ms();
		if (request_can_take(dongle, &request, now))
			return (grant_dongle(coder, dongle));
		pthread_mutex_unlock(&dongle->mutex);
		precise_sleep(coder->sim, 1);
		pthread_mutex_lock(&dongle->mutex);
	}
	heap_remove(dongle, &request);
	pthread_mutex_unlock(&dongle->mutex);
	return (1);
}
