#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int *array;
    int front;
    int size;
} QUEUE;

void test_queue_array();

int main(void)
{
    test_queue_array();
    return 0;
}

QUEUE init(int capacity)
{
    QUEUE q;
    q.front = 0;
    q.size = 0;
    q.array = malloc(capacity * sizeof(int));
    if (q.array == NULL)
    {
        exit(EXIT_FAILURE);
    }
    return q;
}

void enqueue(QUEUE *q, int item, int capacity)
{
    if (q->size == capacity)
    {
        printf("Cannot queue, capacity is full\n");
        exit(EXIT_FAILURE);
    }
    else
    {

        q->array[(q->front + q->size) % capacity] = item;
        q->size++;
    }
}
int dequeue(QUEUE *q)
{
    if (q->size == 0)
    {
        printf("Cannot dequeue, queue is empty\n");
        exit(EXIT_FAILURE);
    }
    else
    {

        int itemRemoved = q->array[q->front];

        q->front++;

        q->size--;

        return itemRemoved;
    }
}
int peek(QUEUE *q)
{
    return q->array[q->front];
}

void test_queue_array()
{
    int capacity = 10;
    QUEUE q = init(capacity);
    enqueue(&q, 28, capacity);
    enqueue(&q, 33, capacity);
    enqueue(&q, 19, capacity);
    dequeue(&q);
    enqueue(&q, 20, capacity);
    enqueue(&q, 49, capacity);
    enqueue(&q, 59, capacity);
    enqueue(&q, 39, capacity);
    enqueue(&q, 99, capacity);
    enqueue(&q, 89, capacity);
    enqueue(&q, 79, capacity);
    enqueue(&q, 69, capacity);
    dequeue(&q);
    int front = peek(&q);
    printf("front is: %i\n", front);

    printf("All tests passed!\n");
}