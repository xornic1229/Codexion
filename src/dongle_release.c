#include "codexion.h"

void	release_dongle(t_dongle *dongle)
{
	if (!dongle)
		return ;
	pthread_mutex_lock(&dongle->mutex);
	dongle->available = 1;
	dongle->last_release_time = get_time_ms();
	pthread_mutex_unlock(&dongle->mutex);
}
