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
	destroy_dongles(sim, dongles);
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

int	init_sim(t_simulation *sim, t_config *config)
{
	int	dongles_done;
	int	print_ready;
	int	state_ready;

	if (!sim || !config)
		return (1);
	memset(sim, 0, sizeof(t_simulation));
	sim->config = *config;
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
	if (!sim)
		return ;
	destroy_dongles(sim, sim->config.number_of_coders);
	pthread_mutex_destroy(&sim->state_mutex);
	pthread_mutex_destroy(&sim->print_mutex);
	free(sim->coders);
	free(sim->dongles);
	sim->coders = NULL;
	sim->dongles = NULL;
}
