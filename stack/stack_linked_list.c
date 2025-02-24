#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct s_node
{
    int number;
    struct s_node *next;
} s_node;

void test_stack();

s_node *createNode(int item);

void push(s_node **s, int item);
void freeMemory(s_node **q);

int main(void)
{
    test_stack();

    return 0;
}

s_node *init()
{
    s_node *s = NULL;
    return s;
}

s_node *createNode(int item)
{
    s_node *n = malloc(sizeof(s_node));
    if (n == NULL)
    {
        exit(EXIT_FAILURE);
    }
    n->number = item;
    n->next = NULL;
    return n;
}

void push(s_node **s, int item)
{
    s_node *n = createNode(item);
    n->next = *s;
    *s = n;
}

int pop(s_node **s)
{
    if (*s == NULL)
    {
        printf("Cannot pop, stack is empty");
        exit(EXIT_FAILURE);
    }
    else
    {
        s_node *ptr = *s;
        int number = ptr->number;
        *s = ptr->next;
        free(ptr);
        return number;
    }
}

int peek(s_node **s)
{
    if (*s == NULL)
    {
        printf("Cannot peek, stack is empty");
        exit(EXIT_FAILURE);
    }
    else
    {
        s_node *ptr = *s;
        int number = ptr->number;
        return number;
    }
}

void print_stack(s_node *s)
{
    for (s_node *node = s; node != NULL; node = node->next)
    {
        printf("%i\n", node->number);
    }
}

void freeMemory(s_node **q)
{
    s_node *ptr = *q;
    while (ptr != NULL)
    {
        s_node *next = ptr->next;
        free(ptr);
        ptr = next;
    }
}

void test_stack()
{
    s_node *stack = init();
    push(&stack, 10);
    push(&stack, 15);
    int deletedEl = pop(&stack);
    printf("deletedEl: %i\n", deletedEl);
    push(&stack, 20);
    int head = peek(&stack);
    printf("peek is: %i\n", head);
    print_stack(stack);
    freeMemory(&stack);

    printf("All tests passed!\n");
}