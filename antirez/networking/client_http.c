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
// #include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define HOST "ilsole24ore-radio.akamaized.net"
#define PORT "443"
#define MAXDATASIZE 1
#define MAXACCUMULATOR 8192
#define MAXLINE 60
#define ABSOLUTE_PATH "/hls/live/2035301/radio24/playlist.m3u8"
#define BASE_PATH "/hls/live/2035301/radio24/"
#define MAXHEADERKEY 128
#define MAXHEADERVALUE 512
#define MAXSIZECHUNKED 128
#define MASTER_PLAYLIST_STR "playlist.m3u8"
#define MAXAUDIOBUFFER                                                         \
  5667553 // 67171 bit/sec, chunk audio is 6.82667 sec, 99 audio chunks

typedef enum {
  INIT_CONNECTION = 0,
  HEADER_STATUS_LINE,
  HEADER_KEY,
  HEADER_CRLF,
  HEADER_VALUE,
  HEADER_DONE,
  HEADER_ERROR,
  BODY_CHUNKED_SIZE,
  BODY_CHUNKED_HTML,
  BODY_CONTENT_LENGTH,
  BODY_CHUNKED_AUDIO,
  BODY_ERROR,
  BODY_DONE,
  SENDING_REQUEST,
  REQUEST_ERROR,
  CONNECTION_ERROR,
  RECONNECT,
  DONE,
} State;

// Audio Buffer is implemented as a Ring Buffer
typedef struct AudioBuffer {
  char data[MAXAUDIOBUFFER];
  int head;
  int tail;

} AudioBuffer;

typedef struct Line {
  char line_buf[MAXLINE];
  size_t l_idx;
} Line;

typedef struct ChunkUrl {
  int i;
  char url[60];
} chunkUrl;

typedef enum { MASTER_PLAYLIST = 0, SUB_PLAYLIST, CHUNK_AUDIO } ResourceType;

typedef struct Parser {
  char recv_buf[MAXDATASIZE];
  size_t recv_idx;
  size_t recv_len;
  int numbytes;
  State state;
  long content_length;
  int isEncodingChunked;
  int status_code;
  Line line;
  ResourceType resource;
  AudioBuffer audio_buf;
  chunkUrl urls_buf[99];
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
  SSL_CTX *ssl_ctx;
  SSL *ssl_handle;

} Connection;

typedef struct Ctx {
  Parser parser;
  Connection conn;
} Ctx;

void enqueueAudioBuffer(Ctx *ctx, char *byte) {
  ctx->parser.audio_buf.data[ctx->parser.audio_buf.head] = *byte;
  ctx->parser.audio_buf.head =
      (ctx->parser.audio_buf.head + 1) % MAXAUDIOBUFFER;
}
// void enqueueAudioBuffer(Ctx *ctx, char *chunk, int numbytes) {
//   if (!numbytes)
//     return;
//
//   int i = 0;
//   while (numbytes--) {
//     ctx->parser.audio_buf.data[ctx->parser.audio_buf.head] = chunk[i++];
//     ctx->parser.audio_buf.head =
//         (ctx->parser.audio_buf.head + 1) % MAXAUDIOBUFFER;
//   }
// }

char dequeueAudioBuffer(Ctx *ctx) {
  char c = ctx->parser.audio_buf.data[ctx->parser.audio_buf.tail];
  ctx->parser.audio_buf.tail =
      (ctx->parser.audio_buf.tail + 1) % MAXAUDIOBUFFER;
  return c;
}

