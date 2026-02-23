#include <stdio.h>
#include <stdlib.h>

#define GRID_ROWS 15
#define GRID_COLS 30
#define GRID_CELLS (GRID_ROWS * GRID_COLS)

typedef struct {
  char x;
  char y;
} Point;

typedef struct {
  Point p;
  int g; // Costo esatto: passi dal punto di partenza
  int h; // Euristica: distanza di Manhattan dal cella n al frutto
  int f; // Costo totale: g + h
} Node;

typedef struct {
  Node frontier[GRID_CELLS];
  int count;
} minHeap;

void initHeap(minHeap *h) {
  h->count = 0;
}

// BUBBLE-UP: Inserts a node and makes it "float" upwards
void insertHeap(minHeap *h, Node n) {
  int index = h->count;
  h->count++;
  // "Hole" optimization: we slide parents down instead of swapping
  while (index > 0) {
    int pIdx = (index - 1) / 2;
    Node parent = h->frontier[pIdx];

    int should_swap = 0;
    if (n.f < parent.f) {
      should_swap = 1;
    } else if (n.f == parent.f && n.h < parent.h) {
      should_swap = 1; // A* prefers nodes closest to the target!
    }
    if (!should_swap) break;

    h->frontier[index] = parent; // Move the parent down
    index = pIdx;                // Move up
  }
  h->frontier[index] = n; // We insert the node into the final "hole"
}

// TRICKLE-DOWN: Extracts the root and rearranges the tree
Node deleteHeap(minHeap *h) {
  Node root = h->frontier[0];
  h->count--;
  if (h->count == 0) return root;

  // Take the last node and look for where to put it starting from the root
  Node lastNode = h->frontier[h->count];
  int index = 0;

  // Continue until there is at least one left child
  while ((index * 2) + 1 < h->count) {
    int leftChild = (index * 2) + 1;
    int rightChild = (index * 2) + 2;
    int smallerChild = leftChild;

    // Find the "smaller" child of the two
    if (rightChild < h->count) {
      Node lNode = h->frontier[leftChild];
      Node rNode = h->frontier[rightChild];

      if (rNode.f < lNode.f || (rNode.f == lNode.f && rNode.h < lNode.h)) {
        smallerChild = rightChild;
      }
    }

    // If the last node is smaller than the smallest child, we are good to go.
    Node sNode = h->frontier[smallerChild];
    if (lastNode.f < sNode.f ||
        (lastNode.f == sNode.f && lastNode.h <= sNode.h)) {
      break;
    }
    // Make the smallest node "go up"
    h->frontier[index] = sNode;
    index = smallerChild; // go down
  }

  // We insert the last node into the final "hole"
  h->frontier[index] = lastNode;

  return root;
}

//  Manhattan distance on a square grid
int heuristic(Point a, Point b) {
  return abs(a.x - b.x) + abs(a.y - b.y);
}

void print(minHeap h) {
  int i = 0;
  while (i < h.count) {
    printf("%d\n", h.frontier[i].f);
    i++;
  }
}

int main(void) {
  minHeap h;
  h.count = 0;
  Node newNode;
  newNode.f = 10;
  Node newNode2;
  newNode2.f = 5;
  Node newNode3;
  newNode3.f = 2;
  insertHeap(&h, newNode);
  insertHeap(&h, newNode2);
  insertHeap(&h, newNode3);
  Node n = deleteHeap(&h);
  printf("%d\n", n.f);
  Node n2 = deleteHeap(&h);
  printf("%d\n", n2.f);
  //   print(h);

  //   Node arr[2];
  //   arr[0] = (Node){1, 2};
  //   arr[1] = (Node){3, 4};
  //   printf("(%d, %d)\n", arr[0].x, arr[0].y);
  //   printf("(%d, %d)\n", arr[1].x, arr[1].y);
}