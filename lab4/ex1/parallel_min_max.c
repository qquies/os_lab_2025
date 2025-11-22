#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные для хранения PID дочерних процессов
pid_t *child_pids = NULL;
int timeout = 0; // 0 означает таймаут не задан
int pnum = 0;    // Количество процессов

// Обработчик для SIGALRM
void timeout_handler(int sig) {
    printf("Timeout reached! Sending SIGKILL to all child processes.\n");
    if (child_pids != NULL) {
        for (int i = 0; i < pnum; i++) {
            if (child_pids[i] > 0) {
                printf("Killing child process %d\n", child_pids[i]);
                kill(child_pids[i], SIGKILL);
            }
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  pnum = -1;
  bool with_files = false;
  timeout = 0; // Инициализация таймаута

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4: // timeout
            timeout = atoi(optarg);
            if (timeout <= 0) {
                printf("timeout must be a positive number\n");
                return 1;
            }
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"seconds\"]\n",
           argv[0]);
    return 1;
  }

  // Инициализация массива для хранения PID дочерних процессов
  child_pids = malloc(sizeof(pid_t) * pnum);
  for (int i = 0; i < pnum; i++) {
      child_pids[i] = 0;
  }

  // Установка обработчика сигнала SIGALRM, если задан таймаут
  if (timeout > 0) {
      if (signal(SIGALRM, timeout_handler) == SIG_ERR) {
          printf("Failed to set signal handler\n");
          free(child_pids);
          return 1;
      }
      printf("Timeout set to %d seconds\n", timeout);
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  // Создаем pipes или файлы для каждого процесса
  int pipes[pnum][2];
  char filenames[pnum][100];
  
  if (!with_files) {
    // Создаем pipes для всех процессов
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        printf("Pipe creation failed!\n");
        free(child_pids);
        free(array);
        return 1;
      }
    }
  } else {
    // Генерируем имена файлов
    for (int i = 0; i < pnum; i++) {
      sprintf(filenames[i], "min_max_result_%d.txt", i);
      // Удаляем файлы если они существуют
      unlink(filenames[i]);
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Устанавливаем будильник, если задан таймаут
  if (timeout > 0) {
      alarm(timeout);
  }

  // Создаем процессы
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process
        
        // Вычисляем границы для этого процесса
        int chunk_size = array_size / pnum;
        int start = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;
        
        printf("Child process %d (PID: %d) processing elements %d to %d\n", 
               i, getpid(), start, end);
        
        // Ищем min/max в своей части массива
        struct MinMax local_min_max = GetMinMax(array, start, end);
        
        printf("Child process %d found min: %d, max: %d\n", 
               i, local_min_max.min, local_min_max.max);
        
        if (with_files) {
          // use files here
          FILE* file = fopen(filenames[i], "w");
          if (file != NULL) {
            fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
            fclose(file);
          } else {
            printf("Child %d: Failed to open file\n", i);
          }
        } else {
          // use pipe here
          close(pipes[i][0]); // закрываем чтение в потомке
          write(pipes[i][1], &local_min_max.min, sizeof(local_min_max.min));
          write(pipes[i][1], &local_min_max.max, sizeof(local_min_max.max));
          close(pipes[i][1]);
        }
        free(array);
        exit(0);
      } else {
        // В родительском процессе сохраняем PID дочернего
        child_pids[i] = child_pid;
        printf("Created child process %d with PID: %d\n", i, child_pid);
      }

    } else {
      printf("Fork failed!\n");
      free(child_pids);
      free(array);
      return 1;
    }
  }

  // Родительский процесс ждет всех потомков и собирает результаты
  int child_status;
  pid_t finished_pid;
  int timeout_occurred = 0;
  
  printf("Parent process waiting for children...\n");
  
  while (active_child_processes > 0) {
    // Используем неблокирующий waitpid с WNOHANG
    finished_pid = waitpid(-1, &child_status, WNOHANG);
    
    if (finished_pid > 0) {
        // Найден завершившийся процесс
        active_child_processes -= 1;
        
        // Убираем PID из массива
        for (int i = 0; i < pnum; i++) {
            if (child_pids[i] == finished_pid) {
                child_pids[i] = 0;
                break;
            }
        }
        
        if (WIFSIGNALED(child_status)) {
            printf("Child process %d was terminated by signal %d\n", 
                   finished_pid, WTERMSIG(child_status));
            if (WTERMSIG(child_status) == SIGKILL) {
                timeout_occurred = 1;
            }
        } else if (WIFEXITED(child_status)) {
            printf("Child process %d exited normally with status %d\n", 
                   finished_pid, WEXITSTATUS(child_status));
        }
    } else if (finished_pid == 0) {
        // Дочерние процессы еще работают, ждем немного
        usleep(100000); // 100ms
    } else {
        // Ошибка
        if (errno != ECHILD) {
            perror("waitpid");
        }
        break;
    }
  }

  // Отменяем будильник, если он еще не сработал
  if (timeout > 0) {
      alarm(0);
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  // Собираем результаты от всех процессов
  int results_collected = 0;
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    int result_available = 1;

    if (with_files) {
      // read from files
      FILE* file = fopen(filenames[i], "r");
      if (file != NULL) {
        if (fscanf(file, "%d %d", &min, &max) != 2) {
            result_available = 0;
        }
        fclose(file);
        // Удаляем временный файл
        unlink(filenames[i]);
      } else {
        result_available = 0;
      }
    } else {
      // read from pipes
      close(pipes[i][1]); // закрываем запись в родителе
      if (read(pipes[i][0], &min, sizeof(min)) != sizeof(min)) {
          result_available = 0;
      }
      if (read(pipes[i][0], &max, sizeof(max)) != sizeof(max)) {
          result_available = 0;
      }
      close(pipes[i][0]);
    }

    if (result_available) {
        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
        results_collected++;
    } else {
        printf("No results from process %d (may have been terminated)\n", i);
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);

  printf("\n=== RESULTS ===\n");
  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Results collected from %d out of %d processes\n", results_collected, pnum);
  printf("Elapsed time: %fms\n", elapsed_time);
  printf("Used method: %s\n", with_files ? "files" : "pipes");
  if (timeout > 0) {
      printf("Timeout: %d seconds\n", timeout);
      if (timeout_occurred) {
          printf("WARNING: Timeout occurred! Some processes were terminated.\n");
      }
  }
  fflush(NULL);
  return 0;
}