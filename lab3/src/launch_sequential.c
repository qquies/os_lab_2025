#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("Родительский процесс (PID: %d)\n", getpid());
    
    pid_t pid = fork();
    
    if (pid == 0) {
        printf("Дочерний процесс (PID: %d)\n", getpid());
        
        // Запускаем sequential_min_max в ДОЧЕРНЕМ процессе
        execl("./sequential_min_max", "sequential_min_max", "42", "1000", NULL);
        
    } else {
        wait(NULL);
        printf("Дочерний процесс завершился\n");
    }
    
    return 0;
}