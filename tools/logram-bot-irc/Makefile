SRC = main.c garbage.c config.c irc.c rss.c cmd.c
OBJ = $(SRC:.c=.o)
EXEC = logram-bot

CC = gcc
CFLAGS = -c -ansi -pedantic -Wall -O2 -g
LDFLAGS = -ansi -pedantic -Wall -O2 -g -lpthread

.PHONY: all clean

all: $(EXEC)

clean:
	@rm -vrf $(OBJ) $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) $^
