/*
HTTP/1.1 200 OK\r\n
Content-Type: text/plain\r\n
Transfer-Encoding: chunked\r\n
\r\n
5\r\n
Wi\rki\r\n
5\r\n
pedia\r\n
0\r\n
\r\n

*/
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
#define MAXDATASIZE 1
#define MAXACCUMULATOR 8192
#define ABSOLUTE_PATH "/"
#define MAXHEADERKEY 128
#define MAXHEADERVALUE 512

typedef enum {
  HEADER_STATUS_LINE = 0,
  HEADER_STATUS_LINE_ACC,
  HEADER_KEY,
  HEADER_KEY_ACC,
  HEADER_CRLF,
  HEADER_CRLF_ACC,
  HEADER_VALUE,
  HEADER_VALUE_ACC,
  HEADER_DONE,
  HEADER_ERROR,
  DONE,
} ParserState;

typedef struct Parser {
  char acc_buf[MAXACCUMULATOR];
  size_t acc_idx;
  size_t acc_len;
  char recv_buf[MAXDATASIZE];
  size_t recv_idx;
  size_t recv_len;
  size_t numbytes;
  ParserState state;
  int body_idx;
  long content_length;
  int isEncodingChunked;
  int isAcc;
  int status_code;
} Parser;

typedef struct Connection {
  char req_buf[512];
  int status;
  int sockfd;
  char ipstr[INET6_ADDRSTRLEN];
  struct addrinfo hints;
  struct addrinfo *servinfo;
  struct addrinfo *p;
  char *ipver;

} Connection;

typedef struct Ctx {
  Parser parser;
  Connection conn;
} Ctx;

void initCtx(Ctx *ctx) {
  ctx->parser.acc_idx = 0;
  ctx->parser.acc_len = 0;
  ctx->parser.body_idx = 0;
  ctx->parser.recv_idx = 0;
  ctx->parser.recv_len = 0;
  ctx->parser.content_length = 0;
  ctx->parser.state = HEADER_STATUS_LINE;
  ctx->parser.isAcc = 0;
  ctx->parser.status_code = 0;

  snprintf(ctx->conn.req_buf, sizeof(ctx->conn.req_buf),
           "GET %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "User-Agent: UndergroundRadio/1.0\r\n"
           "Connection: close\r\n"
           "\r\n",
           ABSOLUTE_PATH, HOST);

  memset(&ctx->conn.hints, 0, sizeof(ctx->conn.hints));
  ctx->conn.hints.ai_family = AF_UNSPEC;
  ctx->conn.hints.ai_socktype = SOCK_STREAM;
}

int initConnection(Ctx *ctx) {
  if ((ctx->conn.status = getaddrinfo(HOST, PORT, &ctx->conn.hints,
                                      &ctx->conn.servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ctx->conn.status));
    return 1;
  }

  for (ctx->conn.p = ctx->conn.servinfo; ctx->conn.p != NULL;
       ctx->conn.p = ctx->conn.p->ai_next) {
    void *addr;
    struct sockaddr_in *ipv4;
    struct sockaddr_in6 *ipv6;

    if (ctx->conn.p->ai_family == AF_INET) {
      ipv4 = (struct sockaddr_in *)ctx->conn.p->ai_addr;

      addr = &(ipv4->sin_addr);
      ctx->conn.ipver = "IPv4";
    } else {
      ipv6 = (struct sockaddr_in6 *)ctx->conn.p->ai_addr;
      addr = &(ipv6->sin6_addr);
      ctx->conn.ipver = "IPv6";
    }

    ctx->conn.sockfd = socket(ctx->conn.p->ai_family, ctx->conn.p->ai_socktype,
                              ctx->conn.p->ai_protocol);
    if (ctx->conn.sockfd == -1) {
      perror("client: socket");
      continue;
    }
    // convert the IP to a string
    inet_ntop(ctx->conn.p->ai_family, addr, ctx->conn.ipstr,
              sizeof ctx->conn.ipstr);

    if (connect(ctx->conn.sockfd, ctx->conn.p->ai_addr,
                ctx->conn.p->ai_addrlen) == -1) {
      perror("client: connect");
      close(ctx->conn.sockfd);
      continue;
    };
    break;
  }

  if (ctx->conn.p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }
  printf("Connected with %s: %s!\n", ctx->conn.ipver, ctx->conn.ipstr);
  freeaddrinfo(ctx->conn.servinfo);
  return 0;
}

