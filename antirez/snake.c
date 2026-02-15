#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define GRID_ROWS 15
#define GRID_COLS 30
#define GRID_CELLS (GRID_ROWS * GRID_COLS)
#define GROUND '.'
#define BODY '*'
#define FRUIT 'O'

#define FPS 60
#define SNAKE_SPEED 10

#define QUEUE_KEY_PRESS_SIZE 3

// Time measured in microseconds(us)
const int64_t FRAME_TIME = 1000000 / FPS;              // ~16.666 us
const int64_t SNAKE_MOVE_TIME = 1000000 / SNAKE_SPEED; // 100.000 us

// ---------------------- DATA STRUCTURES -------------------------------

//  SNAKE is modelled as a doubly linked list

typedef struct Node {
  char x;
  char y;
  struct Node *next;
  struct Node *prev;
} Node;

typedef struct {
  Node *head;
  Node *tail;
  int direction;
} Snake;

enum Directions { IDLE = 0, MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT };

typedef enum {
  MODE_HUMAN,
  MODE_AI_BFS,
} GameMode;

//  FRUIT

typedef struct {
  char x;
  char y;
} Fruit;

// INPUT

/* Input buffer for storing key press within the 100us timeframe */
typedef struct {
  int keys[QUEUE_KEY_PRESS_SIZE];
  int head;  // Where to write new input
  int tail;  // Where to read for the update function
  int count; // How many
} InputBuffer;

enum Keys {
  ARROW_UP = 1000,
  ARROW_DOWN,
  ARROW_LEFT,
  ARROW_RIGHT,
  ESC_KEY,
  NO_KEY
};

// POINT in the grid

typedef struct {
  char x;
  char y;
} Point;

// GAME STATE

enum GameState {
  CONTINUE = 2000,
  GAME_OVER,
  GAME_OVER_WALL,
  GAME_OVER_BODY,
  SCORE_UP,
};

// BFS Queue. The queue is modeled as an array that resets every time BFS
// starts.

typedef struct {
  Point items[GRID_CELLS];
  int head;
  int tail;
} BFSQueue;

// ------------------------------------------------------------------------

// --- FUNCTION PROTOTYPES  ---
// int determineDirection(int curMode, InputBuffer *inputBuf, Snake *s, Fruit
// *f,
//                        char *grid);
// int applyPhysics(int dir, Snake *s, Fruit *f, char *grid);
// int bfs_path(Snake *s, Fruit *f, char *grid);
// GameMode update(char *grid, Snake *s, Fruit *f, InputBuffer *inputBuf,
//                 int curMode);

struct termios original;

// --- HELPER FUNCTIONS ---

void initBuffer(InputBuffer *buf) {
  buf->head = 0;
  buf->tail = 0;
  buf->count = 0;
}

void enqueueKey(InputBuffer *buf, int key) {
  if (buf->count < QUEUE_KEY_PRESS_SIZE) {
    buf->keys[buf->head] = key;
    buf->head = (buf->head + 1) % QUEUE_KEY_PRESS_SIZE;
    buf->count++;
  }
}

int dequeueKey(InputBuffer *buf) {
  if (buf->count == 0) return NO_KEY;
  int key = buf->keys[buf->tail];
  buf->tail = (buf->tail + 1) % QUEUE_KEY_PRESS_SIZE;
  buf->count--;
  return key;
}

void initBFSQueue(BFSQueue *q) {
  q->head = 0;
  q->tail = 0;
}
int isBFSQueueEmpty(BFSQueue *q) {
  return q->head == q->tail;
}

void enqueueBFS(BFSQueue *q, Point p) {
  q->items[q->head] = p;
  q->head++;
}
Point dequeueBFS(BFSQueue *q) {
  Point p = q->items[q->tail];
  q->tail++;
  return p;
}

/* Get new head position based on the direction */
Point getNextHeadPosition(Snake *s) {
  Point p = {s->head->x, s->head->y};
  switch (s->direction) {
    case MOVE_UP:
      p.y--;
      break;
    case MOVE_DOWN:
      p.y++;
      break;
    case MOVE_LEFT:
      p.x--;
      break;
    case MOVE_RIGHT:
      p.x++;
      break;
    default:
      break;
  }
  return p;
}
Point getCoord(int i) {
  char x = i % GRID_COLS;
  char y = i / GRID_COLS;
  return (Point){x, y};
}

