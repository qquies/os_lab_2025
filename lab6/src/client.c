#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pthread.h"
#include "common.h" // для структуры Server и MultModulo

// Конвертация строки в uint64_t
bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Out of uint64_t range: %s\n", str);
        return false;
    }
    if (errno != 0)
        return false;
    *val = i;
    return true;
}

// Структура для передачи аргументов в поток
struct ThreadArgs {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t* result;
    int* success_flag;
};

// Функция, выполняемая в потоке
void* ProcessServer(void* thread_args) {
    struct ThreadArgs* args = (struct ThreadArgs*)thread_args;
    
    // Создаём сокет
    struct hostent *hostname = gethostbyname(args->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s\n", args->server.ip);
        *(args->success_flag) = 0;
        return NULL;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(args->server.port);
    server_addr.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed for %s:%d\n", 
                args->server.ip, args->server.port);
        *(args->success_flag) = 0;
        return NULL;
    }

    // Устанавливаем таймаут на подключение (5 секунд)
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sck, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sck, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Connection failed to %s:%d\n", 
                args->server.ip, args->server.port);
        close(sck);
        *(args->success_flag) = 0;
        return NULL;
    }

    // Подготавливаем задачу для сервера
    char task[sizeof(uint64_t) * 3];
    memcpy(task, &(args->begin), sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &(args->end), sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &(args->mod), sizeof(uint64_t));

    // Отправляем задачу
    if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Send failed to %s:%d\n", 
                args->server.ip, args->server.port);
        close(sck);
        *(args->success_flag) = 0;
        return NULL;
    }

    // Получаем ответ
    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Receive failed from %s:%d\n", 
                args->server.ip, args->server.port);
        close(sck);
        *(args->success_flag) = 0;
        return NULL;
    }

    // Извлекаем результат
    uint64_t answer = 0;
    memcpy(&answer, response, sizeof(uint64_t));
    *(args->result) = answer;
    *(args->success_flag) = 1;

    printf("Server %s:%d returned: %llu (range %llu..%llu)\n", 
           args->server.ip, args->server.port, answer, args->begin, args->end);

    close(sck);
    return NULL;
}

