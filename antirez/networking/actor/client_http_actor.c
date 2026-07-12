
#include <SDL3/SDL.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <libavcodec/avcodec.h>   // handle CODEC (decode AAC in PCM)
#include <libavformat/avformat.h> // handle container MPEG-TS and Custom I/O
#include <libavutil/avutil.h> // system utility (memory management, Frame, Packet)
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

#define MAX_MAILBOX_SIZE 32
#define ACTOR_NUM 3

#define HOST "ilsole24ore-radio.akamaized.net"
#define PORT "443"
#define MAXDATASIZE 8192
#define MAXACCUMULATOR 8192
#define MAXLINE 512
#define ABSOLUTE_PATH "/hls/live/2035301/radio24/playlist.m3u8"
#define BASE_PATH "/hls/live/2035301/radio24/"
#define MAXHEADERKEY 128
#define MAXHEADERVALUE 512
#define MAXSIZECHUNKED 128
#define MASTER_PLAYLIST_STR "playlist.m3u8"
#define MAXAUDIOBUFFER                                                         \
  15000000 // 67171 bit/sec, chunk audio is 6.82667 sec, 99 audio chunks
#define SDLBUFFER 352800 * 3 // 3 seconds
#define THRESHOLD 50000

typedef enum {
  ACTOR_NETWORK,
  ACTOR_BUFFER,
  ACTOR_PLAYER,
  ACTOR_LIST
} ActorType;

typedef enum { STATE_NETWORK_IDLE, STATE_NETWORK_DOWNLOADING } NetworkState;

typedef enum {
  STATE_BUFFER_EMPTY,
  STATE_BUFFER_BELOW_THRESHOLD,
  STATE_BUFFER_FULL
} BufferState;

typedef enum {
  STATE_PLAYER_PLAYING,
  STATE_PLAYER_STOPPED,
  STATE_PLAYER_BUFFERING
} PlayerState;

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
} ParserState;

typedef enum {
  EV_CHUNK_DOWNLOAD,
  EV_CHUNK_READY,
  EV_CHUNK_REQUEST,
  EV_PLAY,
  EV_PAUSE
} EventType;

typedef struct Event {
  EventType type;
  void *payload;
} Event;

typedef struct Mailbox {
  Event events[MAX_MAILBOX_SIZE];
  int head;
  int tail;
} Mailbox;

struct Actor;

typedef void (*TransitionFn)(struct Actor *self, Event ev);

typedef struct Actor {
  ActorType type;
  union {
    struct Actor **ele;
    struct {
      union {
        struct {
          NetworkState networkState;
          BufferState bufferState;
          PlayerState playerState;
        };
      };
      TransitionFn receive;
      Mailbox mailbox;
      void *local_ctx;
    };
  };
} Actor;

typedef struct Connection {
  char req_buf[1024];
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
  char url[512];
} chunkUrl;

typedef struct Parser {
  ParserState state;
  char recv_buf[MAXDATASIZE];
  size_t recv_idx;
  size_t recv_len;
  int numbytes;
  long content_length;
  int isEncodingChunked;
  int status_code;
  Line line;
  ResourceType resource;
  AudioBuffer audio_buf;
  chunkUrl urls_buf[100];
  float pcm_buffer[8192];
  int last_sequence;
} Parser;

typedef struct Ctx {
  Parser parser;
  Connection conn;
} Ctx;

typedef enum { MASTER_PLAYLIST = 0, SUB_PLAYLIST, CHUNK_AUDIO } ResourceType;

/* ------------------------ Allocation wrappers ----------------------------*/

void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    fprintf(stderr, "Out of memory allocation %zu bytes\n", size);
    exit(1);
  }
  return ptr;
}

void *xrealloc(void *old_ptr, size_t size) {
  void *ptr = realloc(old_ptr, size);
  if (ptr == NULL) {
    fprintf(stderr, "Out of memory allocation %zu bytes\n", size);
    exit(1);
  }
  return ptr;
}
/* ---------------------- Actor related functions -------------------------*/

