#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <string.h>

// Глобальный мьютекс для синхронизации доступа к общему результату
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

// Структура для передачи данных в поток
typedef struct {
    int start;
    int end;
    long long mod;
    long long partial_result;
    long long* total_result;  // Указатель на общий результат
} ThreadData;

// Функция для вычисления частичного факториала
void* calculate_partial_factorial(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    data->partial_result = 1;
    
    // Вычисляем частичный результат
    for (int i = data->start; i <= data->end; i++) {
        data->partial_result = (data->partial_result * i) % data->mod;
    }
    
    printf("Thread: factorial from %d to %d mod %lld = %lld\n", 
           data->start, data->end, data->mod, data->partial_result);
    
    // СИНХРОНИЗАЦИЯ: Блокируем мьютекс для обновления общего результата
    pthread_mutex_lock(&result_mutex);
    
    // Обновляем общий результат
    *data->total_result = (*data->total_result * data->partial_result) % data->mod;
    printf("Thread: updated total result to %lld\n", *data->total_result);
    
    // СИНХРОНИЗАЦИЯ: Разблокируем мьютекс
    pthread_mutex_unlock(&result_mutex);
    
    return NULL;
}

// Функция для параллельного вычисления факториала
long long parallel_factorial(int k, int threads_num, long long mod) {
    if (k <= 1) return 1;
    
    pthread_t threads[threads_num];
    ThreadData thread_data[threads_num];
    
    // Общий результат, защищаемый мьютексом
    long long total_result = 1;
    
    // Распределяем числа между потоками
    int numbers_per_thread = k / threads_num;
    int remainder = k % threads_num;
    
    int current_start = 1;
    
    // Создаем потоки
    for (int i = 0; i < threads_num; i++) {
        int numbers_count = numbers_per_thread;
        if (i < remainder) {
            numbers_count++;
        }
        
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + numbers_count - 1;
        thread_data[i].mod = mod;
        thread_data[i].total_result = &total_result;  // Передаем указатель на общий результат
        
        printf("Thread %d: numbers from %d to %d\n", 
               i, thread_data[i].start, thread_data[i].end);
        
        if (pthread_create(&threads[i], NULL, calculate_partial_factorial, &thread_data[i]) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
        
        current_start += numbers_count;
    }
    
    // Ждем завершения всех потоков
    for (int i = 0; i < threads_num; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            exit(1);
        }
    }
    
    return total_result;
}

// Функция для проверки (последовательное вычисление)
long long sequential_factorial(int k, long long mod) {
    long long result = 1;
    for (int i = 2; i <= k; i++) {
        result = (result * i) % mod;
    }
    return result;
}

int main(int argc, char *argv[]) {
    int k = -1;
    int threads_num = -1;
    long long mod = -1;
    
    // Разбор аргументов командной строки
    static struct option long_options[] = {
        {"k", required_argument, 0, 'k'},
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "k:p:m:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'k':
                k = atoi(optarg);
                if (k < 0) {
                    printf("k must be non-negative\n");
                    return 1;
                }
                break;
            case 'p':
                threads_num = atoi(optarg);
                if (threads_num <= 0) {
                    printf("pnum must be positive\n");
                    return 1;
                }
                break;
            case 'm':
                mod = atoll(optarg);
                if (mod <= 0) {
                    printf("mod must be positive\n");
                    return 1;
                }
                break;
            case '?':
                printf("Unknown option\n");
                return 1;
        }
    }
    
    // Проверка обязательных параметров
    if (k == -1 || threads_num == -1 || mod == -1) {
        printf("Usage: %s -k <number> --pnum=<threads> --mod=<modulus>\n", argv[0]);
        printf("Example: %s -k 10 --pnum=4 --mod=1000000007\n", argv[0]);
        return 1;
    }
    
    printf("=== PARALLEL FACTORIAL COMPUTATION WITH MUTEX ===\n");
    printf("k = %d\n", k);
    printf("Threads number = %d\n", threads_num);
    printf("Modulus = %lld\n", mod);
    printf("\n");
    
    // Параллельное вычисление
    long long parallel_result = parallel_factorial(k, threads_num, mod);
    
    // Последовательное вычисление для проверки
    long long sequential_result = sequential_factorial(k, mod);
    
    // Вывод результатов
    printf("\n=== RESULTS ===\n");
    printf("Parallel result: %d! mod %lld = %lld\n", k, mod, parallel_result);
    printf("Sequential result: %d! mod %lld = %lld\n", k, mod, sequential_result);
    printf("Results match: %s\n", 
           (parallel_result == sequential_result) ? "YES" : "NO");
    
    // Уничтожаем глобальный мьютекс
    pthread_mutex_destroy(&result_mutex);
    
    return 0;
}