int main(int argc, char **argv) {
    uint64_t k = -1;
    uint64_t mod = -1;
    char servers_file_path[255] = {'\0'};

    // Обработка аргументов командной строки
    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {"servers", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0:
                if (!ConvertStringToUI64(optarg, &k)) {
                    fprintf(stderr, "Invalid k value: %s\n", optarg);
                    return 1;
                }
                break;
            case 1:
                if (!ConvertStringToUI64(optarg, &mod)) {
                    fprintf(stderr, "Invalid mod value: %s\n", optarg);
                    return 1;
                }
                break;
            case 2:
                strncpy(servers_file_path, optarg, sizeof(servers_file_path) - 1);
                servers_file_path[sizeof(servers_file_path) - 1] = '\0';
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
        } break;

        case '?':
            printf("Arguments error\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    // Проверяем, что все обязательные аргументы установлены
    if (k == -1 || mod == -1 || !strlen(servers_file_path)) {
        fprintf(stderr, "Usage: %s --k <number> --mod <modulus> --servers <file>\n",
                argv[0]);
        fprintf(stderr, "Example: %s --k 1000 --mod 1000000007 --servers servers.txt\n",
                argv[0]);
        return 1;
    }

    // Выводим информацию о вычислении
    printf("Computing %llu! mod %llu\n", k, mod);

    // Открываем файл с серверами
    FILE* file = fopen(servers_file_path, "r");
    if (!file) {
        fprintf(stderr, "Cannot open servers file: %s\n", servers_file_path);
        return 1;
    }

    // Считаем количество серверов
    int servers_num = 0;
    char line[255];
    while (fgets(line, sizeof(line), file)) {
        // Пропускаем пустые строки и строки с комментариями
        if (strlen(line) > 1 && line[0] != '#') {
            servers_num++;
        }
    }
    
    if (servers_num == 0) {
        fprintf(stderr, "No servers found in file: %s\n", servers_file_path);
        fclose(file);
        return 1;
    }
    
    printf("Found %d servers\n", servers_num);
    
    // Выделяем память для серверов
    struct Server* servers = malloc(sizeof(struct Server) * servers_num);
    if (!servers) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return 1;
    }
    
    // Читаем серверы
    rewind(file);
    int idx = 0;
    while (fgets(line, sizeof(line), file) && idx < servers_num) {
        // Пропускаем пустые строки и комментарии
        if (strlen(line) <= 1 || line[0] == '#') {
            continue;
        }
        
        // Убираем символ новой строки
        line[strcspn(line, "\n")] = 0;
        
        // Разбираем ip:port
        char* colon = strchr(line, ':');
        if (!colon) {
            fprintf(stderr, "Invalid server format (should be ip:port): %s\n", line);
            continue;
        }
        
        *colon = '\0';  // Разделяем строку
        strncpy(servers[idx].ip, line, sizeof(servers[idx].ip) - 1);
        servers[idx].ip[sizeof(servers[idx].ip) - 1] = '\0';
        servers[idx].port = atoi(colon + 1);
        
        if (servers[idx].port <= 0 || servers[idx].port > 65535) {
            fprintf(stderr, "Invalid port for server %s: %d\n", 
                    servers[idx].ip, servers[idx].port);
            continue;
        }
        
        idx++;
    }
    
    fclose(file);
    
    // Если не все серверы прочитаны корректно
    if (idx < servers_num) {
        servers_num = idx;
        if (servers_num == 0) {
            fprintf(stderr, "No valid servers found\n");
            free(servers);
            return 1;
        }
    }
    
    // Распределяем работу между серверами
    uint64_t range_size = k / servers_num;
    uint64_t remainder = k % servers_num;
    
    // Подготавливаем данные для потоков
    pthread_t threads[servers_num];
    struct ThreadArgs thread_args[servers_num];
    uint64_t results[servers_num];
    int success_flags[servers_num];
    
    // Инициализируем флаги
    for (int i = 0; i < servers_num; i++) {
        success_flags[i] = 0;
        results[i] = 1;  // Нейтральный элемент для умножения
    }
    
    uint64_t current = 1;
    printf("\nDistributing work:\n");
    for (int i = 0; i < servers_num; i++) {
        thread_args[i].server = servers[i];
        thread_args[i].begin = current;
        thread_args[i].end = current + range_size - 1;
        
        // Распределяем остаток
        if (remainder > 0) {
            thread_args[i].end++;
            remainder--;
        }
        
        thread_args[i].mod = mod;
        thread_args[i].result = &results[i];
        thread_args[i].success_flag = &success_flags[i];
        
        printf("Server %d (%s:%d): %llu..%llu\n", 
               i, servers[i].ip, servers[i].port, 
               thread_args[i].begin, thread_args[i].end);
        
        // Создаём поток
        if (pthread_create(&threads[i], NULL, ProcessServer, &thread_args[i])) {
            fprintf(stderr, "Error creating thread for server %s:%d\n", 
                    servers[i].ip, servers[i].port);
            success_flags[i] = 0;
        }
        
        current = thread_args[i].end + 1;
    }
    
    // Ждём завершения всех потоков
    printf("\nWaiting for results...\n");
    for (int i = 0; i < servers_num; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Объединяем результаты
    uint64_t total = 1;
    int failed_servers = 0;
    for (int i = 0; i < servers_num; i++) {
        if (success_flags[i]) {
            total = MultModulo(total, results[i], mod);
        } else {
            fprintf(stderr, "Warning: Server %s:%d failed or timed out\n", 
                    servers[i].ip, servers[i].port);
            failed_servers++;
        }
    }
    
    // Проверяем, все ли серверы ответили
    if (failed_servers > 0) {
        fprintf(stderr, "\nWarning: %d out of %d servers failed\n", 
                failed_servers, servers_num);
        if (failed_servers == servers_num) {
            fprintf(stderr, "Error: All servers failed!\n");
            free(servers);
            return 1;
        }
        printf("Computing with available results...\n");
    }
    
    printf("\n================================\n");
    printf("Final result: %llu! mod %llu = %llu\n", k, mod, total);
    printf("================================\n");
    
    // Очищаем ресурсы
    free(servers);
    
    return 0;
}