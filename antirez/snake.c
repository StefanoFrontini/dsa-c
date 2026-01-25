#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int main(void) {
  //   printf("%lu\n", sizeof(grid));
  //   grid snakeGrid;
  //   snakeGrid.row = 25;
  //   snakeGrid.column = 25;
  char grid[GRID_CELLS];
  memset(grid, GROUND, GRID_CELLS);
  snake *snake = createSnake(0, 0, grid);
  snake = prepend(snake, 1, 0, grid);
  snake = prepend(snake, 2, 0, grid);
  printNode(snake->head);
  //   snake = deleteTail(snake, grid);
  printf("-------------------------\n");
  printNode(snake->head);
  printf("-------------------------\n");
  //   setCell(grid, 24, 24, BODY);
  //   setCell(grid, 0, 0, BODY);
  printGrid(grid);
  //   initSnake();
  //   initGrid(grid, GROUND);

  return 0;
}