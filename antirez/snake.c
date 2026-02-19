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

// Forward Declaration
struct GameContext;
typedef struct GameContext GameContext;

typedef int (*InputStrategy)(GameContext *ctx);

typedef enum {
  NO_DIRECTION = -1,
  IDLE,
  MOVE_UP,
  MOVE_DOWN,
  MOVE_LEFT,
  MOVE_RIGHT
} Directions;

typedef Directions (*PathfinderFunction)(GameContext *ctx);

typedef struct {
  int body[GRID_CELLS];
  int head;
  int tail;
  int direction;
  int count;
  InputStrategy controller;
} Snake;

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

typedef enum {
  ARROW_UP = 1000,
  ARROW_DOWN,
  ARROW_LEFT,
  ARROW_RIGHT,
  ESC_KEY,
  NO_KEY
} Keys;

// POINT in the grid

typedef struct {
  char x;
  char y;
} Point;

// GAME STATE

typedef enum {
  CONTINUE = 2000,
  GAME_OVER,
  GAME_OVER_WALL,
  GAME_OVER_BODY,
  SCORE_UP,
} GameState;

// BFS Queue. The queue is modeled as an array that resets every time BFS
// starts.

typedef struct {
  Point items[GRID_CELLS];
  int head;
  int tail;
} BFSQueue;

// Game context
struct GameContext {
  char grid[GRID_CELLS];
  Snake snake;
  Fruit f;
  InputBuffer inputBuf;
  GameMode mode;
  int score;
  // BFS state
  char visited[GRID_CELLS];
  int parent[GRID_CELLS];
};

// ------------------------------------------------------------------------

struct termios original;

// --- HELPER FUNCTIONS ---

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

Point getCoord(int i) {
  char x = i % GRID_COLS;
  char y = i / GRID_COLS;
  return (Point){x, y};
}

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

int gridCellIdx(Point p) {
  return p.y * GRID_COLS + p.x;
}

void enqueueSnake(GameContext *ctx, Point p) {
  ctx->snake.body[ctx->snake.head] = gridCellIdx(p);
  ctx->snake.head = (ctx->snake.head + 1) % GRID_CELLS;
  ctx->snake.count++;
  setCell(ctx->grid, p.x, p.y, BODY);
}
Point dequeueSnake(GameContext *ctx) {
  int tailIdx = ctx->snake.body[ctx->snake.tail];
  Point t = getCoord(tailIdx);
  setCell(ctx->grid, t.x, t.y, GROUND);
  ctx->snake.tail = (ctx->snake.tail + 1) % GRID_CELLS;
  ctx->snake.count--;

  return t;
}

