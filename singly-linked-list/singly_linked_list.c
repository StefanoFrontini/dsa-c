#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
  int number;
  struct Node *next;
} Node;

Node *init(void);
Node *createNode(int number, Node **list);
void prepend(Node **list, int item);
void test_linked_list();
void printList(Node *list);
void freeMemory(Node **list);
void append(Node **list, int item);
Node *get(int idx, Node **list);
void removeItem(int item, Node **list);

int main(void) {
  test_linked_list();

  return 0;
}

void printList(Node *list) {
  Node *ptr = list;
  while (ptr != NULL) {
    printf("%i\n", ptr->number);
    ptr = ptr->next;
  }
}

Node *init(void) {
  Node *list = NULL;
  return list;
}

Node *createNode(int number, Node **list) {
  Node *n = malloc(sizeof(Node));
  if (n == NULL) {
    freeMemory(list);
    exit(EXIT_FAILURE);
  }
  n->number = number;
  n->next = NULL;
  return n;
}

void prepend(Node **list, int item) {
  Node *n = createNode(item, list);
  n->next = *list;
  *list = n;
}

void append(Node **list, int item) {
  Node *n = createNode(item, list);
  if (*list == NULL) {
    *list = n;
  } else {
    for (Node *ptr = *list; ptr != NULL; ptr = ptr->next) {
      if (ptr->next == NULL) {
        ptr->next = n;
        break;
      }
    }
  }
}

void freeMemory(Node **list) {
  Node *cursor = *list;
  while (cursor != NULL) {
    Node *tmp = cursor;
    cursor = cursor->next;
    free(tmp);
  }
}

void insertAt(int item, int idx, Node **list) {
  if (idx == 0) {
    prepend(list, item);
    printf("Inserted!\n");
  } else {
    Node *n = createNode(item, list);
    int i = 0;
    int inserted = 0;
    for (Node *ptr = *list; ptr != NULL; ptr = ptr->next, i++) {
      if (i == idx - 1) {
        n->next = ptr->next;
        ptr->next = n;
        inserted++;
        break;
      }
    }
    if (inserted == 0) {
      printf("Not Inserted!\n");
    } else {
      printf("Inserted!\n");
    }
  }
}

void deleteAt(int idx, Node **list) {
  if (*list == NULL) {
    printf("List is empty, nothing to delete");
    return;
  }
  if (idx == 0) {
    printf("Deleting head...\n");
    Node *ptr = *list;
    *list = ptr->next;
  } else {
    int i = 0;
    int deleted = 0;
    for (Node *ptr = *list; ptr != NULL; ptr = ptr->next, i++) {
      if (i == idx - 1 && ptr->next != NULL) {
        Node *next = ptr->next->next;
        free(ptr->next);
        ptr->next = next;
        deleted++;
        break;
      }
    }
    if (deleted == 0) {
      printf("Not deleted!\n");
    } else {
      printf("deleted!\n");
    }
  }
}

Node *get(int idx, Node **list) {
  if (*list == NULL) {
    printf("List is empty, returning null ptr\n");
    return NULL;
  }
  // if (idx == 0)
  // {
  //     node *ptr = *list;
  //     *list = ptr->next;
  //     printf("Getting head...%i\n", ptr->number);
  //     return *list;
  // }
  int i = 0;
  for (Node *ptr = *list; ptr != NULL; ptr = ptr->next, i++) {
    if (i == idx) {
      printf("Returning node with value %i!\n", ptr->number);
      return ptr;
    }
  }
  printf("Nothing to get, returning list ptr!\n");
  return NULL;
}

void removeItem(int item, Node **list) {
  if (*list == NULL) {
    printf("List is empty. Cannot remove\n");
    return;
  } else if ((**list).number == item && (**list).next != NULL) {
    printf("Head %i removed!\n", item);
    *list = (**list).next;
    return;
  } else if ((**list).number == item && (**list).next == NULL) {
    // *list = NULL;
    Node *temp = *list;
    *list = NULL;
    free(temp); // Libera la memoria
    printf("Head %i removed!\n", item);
    return;
  }

  else {
    for (Node *ptr = *list; ptr != NULL; ptr = ptr->next) {
      if (ptr->next != NULL && ptr->next->number == item) {
        ptr->next = ptr->next->next;
        printf("Item %i removed!\n", item);
        return;
      }
    }
    printf("Item not found\n");
  }
}

void test_linked_list() {
  // list is a variable storing the address of the first node
  Node *list = init();
  prepend(&list, 1);
  prepend(&list, 2);
  prepend(&list, 3);
  append(&list, 4);
  insertAt(11, 1, &list);
  deleteAt(0, &list);
  get(0, &list);
  printList(list);
  removeItem(11, &list);
  printList(list);

  freeMemory(&list);
}