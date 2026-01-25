#include <stdio.h>
#include <stdlib.h>
// #include <unistd.h>

// typedef struct {
//   uint8_t row;
//   uint8_t column;
// } grid;
#define GRID_ROWS 25
#define GRID_COLS 25
#define GRID_CELLS (GRID_ROWS * GRID_COLS)
#define GROUND "."
#define BODY "*"

typedef struct {
  char x;
  char y;
} coord;

typedef struct node {
  coord value;
  struct node *next;
  struct node *prev;

} node;

typedef struct {
  node *head;
  node *tail;

} snake;

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

  new->value.x = x;
  new->value.y = y;
  new->next = NULL;
  new->prev = NULL;

  return new;
}
// void freeNode(node *n) { free(n); }

snake *createSnake(char x, char y) {
  node *newNode = malloc(sizeof(node));
  newNode->value.x = x;
  newNode->value.y = y;
  newNode->next = NULL;
  newNode->prev = NULL;
  snake *s = malloc(sizeof(snake));
  s->head = newNode;
  s->tail = newNode;
  return s;
}
snake *prepend(snake *s, char x, char y) {
  node *newNode = createNode(x, y);
  s->head->prev = newNode;
  newNode->next = s->head;
  s->head = newNode;
  return s;
}
snake *deleteTail(snake *s) {
  node *newTail = s->tail->prev;
  newTail->next = NULL;
  free(s->tail);
  s->tail = newTail;
  return s;
}

void printNode(node *head) {
  if (head == NULL)
    return;
  while (head != NULL) {
    printf("(%d, %d) (head: %p) (next: %p, prev: %p)\n", head->value.x,
           head->value.y, head, head->next, head->prev);
    head = head->next;
  }
}

int main(void) {
  //   printf("%lu\n", sizeof(grid));
  //   grid snakeGrid;
  //   snakeGrid.row = 25;
  //   snakeGrid.column = 25;
  //   char grid[GRID_CELLS];
  snake *snake = createSnake(12, 12);
  snake = prepend(snake, 15, 16);
  snake = prepend(snake, 18, 19);
  printNode(snake->head);
  snake = deleteTail(snake);
  printf("-------------------------\n");
  printNode(snake->head);
  //   initSnake();
  //   initGrid(grid, GROUND);

  return 0;
}