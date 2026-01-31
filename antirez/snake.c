#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define GRID_ROWS 15
#define GRID_COLS 30
#define GRID_CELLS (GRID_ROWS * GRID_COLS)
#define GROUND '.'
#define BODY '*'
#define FRUIT 'O'

typedef struct node {
  char x;
  char y;
  struct node *next;
  struct node *prev;
} node;

typedef struct {
  node *head;
  node *tail;
} snake;

typedef struct fruit {
  char x;
  char y;
} fruit;

enum Keys { ARROW_UP = 1000, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT, ESC_KEY };
enum Directions { IDLE = 0, MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT };

struct termios original;

/* this function reads the keyboard inputs  */
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
/* this function sets the state of the element of the grid positioned at x and y
 * coordinates */
void setCell(char *grid, char x, char y, char state) {
  int index = x + GRID_COLS * y;
  grid[index] = state;
}

/* this function gets the state of the element of the grid positioned at x and y
 * coordinates */
char getCell(char *grid, char x, char y) {
  int index = x + GRID_COLS * y;
  return grid[index];
}

/* this function creates an element of the snake body */
node *createNode(char x, char y) {
  node *new = malloc(sizeof(node));

  new->x = x;
  new->y = y;
  new->next = NULL;
  new->prev = NULL;

  return new;
}
/* this function creates a snake. The snake is modelled as a doubly linked list
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
  setCell(grid, x, y, BODY);
  return s;
}
/* this function prepends a new head to the snake body */
snake *prepend(snake *s, char x, char y, char *grid) {
  node *newNode = createNode(x, y);
  s->head->prev = newNode;
  newNode->next = s->head;
  s->head = newNode;
  setCell(grid, x, y, BODY);
  return s;
}
/* this function deletes the tail of the snake */
snake *deleteTail(snake *s, char *grid) {
  node *newTail = s->tail->prev;
  newTail->next = NULL;
  setCell(grid, s->tail->x, s->tail->y, GROUND);
  free(s->tail);
  s->tail = newTail;
  return s;
}
/* helper function to print the snake body to stdout */
void printNode(node *head) {
  if (head == NULL)
    return;
  while (head != NULL) {
    printf("(%d, %d) (head: %p) (next: %p, prev: %p)\n", head->x, head->y, head,
           head->next, head->prev);
    head = head->next;
  }
}
/* this function draw the grid */
void printGrid(char *grid) {
  for (int y = 0; y < GRID_ROWS; y++) {
    for (int x = 0; x < GRID_COLS; x++) {
      printf("%c", getCell(grid, x, y));
    }
    printf("\n");
  }
}
/* restores the terminal settings on game exit */
void cleanup(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
  printf("\033[?25h"); // show cursor
}

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
snake *move(snake *s, char x, char y, char *grid, fruit *f) {
  if (getCell(grid, x, y) == BODY) {
    printf("\033[16;0H");
    printf("Game Over\n");
    exit(1);
  }
  s = prepend(s, x, y, grid);
  if (x == f->x && y == f->y) {
    updateFruit(f, grid);
  } else {
    s = deleteTail(s, grid);
  }

  return s;
}
/* this fuction moves the snake right */
snake *moveRight(snake *s, char *grid, fruit *f) {
  char x = s->head->x;
  char y = s->head->y;
  if (x >= GRID_COLS - 1) {
    printf("\033[16;0H");
    printf("Game Over\n");
    exit(1);
  }
  x++;
  s = move(s, x, y, grid, f);
  return s;
}

/* this fuction moves the snake left */
snake *moveLeft(snake *s, char *grid, fruit *f) {
  char x = s->head->x;
  char y = s->head->y;
  if (x <= 0) {
    printf("\033[16;0H");
    printf("Game Over\n");
    exit(1);
  }
  x--;
  s = move(s, x, y, grid, f);
  return s;
}

/* this fuction moves the snake down */
snake *moveDown(snake *s, char *grid, fruit *f) {
  char x = s->head->x;
  char y = s->head->y;
  if (y >= GRID_ROWS - 1) {
    printf("\033[16;0H");
    printf("Game Over\n");
    exit(1);
  }
  y++;
  s = move(s, x, y, grid, f);
  return s;
}

/* this fuction moves the snake up */
snake *moveUp(snake *s, char *grid, fruit *f) {
  char x = s->head->x;
  char y = s->head->y;
  if (y <= 0) {
    printf("\033[16;0H");
    printf("Game Over\n");
    exit(1);
  }
  y--;
  s = move(s, x, y, grid, f);
  return s;
}

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
  snake *snake = createSnake(0, 0, grid); // creates the snake a (0, 0) position
  snake = prepend(snake, 1, 0, grid); // adds a new head to the snake: (1, 0)
  snake = prepend(snake, 2, 0, grid); // adds a new head to the snake: (2, 0)
  // printNode(snake->head);
  // printf("-------------------------\n");
  // printNode(snake->head);
  // printf("-------------------------\n");
  fruit *f = createFruit(grid);

  printf("\033[?25l"); // hide cursor
  printf("\033[2J");   // clean screen
  printf("\033[H");    // cursor at Home
  printGrid(grid);

  char direction = IDLE;
  while (1) {
    int c = readKeyPress();
    if (c > 0) {
      switch (c) {
        case ARROW_UP: {
          if (direction == MOVE_UP || direction == MOVE_DOWN)
            break;
          direction = MOVE_UP;
        } break;
        case ARROW_DOWN: {
          if (direction == MOVE_UP || direction == MOVE_DOWN)
            break;
          direction = MOVE_DOWN;
        } break;
        case ARROW_LEFT: {
          if (direction == MOVE_LEFT || direction == MOVE_RIGHT)
            break;
          direction = MOVE_LEFT;
        } break;
        case ARROW_RIGHT: {
          if (direction == MOVE_RIGHT || direction == MOVE_LEFT)
            break;
          direction = MOVE_RIGHT;
        } break;
        case ESC_KEY:
          printf("Closing...\n");
          exit(1);
        default:
          // printf("Key pressed is: %c\n", c);
          break;
      }
    }
    printf("\033[H");
    switch (direction) {
      case IDLE:
        break;
      case MOVE_RIGHT:
        snake = moveRight(snake, grid, f);
        break;
      case MOVE_LEFT:
        snake = moveLeft(snake, grid, f);
        break;
      case MOVE_UP:
        snake = moveUp(snake, grid, f);
        break;
      case MOVE_DOWN:
        snake = moveDown(snake, grid, f);
        break;
      default:
        break;
    }

    printGrid(grid);
    usleep(100000);
  }
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);

  return 0;
}