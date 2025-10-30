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
#include <signal.h>
#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int *child_pids;  // массив PIDов дочерних процессов
int pnum = 0;     // количество процессов

// обработчик сигнала SIGALRM
void timeout_handler(int signum) {
    printf("⏰ Таймаут! Завершаем все дочерние процессы...\n");
    for (int i = 0; i < pnum; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGKILL);
        }
    }
}

int main(int argc, char **argv) {
  int seed = 0;
  int array_size = 0;
  bool with_files = false;
  int timeout = 0; // значение таймаута в секундах (по умолчанию 0)

  // ---- разбор аргументов командной строки ----
  while (true) {
    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {"timeout", required_argument, 0, 0},
        {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);
    if (c == -1) break;
    if (c == 0) {
      if (option_index == 0) seed = atoi(optarg);
      if (option_index == 1) array_size = atoi(optarg);
      if (option_index == 2) pnum = atoi(optarg);
      if (option_index == 3) with_files = true;
      if (option_index == 4) timeout = atoi(optarg); // читаем timeout
    } else if (c == 'f') with_files = true;
  }

  if (pnum <= 0 || array_size <= 0) {
    printf("Неверные параметры: pnum и array_size должны быть > 0\n");
    return 1;
  }

  // ---- создаём и заполняем массив ----
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  // ---- время старта ----
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // ---- создаём pipe ----
  int pipefd[2];
  if (!with_files) pipe(pipefd);

  // ---- создаём массив PIDов ----
  child_pids = malloc(sizeof(int) * pnum);

  // ---- если задан таймаут, ставим обработчик и будильник ----
  if (timeout > 0) {
      signal(SIGALRM, timeout_handler);
      alarm(timeout);
  }

  int part = array_size / pnum;

  // ---- создаём дочерние процессы ----
  for (int i = 0; i < pnum; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      // --- дочерний процесс ---
      int start = i * part;
      int end = (i == pnum - 1) ? array_size : (i + 1) * part;

      struct MinMax mm = GetMinMax(array, start, end);

      if (with_files) {
        char filename[256];
        sprintf(filename, "temp_%d.txt", i);
        FILE *f = fopen(filename, "w");
        fprintf(f, "%d %d", mm.min, mm.max);
        fclose(f);
      } else {
        write(pipefd[1], &mm.min, sizeof(int));
        write(pipefd[1], &mm.max, sizeof(int));
      }

      free(array);
      return 0;
    } else {
      // родитель сохраняет PID
      child_pids[i] = pid;
    }
  }

  // ---- ждём завершения всех детей ----
  for (int i = 0; i < pnum; i++) wait(NULL);

  // ---- собираем результаты ----
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = 0, max = 0;
    if (with_files) {
      char filename[256];
      sprintf(filename, "temp_%d.txt", i);
      FILE *f = fopen(filename, "r");
      fscanf(f, "%d %d", &min, &max);
      fclose(f);
    } else {
      read(pipefd[0], &min, sizeof(int));
      read(pipefd[0], &max, sizeof(int));
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  // ---- время окончания ----
  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time =
      (finish_time.tv_sec - start_time.tv_sec) * 1000.0 +
      (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  return 0;
}
