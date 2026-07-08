#ifndef CODEXION_H
# define CODEXION_H

# include <unistd.h>
# include <stdlib.h>
# include <string.h>
# include <limits.h>
# include <stdio.h>
typedef enum e_scheduler
{
	SCHED_FIFO,
	SCHED_EDF
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

int		parse_args(int argc, char **argv, t_config *config);
int     only_digits(char *str);
void	print_error(void);
void	parse_config(char **argv, t_config *config);
int		valid_parametrs(t_config *config);
int		ft_atoi(const char *str);

#endif