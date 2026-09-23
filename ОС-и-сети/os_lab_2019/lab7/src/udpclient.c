#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_BUFSIZE 1024
#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char **argv) {
  // Задание 1: ./udpclient <ip> <port> [bufsize]
  if (argc != 3 && argc != 4) {
    printf("usage: %s <IPaddress of server> <port> [bufsize]\n", argv[0]);
    exit(1);
  }
  int serv_port = atoi(argv[2]);
  if (serv_port <= 0 || serv_port > 65535) {
    fprintf(stderr, "bad port\n");
    exit(1);
  }
  int bufsize = (argc > 3) ? atoi(argv[3]) : DEFAULT_BUFSIZE;
  if (bufsize <= 0 || bufsize > 65536) {
    fprintf(stderr, "bad bufsize\n");
    exit(1);
  }

  int sockfd, n;
  char *sendline = malloc(bufsize);
  char *recvline = malloc(bufsize + 1);
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons((uint16_t)serv_port);

  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0) {
    perror("inet_pton problem");
    exit(1);
  }
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    exit(1);
  }

  write(1, "Enter string\n", 13);

  while ((n = read(0, sendline, bufsize)) > 0) {
    if (sendto(sockfd, sendline, n, 0, (SADDR *)&servaddr, SLEN) == -1) {
      perror("sendto problem");
      exit(1);
    }

    if (recvfrom(sockfd, recvline, bufsize, 0, NULL, NULL) == -1) {
      perror("recvfrom problem");
      exit(1);
    }

    printf("REPLY FROM SERVER= %s\n", recvline);
  }
  close(sockfd);
}
