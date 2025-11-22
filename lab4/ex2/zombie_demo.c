#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

void print_process_info(const char* message) {
    printf("%s: PID=%d, PPID=%d\n", message, getpid(), getppid());
}

void create_zombie() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        print_process_info("Дочерний процесс запущен");
        printf("Дочерний: Я стану зомби через 3 секунды...\n");
        sleep(3);
        print_process_info("Дочерний процесс завершается");
        exit(0);  // Завершаемся, но родитель не ждет нас
    } else if (pid > 0) {
        // Родительский процесс
        print_process_info("Родительский процесс");
        printf("Родитель: Создал дочерний процесс с PID=%d\n", pid);
        printf("Родитель: Я НЕ буду вызывать wait() - дочерний процесс станет зомби!\n");
        sleep(10);  // Родитель спит, не вызывая wait()
    } else {
        perror("Ошибка fork");
        exit(1);
    }
}

void create_normal_process() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        print_process_info("Нормальный дочерний процесс запущен");
        printf("Нормальный дочерний: Я завершусь нормально\n");
        exit(0);
    } else if (pid > 0) {
        // Родительский процесс
        int status;
        print_process_info("Хороший родитель");
        printf("Хороший родитель: Ожидаю завершения дочернего процесса PID=%d\n", pid);
        wait(&status);  // Родитель ждет завершения потомка
        printf("Хороший родитель: Дочерний процесс завершился со статусом %d\n", WEXITSTATUS(status));
    }
}

void demonstrate_zombie_cleanup() {
    printf("\n=== ДЕМОНСТРАЦИЯ ОЧИСТКИ ЗОМБИ-ПРОЦЕССОВ ===\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        print_process_info("Будущий зомби-процесс");
        printf("Дочерний: Я буду зомби 5 секунд\n");
        exit(42);
    } else if (pid > 0) {
        // Родительский процесс
        print_process_info("Родитель с планом очистки");
        printf("Родитель: Дочерний процесс PID=%d будет зомби 5 секунд\n", pid);
        
        // Не вызываем wait() сразу - создаем зомби
        printf("Родитель: Спим 5 секунд (зомби существует)...\n");
        sleep(5);
        
        // Теперь убираем зомби
        printf("Родитель: Теперь вызываю wait() для очистки зомби...\n");
        int status;
        pid_t finished_pid = wait(&status);
        
        if (finished_pid > 0) {
            printf("Родитель: Зомби очищен! PID=%d имел статус %d\n", 
                   finished_pid, WEXITSTATUS(status));
        }
    }
}

void check_zombies() {
    printf("\nПроверка зомби-процессов в системе...\n");
    printf("=== Текущие зомби-процессы ===\n");
    system("ps aux | head -1");  // Заголовок
    system("ps aux | grep defunct | grep -v grep");
    
    printf("\nАльтернативная проверка:\n");
    system("ps -eo pid,ppid,state,comm | awk '$3==\"Z\" {print $0}'");
}

void show_system_info() {
    printf("\n=== ИНФОРМАЦИЯ О СИСТЕМЕ ===\n");
    printf("Максимальное количество процессов: ");
    fflush(stdout);
    system("ulimit -u");
    
    printf("Текущее количество процессов: ");
    fflush(stdout);
    system("ps aux | wc -l");
}

int main() {
    printf("=== ДЕМОНСТРАЦИЯ ЗОМБИ-ПРОЦЕССОВ ===\n\n");
    
    int choice;
    while (1) {
        printf("\nВыберите демонстрацию:\n");
        printf("1. Создать зомби-процесс\n");
        printf("2. Создать нормальный процесс (с wait)\n");
        printf("3. Показать очистку зомби-процессов\n");
        printf("4. Проверить зомби-процессы в системе\n");
        printf("5. Показать информацию о системе\n");
        printf("6. Выход\n");
        printf("Ваш выбор: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Ошибка ввода!\n");
            while (getchar() != '\n'); // Очистка буфера
            continue;
        }
        
        switch (choice) {
            case 1:
                printf("\n=== СОЗДАНИЕ ЗОМБИ-ПРОЦЕССА ===\n");
                create_zombie();
                break;
            case 2:
                printf("\n=== СОЗДАНИЕ НОРМАЛЬНОГО ПРОЦЕССА ===\n");
                create_normal_process();
                break;
            case 3:
                demonstrate_zombie_cleanup();
                break;
            case 4:
                check_zombies();
                break;
            case 5:
                show_system_info();
                break;
            case 6:
                printf("Выход из программы...\n");
                return 0;
            default:
                printf("Неверный выбор!\n");
        }
        
        // Даем время увидеть результаты
        printf("\nНажмите Enter для продолжения...");
        getchar(); getchar(); // Ожидание Enter
    }
    
    return 0;
}