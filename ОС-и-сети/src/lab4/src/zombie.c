// Лабораторная работа №4, задание 2: демонстрация зомби-процессов.
//
// Как появляются зомби: когда дочерний процесс завершается, ядро не удаляет
// сразу его запись в таблице процессов -- родитель должен "забрать" код
// завершения через wait()/waitpid(). Пока родитель этого не сделал, процесс
// находится в состоянии Z (zombie): он уже ничего не потребляет (память,
// дескрипторы освобождены), кроме записи в таблице процессов (PID).
//
// Чем опасны: при массовом/постоянном создании детей без wait() у родителя
// исчерпывается лимит PID (fork перестает работать).
//
// Как избавиться: 1) вызывать wait()/waitpid(); 2) игнорировать SIGCHLD
// (signal(SIGCHLD, SIG_IGN)) -- тогда ядро само уберет запись; 3) двойной
// fork (осиротевший ребенок немедленно удочеряется init/systemd и дожидается).
//
// Демонстрация: родитель создает ребенка, который сразу завершается, а сам
// спит 15 секунд, НЕ вызывая wait(). В другой терминале видно:
//   ps -o pid,ppid,stat,cmd --ppid <pid_родителя>   -> STAT = Z
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(void) {
  printf("Parent pid = %d\n", (int)getpid());
  pid_t child = fork();
  if (child < 0) {
    perror("fork");
    return 1;
  }
  if (child == 0) {
    // Ребенок мгновенно завершается -> становится зомби,
    // потому что родитель не вызывает wait().
    printf("Child pid = %d, exiting immediately\n", (int)getpid());
    exit(0);
  }
  printf("Now run: ps -o pid,ppid,stat,cmd | grep 'Z'  (parent sleeps 15s)\n");
  sleep(15); // родитель не делает wait -> ребенок висит зомби
  printf("Parent woke up; child will be reaped by implicit wait at exit\n");
  return 0;
}
