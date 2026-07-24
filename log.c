#include "codexion.h"
#include <stdio.h>

int	is_finished(t_simulation *sim)
{
	int	finished;

	if (!sim)
		return (1);
	pthread_mutex_lock(&sim->state_mutex);
	finished = sim->finished;
	pthread_mutex_unlock(&sim->state_mutex);
	return (finished);
}

void	set_finished(t_simulation *sim)
{
	if (!sim)
		return ;
	pthread_mutex_lock(&sim->state_mutex);
	sim->finished = 1;
	pthread_mutex_unlock(&sim->state_mutex);
}

void	safe_log(t_coder *coder, char *msg)
{
	t_simulation	*sim;

	if (!coder || !msg)
		return ;
	sim = coder->sim;
	if (!sim)
		return ;
	if (strcmp(msg, "burned out") != 0 && is_finished(sim))
		return ;
	pthread_mutex_lock(&sim->print_mutex);
	if (strcmp(msg, "burned out") != 0 && is_finished(sim))
	{
		pthread_mutex_unlock(&sim->print_mutex);
		return ;
	}
	printf("%lld %d %s\n", elapsed_ms(sim), coder->id, msg);
	pthread_mutex_unlock(&sim->print_mutex);
}