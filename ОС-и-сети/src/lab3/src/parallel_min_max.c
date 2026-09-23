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

// Имя временного файла, через который i-й дочерний процесс
// передает результат родителю (задание 2 -- синхронизация через файлы)
static void ChildFilename(int index, char *buffer, size_t buffer_size) {
  snprintf(buffer, buffer_size, "tmp_%d.txt", index);
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            // Обработка ошибок разбора аргументов командной строки
            if (seed <= 0) {
              printf("Seed is a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("Array size is a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("Processes count is a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          defalut:
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
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" "
           "[--by_files]\n",
           argv[0]);
    return 1;
  }

  if (pnum > array_size) {
    printf("Processes count must not be greater than array size\n");
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  // Массив труб: pipes[i] -- труба для i-го дочернего процесса.
  // Трубы создаются заранее, до fork, чтобы и родитель, и все дети
  // видели одни и те же дескрипторы (наследуются при fork).
  int (*pipes)[2] = NULL;
  if (!with_files) {
    pipes = malloc(sizeof(int[pnum][2]));
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        perror("Pipe creating failed");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Каждый дочерний процесс обрабатывает свой кусок массива:
  // [(i * chunk_size), ((i + 1) * chunk_size)), последний процесс
  // забирает остаток до конца массива.
  int chunk_size = array_size / pnum;

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process
        unsigned int begin = i * chunk_size;
        unsigned int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;

        struct MinMax min_max = GetMinMax(array, begin, end);

        if (with_files) {
          // Задание 2: пишем результат в свой временный файл
          char filename[20];
          ChildFilename(i, filename, sizeof(filename));
          FILE *file = fopen(filename, "w");
          if (file == NULL) {
            perror("File opening failed");
            exit(1);
          }
          fprintf(file, "%d %d", min_max.min, min_max.max);
          fclose(file);
        } else {
          // Задание 3: пишем результат (структуру MinMax) в свою трубу
          close(pipes[i][0]);  // чтение ребенку не нужно, только запись
          if (write(pipes[i][1], &min_max, sizeof(struct MinMax)) == -1) {
            perror("Writing to pipe failed");
            exit(1);
          }
          close(pipes[i][1]);
        }

        free(array);
        if (pipes) free(pipes);
        exit(0);
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Родитель больше не пишет в трубы -- закрываем концы записи,
  // чтобы read() корректно получил EOF после завершения детей.
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[i][1]);
    }
  }

  while (active_child_processes > 0) {
    // Ждем завершения всех дочерних процессов
    if (wait(NULL) == -1) {
      perror("Waiting for child processes failed");
      return 1;
    }
    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // Задание 2: читаем min/max из временного файла и удаляем его
      char filename[20];
      ChildFilename(i, filename, sizeof(filename));
      FILE *file = fopen(filename, "r");
      if (file == NULL) {
        perror("File opening failed");
        return 1;
      }
      if (fscanf(file, "%d %d", &min, &max) != 2) {
        perror("File reading failed");
        fclose(file);
        return 1;
      }
      fclose(file);
      remove(filename);
    } else {
      // Задание 3: читаем структуру MinMax из трубы i-го ребенка
      ssize_t bytes_read = read(pipes[i][0], &min_max, sizeof(struct MinMax));
      if (bytes_read != (ssize_t)sizeof(struct MinMax)) {
        perror("Reading from pipe failed");
        return 1;
      }
      min = min_max.min;
      max = min_max.max;
      close(pipes[i][0]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  if (pipes) free(pipes);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
