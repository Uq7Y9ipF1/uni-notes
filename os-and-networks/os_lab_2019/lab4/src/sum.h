#ifndef SUM_H
#define SUM_H

struct SumArgs {
  int *array;
  int begin;
  int end;
};

// Сумма элементов array[begin..end) -- вынесена в отдельную библиотеку
int Sum(const struct SumArgs *args);

#endif