int initSSL(Ctx *ctx) {
  const SSL_METHOD *method = TLS_client_method();

  if (!method) {
    fprintf(stderr, "Error creating TLS method\n");
    return -1;
  }

  ctx->conn.ssl_ctx = SSL_CTX_new(method);
  if (!ctx->conn.ssl_ctx) {
    fprintf(stderr, "Error creating SSL context\n");
    return -1;
  }

  SSL_CTX_set_default_verify_paths(ctx->conn.ssl_ctx);

  return 0;
}
void setRequest(Ctx *ctx, ResourceType resource) {
  switch (resource) {

  case MASTER_PLAYLIST: {
    snprintf(ctx->conn.req_buf, sizeof(ctx->conn.req_buf),
             "GET %s%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: UndergroundRadio/1.0\r\n"
             "Connection: close\r\n"
             "\r\n",
             BASE_PATH, MASTER_PLAYLIST_STR, HOST);
    break;
  }
  case SUB_PLAYLIST: {
    snprintf(ctx->conn.req_buf, sizeof(ctx->conn.req_buf),
             "GET %s%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: UndergroundRadio/1.0\r\n"
             "Connection: close\r\n"
             "\r\n",
             BASE_PATH, ctx->parser.line.line_buf, HOST);
    // BASE_PATH, ctx->parser.line.line_buf, HOST);
    break;
  }
  // case CHUNK_AUDIO: {
  //   snprintf(ctx->conn.req_buf, sizeof(ctx->conn.req_buf),
  //            "GET %s%s HTTP/1.1\r\n"
  //            "Host: %s\r\n"
  //            "User-Agent: UndergroundRadio/1.0\r\n"
  //            "Connection: close\r\n"
  //            "\r\n",
  //            BASE_PATH, ctx->parser.urls_buf[0].url, HOST);
  //   break;
  // }
  default:
    fprintf(stderr, "\nsetRequest: case not handled\n");
    exit(1);
  }
}
void setAudioRequest(Ctx *ctx, int i) {
  snprintf(ctx->conn.req_buf, sizeof(ctx->conn.req_buf),
           "GET %s%s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "User-Agent: UndergroundRadio/1.0\r\n"
           "Connection: close\r\n"
           "\r\n",
           BASE_PATH, ctx->parser.urls_buf[i].url, HOST);
}

void initCtx(Ctx *ctx) {
  ctx->parser.recv_idx = 0;
  ctx->parser.recv_len = 0;
  ctx->parser.content_length = 0;
  // ctx->parser.state = INIT_CONNECTION;
  ctx->parser.status_code = 0;
  ctx->parser.isEncodingChunked = 0;
  ctx->parser.line.l_idx = 0;
  ctx->parser.audio_buf.head = 0;
  ctx->parser.audio_buf.tail = 0;

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

  ctx->conn.ssl_handle = SSL_new(ctx->conn.ssl_ctx);
  SSL_set_fd(ctx->conn.ssl_handle, ctx->conn.sockfd);
  printf("starting TLS handshake...\n");
  if (SSL_connect(ctx->conn.ssl_handle) <= 0) {
    fprintf(stderr, "Error during TLS handshake\n");
    ERR_print_errors_fp(stderr);
    return -1;
  }
  printf("Connected with %s: %s in HTTPS!\n", ctx->conn.ipver, ctx->conn.ipstr);
  freeaddrinfo(ctx->conn.servinfo);
  return 0;
}

/* Read a chunk of bytes from the network and place it on recv_buf. Return -1 on
 * error and 0 on success.*/
