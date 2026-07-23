
#include <arpa/inet.h>  /* inet(3) functions */
#include <netinet/in.h> /* sockaddr_in{} and other Internet defns */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/errno.h>

#define MAXLINE 4096 /* max text line length */

int Socket(int family, int type, int protocol) {
  int n;
  if ((n = socket(family, type, protocol)) < 0) {
    perror("socket error");
  }
  return n;
}

int main(int argc, char **argv) {
  int sockfd, n;
  int counter = 0;
  char recvline[MAXLINE + 1];
  struct sockaddr_in servaddr;

  if (argc != 2) {
    fprintf(stderr, "usage: a.out <IPaddress>");
  }

  sockfd = Socket(AF_INET, SOCK_STREAM, 0);
  // if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
  //   perror("socket error");
  // }
  // memset(&servaddr, 0, sizeof(servaddr));
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(9999);
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
    fprintf(stderr, "inet_pton error for %s", argv[1]);
  }

  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("connect error");
  }

  while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
    recvline[n] = 0;
    counter++;
    if (fputs(recvline, stdout) == EOF) {
      perror("fputs error");
    }
  }
  printf("Counter is %d\n", counter);

  if (n < 0) {
    perror("read error");
  }

  return 0;
}