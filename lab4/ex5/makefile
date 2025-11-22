# Makefile для parallel_sum
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
LDFLAGS = -pthread

.PHONY: all clean help test

all: parallel_sum

parallel_sum: parallel_sum.o sum_utils.o utils.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

parallel_sum.o: parallel_sum.c utils.h sum_utils.h
	$(CC) $(CFLAGS) -c parallel_sum.c

sum_utils.o: sum_utils.c sum_utils.h
	$(CC) $(CFLAGS) -c sum_utils.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f *.o parallel_sum

help:
	@echo "Available commands:"
	@echo "  make all        - Build parallel_sum"
	@echo "  make clean      - Clean object files and executable"
	@echo "  make help       - Show this help"

test: parallel_sum
	@echo "=== Testing parallel_sum ==="
	./parallel_sum --threads_num 4 --seed 123 --array_size 1000000