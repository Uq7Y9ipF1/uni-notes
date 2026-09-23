// Лабораторная работа №4, задание 1.
// Программа скопирована из лабораторной работы №3 (lab3/src/parallel_min_max.c)
// и дополнена именованным необязательным параметром командной строки
// --timeout N: если по истечении N секунд дочерние процессы еще не завершились,
// родитель шлет каждому оставшемуся живым ребенку SIGKILL.
// Если таймаут не задан (--timeout отсутствует), поведение программы
// в точности совпадает с поведением из лабораторной работы №3.
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
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

#define MAX_CHILDREN 1024

// Имя временного файла, через который i-й дочерний процесс
// передает результат родителю (задание 2 -- синхронизация через файлы)
static void ChildFilename(int index, char *buffer, size_t buffer_size) {
  snprintf(buffer, buffer_size, "tmp_%d.txt", index);
}

// Возвращает true, если pid все еще жив (не завершен и не зомби).
// signal(pid, 0) ничего не посылает, но позволяет проверить существование
// процесса: ESRCH -- процесса нет, EPERM -- есть, но не наш.
static bool IsProcessAlive(pid_t pid) {
  if (pid <= 0) return false;
  if (kill(pid, 0) == 0) return true;
  return errno == EPERM;
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1; // секунды; -1 означает "таймаут не задан"
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"timeout", required_argument, 0, 0},
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
            timeout = atoi(optarg);
            if (timeout <= 0) {
              printf("Timeout is a positive number\n");
              return 1;
            }
            break;
          case 4:
            with_files = true;
            break;

          default:
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
           "[--timeout \"sec\"] [--by_files]\n",
           argv[0]);
    return 1;
  }

  if (pnum > array_size) {
    printf("Processes count must not be greater than array size\n");
    return 1;
  }

  if (pnum > MAX_CHILDREN) {
    printf("Processes count must not be greater than %d\n", MAX_CHILDREN);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;
  pid_t children[MAX_CHILDREN];
  for (int i = 0; i < MAX_CHILDREN; i++) children[i] = -1;

  // Массив труб: pipes[i] -- труба для i-го дочернего процесса.
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

  int chunk_size = array_size / pnum;

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      children[i] = child_pid;
      if (child_pid == 0) {
        // child process
        unsigned int begin = i * chunk_size;
        unsigned int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;

        struct MinMax min_max = GetMinMax(array, begin, end);

        if (with_files) {
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
          close(pipes[i][0]);
          if (write(pipes[i][1], &min_max, sizeof(struct MinMax)) == -1) {
            perror("Writing to pipe failed");
            exit(1);
          }
          close(pipes[i][1]);
        }

        exit(0);
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[i][1]);
    }
  }

  // === Задание 1: ожидание детей с необязательным таймаутом ===
  // Если таймаут не задан, ждем ровно как в лабе 3 (блокирующий wait).
  // Иначе опрашиваем waitpid(..., WNOHANG) в цикле с коротким sleep.
  // Как только суммарное время ожидания превышает timeout, всем еще живым
  // детям отправляется SIGKILL, после чего мы обязательно дожидаемся их,
  // чтобы не оставить зомби-процессов.
  if (timeout <= 0) {
    while (active_child_processes > 0) {
      if (wait(NULL) == -1) {
        perror("Waiting for child processes failed");
        return 1;
      }
      active_child_processes -= 1;
    }
  } else {
    bool timed_out = false;
    while (active_child_processes > 0) {
      pid_t finished = -1;
      for (int i = 0; i < pnum; i++) {
        if (children[i] <= 0) continue;
        int status;
        pid_t r = waitpid(children[i], &status, WNOHANG);
        if (r == children[i]) {
          finished = children[i];
          children[i] = -1;
          active_child_processes--;
          break;
        }
      }

      if (active_child_processes == 0) break;

      // Проверяем, не пора ли убивать
      struct timeval now;
      gettimeofday(&now, NULL);
      double elapsed_ms = (now.tv_sec - start_time.tv_sec) * 1000.0 +
                          (now.tv_usec - start_time.tv_usec) / 1000.0;
      if (!timed_out && elapsed_ms > timeout * 1000.0) {
        timed_out = true;
        for (int i = 0; i < pnum; i++) {
          if (children[i] > 0 && IsProcessAlive(children[i])) {
            printf("Timeout reached, killing child %d\n", (int)children[i]);
            kill(children[i], SIGKILL);
          }
        }
      }

      usleep(50000); // 50 мс — частота опроса
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    if (with_files) {
      char filename[20];
      ChildFilename(i, filename, sizeof(filename));
      FILE *file = fopen(filename, "r");
      if (file == NULL) {
        // Ребенок мог быть убит по таймауту — тогда файла нет.
        printf("Child %d result file missing (likely killed by timeout)\n", i);
        continue;
      }
      if (fscanf(file, "%d %d", &min, &max) != 2) {
        perror("File reading failed");
        fclose(file);
        return 1;
      }
      fclose(file);
      remove(filename);
    } else {
      ssize_t bytes_read = read(pipes[i][0], &min_max, sizeof(struct MinMax));
      if (bytes_read != (ssize_t)sizeof(struct MinMax)) {
        // Ребенок был убит или закрыл трубу без записи
        printf("Child %d did not produce result (killed or crashed)\n", i);
      } else {
        min = min_max.min;
        max = min_max.max;
      }
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
