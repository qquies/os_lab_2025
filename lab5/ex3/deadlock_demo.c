#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

// Два мьютекса для демонстрации deadlock
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// Функция для первого потока
void* thread1_function(void* arg) {
    printf("Thread 1: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 1: Locked mutex1\n");
    
    // Имитация работы
    sleep(1);
    
    printf("Thread 1: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);  // DEADLOCK - mutex2 уже захвачен thread2
    printf("Thread 1: Locked mutex2\n");
    
    // Критическая секция
    printf("Thread 1: Entering critical section\n");
    sleep(1);
    printf("Thread 1: Leaving critical section\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    return NULL;
}

// Функция для второго потока
void* thread2_function(void* arg) {
    printf("Thread 2: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 2: Locked mutex2\n");
    
    // Имитация работы
    sleep(1);
    
    printf("Thread 2: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);  // DEADLOCK - mutex1 уже захвачен thread1
    printf("Thread 2: Locked mutex1\n");
    
    // Критическая секция
    printf("Thread 2: Entering critical section\n");
    sleep(1);
    printf("Thread 2: Leaving critical section\n");
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    return NULL;
}

// Функция с таймаутом для демонстрации решения deadlock
void* thread_safe_function(void* arg) {
    int thread_id = *(int*)arg;
    
    if (thread_id == 1) {
        printf("Safe Thread 1: Locking mutex1 then mutex2\n");
        pthread_mutex_lock(&mutex1);
        printf("Safe Thread 1: Locked mutex1\n");
        
        sleep(1);
        
        pthread_mutex_lock(&mutex2);
        printf("Safe Thread 1: Locked mutex2\n");
        
        printf("Safe Thread 1: Critical section\n");
        sleep(1);
        
        pthread_mutex_unlock(&mutex2);
        pthread_mutex_unlock(&mutex1);
    } else {
        printf("Safe Thread 2: Locking mutex1 then mutex2\n");
        pthread_mutex_lock(&mutex1);  // Тот же порядок блокировки!
        printf("Safe Thread 2: Locked mutex1\n");
        
        sleep(1);
        
        pthread_mutex_lock(&mutex2);
        printf("Safe Thread 2: Locked mutex2\n");
        
        printf("Safe Thread 2: Critical section\n");
        sleep(1);
        
        pthread_mutex_unlock(&mutex2);
        pthread_mutex_unlock(&mutex1);
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    pthread_t thread1, thread2;
    
    printf("=== DEADLOCK DEMONSTRATION ===\n\n");
    
    if (argc > 1 && strcmp(argv[1], "--safe") == 0) {
        // Безопасная версия
        printf("Running SAFE version (no deadlock):\n");
        printf("Both threads lock mutexes in the same order\n\n");
        
        int id1 = 1, id2 = 2;
        pthread_create(&thread1, NULL, thread_safe_function, &id1);
        pthread_create(&thread2, NULL, thread_safe_function, &id2);
    } else {
        // Версия с deadlock
        printf("Running DEADLOCK version:\n");
        printf("Thread 1: mutex1 -> mutex2\n");
        printf("Thread 2: mutex2 -> mutex1\n");
        printf("Program will hang in deadlock...\n\n");
        
        pthread_create(&thread1, NULL, thread1_function, NULL);
        pthread_create(&thread2, NULL, thread2_function, NULL);
    }
    
    // Ждем завершения потоков с таймаутом
    sleep(5);
    
    printf("\n=== MAIN THREAD ===\n");
    printf("If you see this message quickly, deadlock occurred!\n");
    printf("If threads completed normally, safe version was used.\n");
    printf("Press Ctrl+C to exit if program is hanging.\n");
    
    // Пытаемся присоединить потоки (скорее всего, зависнет)
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("All threads completed (this shouldn't happen in deadlock version)\n");
    
    return 0;
}