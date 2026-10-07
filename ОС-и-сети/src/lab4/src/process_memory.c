
/* Program to display address information about the process */
/* Adapted from Gray, J., program 1.4 */
/* Задание 3 лабы 4: показать, в каких сегментах памяти процесса
   расположены код, данные, куча и стек.
   Исправления для чистой сборки (gcc -Wall):
   - %8X заменён на %p: адреса печатаются как void* (портативно, 64 бита);
   - main() получил явный тип int;
   - showit переписан из устаревшего K&R-стиля в ANSI C. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Ниже — определение макроса */
#define SHW_ADR(ID, I)                                                         \
  (printf("ID %s \t is at virtual address: %p\n", ID, (void *)&(I)))

/* Символы, которые генерирует компоновщик (ld):
   etext — конец текстового сегмента,
   edata — конец инициализированных данных,
   end   — конец неинициализированных данных (начало кучи). */
extern char etext, edata, end;

char *cptr = "This message is output by the function showit()\n"; /* Статические данные */
char buffer1[25]; /* Неинициализированные данные (BSS) */
int showit(char *p); /* Прототип функции (ANSI C) */

int main(void) {
  int i = 0; /* Автоматическая переменная (стек) */

  /* Печать адресной информации */
  printf("\nAddress etext: %p \n", (void *)&etext);
  printf("Address edata: %p \n", (void *)&edata);
  printf("Address end  : %p \n", (void *)&end);

  SHW_ADR("main", main);     /* текст (код) */
  SHW_ADR("showit", showit); /* текст (код) */
  SHW_ADR("cptr", cptr);     /* инициализированные данные */
  SHW_ADR("buffer1", buffer1); /* BSS */
  SHW_ADR("i", i);           /* стек */
  strcpy(buffer1, "A demonstration\n");   /* Библиотечная функция */
  write(1, buffer1, strlen(buffer1) + 1); /* Системный вызов */
  showit(cptr);
  return 0;

} /* конец main */

/* Далее следует функция */
int showit(char *p) {
  char *buffer2;
  SHW_ADR("buffer2", buffer2); /* сама buffer2 — на стеке */
  if ((buffer2 = malloc(strlen(p) + 1)) != NULL) {
    printf("Allocated memory at %p\n", (void *)buffer2); /* куча */
    strcpy(buffer2, p);    /* копирование строки */
    printf("%s", buffer2); /* вывод строки */
    free(buffer2);         /* освобождение памяти */
  } else {
    printf("Allocation error\n");
    exit(1);
  }
  return 0;
}
