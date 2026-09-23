// Лабораторная работа №5, задание 3: демонстрация взаимоблокировки (deadlock).
//
// Два потока берут два мьютекса в РАЗНОМ порядке:
//   поток 1: lock(A) ... lock(B)
//   поток 2: lock(B) ... lock(A)
// Если поток 1 захватил A, а поток 2 успел захватить B, каждый ждет освобождения
// мьютекса, который держит другой -> круговое ожидание -> deadlock.
// Программа зависает намеренно; завершите её через Ctrl+C.
//
// Профилактика: всегда захватывать мьютексы в одном и том же глобальном
// порядке, использовать pthread_mutex_trylock с откатом, или уменьшать
// время удержания нескольких локов.
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t b = PTHREAD_MUTEX_INITIALIZER;

void *thread1(void *arg) {
  pthread_mutex_lock(&a);
  printf("thread1 locked A\n");
  usleep(100000); // даем thread2 перехватить B
  printf("thread1 waiting for B...\n");
  pthread_mutex_lock(&b); // блокируется навсегда
  printf("thread1 got B (never happens)\n");
  pthread_mutex_unlock(&b);
  pthread_mutex_unlock(&a);
  return NULL;
}

void *thread2(void *arg) {
  pthread_mutex_lock(&b);
  printf("thread2 locked B\n");
  usleep(100000);
  printf("thread2 waiting for A...\n");
  pthread_mutex_lock(&a); // блокируется навсегда
  printf("thread2 got A (never happens)\n");
  pthread_mutex_unlock(&a);
  pthread_mutex_unlock(&b);
  return NULL;
}

int main(void) {
  pthread_t t1, t2;
  pthread_create(&t1, NULL, thread1, NULL);
  pthread_create(&t2, NULL, thread2, NULL);
  pthread_join(t1, NULL); // сюда никогда не придем
  pthread_join(t2, NULL);
  printf("Done (unreachable)\n");
  return 0;
}
