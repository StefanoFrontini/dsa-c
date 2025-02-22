#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct STACK
{
    int number;
    struct STACK *next;
} STACK;

void test_stack();

STACK *createNode(int item);

void push(STACK **s, int item);

int main(void)
{
    test_stack();

    return 0;
}

STACK *init()
{
    STACK *s = NULL;
    return s;
}

STACK *createNode(int item)
{
    STACK *n = malloc(sizeof(STACK));
    if (n == NULL)
    {
        exit(EXIT_FAILURE);
    }
    n->number = item;
    n->next = NULL;
    return n;
}

void push(STACK **s, int item)
{
    STACK *n = createNode(item);
    n->next = *s;
    *s = n;
}

int pop(STACK **s)
{
    if (*s == NULL)
    {
        printf("Cannot pop, stack is empty");
        exit(EXIT_FAILURE);
    }
    else
    {
        STACK *ptr = *s;
        int number = ptr->number;
        *s = ptr->next;
        free(ptr);
        return number;
    }
}

int peek(STACK **s)
{
    if (*s == NULL)
    {
        printf("Cannot peek, stack is empty");
        exit(EXIT_FAILURE);
    }
    else
    {
        STACK *ptr = *s;
        int number = ptr->number;
        return number;
    }
}

void print_stack(STACK *s)
{
    for (STACK *node = s; node != NULL; node = node->next)
    {
        printf("%i\n", node->number);
    }
}

void test_stack()
{
    STACK *s = init();
    push(&s, 10);
    push(&s, 15);
    int deletedEl = pop(&s);
    printf("deletedEl: %i\n", deletedEl);
    push(&s, 20);
    int head = peek(&s);
    printf("peek is: %i\n", head);
    print_stack(s);

    printf("All tests passed!\n");
}