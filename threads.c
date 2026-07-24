#include "codexion.h"

static void	join_created_coders(t_simulation *sim, int created)
{
	int	i;

	i = 0;
	while (i < created)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
}

static int	take_dongle(t_coder *coder, t_dongle *dongle)
{
	if (!coder || !dongle || !coder->sim)
		return (1);
	while (!is_finished(coder->sim))
	{
		pthread_mutex_lock(&dongle->mutex);
		if (dongle->available)
		{
			dongle->available = 0;
			pthread_mutex_unlock(&dongle->mutex);
			safe_log(coder, "has taken a dongle");
			return (0);
		}
		pthread_mutex_unlock(&dongle->mutex);
		precise_sleep(coder->sim, 1);
	}
	return (1);
}

static void	release_dongle(t_dongle *dongle)
{
	if (!dongle)
		return ;
	pthread_mutex_lock(&dongle->mutex);
	dongle->available = 1;
	dongle->last_release_time = get_time_ms();
	pthread_mutex_unlock(&dongle->mutex);
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

static int	take_two_dongles(t_coder *coder)
{
	t_dongle	*first;
	t_dongle	*second;

	if (!coder || !coder->left_dongle)
		return (1);
	if (coder->right_dongle == NULL)
		return (take_single_dongle(coder));
	if (coder->id % 2 == 0)
	{
		first = coder->right_dongle;
		second = coder->left_dongle;
	}
	else
	{
		first = coder->left_dongle;
		second = coder->right_dongle;
	}
	if (take_dongle(coder, first))
		return (1);
	if (take_dongle(coder, second))
	{
		release_dongle(first);
		return (1);
	}
	return (0);
}

static void	release_two_dongles(t_coder *coder)
{
	if (!coder)
		return ;
	release_dongle(coder->left_dongle);
	release_dongle(coder->right_dongle);
}

static void	update_compile_start(t_coder *coder)
{
	if (!coder || !coder->sim)
		return ;
	pthread_mutex_lock(&coder->sim->state_mutex);
	coder->last_compile_time = get_time_ms();
	pthread_mutex_unlock(&coder->sim->state_mutex);
}

static void	increase_compile_count(t_coder *coder)
{
	if (!coder || !coder->sim)
		return ;
	pthread_mutex_lock(&coder->sim->state_mutex);
	coder->compiles_done++;
	pthread_mutex_unlock(&coder->sim->state_mutex);
}

static int	check_burnout(t_simulation *sim)
{
	int		i;
	long long	current_time;
	long long	last_compile_time;

	i = 0;
	current_time = get_time_ms();
	while (i < sim->config.number_of_coders)
	{
		pthread_mutex_lock(&sim->state_mutex);
		last_compile_time = sim->coders[i].last_compile_time;
		pthread_mutex_unlock(&sim->state_mutex);
		if (current_time - last_compile_time >= sim->config.time_to_burnout)
		{
			safe_log(&sim->coders[i], "burned out");
			set_finished(sim);
			return (1);
		}
		i++;
	}
	return (0);
}

static int	all_coders_done(t_simulation *sim)
{
	int	i;
	int	compiles_done;

	i = 0;
	while (i < sim->config.number_of_coders)
	{
		pthread_mutex_lock(&sim->state_mutex);
		compiles_done = sim->coders[i].compiles_done;
		pthread_mutex_unlock(&sim->state_mutex);
		if (compiles_done < sim->config.number_of_compiles_required)
			return (0);
		i++;
	}
	set_finished(sim);
	return (1);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;

	coder = (t_coder *)arg;
	if (!coder)
		return (NULL);
	if (coder->id % 2 == 0)
		precise_sleep(coder->sim, 1);
	while (!is_finished(coder->sim))
	{
		if (take_two_dongles(coder))
			break ;
		update_compile_start(coder);
		safe_log(coder, "is compiling");
		precise_sleep(coder->sim, coder->sim->config.time_to_compile);
		release_two_dongles(coder);
		increase_compile_count(coder);
		if (all_coders_done(coder->sim))
			break ;
		if (is_finished(coder->sim))
			break ;
		safe_log(coder, "is debugging");
		precise_sleep(coder->sim, coder->sim->config.time_to_debug);
		if (is_finished(coder->sim))
			break ;
		safe_log(coder, "is refactoring");
		precise_sleep(coder->sim, coder->sim->config.time_to_refactor);
	}
	return (NULL);
}

void	*monitor_routine(void *arg)
{
	t_simulation	*sim;

	sim = (t_simulation *)arg;
	if (!sim)
		return (NULL);
	while (!is_finished(sim))
	{
		if (check_burnout(sim))
			return (NULL);
		if (all_coders_done(sim))
			return (NULL);
		usleep(1000);
	}
	return (NULL);
}

int	start_simulation(t_simulation *sim)
{
	int	i;

	if (!sim)
		return (1);
	i = 0;
	while (i < sim->config.number_of_coders)
	{
		if (pthread_create(&sim->coders[i].thread, NULL, coder_routine,
				&sim->coders[i]) != 0)
		{
			set_finished(sim);
			join_created_coders(sim, i);
			return (1);
		}
		i++;
	}
	if (pthread_create(&sim->monitor_thread, NULL, monitor_routine, sim) != 0)
	{
		set_finished(sim);
		join_created_coders(sim, i);
		return (1);
	}
	return (0);
}

void	join_threads(t_simulation *sim)
{
	int	i;

	if (!sim)
		return ;
	i = 0;
	while (i < sim->config.number_of_coders)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
	set_finished(sim);
	pthread_join(sim->monitor_thread, NULL);
}
