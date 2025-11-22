#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <pthread.h>

#include "utils.h"
#include "sum_utils.h"

int main(int argc, char **argv) {
    int threads_num = -1;
    int seed = -1;
    int array_size = -1;

    // Разбор аргументов командной строки
    while (1) {
        static struct option options[] = {
            {"threads_num", required_argument, 0, 't'},
            {"seed", required_argument, 0, 's'},
            {"array_size", required_argument, 0, 'a'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "t:s:a:", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 't':
                threads_num = atoi(optarg);
                if (threads_num <= 0) {
                    printf("threads_num must be a positive number\n");
                    return 1;
                }
                break;
            case 's':
                seed = atoi(optarg);
                if (seed <= 0) {
                    printf("seed must be a positive number\n");
                    return 1;
                }
                break;
            case 'a':
                array_size = atoi(optarg);
                if (array_size <= 0) {
                    printf("array_size must be a positive number\n");
                    return 1;
                }
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (threads_num == -1 || seed == -1 || array_size == -1) {
        printf("Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n", argv[0]);
        return 1;
    }

    // Выделяем память под массив
    int *array = malloc(sizeof(int) * array_size);
    if (array == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }

    // Генерируем массив (не входит в замер времени)
    printf("Generating array with size %d...\n", array_size);
    GenerateArray(array, array_size, seed);

    // Замер времени начала вычислений
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Параллельный подсчет суммы
    long long total_sum = parallel_sum(array, array_size, threads_num);

    // Замер времени окончания вычислений
    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    // Вычисляем время выполнения
    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    // Выводим результаты
    printf("\n=== PARALLEL SUM RESULTS ===\n");
    printf("Threads number: %d\n", threads_num);
    printf("Array size: %d\n", array_size);
    printf("Total sum: %lld\n", total_sum);
    printf("Elapsed time: %.3f ms\n", elapsed_time);

    // Проверка последовательным подсчетом (для верификации)
    long long sequential_sum = 0;
    for (int i = 0; i < array_size; i++) {
        sequential_sum += array[i];
    }
    printf("Sequential sum: %lld\n", sequential_sum);
    printf("Results match: %s\n", (total_sum == sequential_sum) ? "YES" : "NO");

    free(array);
    return 0;
}