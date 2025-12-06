# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -std=c11
LDFLAGS = -pthread -lm

# Имена исполняемых файлов
CLIENT = client
SERVER = server
LIBRARY = libcommon.a

# Исходные файлы
CLIENT_SRC = client.c
SERVER_SRC = server.c
COMMON_SRC = common.c
COMMON_HDR = common.h

# Объектные файлы
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)
COMMON_OBJ = $(COMMON_SRC:.c=.o)

# Цели по умолчанию
all: $(CLIENT) $(SERVER)

# Статическая библиотека
$(LIBRARY): $(COMMON_OBJ)
	ar rcs $@ $^

# Клиент
$(CLIENT): $(CLIENT_OBJ) $(LIBRARY)
	$(CC) $(CFLAGS) $< -o $@ $(LIBRARY) $(LDFLAGS)

# Сервер
$(SERVER): $(SERVER_OBJ) $(LIBRARY)
	$(CC) $(CFLAGS) $< -o $@ $(LIBRARY) $(LDFLAGS)

# Компиляция объектных файлов
%.o: %.c $(COMMON_HDR)
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	rm -f $(CLIENT) $(SERVER) $(LIBRARY) *.o

# Пересборка
rebuild: clean all

# Запуск тестов
test: all
	@echo "=== Тестирование ==="
	@echo "1. Создаём тестовый файл серверов..."
	@echo "127.0.0.1:20001" > servers.txt
	@echo "127.0.0.1:20002" >> servers.txt
	@echo "127.0.0.1:20003" >> servers.txt
	@echo "2. Запускаем серверы в фоне..."
	@for port in 20001 20002 20003; do \
		./server --port $$port --tnum 2 > /dev/null 2>&1 & \
		echo "  Сервер $$port запущен"; \
		sleep 0.5; \
	done
	@sleep 2
	@echo "3. Запускаем клиента..."
	@./client --k 10 --mod 1000000007 --servers servers.txt || true
	@echo "4. Останавливаем серверы..."
	@pkill -f "server" 2>/dev/null || true

# Справка
help:
	@echo "Доступные команды:"
	@echo "  make all     - собрать всё"
	@echo "  make client  - собрать только клиент"
	@echo "  make server  - собрать только сервер"
	@echo "  make clean   - удалить скомпилированные файлы"
	@echo "  make rebuild - пересобрать проект"
	@echo "  make test    - запустить тест"
	@echo "  make help    - показать эту справку"

# Псевдонимы
.PHONY: all clean rebuild help test