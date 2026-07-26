#include "codexion.h"

int	heap_push(t_dongle *dongle, t_request *request)
{
	if (!dongle || !request)
		return (1);
	if (dongle->heap.size >= dongle->heap.capacity)
		return (1);
	dongle->heap.items[dongle->heap.size] = request;
	heap_sift_up(dongle, dongle->heap.size);
	dongle->heap.size++;
	return (0);
}

t_request	*heap_top(t_dongle *dongle)
{
	if (!dongle || dongle->heap.size <= 0)
		return (NULL);
	return (dongle->heap.items[0]);
}

t_request	*heap_pop(t_dongle *dongle)
{
	t_request	*top;

	if (!dongle || dongle->heap.size <= 0)
		return (NULL);
	top = dongle->heap.items[0];
	dongle->heap.size--;
	if (dongle->heap.size > 0)
	{
		dongle->heap.items[0] = dongle->heap.items[dongle->heap.size];
		heap_sift_down(dongle, 0);
	}
	return (top);
}

int	heap_index(t_dongle *dongle, t_request *request)
{
	int	i;

	if (!dongle || !request)
		return (-1);
	i = 0;
	while (i < dongle->heap.size)
	{
		if (dongle->heap.items[i] == request)
			return (i);
		i++;
	}
	return (-1);
}

void	heap_remove(t_dongle *dongle, t_request *request)
{
	int	index;

	index = heap_index(dongle, request);
	if (index < 0)
		return ;
	dongle->heap.size--;
	if (index == dongle->heap.size)
		return ;
	dongle->heap.items[index] = dongle->heap.items[dongle->heap.size];
	heap_sift_up(dongle, index);
	heap_sift_down(dongle, index);
}
