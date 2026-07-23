#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <unistd.h>
# include <stdlib.h>
# include <string.h>
# include <limits.h>
# include <sys/time.h>

typedef enum e_scheduler
{
	SCHEDULER_FIFO,
	SCHEDULER_EDF
}	t_scheduler;

typedef struct s_config
{
	int			number_of_coders;
	int			time_to_burnout;
	int			time_to_compile;
	int			time_to_debug;
	int			time_to_refactor;
	int			number_of_compiles_required;
	int			dongle_cooldown;
	t_scheduler	scheduler;
}	t_config;

typedef struct s_simulation	t_simulation;

typedef struct s_dongle
{
	int				id;
	int				available;
	long long		last_release_time;
	pthread_mutex_t	mutex;
}	t_dongle;

typedef struct s_coder
{
	int				id;
	int				compiles_done;
	long long		last_compile_time;
	pthread_t		thread;
	t_dongle		*left_dongle;
	t_dongle		*right_dongle;
	t_simulation	*sim;
}	t_coder;

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

int		parse_args(int argc, char **argv, t_config *config);
void	parse_config(char **argv, t_config *config);
int		valid_parameters(t_config *config);

int		init_sim(t_simulation *sim, t_config *config);
void	free_sim(t_simulation *sim);

void	print_error(void);
int		only_digits(char *str);
int		ft_atoi(char *str);
long long	get_current_time(void);

#endif