int readChunk(Ctx *ctx) {
  int res = SSL_read(ctx->conn.ssl_handle, ctx->parser.recv_buf,
                     sizeof(ctx->parser.recv_buf));

  if (res <= 0) {
    int err = SSL_get_error(ctx->conn.ssl_handle, res);
    fprintf(stderr, "\n[DEBUG] SSL_read fallita. Codice errore OpenSSL: %d\n",
            err);

    if (err == SSL_ERROR_ZERO_RETURN) {
      fprintf(stderr, "[DEBUG] Motivo: Il server ha chiuso la connessione TLS "
                      "in modo pulito (EOF).\n");
    } else if (err == SSL_ERROR_SYSCALL) {
      fprintf(stderr, "[DEBUG] Motivo: Errore di I/O sotto i piedi (TCP socket "
                      "chiuso o Reset dal server).\n");
    } else {
      ERR_print_errors_fp(stderr);
    }
    return -1;
  }

  ctx->parser.numbytes = res;
  ctx->parser.recv_len = ctx->parser.numbytes;
  ctx->parser.recv_idx = 0;
  return 0;
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
void resetParserForNextRequest(Ctx *ctx) {
  ctx->parser.recv_idx = 0;
  ctx->parser.recv_len = 0;
  ctx->parser.content_length = 0;
  ctx->parser.status_code = 0;
  ctx->parser.isEncodingChunked = 0;
  ctx->parser.line.l_idx = 0;
}

int start(Ctx *ctx) {

  char key_buffer[MAXHEADERKEY] = {0};
  char value_buffer[MAXHEADERVALUE] = {0};
  // char line_buffer[MAXLINE] = {0};
  size_t k_idx = 0, v_idx = 0;
  // , l_idx = 0;
  char size_buffer[MAXSIZECHUNKED];
  int chunk_size = 0;
  char c;

  int i = 0;
  int status_i = 0, size_i = 0, url_count = 0;
  int audioCounter = 0;
  char num_buf[4] = {0};

  while (ctx->parser.state != DONE) {
    switch (ctx->parser.state) {

    case INIT_CONNECTION: {
      // initCtx(ctx);
      if (initSSL(ctx) != 0) {
        ctx->parser.state = CONNECTION_ERROR;
        break;
      }
      if (initConnection(ctx) != 0) {
        ctx->parser.state = CONNECTION_ERROR;
        break;
      }
      ctx->parser.state = SENDING_REQUEST;
      break;
    }
    case RECONNECT: {
      SSL_shutdown(ctx->conn.ssl_handle);
      SSL_free(ctx->conn.ssl_handle);
      // SSL_CTX_free(ctx->conn.ssl_ctx);
      close(ctx->conn.sockfd);

      resetParserForNextRequest(ctx);

      ctx->conn.ssl_handle = NULL;
      ctx->parser.state = INIT_CONNECTION;
      break;
    }

    case SENDING_REQUEST: {
      status_i = 0, size_i = 0;

      int len, bytes_sent;
      if (ctx->parser.resource == MASTER_PLAYLIST) {
        setRequest(ctx, MASTER_PLAYLIST);
      } else if (ctx->parser.resource == SUB_PLAYLIST) {
        printf("[INFO] Sub-Playlist request\n");
        setRequest(ctx, SUB_PLAYLIST);
      } else if (ctx->parser.resource == CHUNK_AUDIO) {
        setAudioRequest(ctx, audioCounter);
      } else {
        ctx->parser.state = REQUEST_ERROR;
        break;
      }

      len = strlen(ctx->conn.req_buf);
      printf("\nSending request: %s\n", ctx->conn.req_buf);

      if (ctx->parser.resource == CHUNK_AUDIO) {
        printf("audioCounter is: %d\n", audioCounter);
        sleep(2);
      }

      bytes_sent = SSL_write(ctx->conn.ssl_handle, ctx->conn.req_buf, len);
      if (bytes_sent <= 0) {
        ctx->parser.state = REQUEST_ERROR;
        break;
      }

      if (readChunk(ctx) != 0) {
        ctx->parser.state = HEADER_ERROR;
        break;
      } else {
        ctx->parser.state = HEADER_STATUS_LINE;
      }
      break;
    }

    case HEADER_STATUS_LINE: {
      if (!getNextByte(ctx, &c)) {
        ctx->parser.state = HEADER_ERROR;
        break;
      }

      if (isdigit(c) && status_i >= 9 && status_i <= 11) {
        num_buf[status_i - 9] = c;
        if (status_i == 11) {
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
      status_i++;
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
          i = 0;
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
        if (v_idx == 0 && c == ' ')
          continue;
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
      // if (ctx->parser.resource == CHUNK_AUDIO) {
      //   printf("\nExiting\n");
      //   exit(1);
      // }
      // Da questo punto in poi, tutto ciò che getNextByte estrarrà
      // sarà il BODY puro (Html o Chunk Audio)!
      if (ctx->parser.isEncodingChunked) {
        ctx->parser.state = BODY_CHUNKED_SIZE;
        i = 0;
      } else if (ctx->parser.content_length > 0) {
        ctx->parser.state = BODY_CONTENT_LENGTH;
        ctx->parser.line.l_idx = 0;
        printf("[INFO] Start downloading static file (%ld byte):\n\n",
               ctx->parser.content_length);
      } else {
        ctx->parser.state = DONE;
      }
      break;

    case BODY_CONTENT_LENGTH: {

      if (ctx->parser.content_length > 0) {
        if (!getNextByte(ctx, &c)) {
          ctx->parser.state = BODY_ERROR;
          break;
        }

        if (ctx->parser.resource == CHUNK_AUDIO) {
          enqueueAudioBuffer(ctx, &c);
          ctx->parser.content_length--;
          break;
        }

        if (c == '\n') {
          ctx->parser.line.line_buf[ctx->parser.line.l_idx] = '\0';

          if (ctx->parser.line.line_buf[0] == '#') {
            ctx->parser.line.l_idx = 0;
            ctx->parser.content_length--;
            break;
          }

          if (ctx->parser.resource == MASTER_PLAYLIST &&
              strstr(ctx->parser.line.line_buf, ".m3u8") != NULL) {
            ctx->parser.resource = SUB_PLAYLIST;
            ctx->parser.state = RECONNECT;
            break;
          }

          if (ctx->parser.resource == SUB_PLAYLIST &&
              strstr(ctx->parser.line.line_buf, ".ts") != NULL) {

            int chunk_num =
                atol(ctx->parser.line.line_buf + ctx->parser.line.l_idx - 10);

            ctx->parser.urls_buf[i].i = chunk_num;

            strncpy(ctx->parser.urls_buf[url_count].url,
                    ctx->parser.line.line_buf,
                    sizeof(ctx->parser.urls_buf[url_count].url) - 1);
            ctx->parser.urls_buf[url_count]
                .url[sizeof(ctx->parser.urls_buf[url_count].url) - 1] = '\0';
            // memcpy(ctx->parser.urls_buf[i].url, ctx->parser.line.line_buf,
            //        strlen(ctx->parser.line.line_buf));
            //
            ctx->parser.line.l_idx = 0;
            url_count++;

            if (url_count == 99) {
              ctx->parser.state = RECONNECT;
              ctx->parser.resource = CHUNK_AUDIO;
              audioCounter = 0;
            }
            ctx->parser.content_length--;
            break;
          }
        }

        if (ctx->parser.line.l_idx < sizeof(ctx->parser.line.line_buf) - 1) {
          ctx->parser.line.line_buf[ctx->parser.line.l_idx++] = c;
        } else {
          ctx->parser.state = BODY_ERROR;
          break;
        }
        ctx->parser.content_length--;
      }

      if (ctx->parser.content_length == 0) {
        if (ctx->parser.resource == CHUNK_AUDIO) {

          if (audioCounter < 98) {
            audioCounter++;
            ctx->parser.state = RECONNECT;
          } else {
            printf("\n[SUCCESS] All 99 audio chunks copied to Ring Buffer!\n");
            audioCounter = 0;
            ctx->parser.state = BODY_DONE;
          }
        } else {
          ctx->parser.state = DONE;
        }
      }
      break;
    }

    case BODY_CHUNKED_SIZE: {

      if (!getNextByte(ctx, &c)) {
        ctx->parser.state = BODY_ERROR;
        break;
      }

      if (c == '\r') {
        char next_c;
        if (getNextByte(ctx, &next_c) && next_c == '\n') {
          size_buffer[i] = '\0';
          chunk_size = strtol(size_buffer, NULL, 16);
          printf("\n[DEBUG] Chunk size rilevato: %d byte\n", chunk_size);
          if (chunk_size == 0) {
            ctx->parser.state = BODY_DONE;
            break;
          }
          ctx->parser.state = BODY_CHUNKED_HTML;
        } else {
          ctx->parser.state = BODY_ERROR;
        }
        break;
      }

      if (i < MAXSIZECHUNKED - 1) {
        size_buffer[i++] = c;
      } else {
        ctx->parser.state = BODY_ERROR;
      }
      break;
    }

    case BODY_CHUNKED_HTML: {
      if (chunk_size > 0) {
        if (!getNextByte(ctx, &c)) {
          ctx->parser.state = BODY_ERROR;
          break;
        }
        printf("%c", c);
        chunk_size--;
      }
      if (chunk_size == 0) {
        char trailing_r, trailing_n;
        if (getNextByte(ctx, &trailing_r) && trailing_r == '\r' &&
            getNextByte(ctx, &trailing_n) && trailing_n == '\n') {
          ctx->parser.state = BODY_CHUNKED_SIZE;
          i = 0;
        } else {
          fprintf(stderr,
                  "Errore: Mancava il \\r\\n di chiusura del chunk data\n");
          ctx->parser.state = BODY_ERROR;
        }
      }
      break;
    }
    case BODY_DONE:
      printf("--- parsing BODY completato con successo ---\n");
      ctx->parser.state = DONE;
      break;

    case DONE:
      break;

    case HEADER_ERROR:
      fprintf(stderr, "Errore fatale nel parsing dell'header HTTP.\n");
      exit(1);

    case BODY_ERROR:
      fprintf(stderr, "Errore fatale nel parsing del body HTTP.\n");
      exit(1);

    case REQUEST_ERROR:
      fprintf(stderr, "Errore fatale nella GET request.\n");
      exit(1);

    case CONNECTION_ERROR:
      fprintf(stderr, "Errore fatale nella connessione.\n");
      exit(1);

    default:
      break;
    }
  }

  SSL_shutdown(ctx->conn.ssl_handle);
  SSL_free(ctx->conn.ssl_handle);
  SSL_CTX_free(ctx->conn.ssl_ctx);
  close(ctx->conn.sockfd);
  return 0;
}

int main(void) {

  Ctx ctx;
  memset(&ctx, 0, sizeof(Ctx));

  initCtx(&ctx);
  ctx.parser.state = INIT_CONNECTION;
  ctx.parser.resource = MASTER_PLAYLIST;

  start(&ctx);
  return 0;
}
