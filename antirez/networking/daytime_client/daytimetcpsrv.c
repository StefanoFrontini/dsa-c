
#include <arpa/inet.h>  /* inet(3) functions */
#include <netinet/in.h> /* sockaddr_in{} and other Internet defns */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAXLINE 4096 /* max text line length */
#define LISTENQ 1024 /* 2nd argument to listen() */

int Socket(int family, int type, int protocol) {
  int n;
  if ((n = socket(family, type, protocol)) < 0) {
    perror("socket error");
  }
  return n;
}
void Bind(int fd, const struct sockaddr *sa, socklen_t salen) {
  if (bind(fd, sa, salen) < 0) perror("bind error");
}

void Connect(int fd, const struct sockaddr *sa, socklen_t salen) {
  if (connect(fd, sa, salen) < 0) perror("connect error");
}

void Listen(int fd, int backlog) {
  char *ptr;

  /*4can override 2nd argument with environment variable */
  if ((ptr = getenv("LISTENQ")) != NULL) backlog = atoi(ptr);

  if (listen(fd, backlog) < 0) perror("listen error");
}

void Write(int fd, void *ptr, ssize_t nbytes) {
  ssize_t bytes_written = 0;
  for (int i = 0; i < nbytes; i++) {
    write(fd, ptr + i, 1);
    bytes_written++;
  }
  if (bytes_written != nbytes) perror("write error");
  //   if (write(fd, ptr, nbytes) != nbytes) perror("write error");
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr) {
  int n;

again:
  if ((n = accept(fd, sa, salenptr)) < 0) {
#ifdef EPROTO
    if (errno == EPROTO || errno == ECONNABORTED)
#else
    if (errno == ECONNABORTED)
#endif
      goto again;
    else
      perror("accept error");
  }
  return (n);
}

void Close(int fd) {
  if (close(fd) == -1) perror("close error");
}

int main(void) {
  int listenfd, connfd;
  struct sockaddr_in servaddr;
  char buff[MAXLINE];
  time_t ticks;

  listenfd = Socket(AF_INET, SOCK_STREAM, 0);

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(9999);

  Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

  Listen(listenfd, LISTENQ);

  while (1) {
    connfd = Accept(listenfd, (struct sockaddr *)NULL, NULL);
    ticks = time(NULL);
    snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
    Write(connfd, buff, strlen(buff));

    Close(connfd);
  }

  return 0;
}