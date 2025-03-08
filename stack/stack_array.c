
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int *array;
    int top;
} s_node;

void test_s_node_array();

void push(s_node *s, int item);

int main(void)
{
    test_s_node_array();
}

s_node init(int capacity)
{
    s_node s;
    s.top = 0;
    s.array = malloc(capacity * sizeof(int));
    if (s.array == NULL)
    {
        exit(EXIT_FAILURE);
    }
    return s;
}

void push(s_node *s, int item)
{
    s->array[s->top] = item;
    s->top++;
}

int pop(s_node *s)
{
    int itemToRemove = s->array[s->top - 1];
    s->top--;
    return itemToRemove;
}

void test_s_node_array()
{
    s_node s = init(10);
    assert(s.top == 0);
    push(&s, 15);
    assert(s.top == 1);
    assert(s.array[s.top - 1] == 15);
    push(&s, 22);
    int itemRemoved = pop(&s);
    assert(itemRemoved == 22);

    assert(s.array[s.top - 1] == 15);

    free(s.array);

    printf("All tests passed!\n");
}