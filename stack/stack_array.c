
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int *array;
    int top;
} s_node;

void test_stack_array();

void push(stack *s, int item);

int main(void)
{
    test_stack_array();
}

stack init(int capacity)
{
    stack s;
    s.top = 0;
    s.array = malloc(capacity * sizeof(int));
    if (s.array == NULL)
    {
        exit(EXIT_FAILURE);
    }
    return s;
}

void push(stack *s, int item)
{
    s->array[s->top] = item;
    s->top++;
}

int pop(stack *s)
{
    int itemToRemove = s->array[s->top - 1];
    s->top--;
    return itemToRemove;
}

void test_stack_array()
{
    stack s = init(10);
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