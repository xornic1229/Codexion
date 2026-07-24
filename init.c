#include "codexion.h"

static int	alloc_sim_arrays(t_simulation *sim)
{
	int	n;

	n = sim->config.number_of_coders;
	sim->coders = malloc(sizeof(t_coder) * n);
	if (!sim->coders)
		return (1);
	sim->dongles = malloc(sizeof(t_dongle) * n);
	if (!sim->dongles)
	{
		free(sim->coders);
		sim->coders = NULL;
		return (1);
	}
	memset(sim->coders, 0, sizeof(t_coder) * n);
	memset(sim->dongles, 0, sizeof(t_dongle) * n);
	return (0);
}

static void	cleanup_init(t_simulation *sim, int dongles, int print, int state)
{
	int	i;

	i = 0;
	while (i < dongles)
	{
		pthread_mutex_destroy(&sim->dongles[i].mutex);
		i++;
	}
	if (state)
		pthread_mutex_destroy(&sim->state_mutex);
	if (print)
		pthread_mutex_destroy(&sim->print_mutex);
	free(sim->coders);
	free(sim->dongles);
	sim->coders = NULL;
	sim->dongles = NULL;
}

static int	init_shared_mutexes(t_simulation *sim, int *print, int *state)
{
	*print = 0;
	*state = 0;
	if (pthread_mutex_init(&sim->print_mutex, NULL) != 0)
		return (1);
	*print = 1;
	if (pthread_mutex_init(&sim->state_mutex, NULL) != 0)
		return (1);
	*state = 1;
	return (0);
}

static int	init_dongles(t_simulation *sim, int *done)
{
	int	i;

	i = 0;
	*done = 0;
	while (i < sim->config.number_of_coders)
	{
		sim->dongles[i].id = i + 1;
		sim->dongles[i].available = 1;
		sim->dongles[i].last_release_time = 0;
		if (pthread_mutex_init(&sim->dongles[i].mutex, NULL) != 0)
			return (1);
		(*done)++;
		i++;
	}
	return (0);
}

static void	init_coders(t_simulation *sim)
{
	int	i;
	int	n;

	i = 0;
	n = sim->config.number_of_coders;
	while (i < n)
	{
		sim->coders[i].id = i + 1;
		sim->coders[i].compiles_done = 0;
		sim->coders[i].last_compile_time = sim->start_time;
		sim->coders[i].thread = 0;
		sim->coders[i].left_dongle = &sim->dongles[i];
		if (n == 1)
			sim->coders[i].right_dongle = NULL;
		else
			sim->coders[i].right_dongle = &sim->dongles[(i + 1) % n];
		sim->coders[i].sim = sim;
		i++;
	}
}

int	init_sim(t_simulation *sim, t_config *config)
{
	int	dongles_done;
	int	print_ready;
	int	state_ready;

	if (!sim || !config)
		return (1);
	memset(sim, 0, sizeof(t_simulation));
	sim->config = *config;
	sim->finished = 0;
	sim->start_time = get_time_ms();
	if (alloc_sim_arrays(sim) != 0)
		return (1);
	if (init_shared_mutexes(sim, &print_ready, &state_ready) != 0)
	{
		cleanup_init(sim, 0, print_ready, state_ready);
		return (1);
	}
	if (init_dongles(sim, &dongles_done) != 0)
	{
		cleanup_init(sim, dongles_done, print_ready, state_ready);
		return (1);
	}
	init_coders(sim);
	return (0);
}

void	free_sim(t_simulation *sim)
{
	int	i;

	if (!sim)
		return ;
	i = 0;
	while (sim->dongles && i < sim->config.number_of_coders)
	{
		pthread_mutex_destroy(&sim->dongles[i].mutex);
		i++;
	}
	pthread_mutex_destroy(&sim->state_mutex);
	pthread_mutex_destroy(&sim->print_mutex);
	free(sim->coders);
	free(sim->dongles);
	sim->coders = NULL;
	sim->dongles = NULL;
}
