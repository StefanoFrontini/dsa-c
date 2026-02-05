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

// Time measured in microseconds(us)
const int64_t FRAME_TIME = 1000000 / FPS;              // ~16.666 us
const int64_t SNAKE_MOVE_TIME = 1000000 / SNAKE_SPEED; // 100.000 us

typedef struct node {
  char x;
  char y;
  struct node *next;
  struct node *prev;
} node;

typedef struct {
  node *head;
  node *tail;
  char direction;
} snake;

typedef struct fruit {
  char x;
  char y;
} fruit;

enum Keys { ARROW_UP = 1000, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT, ESC_KEY };
enum Directions { IDLE = 0, MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT };
enum GameState { CONTINUE = 2000, GAME_OVER, SCORE_UP };

typedef struct {
  char x;
  char y;
} Point;

/* Get new head position based on the direction */
Point getNextHeadPosition(snake *s) {
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

struct termios original;

/* Reads the keyboard inputs  */
int readKeyPress() {
  ssize_t nread;
  char c;

  nread = read(STDIN_FILENO, &c, 1);
  if (nread == 0)
    return 0;
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
node *createNode(char x, char y) {
  node *new = malloc(sizeof(node));

  new->x = x;
  new->y = y;
  new->next = NULL;
  new->prev = NULL;

  return new;
}
/* Creates a snake. The snake is modelled as a doubly linked list
 * for fast adding to the head e removing from the tail. */
snake *createSnake(char x, char y, char *grid) {
  node *newNode = malloc(sizeof(node));
  newNode->x = x;
  newNode->y = y;
  newNode->next = NULL;
  newNode->prev = NULL;
  snake *s = malloc(sizeof(snake));
  s->head = newNode;
  s->tail = newNode;
  s->direction = IDLE;
  setCell(grid, x, y, BODY);
  return s;
}
/* Prepends a new head to the snake body */
snake *prepend(snake *s, char x, char y, char *grid) {
  node *newNode = createNode(x, y);
  s->head->prev = newNode;
  newNode->next = s->head;
  s->head = newNode;
  setCell(grid, x, y, BODY);
  return s;
}
/* Deletes the tail of the snake */
snake *deleteTail(snake *s, char *grid) {
  node *newTail = s->tail->prev;
  newTail->next = NULL;
  setCell(grid, s->tail->x, s->tail->y, GROUND);
  free(s->tail);
  s->tail = newTail;
  return s;
}
/* Prints the snake body to stdout */
void printNode(node *head) {
  if (head == NULL)
    return;
  while (head != NULL) {
    printf("(%d, %d) (head: %p) (next: %p, prev: %p)\n", head->x, head->y, head,
           head->next, head->prev);
    head = head->next;
  }
}
void freeSnake(node *head) {
  if (head == NULL)
    return;
  freeSnake(head->next);
  free(head);
}
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
/* Restores the terminal settings on game exit */
void cleanup(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
  printf("\033[?25h"); // show cursor
}

/* Updates the fruit location on the grid */
void updateFruit(fruit *f, char *grid) {
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
fruit *createFruit(char *grid) {
  fruit *f = malloc(sizeof(*f));
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
char input(void) {
  int pressedKey = readKeyPress();

  if (pressedKey > 0) {
    switch (pressedKey) {
      case ARROW_UP:
        return MOVE_UP;
        // if (snake->direction != MOVE_DOWN)
        //   snake->direction = MOVE_UP;
        // break;
      case ARROW_DOWN:
        return MOVE_DOWN;
        // if (snake->direction != MOVE_UP)
        //   snake->direction = MOVE_DOWN;
        // break;
      case ARROW_LEFT:
        return MOVE_LEFT;
        // if (snake->direction != MOVE_RIGHT)
        //   snake->direction = MOVE_LEFT;
        // break;
      case ARROW_RIGHT:
        return MOVE_RIGHT;
        // if (snake->direction != MOVE_LEFT)
        //   snake->direction = MOVE_RIGHT;
        // break;
      case ESC_KEY:
        return ESC_KEY;
        // goto endgame;
    }
  }
  return IDLE;
}

int main(void) {

  srand(time(NULL));
  struct termios raw;
  tcgetattr(STDIN_FILENO, &original); // saves the terminal settings
  atexit(cleanup);
  raw = original;
  raw.c_lflag &= (~ICANON); // switch off buffer
  raw.c_lflag &= (~ECHO);   // switch off echo
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // sets the terminal in raw mode

  char grid[GRID_CELLS]; // initialize the grid
  memset(grid, GROUND, GRID_CELLS);
  snake *snake = createSnake(0, 0, grid); // creates the snake a (0, 0)
  char score = 0;
  // printNode(snake->head);
  // printf("-------------------------\n");
  // printNode(snake->head);
  // printf("-------------------------\n");
  fruit *f = createFruit(grid);

  printf("\033[?25l"); // hide cursor
  printf("\033[2J");   // clean screen
  printf("\033[H");    // cursor at Home
  printGrid(grid, score);
  int frame_count = 0;
  int64_t total_dt = 0;
  int64_t total_dt_snake = 0;
  int64_t end = current_timestamp();
  char nextMoveRequested = 0;

  while (1) {
    int64_t start = current_timestamp();
    int64_t dt = start - end;
    end = start;

    total_dt += dt;
    total_dt_snake += dt;
    frame_count++;

    if (total_dt >= 1000000) { // print the frame count every second
      printf("\033[%d;%dHFPS: %d", GRID_ROWS + 2, 0, frame_count);
      total_dt = 0;
      frame_count = 0;
    }
    nextMoveRequested = input();
    if (nextMoveRequested == ESC_KEY)
      goto endgame;
    if (nextMoveRequested != IDLE) {
      snake->direction = nextMoveRequested;
    }

    // int pressedKey = readKeyPress();

    // if (pressedKey > 0) {
    //   switch (pressedKey) {
    //     case ARROW_UP:
    //       if (snake->direction != MOVE_DOWN)
    //         snake->direction = MOVE_UP;
    //       break;
    //     case ARROW_DOWN:
    //       if (snake->direction != MOVE_UP)
    //         snake->direction = MOVE_DOWN;
    //       break;
    //     case ARROW_LEFT:
    //       if (snake->direction != MOVE_RIGHT)
    //         snake->direction = MOVE_LEFT;
    //       break;
    //     case ARROW_RIGHT:
    //       if (snake->direction != MOVE_LEFT)
    //         snake->direction = MOVE_RIGHT;
    //       break;
    //     case ESC_KEY:
    //       goto endgame;
    //   }
    // }

    // move the snake every 100 us
    if (total_dt_snake >= SNAKE_MOVE_TIME) {
      if (snake->direction != IDLE) {
        Point next = getNextHeadPosition(snake);

        // 1. Checks wall collisions
        if (next.x < 0 || next.x >= GRID_COLS || next.y < 0 ||
            next.y >= GRID_ROWS) {
          printf("\033[%d;%dHGAME OVER (Wall)!\n", GRID_ROWS + 3,
                 0); // Prints at the end of the grid
          break;     // exit from while
        }

        // 2. Checks body collision
        if (getCell(grid, next.x, next.y) == BODY) {
          printf("\033[%d;%dHGAME OVER (Body)!\n", GRID_ROWS + 3, 0);
          break;
        }

        // 3. Moves the snake
        snake = prepend(snake, next.x, next.y, grid);

        // 4. Checks fruit collision
        if (next.x == f->x && next.y == f->y) {
          score++;
          updateFruit(f, grid);
        } else {
          snake = deleteTail(snake, grid);
        }
      }
      total_dt_snake -= SNAKE_MOVE_TIME;
    }

    // Rendering: moves the cursor at Home e redraw
    printf("\033[H");
    printGrid(grid, score);

    int64_t word_done = current_timestamp() - start;
    if (word_done < FRAME_TIME) {
      usleep(FRAME_TIME - word_done);
    }
  }

endgame:
  freeSnake(snake->head);
  free(snake);
  free(f);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);

  return 0;
}