#include <stdio.h>
#include <stdlib.h>

struct node {
  int val;
  struct node *left;
  struct node *right;
};

struct node *create_node(int val) {
  struct node *n = malloc(sizeof(*n));
  n->val = val;
  n->left = NULL;
  n->right = NULL;
  return n;
}

void print_node(struct node *n) {
  if (n == NULL)
    return;

  print_node(n->left);
  printf("%d\n", n->val);
  print_node(n->right);
}

void add_node(struct node **root, struct node *el) {
  if ((*root) == NULL) {
    *root = el;
    printf("%p\n", *root);
    return;
  }
  if (el->val < (*root)->val) {
    add_node(&(*root)->left, el);
  } else {
    add_node(&(*root)->right, el);
  }
}
void free_tree(struct node *root) {
  if (root == NULL)
    return;
  free_tree(root->left);
  free_tree(root->right);
  free(root);
}

int main(void) {
  struct node *root = NULL;
  struct node *node1 = create_node(10);
  struct node *node2 = create_node(5);
  struct node *node3 = create_node(12);
  add_node(&root, node1);
  add_node(&root, node2);
  add_node(&root, node3);

  print_node(root);
  free_tree(root);
  return 0;
}