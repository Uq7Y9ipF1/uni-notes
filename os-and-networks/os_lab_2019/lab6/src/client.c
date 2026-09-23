// Лабораторная работа №6, задание 1: клиент.
// Аргументы: k mod servers, где
//   k       -- от какого числа считаем факториал (k! % mod),
//   mod     -- модуль,
//   servers -- путь до файла со списком серверов в формате ip:port (по одному
//              на строку).
// Клиент режет диапазон [1..k] на количество серверов и отправляет каждому
// серверу его "кусок" вычислений (begin, end, mod) как 3 uint64 по TCP.
// Замечание из задания: исходный вариант дожидается ответа от каждого сервера
// последовательно. Мы распараллеливаем работу так: на каждый сервер
// заводится отдельный поток pthread (connect + send + recv внутри потока),
// а частичные результаты потоков складываются в общую переменную под
// мьютексом. Выбран именно этот способ, т.к. он минимален по коду, не требует
// неблокирующего I/O или select/poll и гарантирует, что все сервера считают
// свои куски одновременно.
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <pthread.h>

#include "common.h"

#define MAX_SERVERS 64

struct ServerJob {
  char ip[64];
  int port;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};

// Общий результат и мьютекс для его безопасного обновления из потоков.
static uint64_t g_total = 1;
static uint64_t g_mod = 1;
static pthread_mutex_t g_total_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_errors = 0;

void *ServerThread(void *arg) {
  struct ServerJob *job = (struct ServerJob *)arg;

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    fprintf(stderr, "Can not create socket for server %s:%d\n", job->ip,
            job->port);
    pthread_mutex_lock(&g_total_mutex);
    g_errors++;
    pthread_mutex_unlock(&g_total_mutex);
    return NULL;
  }

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons((uint16_t)job->port);
  if (inet_pton(AF_INET, job->ip, &servaddr.sin_addr) <= 0) {
    fprintf(stderr, "Bad address: %s\n", job->ip);
    close(fd);
    pthread_mutex_lock(&g_total_mutex);
    g_errors++;
    pthread_mutex_unlock(&g_total_mutex);
    return NULL;
  }

  if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("connect");
    close(fd);
    pthread_mutex_lock(&g_total_mutex);
    g_errors++;
    pthread_mutex_unlock(&g_total_mutex);
    return NULL;
  }

  // Отправляем свой кусок: begin, end, mod (три uint64 подряд).
  uint64_t payload[3] = {job->begin, job->end, job->mod};
  size_t sent = 0;
  while (sent < sizeof(payload)) {
    ssize_t n = send(fd, (char *)payload + sent, sizeof(payload) - sent, 0);
    if (n <= 0) {
      perror("send");
      break;
    }
    sent += (size_t)n;
  }

  // Ждём ответ — произведение куска по модулю (один uint64).
  uint64_t result = 0;
  size_t got = 0;
  while (got < sizeof(result)) {
    ssize_t n = recv(fd, (char *)&result + got, sizeof(result) - got, 0);
    if (n <= 0) {
      perror("recv");
      break;
    }
    got += (size_t)n;
  }
  close(fd);

  if (got != sizeof(result)) {
    pthread_mutex_lock(&g_total_mutex);
    g_errors++;
    pthread_mutex_unlock(&g_total_mutex);
    return NULL;
  }

  printf("Server %s:%d piece [%llu, %llu] => %llu\n", job->ip, job->port,
         (unsigned long long)job->begin, (unsigned long long)job->end,
         (unsigned long long)result);

  // Объединяем частичный результат с общим под мьютексом.
  pthread_mutex_lock(&g_total_mutex);
  g_total = MultModulo(g_total, result, g_mod);
  pthread_mutex_unlock(&g_total_mutex);

  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <k> <mod> <servers_file>\n", argv[0]);
    return 1;
  }

  uint64_t k = 0;
  uint64_t mod = 0;
  if (!ConvertStringToUI64(argv[1], &k) || !ConvertStringToUI64(argv[2], &mod))
    return 1;
  if (k == 0 || mod == 0) {
    fprintf(stderr, "k and mod must be positive\n");
    return 1;
  }
  g_mod = mod;

  // Читаем список серверов ip:port из файла.
  FILE *fp = fopen(argv[3], "r");
  if (!fp) {
    perror("fopen servers file");
    return 1;
  }

  static struct ServerJob jobs[MAX_SERVERS];
  int server_count = 0;
  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    line[strcspn(line, "\r\n")] = 0;
    if (line[0] == '\0' || line[0] == '#')
      continue;
    char *colon = strrchr(line, ':');
    if (!colon) {
      fprintf(stderr, "Bad server line (expected ip:port): %s\n", line);
      fclose(fp);
      return 1;
    }
    *colon = '\0';
    if (server_count >= MAX_SERVERS) {
      fprintf(stderr, "Too many servers (max %d)\n", MAX_SERVERS);
      break;
    }
    strncpy(jobs[server_count].ip, line, sizeof(jobs[server_count].ip) - 1);
    jobs[server_count].port = atoi(colon + 1);
    if (jobs[server_count].port <= 0 || jobs[server_count].port > 65535) {
      fprintf(stderr, "Bad port in line: %s\n", line);
      fclose(fp);
      return 1;
    }
    server_count++;
  }
  fclose(fp);

  if (server_count == 0) {
    fprintf(stderr, "No servers found in %s\n", argv[3]);
    return 1;
  }

  // Разрезаем диапазон [1..k] между серверами. Последнему серверу достаётся
  // остаток до k включительно.
  uint64_t range = k; // [1..k]
  uint64_t chunk = range / (uint64_t)server_count;
  if (chunk == 0)
    chunk = 1;

  for (int i = 0; i < server_count; i++) {
    uint64_t begin = 1 + (uint64_t)i * chunk;
    if (begin > k) {
      begin = k; // вырожденный кусок {k} — лучше, чем пропуск сервера
    }
    uint64_t end = begin + chunk - 1;
    if (i == server_count - 1 || end > k)
      end = k;
    if (begin > end)
      begin = end; // пустой кусок превращаем в одиночный элемент
    jobs[i].begin = begin;
    jobs[i].end = end;
    jobs[i].mod = mod;
  }

  pthread_t threads[MAX_SERVERS];
  int started = 0;
  for (int i = 0; i < server_count; i++) {
    if (pthread_create(&threads[i], NULL, ServerThread, &jobs[i])) {
      fprintf(stderr, "pthread_create failed\n");
      g_errors++;
      continue;
    }
    started++;
  }
  for (int i = 0; i < started; i++) {
    pthread_join(threads[i], NULL);
  }

  if (g_errors > 0) {
    fprintf(stderr, "%d server(s) failed\n", g_errors);
    return 1;
  }

  printf("answer: %llu\n", (unsigned long long)g_total);
  return 0;
}