/* Get new head position based on the direction */
Point getNextHeadPosition(GameContext *ctx) {
  int currentHeadIdxInArray = (ctx->snake.head - 1 + GRID_CELLS) % GRID_CELLS;
  int gridIdx = ctx->snake.body[currentHeadIdxInArray];
  Point p = getCoord(gridIdx);
  switch (ctx->snake.direction) {
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

/* Reads the keyboard inputs  */
Keys readKeyPress() {
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

/* Restores the terminal settings on game exit */
void cleanup(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
  printf("\033[?25h"); // show cursor
}

/* Updates the fruit location on the grid */
void updateFruit(GameContext *ctx) {
  char x = rand() % GRID_COLS;
  char y = rand() % GRID_ROWS;

  while (getCell(ctx->grid, x, y) == BODY) {
    x = rand() % GRID_COLS;
    y = rand() % GRID_ROWS;
  }
  ctx->f.x = x;
  ctx->f.y = y;
  setCell(ctx->grid, x, y, FRUIT);
}

/* Helper function to get the time in microseconds */
int64_t current_timestamp() {
  struct timeval te;
  gettimeofday(&te, NULL); // get current time
  return te.tv_sec * 1000000LL + te.tv_usec;
}

// Input: Returns key pressed or NO_KEY
Keys input(void) {
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

// BFS

Directions bfs_path(GameContext *ctx) {
  BFSQueue q;
  int currentHeadIdxInArray = (ctx->snake.head - 1 + GRID_CELLS) % GRID_CELLS;
  int startIdx = ctx->snake.body[currentHeadIdxInArray];
  Point start = getCoord(startIdx);

  // Data structures reset
  initBFSQueue(&q);
  memset(ctx->visited, 0, sizeof(ctx->visited));
  memset(ctx->parent, -1, sizeof(ctx->parent));

  enqueueBFS(&q, start);
  setCell(ctx->visited, start.x, start.y, 1);

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
      if (getCell(ctx->visited, nx, ny) == 1 ||
          getCell(ctx->grid, nx, ny) == BODY)
        continue;

      ctx->parent[nIdx] = currentIdx;

      setCell(ctx->visited, nx, ny, 1);

      enqueueBFS(&q, (Point){nx, ny});

      // Fruit found!
      if (nx == ctx->f.x && ny == ctx->f.y) {

        // Backtracking
        int curr = nIdx;
        while (ctx->parent[curr] != startIdx) {
          curr = ctx->parent[curr];
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

Directions bfs_path_measured(GameContext *ctx) {
  static int64_t sum_time = 0;
  static int samples = 0;
  static int64_t last_print = 0;

  int64_t start = current_timestamp();

  int res = bfs_path(ctx);

  int64_t end = current_timestamp();

  sum_time += (end - start);
  samples++;
  if (end - last_print >= 1000000) {
    if (samples > 0) {
      printf("\033[%d;0H", GRID_ROWS + 4);
      printf("BFS Time: %lld us   Samples: %d\n", sum_time / samples, samples);
    }
    sum_time = 0;
    samples = 0;
    last_print = end;
  }
  return res;
}
Directions humanInputController(GameContext *ctx) {
  int key = dequeueKey(&ctx->inputBuf);
  if (key != NO_KEY) {
    switch (key) {
      case ARROW_UP:
        if (ctx->snake.direction != MOVE_DOWN) return MOVE_UP;
        break;
      case ARROW_DOWN:
        if (ctx->snake.direction != MOVE_UP) return MOVE_DOWN;
        break;
      case ARROW_LEFT:
        if (ctx->snake.direction != MOVE_RIGHT) return MOVE_LEFT;
        break;
      case ARROW_RIGHT:
        if (ctx->snake.direction != MOVE_LEFT) return MOVE_RIGHT;
        break;
    }
  }
  return IDLE;
}
Directions runSurvivalMode(GameContext *ctx) {
  int headIdx = ctx->snake.body[ctx->snake.head - 1];
  Point head = getCoord(headIdx);
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
    if (getCell(ctx->grid, nx, ny) == BODY) continue;

    safe_dir = test_dir;
    break;
  }

  if (safe_dir != -1) return safe_dir;
  return NO_DIRECTION;
}

// Generic AI Orchestrator
Directions genericAiLogic(GameContext *ctx, PathfinderFunction algo) {
  int next_dir = algo(ctx);

  if (next_dir != -1) {
    return next_dir;
  } else {
    printf("\033[%d;%dHSurvivalMode!...\n", GRID_ROWS + 4, 0);
    return runSurvivalMode(ctx);
  }
}

Directions aiBFSController(GameContext *ctx) {
  return genericAiLogic(ctx, bfs_path);
}

Directions aiBfsPerfomanceController(GameContext *ctx) {
  return genericAiLogic(ctx, bfs_path_measured);
}
GameState applyPhysics(int dir, GameContext *ctx) {
  if (dir == NO_DIRECTION) return GAME_OVER;
  if (dir != IDLE) {
    ctx->snake.direction = dir;
  };
  if (ctx->snake.direction == IDLE) {
    return CONTINUE;
  }

  Point next = getNextHeadPosition(ctx);

  // Wall collision
  if (next.x < 0 || next.x >= GRID_COLS || next.y < 0 || next.y >= GRID_ROWS) {
    return GAME_OVER_WALL;
  }

  // Body collision
  if (getCell(ctx->grid, next.x, next.y) == BODY) {
    return GAME_OVER_BODY;
  }

  // Movement
  enqueueSnake(ctx, next);

  // Fruit collision
  if (next.x == ctx->f.x && next.y == ctx->f.y) {
    updateFruit(ctx);
    return SCORE_UP;
  } else {
    dequeueSnake(ctx);
    return CONTINUE;
  }
}

// Update state
GameState update(GameContext *ctx) {
  int new_dir = ctx->snake.controller(ctx);
  return applyPhysics(new_dir, ctx);
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
void render(GameContext *ctx) {
  printf("\033[H");
  printGrid(ctx->grid, ctx->score);
}
void initSnake(char x, char y, GameContext *ctx) {
  ctx->snake.head = 0;
  ctx->snake.tail = 0;
  ctx->snake.count = 0;
  ctx->snake.direction = IDLE;
  Point p = {x, y};
  enqueueSnake(ctx, p);
  setCell(ctx->grid, x, y, BODY);
}

// /* Place the fruit on the grid randomly */
void initFruit(GameContext *ctx) {
  char x = rand() % GRID_COLS;
  char y = rand() % GRID_ROWS;
  while (getCell(ctx->grid, x, y) == BODY) {
    x = rand() % GRID_COLS;
    y = rand() % GRID_ROWS;
  }
  ctx->f.x = x;
  ctx->f.y = y;
  setCell(ctx->grid, x, y, FRUIT);
}

void initGame(GameContext *ctx, int mode_flag) {
  memset(ctx->grid, GROUND, GRID_CELLS);
  initSnake(5, 5, ctx);
  ctx->score = 0;
  initFruit(ctx);
  initBuffer(&ctx->inputBuf);
  memset(ctx->visited, 0, sizeof(ctx->visited));
  memset(ctx->parent, -1, sizeof(ctx->parent));
  ctx->mode = MODE_HUMAN;
  if (mode_flag == MODE_AI_BFS) {
    ctx->snake.controller = aiBFSController;
  } else {
    ctx->snake.controller = humanInputController;
  }
}

int main(int argc, char **argv) {
  // Game Setup
  GameContext game_ctx;

  // Default
  initGame(&game_ctx, MODE_HUMAN);

  if (argc > 1) {
    if (strcmp(argv[1], "bfs") == 0) {
      game_ctx.mode = MODE_AI_BFS;

      if (argc > 2 && strcmp(argv[2], "perf") == 0) {
        printf("Mode: AI BFS (Performance Monitoring ON)\n");
        game_ctx.snake.controller = aiBfsPerfomanceController;
      } else {
        printf("Mode: AI BFS (Standard)\n");
        game_ctx.snake.controller = aiBFSController;
      }
    }
  }
  if (game_ctx.mode == MODE_HUMAN) {
    printf("Running in HUMAN mode...Try 'bfs' for AI mode or 'bfs perf' for AI "
           "mode with metrics\n");
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

  // Initial render
  printf("\033[?25l"); // hide cursor
  printf("\033[2J");   // clean screen
  printf("\033[H");    // cursor at Home
  render(&game_ctx);

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
    if (game_ctx.mode == MODE_HUMAN && key != NO_KEY) {
      enqueueKey(&game_ctx.inputBuf, key);
    }

    // 2. Update phase
    // move the snake every 100 us
    if (total_dt_snake >= SNAKE_MOVE_TIME) {
      gameState = update(&game_ctx);

      if (gameState == SCORE_UP)
        game_ctx.score++;
      else if (gameState != CONTINUE) {
        // Handling Game Over
        printf("\033[%d;%dHGAME OVER! Code: %d\n", GRID_ROWS + 3, 0, gameState);
        goto endgame;
      }
      total_dt_snake -= SNAKE_MOVE_TIME;
    }

    // 3. Render phase
    render(&game_ctx);

    // 4. Capping
    int64_t work_done = current_timestamp() - start;
    if (work_done < FRAME_TIME) {
      usleep(FRAME_TIME - work_done);
    }
  }

endgame:
  // free(game_ctx.f);
  return 0;
}