#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void* thread1(void* arg) {
    pthread_mutex_lock(&m1);
    printf("Поток 1 взял m1\n");
    sleep(1);               // даём второму потоку время
    printf("Поток 1 пытается взять m2\n");
    pthread_mutex_lock(&m2); // тут зависнет
    printf("Поток 1 взял m2\n");

    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    return NULL;
}

void* thread2(void* arg) {
    pthread_mutex_lock(&m2);
    printf("Поток 2 взял m2\n");
    sleep(1);               // даём первому потоку время
    printf("Поток 2 пытается взять m1\n");
    pthread_mutex_lock(&m1); // тут зависнет
    printf("Поток 2 взял m1\n");

    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Эта строка почти никогда не выведется (deadlock).\n");
    return 0;
}