void transitionFnPlayer(Actor *self, Event ev) {
  switch (self->playerState) {
    case STATE_PLAYER_PLAYING: {
      switch (ev.type) {
        case EV_PLAY: {
          break;
        }
        case EV_PAUSE: {
          break;
        }
        case EV_CHUNK_READY: {
          break;
        }
        default:
          break;
      }
      break;
    }
    case STATE_PLAYER_STOPPED: {
      switch (ev.type) {
        case EV_PLAY: {
          break;
        }
        case EV_PAUSE: {
          break;
        }
        case EV_CHUNK_READY: {
          break;
        }
        default:
          break;
      }
      break;
    }
    case STATE_PLAYER_BUFFERING: {
      switch (ev.type) {
        case EV_PLAY: {
          break;
        }
        case EV_PAUSE: {
          break;
        }
        case EV_CHUNK_READY: {
          break;
        }
        default:
          break;
      }
    }
    default: {
      fprintf(stderr, "Impossibile Player state\n");
      exit(1);
    }
  }
}
void transitionFnBuffer(Actor *self, Event ev) {
  switch (self->bufferState) {
    case STATE_BUFFER_EMPTY: {
      switch (ev.type) {
        case EV_CHUNK_READY: {
          break;
        }
        case EV_CHUNK_REQUEST: {
          break;
        }
        default:
          break;
      }
      break;
    }
    case STATE_BUFFER_BELOW_THRESHOLD: {
      switch (ev.type) {
        case EV_CHUNK_READY: {
          break;
        }
        case EV_CHUNK_REQUEST: {
          break;
        }
        default:
          break;
      }
      break;
    }
    case STATE_BUFFER_FULL: {
      switch (ev.type) {
        case EV_CHUNK_READY: {
          break;
        }
        case EV_CHUNK_REQUEST: {
          break;
        }
        default:
          break;
      }
      break;
    }
    default: {
      fprintf(stderr, "Impossibile Buffer state\n");
      exit(1);
    }
  }
}
void transitionFnNetwork(Actor *self, Event ev) {
  switch (self->networkState) {
    case STATE_NETWORK_IDLE: {
      switch (ev.type) {
        case EV_CHUNK_DOWNLOAD: {
          break;
        }
        default:
          break;
      }
      break;
    }
    case STATE_NETWORK_DOWNLOADING: {
      switch (ev.type) {
        case EV_CHUNK_DOWNLOAD: {
          break;
        }
        default:
          break;
      }
      break;
    }
    default: {
      fprintf(stderr, "Impossibile Network state\n");
      exit(1);
    }
  }
}

Actor *createNetworkActor(void) {
  Actor *actor = xmalloc(sizeof(Actor));
  actor->type = ACTOR_NETWORK;
  actor->networkState = STATE_NETWORK_IDLE;
  actor->local_ctx = NULL;
  actor->receive = transitionFnNetwork;
  actor->mailbox.head = 0;
  actor->mailbox.tail = 0;
  return actor;
}
Actor *createBufferActor(void) {
  Actor *actor = xmalloc(sizeof(Actor));
  actor->type = ACTOR_BUFFER;
  actor->bufferState = STATE_BUFFER_EMPTY;
  actor->local_ctx = NULL;
  actor->receive = transitionFnBuffer;
  actor->mailbox.head = 0;
  actor->mailbox.tail = 0;
  return actor;
}

Actor *createPlayerActor(void) {
  Actor *actor = xmalloc(sizeof(Actor));
  actor->type = ACTOR_PLAYER;
  actor->playerState = STATE_PLAYER_STOPPED;
  actor->local_ctx = NULL;
  actor->receive = transitionFnBuffer;
  actor->mailbox.head = 0;
  actor->mailbox.tail = 0;
  return actor;
}

Actor *createActorList(Actor *a, Actor *b, Actor *c) {
  Actor *actor = xmalloc(sizeof(Actor));
  actor->type = ACTOR_LIST;
  actor->ele = xmalloc(sizeof(Actor) * ACTOR_NUM);
  actor->ele[0] = a;
  actor->ele[1] = b;
  actor->ele[2] = c;
  return actor;
}

void sendMessage(Actor *dest, Event ev) {
  dest->mailbox.events[dest->mailbox.head] = ev;
  dest->mailbox.head = (dest->mailbox.head + 1) % MAX_MAILBOX_SIZE;
}

/* ------------- SSL -------------*/
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

  SSL_CTX_set_session_cache_mode(ctx->conn.ssl_ctx, SSL_SESS_CACHE_OFF);

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
      break;
    }
    default:
      fprintf(stderr, "\nsetRequest: case not handled\n");
      exit(1);
  }
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

