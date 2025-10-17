#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();  // создаём процесс

    if (pid == 0) {
        // дочерний процесс — запускаем нашу программу sequential_min_max
        // seed = 42, array_size = 100
        execl("./sequential_min_max", "sequential_min_max", "42", "100", NULL);

        // если execl не сработал
        perror("execl failed");
        exit(1);
    } else if (pid > 0) {
        // родитель ждёт завершения ребёнка
        wait(NULL);
        printf("sequential_min_max finished!\n");
    } else {
        perror("fork failed");
        return 1;
    }

    return 0;
}
