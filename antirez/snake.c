#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
// #include <unistd.h>

// typedef struct {
//   uint8_t row;
//   uint8_t column;
// } grid;
#define GRID_ROWS 15
#define GRID_COLS 30
#define GRID_CELLS (GRID_ROWS * GRID_COLS)
#define GROUND '.'
#define BODY '*'

struct termios original;

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

enum Keys { ARROW_UP = 1000, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT, ESC_KEY };

int readEditorKey() {
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

void setCell(char *grid, char x, char y, char state) {
  int index = x + GRID_COLS * y;
  grid[index] = state;
}

char getCell(char *grid, char x, char y) {
  int index = x + GRID_COLS * y;
  return grid[index];
}

// void setGrid(int x, int y, char state) { int index = y * GRID_COLS + x; }

// void initGrid(char *grid, char state) {
//   for (int i = 0; i < GRID_COLS; i++) {
//     for (int j = 0; j < GRID_ROWS; j++) {
//       setGrid(i, j, state);
//     }
//   }
// }
node *createNode(char x, char y) {
  node *new = malloc(sizeof(node));

  new->x = x;
  new->y = y;
  new->next = NULL;
  new->prev = NULL;

  return new;
}
// void freeNode(node *n) { free(n); }

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
snake *prepend(snake *s, char x, char y, char *grid) {
  node *newNode = createNode(x, y);
  s->head->prev = newNode;
  newNode->next = s->head;
  s->head = newNode;
  setCell(grid, x, y, BODY);
  return s;
}
snake *deleteTail(snake *s, char *grid) {
  node *newTail = s->tail->prev;
  newTail->next = NULL;
  setCell(grid, s->tail->x, s->tail->y, GROUND);
  free(s->tail);
  s->tail = newTail;
  return s;
}

void printNode(node *head) {
  if (head == NULL)
    return;
  while (head != NULL) {
    printf("(%d, %d) (head: %p) (next: %p, prev: %p)\n", head->x, head->y, head,
           head->next, head->prev);
    head = head->next;
  }
}
void printGrid(char *grid) {
  for (int y = 0; y < GRID_ROWS; y++) {
    for (int x = 0; x < GRID_COLS; x++) {
      printf("%c", getCell(grid, x, y));
    }
    printf("\n");
  }
}
void cleanup(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &original);
  printf("\033[?25h");
}
snake *moveRight(snake *s, char *grid) {
  char x = s->head->x;
  x++;
  s = prepend(s, x, s->head->y, grid);
  s = deleteTail(s, grid);
  return s;
}

int main(void) {
  struct termios raw;
  tcgetattr(STDIN_FILENO, &original);
  atexit(cleanup);
  raw = original;
  raw.c_lflag &= (~ICANON); // switch off buffer
  raw.c_lflag &= (~ECHO);   // switch off echo
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  printf("\033[?25l"); // hide cursor
  printf("\033[2J");   // clean screen
  printf("\033[H");    // cursor at Home

  char grid[GRID_CELLS];
  memset(grid, GROUND, GRID_CELLS);
  snake *snake = createSnake(0, 0, grid);
  snake = prepend(snake, 1, 0, grid);
  snake = prepend(snake, 2, 0, grid);
  // printNode(snake->head);
  // printf("-------------------------\n");
  // printNode(snake->head);
  // printf("-------------------------\n");
  printGrid(grid);
  // usleep(1);

  // char buf[3];
  // memset(buf, 0, 4);
  // ssize_t nread;

  char direction = 0;
  while (1) {
    int c = readEditorKey();
    if (c > 0) {
      switch (c) {
        case ARROW_UP:
          printf("Up\n");
          break;
        case ARROW_DOWN:
          printf("Down\n");
          break;
        case ARROW_LEFT:
          printf("Left\n");
          break;
        case ARROW_RIGHT:
          direction = 1;
          // printf("Right\n");
          break;
        case ESC_KEY:
          printf("Closing...\n");
          return 1;
        default:
          // printf("Key pressed is: %c\n", c);
          break;
      }
    }
    printf("\033[H");
    switch (direction) {
      case 0:
        break;
      case 1:
        snake = moveRight(snake, grid);
        break;
      default:
        break;
    }
    printGrid(grid);
    usleep(100000);
    // nread = read(STDIN_FILENO, buf, sizeof(buf));
    // if (nread == -1) {
    //   perror("Reading key");
    //   return 1;
    // };
    // if (nread == 1) {
    //   if (buf[0] == '\033') {
    //     printf("Closing...\n");
    //     break;
    //   }
    //   printf("Reading char: %c\n", buf[0]);
    // }
    // if (nread == 3 && buf[0] == '\033' && buf[1] == '[') {
    //   if (buf[2] == 'A') {
    //     printf("Su\n");
    //   }
    //   if (buf[2] == 'B') {
    //     printf("Giù\n");
    //   }
    //   if (buf[2] == 'C') {
    //     printf("Destra\n");
    //   }
    //   if (buf[2] == 'D') {
    //     printf("Sinistra\n");
    //   }
    // }
  }
  // ...play game
  tcsetattr(STDIN_FILENO, TCSANOW, &original);

  return 0;
}