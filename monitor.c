#include "codexion.h"

static int	check_burnout(t_simulation *sim)
{
	int				 i;
	long long		current_time;
	long long		last_compile_time;

	i = 0;
	current_time = get_time_ms();
	while (i < sim->config.number_of_coders)
	{
		last_compile_time = coder_last_compile(&sim->coders[i]);
		if (current_time - last_compile_time >= sim->config.time_to_burnout)
		{
			safe_log(&sim->coders[i], MSG_BURNED);
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

	i = 0;
	while (i < sim->config.number_of_coders)
	{
		if (!coder_has_required_compiles(&sim->coders[i]))
			return (0);
		i++;
	}
	set_finished(sim);
	return (1);
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
