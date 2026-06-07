#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define HOST "example.com"
#define PORT "80"
#define MAXDATASIZE 100
#define MAXHEADER 8192
#define ABSOLUTE_PATH "/"

typedef enum {
  HEADER_ACC = 0,
  HEADER_PARSING,
  BODY_READING,
  BODY_PARSING_CHUNKED,
  CONNECTION_CLOSED,
  RECV_ERROR
} HttpParserState;

typedef enum {
  HEADER_STATUS_LINE = 0,
  HEADER_KEY,
  HEADER_VALUE,
  HEADER_DONE,
  HEADER_CRLF

} HeaderState;

int main(void) {
  int status, sockfd, numbytes;
  int header_idx = 0, body_idx = 0, status_code = 0;
  size_t k_idx = 0, v_idx = 0;
  struct addrinfo hints, *servinfo, *p;
  char ipstr[INET6_ADDRSTRLEN];

  char recv_buf[MAXDATASIZE];
  char header_accumulator[MAXHEADER];
  char *ipver;
  char get_buf[512];
  char key_buffer[128];
  char value_buffer[512];
  long content_length = 0;
  int isEncodingChunked = 0;
  HttpParserState state = HEADER_ACC;
  HeaderState headerState = HEADER_STATUS_LINE;
  snprintf(get_buf, sizeof(get_buf),
           "GET %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "User-Agent: UndergroundRadio/1.0\r\n"
           "Connection: close\r\n"
           "\r\n",
           ABSOLUTE_PATH, HOST);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(HOST, PORT, &hints, &servinfo)) != 0) {
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
    // printf("Socket created with %s: %s!\n", ipver, ipstr);

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

  int len, bytes_send;
  len = strlen(get_buf);
  printf("Sending msg, len: %d\n", len);
  bytes_send = send(sockfd, get_buf, len, 0);
  if (bytes_send == -1) {
    perror("send");
    exit(1);
  }
  printf("Bytes sent: %d\n", bytes_send);
  while (state != CONNECTION_CLOSED) {
    switch (state) {
      case HEADER_ACC: {
        while ((numbytes = recv(sockfd, recv_buf, sizeof(recv_buf), 0)) > 0) {
          if(header_idx > MAXHEADER){
            fprintf(stderr, "Error reading header");
            exit(1);
          }
          memcpy(header_accumulator + header_idx, recv_buf, numbytes);
          int old_header_idx = header_idx;
          header_idx += numbytes;
          /*
          we have already scanned header_accumulator till header_idx. So we scan
          only the added chunk. The sequence \n\r could be split between two
          recv calls: one call ends with \r and the second one starts with \n.
          In order to handle this case we subtract 1 to old_header_idx.
          */
          int i = old_header_idx - 1;
          for (; i < header_idx; i++) {
            if (header_accumulator[i] == '\n' &&
                header_accumulator[i + 1] == '\r') {
              state = HEADER_PARSING;
              body_idx = i + 3;
              printf("body_idx is: %d\n", body_idx);
              break; // break from for loop
            }
          }
          if (state == HEADER_PARSING) break; // break from while loop
        }
        if (numbytes == -1) {
          perror("recv");
          exit(1);
        }
        break;
      }
      case HEADER_PARSING: {
        printf("HeaderParsing: \n");
        for (int i = 0; i < header_idx; i++) {
          char c = header_accumulator[i];

          if (headerState == HEADER_DONE) {
            printf("--- Fine Header. Inizio Body al byte indicizzato: %d ---\n",
                   i);
            if (status_code == 200 && isEncodingChunked) {
              state = BODY_PARSING_CHUNKED;
            }
            break;
          }

          switch (headerState) {
            case HEADER_STATUS_LINE: {
              if (c > 8 && isdigit(c) && isdigit(header_accumulator[i + 1]) &&
                  isdigit(header_accumulator[i + 2])) {
                char num_buf[4];
                memcpy(num_buf, header_accumulator + i, 3);
                num_buf[3] = '\0';
                status_code = atoi(num_buf);
                printf("Status Code Rilevato: %d\n", status_code);

                if (status_code == 400) {
                  fprintf(stderr, "Error: 400\n");
                  exit(1);
                }
              }
              if (c == '\n') {
                headerState = HEADER_KEY;
              }
              break;
            }
            case HEADER_KEY: {
              if (c == '\r') {
                headerState = HEADER_CRLF; // Riga vuota? Controlliamo se è la
                                           // fine degli header
              } else if (c == ':') {
                key_buffer[k_idx] = '\0'; // Chiudiamo la stringa della chiave
                headerState = HEADER_VALUE;
                v_idx = 0;
              } else if (k_idx < sizeof(key_buffer) - 1 && c != '\n') {
                key_buffer[k_idx++] = c; // Accumuliamo il nome dell'header
              }
              break;
            }
            case HEADER_VALUE: {
              if (c == '\r') {
                value_buffer[v_idx] = '\0'; // Chiudiamo la stringa del valore

                printf("Header: [%s] -> [%s]\n", key_buffer, value_buffer);
                if (strcasecmp(key_buffer, "Content-Length") == 0) {
                  content_length = atol(value_buffer);
                } else if ((strcasecmp(key_buffer, "Transfer-Encoding") == 0) &&
                           (strcasecmp(value_buffer, "chunked") == 0)) {
                  isEncodingChunked = 1;
                  printf("Encoding Chunked: true\n");
                }

                k_idx =
                    0; // Resettiamo l'indice della chiave per il prossimo ciclo
                headerState = HEADER_CRLF;
              } else if (v_idx < sizeof(value_buffer) - 1) {
                // Salto lo spazio iniziale dopo i due punti se presente
                if (v_idx == 0 && c == ' ') continue;
                value_buffer[v_idx++] = c;
              }
              break;
            }
            case HEADER_CRLF: {
              if (c == '\n') {
                // Se il carattere successivo è un altro '\r', significa che
                // abbiamo fatto \r\n\r
                if (header_accumulator[i + 1] == '\r') {
                  headerState = HEADER_DONE;
                  i++; // Salto anche l'ultimo '\n' che seguirà
                } else {
                  headerState = HEADER_KEY; // C'è un altro header in arrivo
                }
              }
              break;
            }
            default:
              break;
          }
        }
        // parseHeader();
        // caso 1: statuscode 400 -> ERROR
        // caso 2: statuscode 301 -> redirect
        // caso 3: 200 e chunk-encoded = false -> BODY_PARSING_CHUNKED
        // caso 4: 200 e chunk-encoded = true  -> BODY_PARSING
        continue;
      }
      case BODY_PARSING_CHUNKED: {
        printf("\n--- RISULTATO PARSING ---\n");
        printf("HTTP Status: %d\n", status_code);
        printf("Content-Length: %ld byte\n", content_length);
        printf("Start parsing body chunked at %d\n", body_idx);
        state = CONNECTION_CLOSED;
        break;
      }
      default:
        break;
    }
  }
  // while ((numbytes = recv(sockfd, recv_buf, sizeof(recv_buf), 0)) > 0) {
  //   switch (state) {
  //     case HEADER_ACC: {
  //       for (int i = 0; i < numbytes; i++) {
  //         if(recv_buf[i] == '\n' && recv_buf[i + 1] == '\r'){
  //           state = HEADER_PARSING;
  //         }
  //       }
  //       break;
  //     }
  //   }
  // }
  // if (numbytes == -1) {
  //   perror("recv");
  //   exit(1);
  // }
  // if ((numbytes = recv(sockfd, buf, sizeof(buf), 0)) == -1) {
  //   perror("recv");
  //   exit(1);
  // };
  // printf("Bytes received: %d\n", numbytes);
  // buf[numbytes] = '\0';

  // printf("Message received from the server: %s\n", buf);

  close(sockfd);

  return 0;
}