/* Reads the keyboard inputs  */
int readKeyPress() {
  ssize_t nread;
  char c;

  nread = read(STDIN_FILENO, &c, 1);
  if (nread == 0) return 0;
  if (c != '\033') {
    return c;
  }
  char buf[2];
  nread = read(STDIN_FILENO, buf, sizeof(buf));
  if (nread == 0) {
    return ESC_KEY;
  }
  if (buf[0] == '[') {
    switch (buf[1]) {
      case 'A':
        return ARROW_UP;
      case 'B':
        return ARROW_DOWN;
      case 'C':
        return ARROW_RIGHT;
      case 'D':
        return ARROW_LEFT;
    }
  }
  return 0;
}

/* Sets the state of the element of the grid positioned at x and y
 * coordinates */
void setCell(char *grid, char x, char y, char state) {
  int index = x + GRID_COLS * y;
  grid[index] = state;
}

/* Gets the state of the element of the grid positioned at x and y
  coordinates */
char getCell(char *grid, char x, char y) {
  int index = x + GRID_COLS * y;
  return grid[index];
}

/*  Creates an element of the snake body */
Node *createNode(char x, char y) {
  Node *new = malloc(sizeof(Node));

  new->x = x;
  new->y = y;
  new->next = NULL;
  new->prev = NULL;

  return new;
}

/* Creates a snake. The snake is modelled as a doubly linked list
 * for fast adding to the head e removing from the tail. */
Snake *createSnake(char x, char y, char *grid) {
  Node *newNode = malloc(sizeof(Node));
  newNode->x = x;
  newNode->y = y;
  newNode->next = NULL;
  newNode->prev = NULL;
  Snake *s = malloc(sizeof(Snake));
  s->head = newNode;
  s->tail = newNode;
  s->direction = IDLE;
  setCell(grid, x, y, BODY);
  return s;
}

/* Prepends a new head to the snake body */
Snake *prepend(Snake *s, char x, char y, char *grid) {
  Node *newNode = createNode(x, y);
  s->head->prev = newNode;
  newNode->next = s->head;
  s->head = newNode;
  setCell(grid, x, y, BODY);
  return s;
}

/* Deletes the tail of the snake */
Snake *deleteTail(Snake *s, char *grid) {
  Node *newTail = s->tail->prev;
  newTail->next = NULL;
  setCell(grid, s->tail->x, s->tail->y, GROUND);
  free(s->tail);
  s->tail = newTail;
  return s;
}

/* Prints the snake body to stdout */
void printNode(Node *head) {
  if (head == NULL) return;
  while (head != NULL) {
    printf("(%d, %d) (head: %p) (next: %p, prev: %p)\n", head->x, head->y, head,
           head->next, head->prev);
    head = head->next;
  }
}
void freeSnake(Node *head) {
  if (head == NULL) return;
  freeSnake(head->next);
  free(head);
}

/* Restores the terminal settings on game exit */
void cleanup(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
  printf("\033[?25h"); // show cursor
}

/* Updates the fruit location on the grid */
void updateFruit(Fruit *f, char *grid) {
  char x = rand() % GRID_COLS;
  char y = rand() % GRID_ROWS;

  while (getCell(grid, x, y) == BODY) {
    x = rand() % GRID_COLS;
    y = rand() % GRID_ROWS;
  }
  f->x = x;
  f->y = y;
  setCell(grid, x, y, FRUIT);
}

/* Place the fruit on the grid randomly */
Fruit *createFruit(char *grid) {
  Fruit *f = malloc(sizeof(*f));
  char x = rand() % GRID_COLS;
  char y = rand() % GRID_ROWS;
  while (getCell(grid, x, y) == BODY) {
    x = rand() % GRID_COLS;
    y = rand() % GRID_ROWS;
  }
  f->x = x;
  f->y = y;
  setCell(grid, x, y, FRUIT);
  return f;
}

/* Helper function to get the time in microseconds */
int64_t current_timestamp() {
  struct timeval te;
  gettimeofday(&te, NULL); // get current time
  return te.tv_sec * 1000000LL + te.tv_usec;
}

// Input: Returns key pressed or NO_KEY
int input(void) {
  int pressedKey = readKeyPress();

  switch (pressedKey) {
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
    case ESC_KEY:
      return pressedKey;
    default:
      return NO_KEY;
  }
}

int update_physics_only(char *grid, Snake *s, Fruit *f) {
  Point next = getNextHeadPosition(s);

  // Wall collision
  if (next.x < 0 || next.x >= GRID_COLS || next.y < 0 || next.y >= GRID_ROWS) {
    return GAME_OVER_WALL;
  }

  // Body collision
  if (getCell(grid, next.x, next.y) == BODY) {
    return GAME_OVER_BODY;
  }

  // Movement
  s = prepend(s, next.x, next.y, grid);

  // Fruit collision
  if (next.x == f->x && next.y == f->y) {
    updateFruit(f, grid);
    return SCORE_UP;
  } else {
    s = deleteTail(s, grid);
    return CONTINUE;
  }
}

