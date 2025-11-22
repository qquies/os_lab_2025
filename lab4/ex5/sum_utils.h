#ifndef SUM_UTILS_H
#define SUM_UTILS_H

#include <pthread.h>

// Структура для передачи данных в поток
typedef struct {
    int* array;
    int start;
    int end;
    long long partial_sum;
} ThreadData;

// Функция для подсчета частичной суммы (будет использоваться в потоках)
void* calculate_partial_sum(void* arg);

// Функция для параллельного подсчета суммы массива
long long parallel_sum(int* array, int array_size, int threads_num);

#endif