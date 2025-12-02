CC = gcc
CFLAGS = -Wall -Wextra -O2
OBJS = main.o indexer.o search.o

all: mini_search

mini_search: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) mini_search