// BFS
char visited[GRID_CELLS];
int parent[GRID_CELLS];

int bfs_path(Snake *s, Fruit *f, char *grid) {
  BFSQueue q;
  Point start = {s->head->x, s->head->y};
  int startIdx = start.y * GRID_COLS + start.x;

  // Data structures reset
  initBFSQueue(&q);
  memset(visited, 0, sizeof(visited));
  memset(parent, -1, sizeof(parent));

  enqueueBFS(&q, start);
  setCell(visited, start.x, start.y, 1);

  while (!isBFSQueueEmpty(&q)) {
    Point current = dequeueBFS(&q);

    // Arrays for movements: Up, Down, Left, Right
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    for (int i = 0; i < 4; i++) {
      int nx = current.x + dx[i];
      int ny = current.y + dy[i];
      int currentIdx = current.y * GRID_COLS + current.x;
      int nIdx = ny * GRID_COLS + nx;

      // Borders checks
      if (nx < 0 || nx >= GRID_COLS || ny < 0 || ny >= GRID_ROWS) continue;

      // Visited and body checks
      if (getCell(visited, nx, ny) == 1 || getCell(grid, nx, ny) == BODY)
        continue;

      parent[nIdx] = currentIdx;

      setCell(visited, nx, ny, 1);

      enqueueBFS(&q, (Point){nx, ny});

      // Fruit found!
      if (nx == f->x && ny == f->y) {

        // Backtracking
        int curr = nIdx;
        while (parent[curr] != startIdx) {
          curr = parent[curr];
          if (curr == -1) return -1;
        }

        // curr is now the next node from the head on the optimal path
        Point nextMovePoint = getCoord(curr);

        int x_diff = nextMovePoint.x - start.x;
        int y_diff = nextMovePoint.y - start.y;

        if (x_diff == 0 && y_diff == -1) return MOVE_UP;
        if (x_diff == 1 && y_diff == 0) return MOVE_RIGHT;
        if (x_diff == 0 && y_diff == 1) return MOVE_DOWN;
        if (x_diff == -1 && y_diff == 0) return MOVE_LEFT;

        return -1;
      }
    }
  }
  return -1;
}

int determineDirection(int curMode, InputBuffer *inputBuf, Snake *s, Fruit *f,
                       char *grid) {

  if (curMode == MODE_HUMAN) {
    int key = dequeueKey(inputBuf);
    if (key != NO_KEY) {
      switch (key) {
        case ARROW_UP:
          if (s->direction != MOVE_DOWN) return MOVE_UP;
          break;
        case ARROW_DOWN:
          if (s->direction != MOVE_UP) return MOVE_DOWN;
          break;
        case ARROW_LEFT:
          if (s->direction != MOVE_RIGHT) return MOVE_LEFT;
          break;
        case ARROW_RIGHT:
          if (s->direction != MOVE_LEFT) return MOVE_RIGHT;
          break;
      }
    }
  } else if (curMode == MODE_AI_BFS) {

    int next_dir = bfs_path(s, f, grid);

    if (next_dir != -1) {
      // Case 1: BFS fount the path to the fruit
      return next_dir;
    } else {
      // Caso 2: BFS failed -> Survival Mode
      Point head = {s->head->x, s->head->y};
      int dirs[] = {MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT};
      int safe_dir = -1;

      for (int i = 0; i < 4; i++) {
        int test_dir = dirs[i];
        int nx = head.x;
        int ny = head.y;

        switch (test_dir) {
          case MOVE_UP:
            ny--;
            break;
          case MOVE_DOWN:
            ny++;
            break;
          case MOVE_LEFT:
            nx--;
            break;
          case MOVE_RIGHT:
            nx++;
            break;
        }

        if (nx < 0 || nx >= GRID_COLS || ny < 0 || ny >= GRID_ROWS) continue;
        if (getCell(grid, nx, ny) == BODY) continue;

        safe_dir = test_dir;
        break;
      }

      if (safe_dir != -1) {
        return safe_dir;
      } else {
        return GAME_OVER;
      }
    }
  }

  // If we are here it means in HUMAN MODE no key was pressed. The snakes
  // continues in the previous direction.
  return NO_KEY;
}
int applyPhysics(int dir, Snake *s, Fruit *f, char *grid) {
  if (dir == GAME_OVER) return GAME_OVER;
  if (dir != NO_KEY) {
    s->direction = dir;
  };
  if (s->direction == IDLE) {
    return CONTINUE;
  }
  Point next = getNextHeadPosition(s);

  // Wall collision
  if (next.x < 0 || next.x >= GRID_COLS || next.y < 0 || next.y >= GRID_ROWS) {
    return GAME_OVER_WALL;
  }

  // Body collision
  if (getCell(grid, next.x, next.y) == BODY) {
    return GAME_OVER_BODY;
  }

  // Movement
  s = prepend(s, next.x, next.y, grid);

  // Fruit collision
  if (next.x == f->x && next.y == f->y) {
    updateFruit(f, grid);
    return SCORE_UP;
  } else {
    s = deleteTail(s, grid);
    return CONTINUE;
  }
}

