#include "sum_utils.h"
#include <stdio.h>
#include <stdlib.h>

void* calculate_partial_sum(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    data->partial_sum = 0;
    
    for (int i = data->start; i < data->end; i++) {
        data->partial_sum += data->array[i];
    }
    
    return NULL;
}

long long parallel_sum(int* array, int array_size, int threads_num) {
    pthread_t threads[threads_num];
    ThreadData thread_data[threads_num];
    
    int chunk_size = array_size / threads_num;
    
    // Создаем потоки для подсчета частичных сумм
    for (int i = 0; i < threads_num; i++) {
        thread_data[i].array = array;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i == threads_num - 1) ? array_size : (i + 1) * chunk_size;
        
        if (pthread_create(&threads[i], NULL, calculate_partial_sum, &thread_data[i]) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }
    
    // Ждем завершения всех потоков и суммируем результаты
    long long total_sum = 0;
    for (int i = 0; i < threads_num; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            exit(1);
        }
        total_sum += thread_data[i].partial_sum;
    }
    
    return total_sum;
}