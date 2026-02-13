#include <stdio.h>
#include <stdlib.h>

struct Node {
  int val;
  struct Node *left;
  struct Node *right;
};

struct Node *create_node(int val) {
  struct Node *n = malloc(sizeof(*n));
  n->val = val;
  n->left = NULL;
  n->right = NULL;
  return n;
}

void print_node(struct Node *n) {
  if (n == NULL)
    return;

  print_node(n->left);
  printf("%d\n", n->val);
  print_node(n->right);
}

void add_node(struct Node **root, int val) {
  if ((*root) == NULL) {
    struct Node *new = create_node(val);
    *root = new;
    return;
  }
  if (val < (*root)->val) {
    add_node(&(*root)->left, val);
  } else {
    add_node(&(*root)->right, val);
  }
}
void free_tree(struct Node *root) {
  if (root == NULL)
    return;
  free_tree(root->left);
  free_tree(root->right);
  free(root);
}

int main(void) {
  struct Node *root = NULL;
  add_node(&root, 10);
  add_node(&root, 5);
  add_node(&root, 12);

  print_node(root);
  free_tree(root);
  return 0;
}