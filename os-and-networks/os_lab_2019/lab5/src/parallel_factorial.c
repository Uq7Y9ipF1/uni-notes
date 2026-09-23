// Лабораторная работа №5, задание 2: параллельное вычисление k! % mod.
// Запуск: ./parallel_factorial -k 10 --pnum=4 --mod=10
// Массив множителей [1..k] делится между pnum потоками; каждый поток считает
// произведение своего куска по модулю (с защитой от переполнения через
// MultModulo), затем результаты объединяются. Для синхронизации доступа к
// общей переменной result используется мьютекс.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <pthread.h>

struct FacArgs {
  uint64_t begin;
  uint64_t end; // [begin, end)
  uint64_t mod;
};

static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
static uint64_t total = 1;

// Умножение по модулю без переполнения (метод "быстрого умножения в столбик")
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
  uint64_t result = 0;
  a = a % mod;
  while (b > 0) {
    if (b % 2 == 1)
      result = (result + a) % mod;
    a = (a * 2) % mod;
    b /= 2;
  }
  return result % mod;
}

uint64_t Factorial(const struct FacArgs *args) {
  uint64_t ans = 1;
  for (uint64_t i = args->begin; i < args->end; i++) {
    ans = MultModulo(ans, i % args->mod, args->mod);
  }
  return ans;
}

void *ThreadFactorial(void *args) {
  struct FacArgs *fargs = (struct FacArgs *)args;
  uint64_t part = Factorial(fargs);
  // Критическая секция: общий накопитель total защищен мьютексом
  pthread_mutex_lock(&mut);
  total = MultModulo(total, part, fargs->mod);
  pthread_mutex_unlock(&mut);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = 0, mod = 0;
  int pnum = 0;

  static struct option options[] = {{"k", required_argument, 0, 'k'},
                                    {"pnum", required_argument, 0, 0},
                                    {"mod", required_argument, 0, 0},
                                    {0, 0, 0, 0}};
  int option_index = 0;
  int c;
  while ((c = getopt_long(argc, argv, "k:", options, &option_index)) != -1) {
    switch (c) {
    case 'k': k = strtoull(optarg, NULL, 10); break;
    case 0:
      switch (option_index) {
      case 1: pnum = atoi(optarg); break;
      case 2: mod = strtoull(optarg, NULL, 10); break;
      }
      break;
    default: fprintf(stderr, "Arguments error\n"); return 1;
    }
  }

  if (!k || !mod || pnum <= 0) {
    fprintf(stderr, "Using: %s -k 10 --pnum=4 --mod=10\n", argv[0]);
    return 1;
  }
  if ((uint64_t)pnum > k) pnum = (int)k;

  pthread_t threads[pnum];
  struct FacArgs args[pnum];
  uint64_t chunk = k / pnum;

  for (int i = 0; i < pnum; i++) {
    args[i].begin = 1 + (uint64_t)i * chunk;
    args[i].end = (i == pnum - 1) ? k + 1 : 1 + (uint64_t)(i + 1) * chunk;
    args[i].mod = mod;
    if (pthread_create(&threads[i], NULL, ThreadFactorial, &args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }
  for (int i = 0; i < pnum; i++) pthread_join(threads[i], NULL);

  printf("%llu! %% %llu = %llu\n", (unsigned long long)k,
         (unsigned long long)mod, (unsigned long long)total);
  return 0;
}