// Update state
GameMode update(char *grid, Snake *s, Fruit *f, InputBuffer *inputBuf,
                int curMode) {

  int new_dir = determineDirection(curMode, inputBuf, s, f, grid);
  return applyPhysics(new_dir, s, f, grid);
}

// Rendering: moves the cursor at Home and redraw

/* Draws the grid */
void printGrid(char *grid, char score) {
  for (int y = 0; y < GRID_ROWS; y++) {
    for (int x = 0; x < GRID_COLS; x++) {
      printf("%c", getCell(grid, x, y));
      if (y == 0 && x == GRID_COLS - 1) {
        printf("\tScore: %d", score);
      }
    }
    printf("\n");
  }
}
void render(char *grid, char score) {
  printf("\033[H");
  printGrid(grid, score);
}

int main(int argc, char **argv) {
  GameMode currentMode = MODE_HUMAN;
  if (argc == 1) {
    printf("Running in HUMAN mode...\n");
  }
  if (argc > 1) {
    if (strcmp(argv[1], "bfs") == 0) {
      currentMode = MODE_AI_BFS;
      printf("Running in AI mode: Breadth-First Search...\n");
    } else {
      printf("Unrecognize mode '%s'. Running in HUMAN mode. Try 'bfs'\n",
             argv[1]);
    }
  }
  sleep(2);

  srand(time(NULL));

  // Terminal setup
  tcgetattr(STDIN_FILENO, &original); // saves the terminal settings
  atexit(cleanup);
  struct termios raw = original;
  raw.c_lflag &= ~(ICANON | ECHO); // switch off buffer and echo
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // sets the terminal in raw mode

  // Game Setup
  char grid[GRID_CELLS]; // initialize the grid
  memset(grid, GROUND, GRID_CELLS);
  Snake *snake = createSnake(5, 5, grid); // creates the snake a (0, 0)
  char score = 0;
  Fruit *f = createFruit(grid);

  // Input Buffer Setup
  InputBuffer inputBuf;
  initBuffer(&inputBuf);

  // Initial render
  printf("\033[?25l"); // hide cursor
  printf("\033[2J");   // clean screen
  printf("\033[H");    // cursor at Home
  render(grid, score);
  // printf("Fruit is at: (%d, %d)\n", f->x, f->y);

  // Time variables
  int frame_count = 0;
  int64_t total_dt = 0;
  int64_t total_dt_snake = 0;
  int64_t end = current_timestamp();
  int gameState = CONTINUE;

  // GAME LOOP
  while (1) {
    int64_t start = current_timestamp();
    int64_t dt = start - end;
    end = start;

    total_dt += dt;
    total_dt_snake += dt;
    frame_count++;

    // FPS Counter
    if (total_dt >= 1000000) { // print the frame count every second
      printf("\033[%d;%dHFPS: %d", GRID_ROWS + 2, 0, frame_count);
      total_dt = 0;
      frame_count = 0;
    }

    // 1. Input phase
    int key = input();
    if (key == ESC_KEY) {
      printf("\033[%d;%dHExiting...\n", GRID_ROWS + 2, 0);
      goto endgame;
    }
    if (currentMode == MODE_HUMAN && key != NO_KEY) {
      enqueueKey(&inputBuf, key);
    }

    // 2. Update phase
    // move the snake every 100 us
    if (total_dt_snake >= SNAKE_MOVE_TIME) {
      gameState = update(grid, snake, f, &inputBuf, currentMode);

      if (gameState == SCORE_UP)
        score++;
      else if (gameState != CONTINUE) {
        // Handling Game Over
        printf("\033[%d;%dHGAME OVER! Code: %d\n", GRID_ROWS + 3, 0, gameState);
        goto endgame;
      }
      total_dt_snake -= SNAKE_MOVE_TIME;
    }

    // 3. Render phase
    render(grid, score);

    // 4. Capping
    int64_t work_done = current_timestamp() - start;
    if (work_done < FRAME_TIME) {
      usleep(FRAME_TIME - work_done);
    }
  }

endgame:
  freeSnake(snake->head);
  free(snake);
  free(f);
  return 0;
}