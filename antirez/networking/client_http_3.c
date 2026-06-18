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
#include <assert.h>
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

int getNextByte(Ctx *ctx, char *byte) {
  if (ctx->parser.recv_idx < ctx->parser.recv_len) {
    *byte = ctx->parser.recv_buf[ctx->parser.recv_idx++];
    return 1; // Success
  }
  if (readChunk(ctx) == 0) {
    *byte = ctx->parser.recv_buf[ctx->parser.recv_idx++];
    return 1;
  }
  printf("getNextByte returns 0\n");
  return 0; // End of stream or error
}

int parseResponse(Ctx *ctx) {
  char key_buffer[MAXHEADERKEY] = {0};
  char value_buffer[MAXHEADERVALUE] = {0};
  size_t k_idx = 0, v_idx = 0;
  char c;

  if (readChunk(ctx) != 0) {
    ctx->parser.state = HEADER_ERROR;
    return -1;
  }

  int i = 0;
  char num_buf[4] = {0};

  while (ctx->parser.state != DONE) {
    switch (ctx->parser.state) {

      case HEADER_STATUS_LINE: {
        if (!getNextByte(ctx, &c)) {
          ctx->parser.state = HEADER_ERROR;
          break;
        }

        if (isdigit(c) && i >= 9 && i <= 11) {
          num_buf[i - 9] = c;
          if (i == 11) {
            num_buf[3] = '\0';
            ctx->parser.status_code = atoi(num_buf);
            printf("Status Code Rilevato: %d\n", ctx->parser.status_code);
            if (ctx->parser.status_code >= 400) {
              ctx->parser.state = HEADER_ERROR;
              break;
            }
          }
        }
        if (c == '\n') {
          ctx->parser.state = HEADER_KEY;
        }
        i++;
        break;
      }

      case HEADER_KEY: {
        if (!getNextByte(ctx, &c)) {
          ctx->parser.state = HEADER_ERROR;
          break;
        }

        // CASO CRITICO: Se la riga inizia con \r, gli header sono FINITI!
        if (k_idx == 0 && c == '\r') {
          // Consumiamo il \n successivo per ripulire totalmente il flusso
          char next_c;
          if (getNextByte(ctx, &next_c) && next_c == '\n') {
            ctx->parser.state = HEADER_DONE;
          } else {
            ctx->parser.state = HEADER_ERROR;
          }
          break;
        }

        if (c == ':') {
          key_buffer[k_idx] = '\0';
          ctx->parser.state = HEADER_VALUE;
          v_idx = 0;
          break;
        }

        if (k_idx < sizeof(key_buffer) - 1) {
          if (c != '\n' && c != '\r') {
            key_buffer[k_idx++] = c;
          }
        } else {
          ctx->parser.state = HEADER_ERROR; // Buffer Overflow Guard
        }
        break;
      }
      case HEADER_VALUE: {

        if (!getNextByte(ctx, &c)) {
          ctx->parser.state = HEADER_ERROR;
          break;
        }

        if (c == '\r') {
          value_buffer[v_idx] = '\0';
          printf("Header: [%s] -> [%s]\n", key_buffer, value_buffer);

          // Elaborazione Metadati
          if (strcasecmp(key_buffer, "Content-Length") == 0) {
            ctx->parser.content_length = atol(value_buffer);
          } else if (strcasecmp(key_buffer, "Transfer-Encoding") == 0 &&
                     strcasecmp(value_buffer, "chunked") == 0) {
            ctx->parser.isEncodingChunked = 1;
            printf("Encoding Chunked: true\n");
          }

          ctx->parser.state = HEADER_CRLF;
          break;
        }

        if (v_idx < sizeof(value_buffer) - 1) {
          // Salta lo spazio iniziale standard dopo i due punti
          if (v_idx == 0 && c == ' ') continue;
          value_buffer[v_idx++] = c;
        } else {
          ctx->parser.state = HEADER_ERROR;
        }
        break;
      }

      case HEADER_CRLF: {
        if (!getNextByte(ctx, &c)) {
          ctx->parser.state = HEADER_ERROR;
          break;
        }
        if (c == '\n') {
          // Reset per il prossimo header
          k_idx = 0;
          ctx->parser.state = HEADER_KEY;
        } else {
          ctx->parser.state = HEADER_ERROR;
        }
        break;
      }
      case HEADER_DONE:
        printf("--- parsing HEADER completato con successo ---\n");
        // Da questo punto in poi, tutto ciò che getNextByte estrarrà
        // sarà il BODY puro (Html o Chunk Audio)!
        ctx->parser.state = DONE;
        break;

      case DONE:
        printf("Done!\n");
        exit(1);
        break;

      case HEADER_ERROR:
        fprintf(stderr, "Errore fatale nel parsing dell'header HTTP.\n");
        exit(1);

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