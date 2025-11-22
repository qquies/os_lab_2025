# Makefile for deadlock demonstration
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
LDFLAGS = -pthread

.PHONY: all clean deadlock safe test help

all: deadlock_demo

deadlock_demo: deadlock_demo.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

deadlock_demo.o: deadlock_demo.c
	$(CC) $(CFLAGS) -c deadlock_demo.c

clean:
	rm -f *.o deadlock_demo

# Запуск версии с deadlock
deadlock: deadlock_demo
	@echo "=== RUNNING DEADLOCK VERSION ==="
	@echo "Program will hang - press Ctrl+C to stop"
	./deadlock_demo

# Запуск безопасной версии
safe: deadlock_demo
	@echo "=== RUNNING SAFE VERSION ==="
	./deadlock_demo --safe

# Тест обеих версий
test: safe
	@echo ""
	@echo "Now run deadlock version manually: ./deadlock_demo"

help:
	@echo "Available commands:"
	@echo "  make all        - Build program"
	@echo "  make deadlock   - Run deadlock version (will hang)"
	@echo "  make safe       - Run safe version"
	@echo "  make test       - Test safe version"
	@echo "  make clean      - Clean project"
	@echo "  make help       - Show this help"