/* Accumulates bytes on the the acc_buf. Return -1 on error and 0 on
 * success.*/
int accumulate(Ctx *ctx, size_t recv_start_idx) {
  ctx->parser.acc_len = 0;

  memcpy(ctx->parser.acc_buf, ctx->parser.recv_buf + recv_start_idx,
         ctx->parser.recv_len - recv_start_idx);
  ctx->parser.acc_len = ctx->parser.recv_len - recv_start_idx;

  ctx->parser.numbytes = recv(ctx->conn.sockfd, ctx->parser.recv_buf,
                              sizeof(ctx->parser.recv_buf), 0);

  if (ctx->parser.numbytes == 0) {
    fprintf(stderr, "Error: recv returned 0 bytes\n");
    return -1;
  }

  memcpy(ctx->parser.acc_buf + ctx->parser.acc_len, ctx->parser.recv_buf,
         ctx->parser.numbytes);

  ctx->parser.acc_len += ctx->parser.numbytes;
  ctx->parser.acc_idx = 0;
  return 0;
}

/* Keeps Accumulating bytes on the the acc_buf. Return -1 on error and 0 on
 * success.*/
int keepAccumulating(Ctx *ctx) {
  ctx->parser.numbytes = recv(ctx->conn.sockfd, ctx->parser.recv_buf,
                              sizeof(ctx->parser.recv_buf), 0);

  if (ctx->parser.numbytes == 0) {
    fprintf(stderr, "Error: numbytes = 0\n");
    return -1;
  }
  memcpy(ctx->parser.acc_buf + ctx->parser.acc_len, ctx->parser.recv_buf,
         ctx->parser.numbytes);

  ctx->parser.acc_len += ctx->parser.numbytes;
  return 0;
}

/* Read a chunk of bytes from the network and place it on recv_buf. Return -1 on
 * error and 0 on success.*/
int readChunk(Ctx *ctx) {
  ctx->parser.numbytes = recv(ctx->conn.sockfd, ctx->parser.recv_buf,
                              sizeof(ctx->parser.recv_buf), 0);

  if (ctx->parser.numbytes == 0) {
    fprintf(stderr, "Error reading chunk from network\n");
    return -1;
  }
  ctx->parser.recv_len = ctx->parser.numbytes;
  ctx->parser.recv_idx = 0;
  return 0;
}

void copyFromAccToRecvBuf(Ctx *ctx) {

  memcpy(ctx->parser.recv_buf, ctx->parser.acc_buf + ctx->parser.acc_idx,
         ctx->parser.acc_len - ctx->parser.acc_idx);

  ctx->parser.recv_idx = 0;
  ctx->parser.recv_len = ctx->parser.acc_len - ctx->parser.acc_idx;

  ctx->parser.acc_idx = 0;
  ctx->parser.acc_len = 0;
}

