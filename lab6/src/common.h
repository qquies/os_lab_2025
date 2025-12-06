#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>

// Общая структура для вычислений факториала
struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};

// Общая структура для сервера
struct Server {
    char ip[255];
    int port;
};

// Прототипы функций
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);
bool ConvertStringToUI64(const char *str, uint64_t *val);
void PrintServerInfo(const struct Server *server);

#endif // COMMON_H