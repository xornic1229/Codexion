#include "codexion.h"

long long	coder_last_compile(t_coder *coder)
{
	long long	last_compile;

	if (!coder || !coder->sim)
		return (0);
	pthread_mutex_lock(&coder->sim->state_mutex);
	last_compile = coder->last_compile_time;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (last_compile);
}

int	coder_compiles(t_coder *coder)
{
	int	compiles;

	if (!coder || !coder->sim)
		return (0);
	pthread_mutex_lock(&coder->sim->state_mutex);
	compiles = coder->compiles_done;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (compiles);
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
	if (!coder || !coder->sim)
		return (1);
	return (coder_compiles(coder)
		>= coder->sim->config.number_of_compiles_required);
}
