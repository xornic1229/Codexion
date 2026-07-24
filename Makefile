NAME = codexion

CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread

SRCS = main.c parser.c utils.c init.c init_objects.c destroy.c time.c log.c \
	coder_state.c request.c heap_order.c heap.c dongle.c dongle_release.c cycle.c \
	routine.c monitor.c threads.c

OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
