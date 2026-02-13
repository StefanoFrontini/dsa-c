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
#define SNAKE_SPEED 1

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

// Update state
int update(char *grid, Snake *s, Fruit *f, InputBuffer *inputBuf) {

  int key = dequeueKey(inputBuf);

  if (key != NO_KEY) {
    switch (key) {
      case ARROW_UP:
        if (s->direction != MOVE_DOWN) s->direction = MOVE_UP;
        break;
      case ARROW_DOWN:
        if (s->direction != MOVE_UP) s->direction = MOVE_DOWN;
        break;
      case ARROW_LEFT:
        if (s->direction != MOVE_RIGHT) s->direction = MOVE_LEFT;
        break;
      case ARROW_RIGHT:
        if (s->direction != MOVE_LEFT) s->direction = MOVE_RIGHT;
        break;
    }
  }

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
// BFS
char visited[GRID_CELLS];
int path[GRID_CELLS];

int bfs_path(Snake *s, Fruit *f, char *grid) {
  BFSQueue q;
  Point p = {s->head->x, s->tail->x};
  initBFSQueue(&q);
  memset(visited, 0, sizeof(visited));
  memset(path, 0, sizeof(path));
  int indexPath = 0;
  enqueueBFS(&q, p);
  setCell(visited, p.x, p.y, 1);
  while (!isBFSQueueEmpty(&q)) {
    Point current = dequeueBFS(&q);
    path[indexPath] = current.y * GRID_COLS + current.x;
    indexPath++;
    // printf("indexPath %d\n", indexPath);
    // printf("current.x %d\n", current.x);
    // printf("current.y %d\n", current.y);
    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        // printf("i: %d, j: %d\n", i, j);
        if (i == 0 && j == 0) continue;
        if (((i * j) != 0)) continue;
        if (current.x + i < 0 || current.x + i > GRID_COLS) continue;
        if (current.y + j < 0 || current.y + j > GRID_ROWS) continue;
        if (getCell(visited, current.x + i, current.y + j) == 1 ||
            getCell(grid, current.x + i, current.y + j) == BODY)
          continue;
        if (((current.x + i) == f->x) && ((current.y + j) == f->y)) {
          // Point r = getCoord(path[1]);
          // printf("r: (%d, %d)\n", r.x, r.y);
          // int x = r.x - p.x;
          // int y = r.y - p.y;
          // if (x == 0 && y == -1) return MOVE_UP;
          // if (x == 1 && y == 0) return MOVE_RIGHT;
          // if (x == 0 && y == 1) return MOVE_DOWN;
          // if (x == -1 && y == 0) return MOVE_LEFT;
          for (int k = 0; k < indexPath; k++) {
            printf("(%d, %d)\n", getCoord(path[k]).x, getCoord(path[k]).y);
          }
          return IDLE;
          // return getCoord(path[1]);
        }
        setCell(visited, current.x + i, current.y + j, 1);
        enqueueBFS(&q, (Point){current.x + i, current.y + j});
      }
    }
  }
  return -1;
}

int main(void) {
  int currentMode = MODE_AI_BFS;

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
  printf("Fruit is at: (%d, %d)\n", f->x, f->y);

  int result = bfs_path(snake, f, grid);
  // printf("result is: (%d, %d)\n", result.x, result.y);
  // return 0;

  // Input Buffer Setup
  InputBuffer inputBuf;
  initBuffer(&inputBuf);

  // Initial render
  printf("\033[?25l"); // hide cursor
  printf("\033[2J");   // clean screen
  printf("\033[H");    // cursor at Home
  render(grid, score);

  // Time variables
  int frame_count = 0;
  int64_t total_dt = 0;
  int64_t total_dt_snake = 0;
  int64_t end = current_timestamp();
  int gameState = CONTINUE;

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
      if (currentMode == MODE_HUMAN) {
        gameState = update(grid, snake, f, &inputBuf);

      } else if (currentMode == MODE_AI_BFS) {
        int next_dir = bfs_path(snake, f, grid);
        // printf("next_dir: %d\n", next_dir);
        if (next_dir != -1) {
          snake->direction = next_dir;
        } else {
          snake->direction = IDLE;
        }
        gameState = update_physics_only(grid, snake, f);
      }

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