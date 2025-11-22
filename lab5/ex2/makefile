# Makefile для parallel_factorial
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
LDFLAGS = -pthread

.PHONY: all clean help test

all: parallel_factorial

parallel_factorial: parallel_factorial.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

parallel_factorial.o: parallel_factorial.c
	$(CC) $(CFLAGS) -c parallel_factorial.c

clean:
	rm -f *.o parallel_factorial

help:
	@echo "Available commands:"
	@echo "  make all        - Build program"
	@echo "  make clean      - Clean project"
	@echo "  make test       - Run tests"
	@echo "  make help       - Show help"

test: parallel_factorial
	@echo "=== Test 1: Small numbers ==="
	./parallel_factorial -k 10 --pnum=4 --mod=1000000
	@echo ""
	@echo "=== Test 2: Large numbers ==="
	./parallel_factorial -k 20 --pnum=8 --mod=1000000007
	@echo ""
	@echo "=== Test 3: Small modulus ==="
	./parallel_factorial -k 15 --pnum=3 --mod=100