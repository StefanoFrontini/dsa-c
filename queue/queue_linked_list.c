
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct q_node
{
    int number;
    struct q_node *next;
    struct q_node *prev;
} q_node;

void test_queue();

q_node *createNode(int item);

void enqueue(q_node **q, int item);

void print_queue(q_node **q);

void freeMemory(q_node **q);

q_node *tail = NULL;

int main(void)
{
    test_queue();

    return 0;
}

q_node *init()
{
    q_node *q = NULL;
    tail = NULL;
    return q;
}

q_node *createNode(int item)
{
    q_node *n = malloc(sizeof(q_node));
    if (n == NULL)
    {
        exit(EXIT_FAILURE);
    }
    n->number = item;
    n->next = NULL;
    n->prev = NULL;
    return n;
}

void enqueue(q_node **q, int item)
{
    q_node *n = createNode(item);
    if (tail == NULL)
    {
        tail = n;
        *q = n;
    }
    else
    {
        n->prev = tail;
        tail->next = n;
        tail = n;
    }
}

void dequeue(q_node **q)
{

    if (*q == NULL)
    {
        // tail = NULL;
        printf("Cannot dequeue, queue is empty");
        exit(EXIT_FAILURE);
    }
    else if ((*q)->next == NULL)
    {
        *q = tail = NULL;
    }
    else
    {
        q_node *head = *q;
        q_node *new_head = (*q)->next;
        free(head);
        *q = new_head;
        new_head->prev = NULL;
    }
}

int peek(q_node **q)
{
    if (*q == NULL)
    {
        printf("Cannot peek, queue is empty\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        return (*q)->number;
    }
}

void print_queue(q_node **q)
{
    for (q_node *node = *q; node != NULL; node = node->next)
    {
        printf("%i\n", node->number);
    }
}

void freeMemory(q_node **q)
{
    q_node *ptr = *q;
    while (ptr != NULL)
    {
        q_node *next = ptr->next;
        free(ptr);
        ptr = next;
    }
}

void test_queue()
{
    q_node *queue = init();
    enqueue(&queue, 10);
    enqueue(&queue, 20);
    enqueue(&queue, 30);
    dequeue(&queue);
    dequeue(&queue);
    dequeue(&queue);
    enqueue(&queue, 40);
    int head_number = peek(&queue);
    printf("head number is: %i\n", head_number);
    print_queue(&queue);
    freeMemory(&queue);
}