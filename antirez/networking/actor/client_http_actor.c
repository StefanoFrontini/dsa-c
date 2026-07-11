
#include <SDL3/SDL.h>
#include <arpa/inet.h>
#include <ctype.h>
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

typedef enum {
  ACTOR_NETWORK,
  ACTOR_BUFFER,
  ACTOR_PLAYER,
  ACTOR_LIST
} ActorType;

// typedef enum {
//   STATE_NETWORK_IDLE,
//   STATE_NETWORK_DOWNLOADING,
//   STATE_BUFFER_EMPTY,
//   STATE_BUFFER_BELOW_THRESHOLD,
//   STATE_BUFFER_FULL,
//   STATE_PLAYER_PLAYING,
//   STATE_PLAYER_STOPPED,
//   STATE_PLAYER_BUFFERING
// } ActorState;

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

Actor *createActorList(Actor *a, Actor *b, Actor *c){
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

int main(void) {
  Actor *networkActor = createNetworkActor();
  Actor *bufferActor = createBufferActor();
  Actor *playerActor = createPlayerActor();
  Actor *actorList = createActorList(networkActor, bufferActor, playerActor);

  return 0;
}