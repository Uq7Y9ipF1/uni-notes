#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Задание 1: констаны вынесены в аргументы командной строки:
//   ./tcpserver <port> [bufsize]
#define DEFAULT_BUFSIZE 100
#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("usage: %s <port> [bufsize]\n", argv[0]);
    return 1;
  }
  int serv_port = atoi(argv[1]);
  if (serv_port <= 0 || serv_port > 65535) {
    fprintf(stderr, "bad port\n");
    return 1;
  }
  int bufsize = (argc > 2) ? atoi(argv[2]) : DEFAULT_BUFSIZE;
  if (bufsize <= 0 || bufsize > 65536) {
    fprintf(stderr, "bad bufsize\n");
    return 1;
  }

  const size_t kSize = sizeof(struct sockaddr_in);

  int lfd, cfd;
  int nread;
  char *buf = malloc(bufsize);
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  memset(&servaddr, 0, kSize);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons((uint16_t)serv_port);

  if (bind(lfd, (SADDR *)&servaddr, kSize) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(lfd, 5) < 0) {
    perror("listen");
    exit(1);
  }

  while (1) {
    unsigned int clilen = kSize;

    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept");
      exit(1);
    }
    printf("connection established\n");

    while ((nread = read(cfd, buf, bufsize)) > 0) {
      write(1, &buf, nread);
    }

    if (nread == -1) {
      perror("read");
      exit(1);
    }
    close(cfd);
  }
}
