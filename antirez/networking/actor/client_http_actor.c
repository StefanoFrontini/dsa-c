
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

typedef enum {
  STATE_NETWORK_IDLE,
  STATE_NETWORK_DOWNLOADING
} NetworkActorState;

typedef enum {
  STATE_BUFFER_EMPTY,
  STATE_BUFFER_BELOW_THRESHOLD,
  STATE_BUFFER_FULL
} BufferActorState;

typedef enum {
  STATE_PLAYER_PLAYING,
  STATE_PLAYER_STOPPED,
  STATE_PLAYER_BUFFERING
} PlayerActorState;

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
    struct Actor *ele[3];
    struct {
      union {
        struct {
          NetworkActorState networkState;
          BufferActorState bufferState;
          PlayerActorState playerState;
        };
      };
      // ActorState state;
      TransitionFn receive;
      Mailbox mailbox;
      void *local_ctx;
      // union {
      //   struct {

      //   }
      //   // struct {
      //   //   NetworkActorState networkState;
      //   //   BufferActorState bufferState;
      //   //   PlayerActorState playerState;
      //   // } state;
      // };
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

void sendMessage(Actor *dest, Event ev) {
  dest->mailbox.events[dest->mailbox.head] = ev;
  dest->mailbox.head = (dest->mailbox.head + 1) % MAX_MAILBOX_SIZE;

}

void receiveMessage(Actor *self, Event ev) {
  switch (self->type) {
    case ACTOR_NETWORK: {
      break;
    }
    case ACTOR_BUFFER: {
      break;
    }
    case ACTOR_PLAYER: {
      break;
    }
    default:
    break;

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
      break;
    }
    default:
      break;
  }
}

int main(void) {
  Actor networkActor;
  networkActor.type = ACTOR_NETWORK;
  networkActor.state = STATE_NETWORK_IDLE;
  networkActor.mailbox.head = 0;
  networkActor.mailbox.tail = 0;
  networkActor.local_ctx = NULL;

  Actor bufferActor;
  Actor playerActor;
  Actor actors[3];
  return 0;
}