static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
  Ctx *ctx = (Ctx *)opaque;
  int bytes_to_read = 0;

  while (1) {
    int head = ctx->parser.audio_buf.head;
    int tail = ctx->parser.audio_buf.tail;

    int available =
        (head >= tail) ? (head - tail) : (MAXAUDIOBUFFER - tail + head);

    if (available > 0) {
      bytes_to_read = (buf_size < available) ? buf_size : available;
      break;
    }
  }

  for (int i = 0; i < bytes_to_read; i++) {
    buf[i] = dequeueAudioBuffer(ctx);
  }

  return bytes_to_read;
}
void fetch(Ctx *ctx) {

  char key_buffer[MAXHEADERKEY] = {0};
  char value_buffer[MAXHEADERVALUE] = {0};
  size_t k_idx = 0, v_idx = 0;
  char size_buffer[MAXSIZECHUNKED];
  int chunk_size = 0;
  char c;

  int i = 0;
  int status_i = 0, url_count = 0;
  int audioCounter = 0;
  char num_buf[4] = {0};

  while (ctx->parser.state != DONE) {
    switch (ctx->parser.state) {

      case INIT_CONNECTION: {
        if (initConnection(ctx) != 0) {
          ctx->parser.state = CONNECTION_ERROR;
          break;
        }
        ctx->parser.state = SENDING_REQUEST;
        break;
      }
      case RECONNECT: {
        if (ctx->conn.ssl_handle) {
          SSL_shutdown(ctx->conn.ssl_handle);
          SSL_free(ctx->conn.ssl_handle);
          ctx->conn.ssl_handle =
              NULL; // Mettiamo a NULL per evitare dangling pointers
        }
        if (ctx->conn.sockfd >= 0) {
          close(ctx->conn.sockfd);
          ctx->conn.sockfd = -1;
        }

        resetParserForNextRequest(ctx);

        ctx->parser.state = INIT_CONNECTION;
        break;
      }

      case SENDING_REQUEST: {
        status_i = 0;

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

        // if (ctx->parser.resource == CHUNK_AUDIO) {
        //   printf("audioCounter is: %d\n", audioCounter);
        //   sleep(2);
        // }

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
            // printf("Status Code Rilevato: %d\n", ctx->parser.status_code);
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
          // printf("Header: [%s] -> [%s]\n", key_buffer, value_buffer);

          // Elaborazione Metadati
          if (strcasecmp(key_buffer, "Content-Length") == 0) {
            ctx->parser.content_length = atol(value_buffer);
          } else if (strcasecmp(key_buffer, "Transfer-Encoding") == 0 &&
                     strcasecmp(value_buffer, "chunked") == 0) {
            ctx->parser.isEncodingChunked = 1;
            // printf("Encoding Chunked: true\n");
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

              ctx->parser.urls_buf[url_count].i = chunk_num;

              snprintf(ctx->parser.urls_buf[url_count].url,
                       sizeof(ctx->parser.urls_buf[url_count].url), "%s",
                       ctx->parser.line.line_buf);
              ctx->parser.line.l_idx = 0;
              url_count++;

              if (url_count == 100) {
                if (ctx->parser.last_sequence ==
                    ctx->parser.urls_buf[url_count - 1].i) {
                  printf("\n No new chunks to fetch\n");
                  return;
                }
                int previous = ctx->parser.last_sequence;
                ctx->parser.last_sequence =
                    ctx->parser.urls_buf[url_count - 1].i;
                printf("\n Previous last sequence was: %d\n", previous);
                printf("\n Last sequence is: %d\n",
                       ctx->parser.urls_buf[url_count - 1].i);
                int chunks_to_fetch = ctx->parser.last_sequence - previous;
                if (chunks_to_fetch > 4) {
                  chunks_to_fetch = 4;
                }
                audioCounter = 100 - chunks_to_fetch;
                printf("\nChunks to fetch: %d\n", chunks_to_fetch);
                for (int i = 0; i < chunks_to_fetch; i++) {
                  printf("\nurl to fetch is: %s\n",
                         ctx->parser.urls_buf[audioCounter + i].url);
                }
                // exit(1);
                ctx->parser.state = RECONNECT;
                ctx->parser.resource = CHUNK_AUDIO;
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
            printf("AudioCounter is: %d\n", audioCounter);

            if (audioCounter < 99) {
              audioCounter++;
              ctx->parser.state = RECONNECT;
            } else {
              printf("\n[SUCCESS] All audio chunks copied to Ring Buffer!\n");
              audioCounter = 98;
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
        return;

      case DONE:
        return;

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
}

void *get_data(void *arg) {

  Ctx *ctx = (Ctx *)arg;

  while (1) {

    int head = ctx->parser.audio_buf.head;
    int tail = ctx->parser.audio_buf.tail;

    int available =
        (head >= tail) ? (head - tail) : (MAXAUDIOBUFFER - tail + head);

    printf("Available is: %d\n", available);

    while (available > THRESHOLD) {
      printf("[NETWORK] Buffer pieno (%d byte). Mi metto in pausa...\n",
             available);
      head = ctx->parser.audio_buf.head;
      tail = ctx->parser.audio_buf.tail;

      available =
          (head >= tail) ? (head - tail) : (MAXAUDIOBUFFER - tail + head);
    }
    printf("\n Fetching data...\n");
    ctx->parser.state = INIT_CONNECTION;
    ctx->parser.resource = MASTER_PLAYLIST;
    fetch(ctx);
    if (ctx->conn.ssl_handle) {
      SSL_shutdown(ctx->conn.ssl_handle);
      SSL_free(ctx->conn.ssl_handle);
      ctx->conn.ssl_handle = NULL; // Evita dangling pointers
    }
    if (ctx->conn.sockfd >= 0) {
      close(ctx->conn.sockfd);
      ctx->conn.sockfd = -1;
    }
  }

  SSL_shutdown(ctx->conn.ssl_handle);
  SSL_free(ctx->conn.ssl_handle);
  SSL_CTX_free(ctx->conn.ssl_ctx);
  close(ctx->conn.sockfd);
}

void *decode(void *arg) {
  Ctx *ctx = (Ctx *)arg;

  if (ctx->parser.audio_buf.head == ctx->parser.audio_buf.tail) {
    printf("\n Condition signal received\n");
  }

  // 2. INIZIALIZZAZIONE CONTESTI FFMPEG
  AVFormatContext *fmt_ctx = avformat_alloc_context();

  size_t avio_ctx_buffer_size = 4096;
  uint8_t *avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);

  AVIOContext *avio_ctx = avio_alloc_context(
      avio_ctx_buffer, avio_ctx_buffer_size, 0, ctx, &read_packet, NULL, NULL);

  fmt_ctx->pb = avio_ctx;

  // 3. ALLOCAZIONE SICURA DI PACKET E FRAME

  AVPacket *pkt = av_packet_alloc();
  AVFrame *decoded_frame = av_frame_alloc();

  if (!pkt || !decoded_frame) {
    fprintf(stderr, "Errore: Impossibile allocare Packet o Frame\n");
    free(ctx);
    exit(1);
  }

  // 4. APERTURA E AGGANCIO AL RING BUFFER
  int decode_ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
  if (decode_ret < 0) {
    fprintf(stderr, "Error: FFmpeg cannot read the Ring Buffer\n");
    free(ctx);
    exit(1);
  }
  if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
    fprintf(stderr, "Error: Cannot find stream info\n");
    free(ctx);
    exit(1);
  }
  printf("\n[SUCCESS] FFmpeg ha agganciato il Ring Buffer con successo!\n");

  av_dump_format(fmt_ctx, 0, "Radio24_Stream", 0);
  // 5. SELEZIONE AUTOMATICA DEL CODEC AUDIO (Rileva l'AAC da solo)

  const AVCodec *codec = NULL;

  int audio_stream_idx =
      av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
  if (audio_stream_idx < 0) {
    fprintf(stderr, "Error: No audio stream found in the HLS chunks\n");
    free(ctx);
    exit(1);
  }
  AVStream *audio_stream = fmt_ctx->streams[audio_stream_idx];

  AVCodecContext *c = avcodec_alloc_context3(codec);

  if (!c) {
    fprintf(stderr, "Could not allocate audio codec context\n");
    free(ctx);
    exit(1);
  }

  // Copiamo i parametri della traccia (canali, sample rate) nel contesto del
  // decoder
  avcodec_parameters_to_context(c, audio_stream->codecpar);

  if (avcodec_open2(c, codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    free(ctx);
    exit(1);
  }
  // 6. INIZIALIZZAZIONE HARDWARE AUDIO CON SDL3
  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    fprintf(stderr, "Error SDL_init: %s\n", SDL_GetError());
    free(ctx);
    exit(1);
  }
  SDL_AudioSpec spec;
  spec.format = SDL_AUDIO_F32; // Vogliamo campioni float a 32-bit
  spec.channels = c->ch_layout.nb_channels;
  spec.freq = c->sample_rate;

  SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(
      SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
  if (!stream) {
    fprintf(stderr, "Cannot open audio device: %s\n", SDL_GetError());
    free(ctx);
    exit(1);
  }
  SDL_ResumeAudioStreamDevice(stream);
  printf("\n[AUDIO] Motore SDL3 avviato (%d canali @ %d Hz). Riproduzione in "
         "corso...\n",
         spec.channels, spec.freq);

  while (1) {
    if (SDL_GetAudioStreamQueued(stream) > SDLBUFFER) {
      SDL_Delay(100);
    } else {
      int ret = av_read_frame(fmt_ctx, pkt);
      if (ret >= 0) {
        if (pkt->stream_index == audio_stream_idx) {

          // Spingiamo il pacchetto compresso nel decoder
          decode_ret = avcodec_send_packet(c, pkt);
          if (decode_ret < 0) {
            fprintf(stderr, "Error submitting the packet to the decoder\n");
            break;
          }

          // Estraiamo tutti i frame decompressi disponibili
          while (decode_ret >= 0) {
            decode_ret = avcodec_receive_frame(c, decoded_frame);
            if (decode_ret == AVERROR(EAGAIN) || decode_ret == AVERROR_EOF) {
              break;
            } else if (decode_ret < 0) {
              fprintf(stderr, "Error during decoding\n");
              free(ctx);
              exit(1);
            }

            // --- GESTIONE INTERLEAVING (FLTP -> FLT) ---
            int samples = decoded_frame->nb_samples;
            int channels = c->ch_layout.nb_channels;

            // Usiamo il pcm_buffer del tuo parser per riordinare i byte
            int pcm_idx = 0;
            if (decoded_frame->format == AV_SAMPLE_FMT_FLTP) {
              // Se è Planar, intrecciamo i canali a mano (L, R, L, R...)
              for (int s = 0; s < samples; s++) {
                for (int ch = 0; ch < channels; ch++) {
                  float *channel_data = (float *)decoded_frame->data[ch];
                  ctx->parser.pcm_buffer[pcm_idx++] = channel_data[s];
                }
              }
              // Spediamo il buffer intrecciato alle casse del tuo PC
              SDL_PutAudioStreamData(stream, ctx->parser.pcm_buffer,
                                     samples * channels * sizeof(float));
            } else {
              // Se fosse già packed (raro con l'AAC), lo inviamo direttamente
              SDL_PutAudioStreamData(stream, decoded_frame->data[0],
                                     samples * channels * sizeof(float));
            }
            av_frame_unref(decoded_frame);
          }
        }
        av_packet_unref(pkt);
      }
    }
  }

  // 8. PULIZIA FINALE DELLA MEMORIA
  printf("\n[END] Error.\n");
  SDL_DestroyAudioStream(stream);
  av_frame_free(&decoded_frame);
  av_packet_free(&pkt);
  avcodec_free_context(&c);
  avformat_close_input(&fmt_ctx);

  if (avio_ctx) {
    av_freep(&avio_ctx->buffer);
    avio_context_free(&avio_ctx);
  }
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
    // set TCP socket in non-blocking mode

    fcntl(ctx->conn.sockfd, F_SETFL, O_NONBLOCK);

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

void initCtx(Ctx *ctx) {
  ctx->parser.recv_idx = 0;
  ctx->parser.recv_len = 0;
  ctx->parser.content_length = 0;
  ctx->parser.status_code = 0;
  ctx->parser.isEncodingChunked = 0;
  ctx->parser.line.l_idx = 0;
  ctx->parser.audio_buf.head = 0;
  ctx->parser.audio_buf.tail = 0;
  ctx->parser.last_sequence = 0;

  memset(&ctx->conn.hints, 0, sizeof(ctx->conn.hints));
  ctx->conn.hints.ai_family = AF_UNSPEC;
  ctx->conn.hints.ai_socktype = SOCK_STREAM;
}

int main(void) {
  Actor *networkActor = createNetworkActor();
  Actor *bufferActor = createBufferActor();
  Actor *playerActor = createPlayerActor();
  Actor *actorList = createActorList(networkActor, bufferActor, playerActor);

  Ctx *ctx = calloc(1, sizeof(Ctx));
  if (!ctx) {
    fprintf(stderr,
            "Errore fatale: Impossibile allocare 15MB di memoria nell'Heap.\n");
    return 1;
  }

  initCtx(ctx);

  if (initSSL(ctx) != 0) {
    printf("Error initializing SSL context\n");
    exit(1);
  }
  ctx->parser.state = INIT_CONNECTION;
  ctx->parser.resource = MASTER_PLAYLIST;

  // Event Loop
  while (1) {
    handleNonBlockingIOnetwork(networkActor);
    handleNonBlockingIOaudio(playerActor);
    for (int i = 0; i < ACTOR_NUM; i++) {
      Actor *actor = actorList->ele[i];
      while (actor->mailbox.head != actor->mailbox.tail) {
        Event ev = actor->mailbox.events[actor->mailbox.tail];
        actor->mailbox.tail = (actor->mailbox.tail + 1) % MAX_MAILBOX_SIZE;
        actor->receive(actor, ev);
      }
    }
    SDL_Delay(1);
  }

  return 0;
}