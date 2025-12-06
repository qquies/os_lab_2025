#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pthread.h"
#include "common.h" // для структуры FactorialArgs и MultModulo


// Вычисление частичного факториала для заданного диапазона
uint64_t Factorial(const struct FactorialArgs *args) {
    uint64_t ans = 1;
    
    // Проверка на пустой диапазон
    if (args->begin > args->end) {
        return 1;
    }
    
    for (uint64_t i = args->begin; i <= args->end; i++) {
        ans = MultModulo(ans, i, args->mod);
    }
    
    return ans;
}

// Функция-обёртка для запуска в потоке
void *ThreadFactorial(void *args) {
    struct FactorialArgs *fargs = (struct FactorialArgs *)args;
    uint64_t result = Factorial(fargs);
    return (void *)(uintptr_t)result;  // Безопасное приведение
}

int main(int argc, char **argv) {
    int tnum = -1;
    int port = -1;

    // Обработка аргументов командной строки
    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"port", required_argument, 0, 0},
            {"tnum", required_argument, 0, 0},
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
                        port = atoi(optarg);
                        break;
                    case 1:
                        tnum = atoi(optarg);
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
            } break;

            case '?':
                printf("Unknown argument\n");
                break;
            default:
                fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (port == -1 || tnum == -1) {
        fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        fprintf(stderr, "Can not create server socket!");
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

    int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
    if (err < 0) {
        fprintf(stderr, "Can not bind to socket!");
        return 1;
    }

    err = listen(server_fd, 128);
    if (err < 0) {
        fprintf(stderr, "Could not listen on socket\n");
        return 1;
    }

    printf("Server listening at %d\n", port);

    // Основной цикл обработки клиентских соединений
    while (true) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

        if (client_fd < 0) {
            fprintf(stderr, "Could not establish new connection\n");
            continue;
        }

        // Цикл обработки данных от клиента
        while (true) {
            unsigned int buffer_size = sizeof(uint64_t) * 3;
            char from_client[buffer_size];
            int read_bytes = recv(client_fd, from_client, buffer_size, 0);

            if (!read_bytes)
                break;
            if (read_bytes < 0) {
                fprintf(stderr, "Client read failed\n");
                break;
            }
            if (read_bytes < buffer_size) {
                fprintf(stderr, "Client send wrong data format\n");
                break;
            }

            // Разбираем данные из буфера
            uint64_t begin = 0;
            uint64_t end = 0;
            uint64_t mod = 0;
            memcpy(&begin, from_client, sizeof(uint64_t));
            memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
            memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

            fprintf(stdout, "Receive: %llu %llu %llu\n", begin, end, mod);

            // Проверяем и корректируем диапазон
            if (begin > end) {
                uint64_t temp = begin;
                begin = end;
                end = temp;
            }

            // Вычисляем общее количество чисел
            uint64_t total_numbers = end - begin + 1;
            
            // Защита от tnum > total_numbers
            int actual_tnum = tnum;
            if (actual_tnum > total_numbers) {
                actual_tnum = total_numbers;
            }
            if (actual_tnum == 0) {
                actual_tnum = 1;
            }

            pthread_t threads[actual_tnum];
            struct FactorialArgs args[actual_tnum];

            uint64_t range_size = total_numbers / actual_tnum;
            uint64_t remainder = total_numbers % actual_tnum;

            uint64_t current = begin;
            for (int i = 0; i < actual_tnum; i++) {
                args[i].begin = current;
                args[i].end = current + range_size - 1;
                
                if (remainder > 0) {
                    args[i].end++;
                    remainder--;
                }
                
                // Гарантируем корректность диапазона
                if (args[i].end < args[i].begin) {
                    args[i].end = args[i].begin;
                }
                
                args[i].mod = mod;
                
                fprintf(stdout, "Thread %d: %llu..%llu mod %llu\n", 
                        i, args[i].begin, args[i].end, args[i].mod);
                
                if (pthread_create(&threads[i], NULL, ThreadFactorial, 
                                  (void *)&args[i])) {
                    fprintf(stderr, "Error: pthread_create failed!\n");
                    return 1;
                }
                
                current = args[i].end + 1;
            }

            // Собираем результаты
            uint64_t total = 1;
            for (int i = 0; i < actual_tnum; i++) {
                void *thread_result;
                pthread_join(threads[i], &thread_result);
                
                uint64_t result = (uint64_t)(uintptr_t)thread_result;
                total = MultModulo(total, result, mod);
            }

            printf("Total: %llu\n", total);

            char buffer[sizeof(total)];
            memcpy(buffer, &total, sizeof(total));
            err = send(client_fd, buffer, sizeof(total), 0);
            if (err < 0) {
                fprintf(stderr, "Can't send data to client\n");
                break;
            }
        }

        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
    }

    return 0;
}