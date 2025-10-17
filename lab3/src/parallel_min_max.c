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

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = 0;         // для генерации массива
  int array_size = 0;   // размер массива
  int pnum = 0;         // количество процессов
  bool with_files = false; // использовать ли файлы для обмена

  // ---- разбор аргументов командной строки ----
  while (true) {
    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);
    if (c == -1) break; // больше нет аргументов
    if (c == 0) {
      if (option_index == 0) seed = atoi(optarg);
      if (option_index == 1) array_size = atoi(optarg);
      if (option_index == 2) pnum = atoi(optarg);
      if (option_index == 3) with_files = true;
    } else if (c == 'f') with_files = true;
  }

  // ---- создаём и заполняем массив ----
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  // ---- время старта ----
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // ---- если используем pipe, создаём его ----
  int pipefd[2];
  if (!with_files) pipe(pipefd);

  // ---- делим массив на части для процессов ----
  int part = array_size / pnum;

  // ---- создаём дочерние процессы ----
  for (int i = 0; i < pnum; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      // --- дочерний процесс ---
      int start = i * part;
      int end = (i == pnum - 1) ? array_size : (i + 1) * part;

      // считаем min и max для своей части
      struct MinMax mm = GetMinMax(array, start, end);

      if (with_files) {
        // если передача через файл — записываем результаты
        char filename[256];
        sprintf(filename, "temp_%d.txt", i);
        FILE *f = fopen(filename, "w");
        fprintf(f, "%d %d", mm.min, mm.max);
        fclose(f);
      } else {
        // иначе — передаём через pipe
        write(pipefd[1], &mm.min, sizeof(int));
        write(pipefd[1], &mm.max, sizeof(int));
      }

      free(array);
      return 0; // завершаем процесс ребёнка
    }
  }

  // ---- ждём завершения всех детей ----
  for (int i = 0; i < pnum; i++) wait(NULL);

  // ---- собираем результаты от всех процессов ----
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = 0, max = 0;
    if (with_files) {
      // читаем из временных файлов
      char filename[256];
      sprintf(filename, "temp_%d.txt", i);
      FILE *f = fopen(filename, "r");
      fscanf(f, "%d %d", &min, &max);
      fclose(f);
    } else {
      // читаем из pipe
      read(pipefd[0], &min, sizeof(int));
      read(pipefd[0], &max, sizeof(int));
    }

    // обновляем общий минимум и максимум
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

  // ---- выводим результат ----
  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  return 0;
}
