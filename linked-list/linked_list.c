#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct node
{
    int number;
    struct node *next;
} node;

node *init(void);
node *createNode(int number);
void prepend(node **list, int item);
void test_linked_list();
void printList(node *list);

int main(void)
{
    test_linked_list();

    return 0;
}

void printList(node *list)
{
    node *ptr = list;
    while (ptr != NULL)
    {
        printf("%i\n", ptr->number);
        ptr = ptr->next;
    }
}

node *init(void)
{
    node *list = NULL;
    return list;
}

node *createNode(int number)
{
    node *n = malloc(sizeof(node));
    if (n == NULL)
    {
        exit(EXIT_FAILURE);
    }
    n->number = number;
    n->next = NULL;
    return n;
}

void prepend(node **list, int item)
{
    node *n = createNode(item);
    n->next = *list;
    *list = n;
}

void test_linked_list()
{
    node *list = init();
    prepend(&list, 1);
    prepend(&list, 2);
    printList(list);
}