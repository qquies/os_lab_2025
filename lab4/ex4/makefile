# Makefile для сборки программ лабораторной работы 4
# Задание 1: Программа с таймаутом (parallel_min_max)
# Задание 3: Программа анализа памяти (process_memory)

# Компилятор и флаги
CC = clang
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = 

# Цели по умолчанию
.PHONY: all clean help

# Основные цели
all: parallel_min_max process_memory

# --- ЗАДАНИЕ 1: Программа с таймаутом ---
parallel_min_max: parallel_min_max.o find_min_max.o utils.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

parallel_min_max.o: parallel_min_max.c find_min_max.h utils.h
	$(CC) $(CFLAGS) -c parallel_min_max.c

find_min_max.o: find_min_max.c find_min_max.h
	$(CC) $(CFLAGS) -c find_min_max.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

# --- ЗАДАНИЕ 3: Программа анализа памяти ---
process_memory: process_memory.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

process_memory.o: process_memory.c
	$(CC) $(CFLAGS) -c process_memory.c

# --- Вспомогательные цели ---
clean:
	rm -f *.o
	rm -f parallel_min_max process_memory
	rm -f min_max_result_*.txt

clean_all: clean
	rm -f *~

# Тестовые запуски
test_task1: parallel_min_max
	@echo "=== Тест задания 1 (без таймаута) ==="
	./parallel_min_max --seed 123 --array_size 1000 --pnum 4
	@echo ""
	@echo "=== Тест задания 1 (с таймаутом) ==="
	./parallel_min_max --seed 123 --array_size 1000 --pnum 4 --timeout 5

test_task3: process_memory
	@echo "=== Тест задания 3 ==="
	./process_memory

test: test_task1 test_task3

# Справка
help:
	@echo "Доступные команды:"
	@echo "  make all          - собрать все программы"
	@echo "  make parallel_min_max - собрать программу с таймаутом"
	@echo "  make process_memory  - собрать программу анализа памяти"
	@echo "  make test_task1   - протестировать задание 1"
	@echo "  make test_task3   - протестировать задание 3"
	@echo "  make test         - протестировать все задания"
	@echo "  make clean        - удалить объектные файлы и программы"
	@echo "  make help         - показать эту справку"

# Зависимости
find_min_max.h:
utils.h: