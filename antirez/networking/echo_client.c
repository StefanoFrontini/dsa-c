#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "4242"
#define MAXDATASIZE 100

int main(int argc, char *argv[]) {
  int status, sockfd, numbytes;
  struct addrinfo hints, *servinfo, *p;
  char ipstr[INET6_ADDRSTRLEN];

  char buf[MAXDATASIZE];
  char *ipver;

  if (argc != 2) {
    fprintf(stderr, "usage: server hostname\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 1;
  }

  for (p = servinfo; p != NULL; p = p->ai_next) {
    void *addr;
    struct sockaddr_in *ipv4;
    struct sockaddr_in6 *ipv6;

    if (p->ai_family == AF_INET) {
      ipv4 = (struct sockaddr_in *)p->ai_addr;

      addr = &(ipv4->sin_addr);
      ipver = "IPv4";
    } else {
      ipv6 = (struct sockaddr_in6 *)p->ai_addr;
      addr = &(ipv6->sin6_addr);
      ipver = "IPv6";
    }

    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd == -1) {
      perror("client: socket");
      continue;
    }
    // convert the IP to a string
    inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    printf("Socket created with %s: %s!\n", ipver, ipstr);

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("client: connect");
      close(sockfd);
      continue;
    };
    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }
  printf("Connected with %s: %s!\n", ipver, ipstr);
  freeaddrinfo(servinfo);

  char *msg = "Hello world!\n";
  int len, bytes_send;
  len = strlen(msg);
  printf("Sending msg, len: %d\n", len);
  bytes_send = send(sockfd, msg, len, 0);
  if (bytes_send == -1) {
    perror("send");
    exit(1);
  }
  printf("Bytes sent: %d\n", bytes_send);
  if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
    perror("recv");
    exit(1);
  };
  printf("Bytes received: %d\n", numbytes);
  buf[numbytes] = '\0';

  printf("Message received from the server: %s\n", buf);

  close(sockfd);

  return 0;
}