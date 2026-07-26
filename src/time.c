#include "codexion.h"

long long	get_time_ms(void)
{
	struct timeval	time;

	if (gettimeofday(&time, NULL) != 0)
		return (0);
	return ((long long)time.tv_sec * 1000 + time.tv_usec / 1000);
}

long long	elapsed_ms(t_simulation *sim)
{
	if (!sim)
		return (0);
	return (get_time_ms() - sim->start_time);
}

void	precise_sleep(t_simulation *sim, long long time_ms)
{
	long long	end_time;
	long long	now;

	if (!sim || time_ms <= 0)
		return ;
	end_time = get_time_ms() + time_ms;
	while (!is_finished(sim))
	{
		now = get_time_ms();
		if (now >= end_time)
			break ;
		if (end_time - now > 5)
			usleep(1000);
		else
			usleep(100);
	}
}
