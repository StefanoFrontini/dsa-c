
#include <arpa/inet.h>  /* inet(3) functions */
#include <netinet/in.h> /* sockaddr_in{} and other Internet defns */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAXLINE 4096 /* max text line length */

int main(int argc, char **argv) {
  int sockfd, n;
  char recvline[MAXLINE + 1];
  struct sockaddr_in6 servaddr;

  if (argc != 2) {
    fprintf(stderr, "usage: a.out <IPaddress>");
  }

  if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
    perror("socket error");
  }
  // memset(&servaddr, 0, sizeof(servaddr));
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin6_family = AF_INET6;
  servaddr.sin6_port = htons(13);
  if (inet_pton(AF_INET6, argv[1], &servaddr.sin6_addr) <= 0) {
    fprintf(stderr, "inet_pton error for %s", argv[1]);
  }

  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("connect error");
  }

  while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
    recvline[n] = 0;
    if (fputs(recvline, stdout) == EOF) {
      perror("fputs error");
    }
  }

  if (n < 0) {
    perror("read error");
  }

  return 0;
}