int parseResponse(Ctx *ctx) {
  char key_buffer[128] = {0};
  char value_buffer[512] = {0};
  size_t k_idx = 0, v_idx = 0;

  int res = readChunk(ctx);
  if (res == -1) {
    exit(1);
  }

  // ctx->parser.numbytes = recv(ctx->conn.sockfd, ctx->parser.recv_buf,
  //                             sizeof(ctx->parser.recv_buf), 0);

  // if (ctx->parser.numbytes == 0) {
  //   fprintf(stderr, "Error\n");
  //   return 1;
  // }
  // ctx->parser.recv_len = ctx->parser.numbytes;

  while (ctx->parser.state != DONE) {
    switch (ctx->parser.state) {
      case HEADER_STATUS_LINE: {
        size_t start = ctx->parser.recv_idx;
        size_t len = ctx->parser.recv_len;
        char *recv_buf = ctx->parser.recv_buf;

        while (ctx->parser.recv_idx < len) {

          if (ctx->parser.recv_idx > 8 &&
              isdigit(recv_buf[ctx->parser.recv_idx]) &&
              isdigit(recv_buf[ctx->parser.recv_idx + 1]) &&
              isdigit(recv_buf[ctx->parser.recv_idx + 2]) &&
              recv_buf[ctx->parser.recv_idx + 7] == '\n') {
            char num_buf[4];
            memcpy(num_buf, recv_buf + ctx->parser.recv_idx, 3);
            num_buf[3] = '\0';
            ctx->parser.status_code = atoi(num_buf);
            printf("1 - Status Code Rilevato: %d\n", ctx->parser.status_code);

            if (ctx->parser.status_code == 400) {
              fprintf(stderr, "Error: 400\n");
              exit(1);
            }
          }
          if (recv_buf[ctx->parser.recv_idx] == '\n') {
            ctx->parser.state =
                HEADER_KEY; // prima riga finita iniziamo gli header.
            // printf("recv_idx: %zu, recv_len: %zu\n", ctx->parser.recv_idx,
            // ctx->parser.recv_len);

            // if (ctx->parser.recv_idx == ctx->parser.recv_len ) {
            //   printf("Reading recv()\n");

            //   ctx->parser.numbytes =
            //       recv(ctx->conn.sockfd, ctx->parser.recv_buf,
            //            sizeof(ctx->parser.recv_buf), 0);

            //   if (ctx->parser.numbytes == 0) {
            //     fprintf(stderr, "Error: recv returned 0 bytes\n");
            //     return 1;
            //   }
            //   ctx->parser.recv_idx = 0;
            //   ctx->parser.recv_len = ctx->parser.numbytes;
            // } else {

            //   ctx->parser.recv_idx++;
            // }
            //   ctx->parser.recv_idx++;
            break; // esce dal while loop
          }
          ctx->parser.recv_idx++;
        }
        /*
        se sono arrivato qui e ancora lo stato non è in HEADER_KEY_RECV il
        recv_buf è troppo piccolo e non riesco a fare il parsing dello
        statuscode per cui devo accumulare i byte in acc_buf.
        */
        if (ctx->parser.state != HEADER_KEY) {
          int res = accumulate(ctx, start);
          if (res == -1) {
            ctx->parser.state = HEADER_ERROR;
            break;
          }
          // memcpy(ctx->parser.acc_buf, recv_buf + start, len - start);
          // ctx->parser.acc_len = len - start;

          // ctx->parser.numbytes = recv(ctx->conn.sockfd, ctx->parser.recv_buf,
          //                             sizeof(ctx->parser.recv_buf), 0);

          // if (ctx->parser.numbytes == 0) {
          //   fprintf(stderr, "Error: recv returned 0 bytes\n");
          //   return 1;
          // }
          // memcpy(ctx->parser.acc_buf + ctx->parser.acc_len,
          //        ctx->parser.recv_buf, ctx->parser.numbytes);

          // ctx->parser.acc_len += ctx->parser.numbytes;
          ctx->parser.state = HEADER_STATUS_LINE_ACC;
        }
        break;
      }
      case HEADER_STATUS_LINE_ACC: {

        // printf("acc_len: %zu\n", ctx->parser.acc_len);
        char *acc_buf = ctx->parser.acc_buf;
        // printf("acc_buf: %s", acc_buf );
        ctx->parser.acc_idx = 0;

        while (ctx->parser.acc_idx < ctx->parser.acc_len) {

          if (ctx->parser.acc_idx > 8 &&
              isdigit(acc_buf[ctx->parser.acc_idx]) &&
              isdigit(acc_buf[ctx->parser.acc_idx + 1]) &&
              isdigit(acc_buf[ctx->parser.acc_idx + 2]) &&
              acc_buf[ctx->parser.acc_idx + 7] == '\n') {

            char num_buf[4];
            memcpy(num_buf, acc_buf + ctx->parser.acc_idx, 3);
            num_buf[3] = '\0';
            ctx->parser.status_code = atoi(num_buf);
            printf("2 - Status Code Rilevato: %d\n", ctx->parser.status_code);

            if (ctx->parser.status_code == 400) {
              fprintf(stderr, "Error: 400\n");
              ctx->parser.state = HEADER_ERROR;
              break;
            }
          }

          if (acc_buf[ctx->parser.acc_idx] == '\n') {
            ctx->parser.state =
                HEADER_KEY; // prima riga finita iniziamo gli header.
            if (ctx->parser.acc_idx == ctx->parser.acc_len - 1) {

              int res = readChunk(ctx);
              if (res == -1) {
                ctx->parser.state = HEADER_ERROR;
                break;
              }

              // ctx->parser.numbytes =
              //     recv(ctx->conn.sockfd, ctx->parser.recv_buf,
              //          sizeof(ctx->parser.recv_buf), 0);

              // if (ctx->parser.numbytes == 0) {
              //   fprintf(stderr, "Error: recv returned 0 bytes\n");
              //   return 1;
              // }

              // ctx->parser.recv_idx = 0;
              // ctx->parser.recv_len = ctx->parser.numbytes;

            } else {

              ctx->parser.acc_idx++;
              copyFromAccToRecvBuf(ctx);
              // memcpy(ctx->parser.recv_buf, acc_buf + ctx->parser.acc_idx,
              //        ctx->parser.acc_len - ctx->parser.acc_idx);

              // ctx->parser.recv_idx = 0;
              // ctx->parser.recv_len = ctx->parser.acc_len - ctx->parser.acc_idx;

              // ctx->parser.acc_idx = 0;
              // ctx->parser.acc_len = 0;
            }
            break;
          }
          ctx->parser.acc_idx++;
        }
        // se sono arrivato qui e lo stato non è HEADER_KEY_RECV leggo con
        // recv() e continuo ad accumulare
        if (ctx->parser.state != HEADER_KEY) {
          int res = keepAccumulating(ctx);
          if (res == -1) {
            ctx->parser.state = HEADER_ERROR;
            break;
          }

          // ctx->parser.numbytes = recv(ctx->conn.sockfd, ctx->parser.recv_buf,
          //                             sizeof(ctx->parser.recv_buf), 0);

          // if (ctx->parser.numbytes == 0) {
          //   fprintf(stderr, "Error: numbytes = 0\n");
          //   return 1;
          // }
          // memcpy(ctx->parser.acc_buf + ctx->parser.acc_len,
          //        ctx->parser.recv_buf, ctx->parser.numbytes);

          // ctx->parser.acc_len += ctx->parser.numbytes;
          break;
        }
        break;
      }

      case HEADER_KEY: {
        if (ctx->parser.recv_buf[ctx->parser.recv_idx] == '\r') {
          ctx->parser.state = HEADER_CRLF;
          break;
        }
        size_t start = ctx->parser.recv_idx;
        // printf("start is: %zu\n", start);
        // printf("end is: %zu\n", ctx->parser.recv_len);

        while (ctx->parser.recv_idx < ctx->parser.recv_len) {
          char c = ctx->parser.recv_buf[ctx->parser.recv_idx];
          // printf("char is: %c\n", c);

          if (c == ':') {
            key_buffer[k_idx] = '\0';
            ctx->parser.state = HEADER_VALUE;
            v_idx = 0;

            if (ctx->parser.recv_idx == ctx->parser.recv_len - 1) {
              int res = readChunk(ctx);
              if (res == -1) {
                ctx->parser.state = HEADER_ERROR;
                break;
              }

              // ctx->parser.numbytes =
              //     recv(ctx->conn.sockfd, ctx->parser.recv_buf,
              //          sizeof(ctx->parser.recv_buf), 0);

              // if (ctx->parser.numbytes == 0) {
              //   fprintf(stderr, "Error: recv returned 0 bytes\n");
              //   return 1;
              // }
              // ctx->parser.recv_idx = 0;
              // ctx->parser.recv_len = ctx->parser.numbytes;
            } else {

              ctx->parser.recv_idx++;
            }
            break;
          } else if (k_idx < sizeof(key_buffer) - 1 && c != '\n') {
            key_buffer[k_idx++] = c;
          }
          ctx->parser.recv_idx++;
        }

        if (!(ctx->parser.state == HEADER_CRLF ||
              ctx->parser.state == HEADER_VALUE)) {

          int res = accumulate(ctx, start);
          if (res == -1) {
            ctx->parser.state = HEADER_ERROR;
            break;
          }
          // ctx->parser.acc_len = 0;

          // memcpy(ctx->parser.acc_buf, ctx->parser.recv_buf + start,
          //        ctx->parser.recv_len - start);
          // ctx->parser.acc_len = (ctx->parser.recv_len - start);

          // ctx->parser.numbytes = recv(ctx->conn.sockfd, ctx->parser.recv_buf,
          //                             sizeof(ctx->parser.recv_buf), 0);

          // if (ctx->parser.numbytes == 0) {
          //   fprintf(stderr, "Error header_key_recv: recv returned 0
          //   bytes\n"); return 1;
          // }
          // memcpy(ctx->parser.acc_buf + ctx->parser.acc_len,
          //        ctx->parser.recv_buf, ctx->parser.numbytes);

          // ctx->parser.acc_len += ctx->parser.numbytes;
          ctx->parser.state = HEADER_KEY_ACC;
          k_idx = 0;
          break;
        }
        break;
      }
      case HEADER_KEY_ACC: {
        // printf("Header key acc!\n");
        /*
        iniziare il parsing in acc fino a quando non trovo \r
        copiare da \r a len da acc in recv
        cambiare lo stato in HEADER_VALUE
        */

        while (ctx->parser.acc_idx < ctx->parser.acc_len) {
          char c = ctx->parser.acc_buf[ctx->parser.acc_idx];

          if (c == ':') {
            key_buffer[k_idx] = '\0';
            ctx->parser.state = HEADER_VALUE;
            v_idx = 0;

            if (ctx->parser.acc_idx == ctx->parser.acc_len - 1) {
              int res = readChunk(ctx);
              if (res == -1){
                ctx->parser.state = HEADER_ERROR;
                break;
              }

              // ctx->parser.numbytes =
              //     recv(ctx->conn.sockfd, ctx->parser.recv_buf,
              //          sizeof(ctx->parser.recv_buf), 0);

              // if (ctx->parser.numbytes == 0) {
              //   fprintf(stderr, "Error: recv returned 0 bytes\n");
              //   return 1;
              // }
              // ctx->parser.recv_idx = 0;
              // ctx->parser.recv_len = ctx->parser.numbytes;
            } else {
              // copio i byte successivi a : da acc in recv
              ctx->parser.acc_idx++;
              copyFromAccToRecvBuf(ctx);
              // memcpy(ctx->parser.recv_buf,
              //        ctx->parser.acc_buf + ctx->parser.acc_idx,
              //        ctx->parser.acc_len - ctx->parser.acc_idx);
              // ctx->parser.recv_idx = 0;
              // ctx->parser.recv_len = ctx->parser.acc_len - ctx->parser.acc_idx;

            }
            break;
          } else if (k_idx < sizeof(key_buffer) - 1 && c != '\n') {
            key_buffer[k_idx++] = c;
          }
          ctx->parser.acc_idx++;
        }
        // se sono arrivato qui non ho ancora incontrato il byte : in acc quindi
        // leggo con recv() e sposto in acc
        if (!(ctx->parser.state == HEADER_CRLF ||
              ctx->parser.state == HEADER_VALUE)) {

          int res = keepAccumulating(ctx);
          if (res == -1) {
            ctx->parser.state = HEADER_ERROR;
            break;
          }
          // ctx->parser.numbytes = recv(ctx->conn.sockfd, ctx->parser.recv_buf,
          //                             sizeof(ctx->parser.recv_buf), 0);

          // if (ctx->parser.numbytes == 0) {
          //   fprintf(stderr, "Error header_key_recv: recv returned 0
          //   bytes\n"); return 1;
          // }

          // memcpy(ctx->parser.acc_buf + ctx->parser.acc_len,
          //        ctx->parser.recv_buf, ctx->parser.numbytes);

          // ctx->parser.acc_len += ctx->parser.numbytes;
          ctx->parser.state = HEADER_KEY_ACC;
          k_idx = 0;
          ctx->parser.acc_idx = 0;
          break;
        }

        break;
      }
      case HEADER_VALUE: {
        size_t start = ctx->parser.recv_idx;

        while (ctx->parser.recv_idx < ctx->parser.recv_len) {
          char c = ctx->parser.recv_buf[ctx->parser.recv_idx];
          // printf("header value - char c is: %c\n", c);

          if (c == '\r') {
            value_buffer[v_idx] = '\0';

            printf("Header: [%s] -> [%s]\n", key_buffer, value_buffer);
            if (strcasecmp(key_buffer, "Content-Length") == 0) {
              ctx->parser.content_length = atoi(value_buffer);
            } else if ((strcasecmp(key_buffer, "Transfer-Encoding") == 0) &&
                       (strcasecmp(value_buffer, "chunked") == 0)) {
              ctx->parser.isEncodingChunked = 1;
              printf("Encoding Chunked: true\n");
            }

            k_idx = 0;
            ctx->parser.state = HEADER_CRLF;
            break;
          } else if (v_idx < sizeof(value_buffer) - 1 && c != '\n') {

            if (!(v_idx == 0 && c == ' ')) {
              value_buffer[v_idx++] = c;
            };
          }
          ctx->parser.recv_idx++;
        }

        if (ctx->parser.state != HEADER_CRLF) {

          int res = accumulate(ctx, start);
          if (res == -1) {
            ctx->parser.state = HEADER_ERROR;
            break;
          }
          // ctx->parser.acc_len = 0;

          // memcpy(ctx->parser.acc_buf, ctx->parser.recv_buf + start,
          //        ctx->parser.recv_len - start);
          // ctx->parser.acc_len = (ctx->parser.recv_len - start);

          // ctx->parser.numbytes = recv(ctx->conn.sockfd, ctx->parser.recv_buf,
          //                             sizeof(ctx->parser.recv_buf), 0);

          // if (ctx->parser.numbytes == 0) {
          //   fprintf(
          //       stderr,
          //       "Error Header_Value header_key_recv: recv returned 0
          //       bytes\n");
          //   return 1;
          // }
          // memcpy(ctx->parser.acc_buf + ctx->parser.acc_len,
          //        ctx->parser.recv_buf, ctx->parser.numbytes);

          // ctx->parser.acc_len += ctx->parser.numbytes;
          ctx->parser.state = HEADER_VALUE_ACC;
          v_idx = 0;
          break;
        }

        break;
      }
      case HEADER_VALUE_ACC: {
        // printf("Header value acc!\n");
        while (ctx->parser.acc_idx < ctx->parser.acc_len) {
          char c = ctx->parser.acc_buf[ctx->parser.acc_idx];

          if (c == '\r') {
            value_buffer[v_idx] = '\0';
            printf("Header: [%s] -> [%s]\n", key_buffer, value_buffer);
            if (strcasecmp(key_buffer, "Content-Length") == 0) {
              ctx->parser.content_length = atoi(value_buffer);
            } else if ((strcasecmp(key_buffer, "Transfer-Encoding") == 0) &&
                       (strcasecmp(value_buffer, "chunked") == 0)) {
              ctx->parser.isEncodingChunked = 1;
              printf("Encoding Chunked: true\n");
            }

            k_idx = 0;
            ctx->parser.state = HEADER_CRLF;

            if (ctx->parser.acc_idx == ctx->parser.acc_len - 1) {

              int res = readChunk(ctx);
              if (res == -1) {
                ctx->parser.state = HEADER_ERROR;
                break;
              }

              // ctx->parser.numbytes =
              //     recv(ctx->conn.sockfd, ctx->parser.recv_buf,
              //          sizeof(ctx->parser.recv_buf), 0);

              // if (ctx->parser.numbytes == 0) {
              //   fprintf(stderr, "Error: recv returned 0 bytes\n");
              //   return 1;
              // }
              // ctx->parser.recv_idx = 0;
              // ctx->parser.recv_len = ctx->parser.numbytes;
            } else {
              // copio i byte successivi a \r da acc in recv
              copyFromAccToRecvBuf(ctx);
              // memcpy(ctx->parser.recv_buf,
              //        ctx->parser.acc_buf + ctx->parser.acc_idx,
              //        ctx->parser.acc_len - ctx->parser.acc_idx);
              // ctx->parser.recv_idx = 0;
              // ctx->parser.recv_len = ctx->parser.acc_len - ctx->parser.acc_idx;
            }

            break;
          } else if (v_idx < sizeof(value_buffer) - 1 && c != '\n') {
            // printf("recv_idx: %zu\n", ctx->parser.recv_idx);
            // printf("v_idx is: %zu\n", v_idx);

            if (!(v_idx == 0 && c == ' ')) {
              value_buffer[v_idx++] = c;
            };
          }
          ctx->parser.acc_idx++;
        }
        // printf("acc_idx is: %zu\n", ctx->parser.acc_idx);
        // printf("acc_len is: %zu\n", ctx->parser.acc_len);

        if (ctx->parser.state != HEADER_CRLF) {

          int res = keepAccumulating(ctx);
          if (res == -1) {
            ctx->parser.state = HEADER_ERROR;
            break;
          }
          // leggo con recv()
          // ctx->parser.numbytes = recv(ctx->conn.sockfd, ctx->parser.recv_buf,
          //                             sizeof(ctx->parser.recv_buf), 0);

          // if (ctx->parser.numbytes == 0) {
          //   fprintf(stderr, "Error: recv returned 0 bytes\n");
          //   return 1;
          // }
          // memcpy(ctx->parser.acc_buf + ctx->parser.acc_idx,
          //        ctx->parser.recv_buf, ctx->parser.numbytes);

          // ctx->parser.acc_len += ctx->parser.numbytes;
          ctx->parser.state = HEADER_VALUE_ACC;
          v_idx = 0;
          ctx->parser.acc_idx = 0;
        }
        break;
      }
      case HEADER_CRLF: {
        // printf("HEADER_CRLF!\n");
        // printf("(%zu, %zu)\n", ctx->parser.recv_idx, ctx->parser.recv_len);
        // printf("c is: %d\n", ctx->parser.recv_buf[ctx->parser.recv_idx]);

        // if (ctx->parser.recv_idx == ctx->parser.recv_len - 1) {
        //   // leggo con recv()
        //   ctx->parser.numbytes = recv(ctx->conn.sockfd, ctx->parser.recv_buf,
        //                               sizeof(ctx->parser.recv_buf), 0);

        //   if (ctx->parser.numbytes == 0) {
        //     fprintf(stderr, "Header_CRLF Error: recv returned 0 bytes\n");
        //     return 1;
        //   }
        //   ctx->parser.recv_idx = 0;
        //   ctx->parser.recv_len = ctx->parser.numbytes;
        // }

        if (ctx->parser.recv_buf[ctx->parser.recv_idx] == '\n') {
          // printf("(%zu, %zu)\n", ctx->parser.recv_idx, ctx->parser.recv_len);
          if (ctx->parser.recv_idx == ctx->parser.recv_len - 1) {
            // leggo con recv()
            int res = readChunk(ctx);
            if (res == -1){
              ctx->parser.state = HEADER_ERROR;
              break;
            }
            // ctx->parser.numbytes = recv(ctx->conn.sockfd, ctx->parser.recv_buf,
            //                             sizeof(ctx->parser.recv_buf), 0);

            // if (ctx->parser.numbytes == 0) {
            //   fprintf(stderr, "Error: recv returned 0 bytes\n");
            //   return 1;
            // }
            // ctx->parser.recv_idx = 0;
            // ctx->parser.recv_len = ctx->parser.numbytes;
            // printf("Reading %zu bytes from recv()\n", ctx->parser.numbytes);
            // printf("HERE!\n");
            if (ctx->parser.recv_buf[ctx->parser.recv_idx] == '\r') {
              ctx->parser.state = HEADER_DONE;
              // if (ctx->parser.recv_idx == ctx->parser.recv_len - 1) {
              //   // leggo con recv()
              //   ctx->parser.numbytes =
              //       recv(ctx->conn.sockfd, ctx->parser.recv_buf,
              //            sizeof(ctx->parser.recv_buf), 0);

              //   if (ctx->parser.numbytes == 0) {
              //     fprintf(stderr, "Error: recv returned 0 bytes\n");
              //     return 1;
              //   }
              //   ctx->parser.recv_idx = 0;
              //   ctx->parser.recv_len = ctx->parser.numbytes;
              // }
              ctx->parser.recv_idx++; // Salto anche l'ultimo '\n' che seguirà
              break;
            } else {
              // altrimenti iniziamo a leggere un altro header
              ctx->parser.state = HEADER_KEY;
              // printf("Reading another header_key_recv\n");
              break;
            }
          }
          // Se il carattere successivo è un altro '\r', significa che
          // abbiamo fatto \r\n\r
          if (ctx->parser.recv_buf[ctx->parser.recv_idx + 1] == '\r') {
            ctx->parser.state = HEADER_DONE;
            // if (ctx->parser.recv_idx == ctx->parser.recv_len - 1) {
            //   // leggo con recv()
            //   ctx->parser.numbytes =
            //       recv(ctx->conn.sockfd, ctx->parser.recv_buf,
            //            sizeof(ctx->parser.recv_buf), 0);

            //   if (ctx->parser.numbytes == 0) {
            //     fprintf(stderr, "Error: recv returned 0 bytes\n");
            //     return 1;
            //   }
            //   ctx->parser.recv_idx = 0;
            //   ctx->parser.recv_len = ctx->parser.numbytes;
            // } else {
            //   ctx->parser.recv_idx++; // Salto anche l'ultimo '\n' che
            //   seguirà
            // }
            ctx->parser.recv_idx++; // Salto anche l'ultimo '\n' che seguirà
          } else {
            // altrimenti iniziamo a leggere un altro header
            ctx->parser.state = HEADER_KEY;
            // printf("Reading another header_key_recv\n");
          }
        }
        ctx->parser.recv_idx++;

        break;
      }
      case HEADER_DONE:
        printf("Header Done!\n");
        exit(1);
        break;

      case DONE:
        printf("Done!\n");
        break;

      case HEADER_ERROR:
        printf("Header error!\n");
        exit(1);
        break;

      default:
        break;
    }
  }

  close(ctx->conn.sockfd);
  return 0;
}

int main(void) {
  Ctx ctx;
  initCtx(&ctx);
  if (initConnection(&ctx) != 0) return 1;

  int len, bytes_send;
  len = strlen(ctx.conn.req_buf);
  printf("Sending msg, len: %d\n", len);
  bytes_send = send(ctx.conn.sockfd, ctx.conn.req_buf, len, 0);
  if (bytes_send == -1) {
    perror("send");
    exit(1);
  }
  printf("Bytes sent: %d\n", bytes_send);

  parseResponse(&ctx);
  return 0;
}