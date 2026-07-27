#include "codexion.h"

static int	fifo_higher(t_request *a, t_request *b)
{
	if (a->request_time < b->request_time)
		return (1);
	if (a->request_time > b->request_time)
		return (0);
	return (a->coder->id < b->coder->id);
}

static int	edf_higher(t_request *a, t_request *b)
{
	if (a->deadline < b->deadline)
		return (1);
	if (a->deadline > b->deadline)
		return (0);
	return (a->coder->id > b->coder->id);
}

int	heap_higher(t_dongle *dongle, t_request *a, t_request *b)
{
	if (!dongle || !a || !b)
		return (0);
	if (a->coder->sim->config.scheduler == SCHEDULER_EDF)
		return (edf_higher(a, b));
	return (fifo_higher(a, b));
}

void	heap_swap(t_request **a, t_request **b)
{
	t_request	*tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

void	heap_sift_up(t_dongle *dongle, int index)
{
	int	parent;

	while (index > 0)
	{
		parent = (index - 1) / 2;
		if (!heap_higher(dongle, dongle->heap.items[index],
				dongle->heap.items[parent]))
			break ;
		heap_swap(&dongle->heap.items[index], &dongle->heap.items[parent]);
		index = parent;
	}
}

void	heap_sift_down(t_dongle *dongle, int index)
{
	int	left;
	int	right;
	int	best;

	while (1)
	{
		left = index * 2 + 1;
		right = index * 2 + 2;
		best = index;
		if (left < dongle->heap.size && heap_higher(dongle,
				dongle->heap.items[left], dongle->heap.items[best]))
			best = left;
		if (right < dongle->heap.size && heap_higher(dongle,
				dongle->heap.items[right], dongle->heap.items[best]))
			best = right;
		if (best == index)
			break ;
		heap_swap(&dongle->heap.items[index], &dongle->heap.items[best]);
		index = best;
	}
}
