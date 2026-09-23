#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Задание 5: запускаем sequential_min_max в отдельном процессе
// с помощью fork + execvp. Родитель ждет завершения ребенка.
int main(int argc, char **argv) {
  pid_t pid = fork();

  if (pid == -1) {
    perror("Fork failed");
    return 1;
  }

  if (pid == 0) {
    // Дочерний процесс: заменяем свой образ на sequential_min_max.
    // Аргументы: seed = 42, array_size = 100000
    char *args[] = {"./sequential_min_max", "42", "100000", NULL};
    execvp(args[0], args);

    // Если execvp вернул управление -- произошла ошибка
    perror("Execvp failed");
    exit(1);
  }

  // Родительский процесс: ждем завершения ребенка
  int status = 0;
  if (waitpid(pid, &status, 0) == -1) {
    perror("Waitpid failed");
    return 1;
  }

  if (WIFEXITED(status)) {
    printf("Child process finished with status %d\n", WEXITSTATUS(status));
  } else {
    printf("Child process terminated abnormally\n");
    return 1;
  }

  return 0;
}
