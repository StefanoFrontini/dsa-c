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

void free_tree(struct Node *root) {
  if (root == NULL)
    return;
  free_tree(root->left);
  free_tree(root->right);
  free(root);
}
struct Node *add_node(struct Node *root, int val) {
  if (root == NULL) {
    struct Node *new = create_node(val);
    return new;
  }

  if (val < root->val) {
    root->left = add_node(root->left, val);
  } else if (val > root->val) {
    root->right = add_node(root->right, val);
  }
  return root;
}

int main(void) {
  struct Node *root = NULL;
  root = add_node(root, 10);
  printf("%p\n", root);
  root = add_node(root, 5);
  printf("%p\n", root);
  root = add_node(root, 12);
  printf("%p\n", root);
  print_node(root);
  free_tree(root);
  return 0;
}