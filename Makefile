NAME = codexion

CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread

INCLUDES = -I.

SRCS = codexion.c \
	src/parser.c \
	src/utils.c \
	src/init.c \
	src/init_objects.c \
	src/destroy.c \
	src/time.c \
	src/log.c \
	src/coder_state.c \
	src/request.c \
	src/heap_order.c \
	src/heap.c \
	src/dongle.c \
	src/dongle_release.c \
	src/cycle.c \
	src/routine.c \
	src/monitor.c \
	src/threads.c \
	src/heap_sift.c 

OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.c codexion.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
