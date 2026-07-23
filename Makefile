NAME = codexion

CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread

SRCS = main.c parser.c utils.c init.c

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