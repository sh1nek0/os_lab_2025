#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    int start;
    int end;
    int mod;
} ThreadArgs;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned long long global_result = 1;  // общий результат (по модулю mod)

// Функция потока: считает произведение чисел от start до end по модулю mod,
// затем под мьютексом домножает общий результат.
void* thread_func(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int start = args->start;
    int end   = args->end;
    int mod   = args->mod;

    if (mod <= 0) {
        return NULL;
    }

    unsigned long long local = 1;

    for (int i = start; i <= end; i++) {
        local = (local * i) % mod;
    }

    // Критическая секция: обновление общего результата
    pthread_mutex_lock(&mutex);
    global_result = (global_result * local) % mod;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

// Простейший парсер аргументов вида: -k 10 --pnum=4 --mod=10
int main(int argc, char* argv[]) {
    int k = -1;
    int pnum = -1;
    int mod = -1;

    // Парсинг аргументов
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            mod = atoi(argv[i] + 6);
        }
    }

    if (k < 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr,
                "Usage: %s -k <number> --pnum=<threads> --mod=<mod>\n",
                argv[0]);
        return 1;
    }

    // Частный случай: 0! = 1
    if (k == 0) {
        printf("Result: %d\n", 1 % mod);
        return 0;
    }

    pthread_t* threads = malloc(sizeof(pthread_t) * pnum);
    ThreadArgs* targs = malloc(sizeof(ThreadArgs) * pnum);

    if (!threads || !targs) {
        fprintf(stderr, "Memory allocation error\n");
        free(threads);
        free(targs);
        return 1;
    }

    // Разбиваем диапазон [1..k] на pnum отрезков
    int base = k / pnum;
    int rem  = k % pnum;

    int current = 1;
    for (int i = 0; i < pnum; i++) {
        int chunk = base + (i < rem ? 1 : 0); // первые rem потоков получают на 1 элемент больше

        if (chunk > 0) {
            targs[i].start = current;
            targs[i].end   = current + chunk - 1;
            targs[i].mod   = mod;
            current += chunk;
        } else {
            // Пустой диапазон для "лишних" потоков
            targs[i].start = 1;
            targs[i].end   = 0;
            targs[i].mod   = mod;
        }

        if (pthread_create(&threads[i], NULL, thread_func, &targs[i]) != 0) {
            perror("pthread_create");
            free(threads);
            free(targs);
            return 1;
        }
    }

    // Ждём завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            free(threads);
            free(targs);
            return 1;
        }
    }

    printf("Result: %llu\n", global_result % mod);

    free(threads);
    free(targs);
    return 0;
}
