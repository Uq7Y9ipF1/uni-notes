// Лабораторная работа №4, задание 5: параллельное суммирование массива
// с помощью POSIX threads. Функция Sum вынесена в библиотеку sum.c/sum.h.
// Запуск: ./psum --threads_num N --seed N --array_size N
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <sys/time.h>
#include <getopt.h>

#include "sum.h"
#include "utils.h" // GenerateArray из лабораторной работы №3

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  while (true) {
    static struct option options[] = {{"threads_num", required_argument, 0, 0},
                                      {"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;

    switch (c) {
    case 0:
      switch (option_index) {
      case 0: threads_num = (uint32_t)atoi(optarg); break;
      case 1: seed = (uint32_t)atoi(optarg); break;
      case 2: array_size = (uint32_t)atoi(optarg); break;
      default: printf("Index %d is out of options\n", option_index);
      }
      break;
    case '?': printf("Arguments error\n"); break;
    default: fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (threads_num == 0 || array_size == 0) {
    fprintf(stderr, "Using: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n",
            argv[0]);
    return 1;
  }
  if (threads_num > array_size) threads_num = array_size;

  // Генерация массива НЕ входит в замер времени
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];
  uint32_t chunk = array_size / threads_num;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = (int)(i * chunk);
    // последний поток забирает остаток до конца массива
    args[i].end = (i == threads_num - 1) ? (int)array_size
                                         : (int)((i + 1) * chunk);

    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    int sum = 0;
    pthread_join(threads[i], (void **)&sum);
    total_sum += sum;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);
  double elapsed_ms = (finish_time.tv_sec - start_time.tv_sec) * 1000.0 +
                      (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  printf("Total: %d\n", total_sum);
  printf("Elapsed time: %fms\n", elapsed_ms);
  return 0;
}
