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
#define MAXDATASIZE 1000
#define MAXHEADER 8192
#define ABSOLUTE_PATH "/"

typedef enum {
  HEADER_ACC = 0,
  HEADER_PARSING,
  BODY_PARSING,
  BODY_PARSING_CHUNKED,
  CONNECTION_CLOSED,
  RECV_ERROR
} ParserState;

typedef enum {
  HEADER_STATUS_LINE = 0,
  HEADER_KEY,
  HEADER_VALUE,
  HEADER_DONE,
  HEADER_CRLF
} HeaderState;

typedef enum {
  CHUNK_HEX_PARSING_HEADER_ACC = 0,
  CHUNK_HEX_PARSING_RECV_BUF,
  CHUNK_BODY_PARSING_HEADER_ACC,
  CHUNK_BODY_PARSING_RECV_BUF,
  CHUNK_PARSING_DONE,
} ChunkedState;

typedef struct Parser {
  ParserState state;
  HeaderState headerState;
  ChunkedState chunkedState;
  int header_idx;
  int body_idx;
  size_t k_idx;
  size_t v_idx;
  char header_accumulator[MAXHEADER];
  char key_buffer[128];
  char value_buffer[512];
  long content_length;
  int isEncodingChunked;
} Parser;

typedef struct Connection {
  char req_buf[512];
  int status;
  int sockfd;
  int numbytes;
  int status_code;
  char ipstr[INET6_ADDRSTRLEN];
  char recv_buf[MAXDATASIZE];
  int recv_len;
  int recv_idx;
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
  ctx->parser.header_idx = 0;
  ctx->parser.body_idx = 0;
  ctx->conn.status_code = 0;
  ctx->conn.recv_idx = 0;
  ctx->conn.recv_len = 0;
  ctx->parser.k_idx = 0;
  ctx->parser.v_idx = 0;
  ctx->parser.content_length = 0;
  ctx->parser.state = HEADER_ACC;
  ctx->parser.headerState = HEADER_STATUS_LINE;
  ctx->parser.chunkedState = CHUNK_HEX_PARSING_HEADER_ACC;

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
    // printf("Socket created with %s: %s!\n", ipver, ipstr);

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
int parseResponse(Ctx *ctx) {
  while (ctx->parser.state != CONNECTION_CLOSED) {
    switch (ctx->parser.state) {
      case HEADER_ACC: {
        while ((ctx->conn.numbytes = recv(ctx->conn.sockfd, ctx->conn.recv_buf,
                                          sizeof(ctx->conn.recv_buf), 0)) > 0) {
          if (ctx->parser.header_idx + ctx->conn.numbytes > MAXHEADER) {
            fprintf(stderr, "Error reading header");
            return 1;
          }
          memcpy(ctx->parser.header_accumulator + ctx->parser.header_idx,
                 ctx->conn.recv_buf, ctx->conn.numbytes);
          int old_header_idx = ctx->parser.header_idx;
          ctx->parser.header_idx += ctx->conn.numbytes;
          /*
          we have already scanned header_accumulator till header_idx. So we scan
          only the added chunk. The sequence \n\r could be split between two
          recv calls: one call ends with \r and the second one starts with \n.
          In order to handle this case we subtract 1 to old_header_idx.
          */
          int i = old_header_idx - 1;
          for (; i < ctx->parser.header_idx; i++) {
            if (ctx->parser.header_accumulator[i] == '\n' &&
                ctx->parser.header_accumulator[i + 1] == '\r') {
              ctx->parser.state = HEADER_PARSING;
              ctx->parser.body_idx = i + 3;
              printf("body_idx is: %d\n", ctx->parser.body_idx);
              break; // break from for loop
            }
          }
          if (ctx->parser.state == HEADER_PARSING)
            break; // break from while loop
        }
        if (ctx->conn.numbytes == -1) {
          perror("recv");
          return 1;
        }
        break;
      }
      case HEADER_PARSING: {
        printf("HeaderParsing: \n");
        for (int i = 0; i < ctx->parser.header_idx; i++) {
          char c = ctx->parser.header_accumulator[i];

          if (ctx->parser.headerState == HEADER_DONE) {
            printf("--- Fine Header. Inizio Body al byte indicizzato: %d ---\n",
                   i);
            if (ctx->conn.status_code == 200 && ctx->parser.isEncodingChunked) {
              ctx->parser.state = BODY_PARSING_CHUNKED;
            }
            break;
          }

          switch (ctx->parser.headerState) {
            case HEADER_STATUS_LINE: {
              if (c > 8 && isdigit(c) &&
                  isdigit(ctx->parser.header_accumulator[i + 1]) &&
                  isdigit(ctx->parser.header_accumulator[i + 2])) {
                char num_buf[4];
                memcpy(num_buf, ctx->parser.header_accumulator + i, 3);
                num_buf[3] = '\0';
                ctx->conn.status_code = atoi(num_buf);
                printf("Status Code Rilevato: %d\n", ctx->conn.status_code);

                if (ctx->conn.status_code == 400) {
                  fprintf(stderr, "Error: 400\n");
                  exit(1);
                }
              }
              if (c == '\n') {
                ctx->parser.headerState = HEADER_KEY;
              }
              break;
            }
            case HEADER_KEY: {
              if (c == '\r') {
                ctx->parser.headerState = HEADER_CRLF;

              } else if (c == ':') {
                ctx->parser.key_buffer[ctx->parser.k_idx] = '\0';
                ctx->parser.headerState = HEADER_VALUE;
                ctx->parser.v_idx = 0;
              } else if (ctx->parser.k_idx <
                             sizeof(ctx->parser.key_buffer) - 1 &&
                         c != '\n') {
                ctx->parser.key_buffer[ctx->parser.k_idx++] = c;
              }
              break;
            }
            case HEADER_VALUE: {
              if (c == '\r') {
                ctx->parser.value_buffer[ctx->parser.v_idx] = '\0';

                printf("Header: [%s] -> [%s]\n", ctx->parser.key_buffer,
                       ctx->parser.value_buffer);
                if (strcasecmp(ctx->parser.key_buffer, "Content-Length") == 0) {
                  ctx->parser.content_length = atoi(ctx->parser.value_buffer);
                } else if ((strcasecmp(ctx->parser.key_buffer,
                                       "Transfer-Encoding") == 0) &&
                           (strcasecmp(ctx->parser.value_buffer, "chunked") ==
                            0)) {
                  ctx->parser.isEncodingChunked = 1;
                  printf("Encoding Chunked: true\n");
                }

                ctx->parser.k_idx = 0;
                ctx->parser.headerState = HEADER_CRLF;
              } else if (ctx->parser.v_idx <
                         sizeof(ctx->parser.value_buffer) - 1) {

                if (ctx->parser.v_idx == 0 && c == ' ') continue;
                ctx->parser.value_buffer[ctx->parser.v_idx++] = c;
              }
              break;
            }
            case HEADER_CRLF: {
              if (c == '\n') {
                // Se il carattere successivo è un altro '\r', significa che
                // abbiamo fatto \r\n\r
                if (ctx->parser.header_accumulator[i + 1] == '\r') {
                  ctx->parser.headerState = HEADER_DONE;
                  i++; // Salto anche l'ultimo '\n' che seguirà
                } else {
                  ctx->parser.headerState = HEADER_KEY;
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
        char hex_string[128];
        int hex_len = 0;
        int hex_len_recv = 0;
        int recv_len = 0;
        int recv_idx = 0;
        long decimalValue;
        // printf("Header_idx: %d\n", ctx->parser.header_idx);
        // printf("body_idx %d\n", ctx->parser.body_idx);
        while (ctx->parser.chunkedState != CHUNK_PARSING_DONE) {
          switch (ctx->parser.chunkedState) {
            case CHUNK_HEX_PARSING_HEADER_ACC: {
              char *start =
                  ctx->parser.header_accumulator + ctx->parser.body_idx;
              while (start[hex_len] != '\r' && (ctx->parser.body_idx + hex_len <
                                                ctx->parser.header_idx)) {
                printf("char is: %c\n", start[hex_len]);
                hex_len++;
              }
              memcpy(hex_string, start, hex_len);
              if (ctx->parser.body_idx + hex_len == ctx->parser.header_idx) {
                ctx->parser.chunkedState = CHUNK_HEX_PARSING_RECV_BUF;
              } else {
                decimalValue = strtol(hex_string, NULL, 16);
                ctx->parser.chunkedState = CHUNK_BODY_PARSING_HEADER_ACC;
              }

              break;
            }
            case CHUNK_HEX_PARSING_RECV_BUF: {
              int numbytes = 0;
              int total = 0;

              while (numbytes = recv(ctx->conn.sockfd, ctx->conn.recv_buf,
                                     sizeof(ctx->conn.recv_buf), 0) > 0) {
                int counter = 0;
                while (ctx->conn.recv_buf[counter] != '\r' &&
                       counter < numbytes) {
                  counter++;
                }
                memcpy(hex_string + hex_len, ctx->conn.recv_buf, counter);
                total += counter;
                if (counter == numbytes) {
                  continue;
                } else {
                  hex_string[hex_len + total] = 0;
                  decimalValue = strtol(hex_string, NULL, 16);
                  ctx->parser.chunkedState = CHUNK_BODY_PARSING_RECV_BUF;
                  recv_idx = counter;
                  // ctx->conn.recv_idx = counter;
                  // ctx->conn.recv_len = numbytes;
                  recv_len = numbytes;
                  break;
                }
              }
              if (numbytes == 0) {
                decimalValue = strtol(hex_string, NULL, 16);
                if (decimalValue != 0) {
                  fprintf(stderr, "Error parsing chunk header");
                  exit(1);
                }
                ctx->parser.chunkedState = CHUNK_BODY_PARSING_RECV_BUF;
              }
              break;
            }
            case CHUNK_BODY_PARSING_HEADER_ACC: {
              int start_body_in_header = ctx->parser.body_idx + hex_len;
              int body_len_in_header =
                  ctx->parser.header_idx - start_body_in_header;
              if (body_len_in_header > 0) {
                for (int i = body_len_in_header; i < ctx->parser.header_idx;
                     i++) {
                  printf("%c", ctx->parser.header_accumulator[i]);
                }
              }
              ctx->parser.chunkedState = CHUNK_BODY_PARSING_RECV_BUF;
              break;
            }
            case CHUNK_BODY_PARSING_RECV_BUF: {
              if (recv_len == 0) {
                if (decimalValue != 0) {
                  fprintf(stderr, "Error");
                }
              }
              for (int i = 0; i < recv_len; i++) {
                printf("%s", ctx->conn.recv_buf[i]);
              }
              int numbytes = 0;
              int total = 0;

              while (numbytes = recv(ctx->conn.sockfd, ctx->conn.recv_buf,
                                     sizeof(ctx->conn.recv_buf), 0) > 0) {
                int counter = 0;
                while (ctx->conn.recv_buf[counter] != '\r' &&
                       counter < numbytes) {
                  counter++;
                }
                for (int j = 0; j < counter; j++) {
                  printf("%c");
                }
                // memcpy(hex_string + hex_len, ctx->conn.recv_buf, counter);
                total += counter;
                if (counter == numbytes) {
                  continue;
                } else {
                  hex_string[hex_len + total] = 0;
                  decimalValue = strtol(hex_string, NULL, 16);
                  ctx->parser.chunkedState = CHUNK_BODY_PARSING_RECV_BUF;
                  ctx->conn.recv_idx = counter;
                  break;
                }
              }
              if (numbytes == 0) {
                decimalValue = strtol(hex_string, NULL, 16);
                if (decimalValue != 0) {
                  fprintf(stderr, "Error parsing chunk header");
                  exit(1);
                }
                ctx->parser.chunkedState = CHUNK_BODY_PARSING_RECV_BUF;
              }
              break;
            }
            case CHUNK_PARSING_DONE:
              break;

            default:
              break;
          }
        }

        if (ctx->parser.body_idx + hex_len == ctx->parser.header_idx) {
          // i byte che definiscono il valore hex potrebbero essere in recv_buf

          recv(ctx->conn.sockfd, ctx->conn.recv_buf, sizeof(ctx->conn.recv_buf),
               0);
          char *start = ctx->conn.recv_buf;
          size_t i = 0;
          while (start[i] == '\r')
            i++;
          memcpy(hex_string + hex_len, start, i);
          hex_string[hex_len + i] = 0;
          printf("Unhandled");
          exit(1);
        } else {
          hex_string[hex_len] = 0;
          printf("Hex string: %s\n", hex_string);
          decimalValue = strtol(hex_string, NULL, 16);
          if (decimalValue == 0) {
            ctx->parser.state = CONNECTION_CLOSED;
            break;
          };
          printf("Chunk length: %ld\n", decimalValue);
          int j = ctx->parser.body_idx + hex_len;
          while (j < j + decimalValue && (j < ctx->parser.header_idx)) {
            printf("%c", ctx->parser.header_accumulator[j]);
            j++;
          }
          exit(1);
        }

        printf("\n--- RISULTATO PARSING ---\n");
        printf("HTTP Status: %d\n", ctx->conn.status_code);
        printf("Content-Length: %ld byte\n", ctx->parser.content_length);
        printf("Start parsing body chunked at %d\n", ctx->parser.body_idx);
        ctx->parser.state = CONNECTION_CLOSED;
        break;
      }
      case CONNECTION_CLOSED:
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