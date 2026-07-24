#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <unistd.h>
# include <stdlib.h>
# include <string.h>
# include <limits.h>
# include <sys/time.h>

# define MSG_TAKEN "has taken a dongle"
# define MSG_COMPILING "is compiling"
# define MSG_DEBUGGING "is debugging"
# define MSG_REFACTORING "is refactoring"
# define MSG_BURNED "burned out"

typedef enum e_scheduler
{
	SCHEDULER_FIFO,
	SCHEDULER_EDF
}   t_scheduler;

typedef struct s_config
{
	int				number_of_coders;
	int				time_to_burnout;
	int				time_to_compile;
	int				time_to_debug;
	int				time_to_refactor;
	int				number_of_compiles_required;
	int				dongle_cooldown;
	t_scheduler	scheduler;
}   t_config;

typedef struct s_simulation	t_simulation;
typedef struct s_coder		t_coder;

typedef struct s_request
{
	t_coder			*coder;
	long long		request_time;
	long long		deadline;
}   t_request;

typedef struct s_heap
{
	t_request		**items;
	int				size;
	int				capacity;
}   t_heap;

typedef struct s_dongle
{
	int				id;
	int				available;
	long long		last_release_time;
	pthread_mutex_t	mutex;
	t_heap			heap;
}   t_dongle;

struct s_coder
{
	int				id;
	int				compiles_done;
	long long		last_compile_time;
	pthread_t		thread;
	t_dongle		*left_dongle;
	t_dongle		*right_dongle;
	t_simulation	*sim;
};

struct s_simulation
{
	t_config		config;
	t_coder			*coders;
	t_dongle		*dongles;
	pthread_t		monitor_thread;
	pthread_mutex_t	print_mutex;
	pthread_mutex_t	state_mutex;
	long long		start_time;
	int				finished;
};

int			parse_args(int argc, char **argv, t_config *config);
void		parse_config(char **argv, t_config *config);
int			valid_parameters(t_config *config);
int			init_sim(t_simulation *sim, t_config *config);
void		free_sim(t_simulation *sim);
int			init_dongles(t_simulation *sim, int *done);
void		init_coders(t_simulation *sim);
void		destroy_dongles(t_simulation *sim, int count);
int			start_simulation(t_simulation *sim);
void		join_threads(t_simulation *sim);
void		*coder_routine(void *arg);
void		*monitor_routine(void *arg);
void		print_error(void);
int			only_digits(char *str);
int			ft_atoi(char *str);
long long	get_time_ms(void);
long long	elapsed_ms(t_simulation *sim);
void		precise_sleep(t_simulation *sim, long long time_ms);
int			is_finished(t_simulation *sim);
void		set_finished(t_simulation *sim);
void		safe_log(t_coder *coder, const char *msg);
int			heap_higher(t_dongle *dongle, t_request *a, t_request *b);
void		heap_swap(t_request **a, t_request **b);
void		heap_sift_up(t_dongle *dongle, int index);
void		heap_sift_down(t_dongle *dongle, int index);
int			heap_push(t_dongle *dongle, t_request *request);
t_request	*heap_top(t_dongle *dongle);
t_request	*heap_pop(t_dongle *dongle);
int			heap_index(t_dongle *dongle, t_request *request);
void		heap_remove(t_dongle *dongle, t_request *request);
long long	coder_last_compile(t_coder *coder);
int			coder_compiles(t_coder *coder);
void		update_compile_start(t_coder *coder);
void		increase_compile_count(t_coder *coder);
int			coder_has_required_compiles(t_coder *coder);
void		init_request(t_request *request, t_coder *coder);
int			take_dongle(t_coder *coder, t_dongle *dongle);
void		release_dongle(t_dongle *dongle);
int			take_two_dongles(t_coder *coder);
void		release_two_dongles(t_coder *coder);

#endif
