#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int capacity;
    int size;
    int *data;

} DINARRAY;

DINARRAY init(int capacity);
void printData(DINARRAY *arr);
void push(int item, DINARRAY *arr);
int size(DINARRAY *arr);
int capacity(DINARRAY *arr);
int at(DINARRAY *arr, int index);
void insert(DINARRAY *arr, int index, int item);
void checkBounds(DINARRAY *arr, int index);
void prepend(DINARRAY *arr, int item);
int pop(DINARRAY *arr);
void delete(DINARRAY *arr, int index);
void removeItem(DINARRAY *arr, int item);
int findItem(DINARRAY *arr, int item);
void resize(DINARRAY *arr);

void test_jarray();

int main(void)
{
    test_jarray();
}

void resize(DINARRAY *arr)
{
    if (arr->size == arr->capacity)
    {
        int *tmp = realloc(arr->data, 2 * arr->capacity * sizeof(int));
        if (tmp == NULL)
        {
            free(arr->data);
            exit(EXIT_FAILURE);
        }
        arr->capacity = 2 * arr->capacity;

        arr->data = tmp;
    }
    else if (arr->size < (1.0 / 4.0) * (float)arr->capacity && ((1.0 / 4.0) * (float)arr->capacity) >= 1.0)
    {
        int *tmp = realloc(arr->data, floor((1.0 / 4.0) * (float)arr->capacity) * sizeof(int));
        if (tmp == NULL)
        {
            free(arr->data);
            exit(EXIT_FAILURE);
        }
        arr->capacity = floor((1.0 / 4.0) * (float)arr->capacity);

        arr->data = tmp;
    }
    else
    {
        return;
    }
}

DINARRAY init(int capacity)
{
    DINARRAY newArr;
    newArr.capacity = capacity;
    newArr.size = 0;
    newArr.data = malloc(capacity * sizeof(int));
    if (newArr.data == NULL)
    {
        exit(EXIT_FAILURE);
    }

    return newArr;
}

void push(int item, DINARRAY *arr)
{
    resize(arr);
    arr->data[arr->size] = item;
    arr->size++;
}

void printData(DINARRAY *arr)
{
    if (arr->size == 0)
    {
        printf("the array is empty\n");
        return;
    }
    printf("The elements of the array are:\n");

    for (int i = 0; i < arr->size; i++)
    {
        printf("%i\n", arr->data[i]);
    }
}

int size(DINARRAY *arr)
{
    return arr->size;
}

int capacity(DINARRAY *arr)
{
    return arr->capacity;
}

int isEmpty(DINARRAY *arr)
{
    if (arr->size == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void checkBounds(DINARRAY *arr, int index)
{
    if (index >= arr->size || index < 0)
    {
        printf("index is greater than size or less than 0\n");
        exit(EXIT_FAILURE);
    }
}

int at(DINARRAY *arr, int index)
{
    checkBounds(arr, index);

    return arr->data[index];
}

void insert(DINARRAY *arr, int index, int item)
{

    checkBounds(arr, index);

    if (arr->size == arr->capacity)
    {
        arr->capacity++;
    }

    int *tmp = malloc(arr->capacity * sizeof(int));

    // printData(arr);

    if (tmp == NULL)
    {
        free(arr->data);
        exit(EXIT_FAILURE);
    }

    int i = 0;
    for (; i < index; i++)
    {
        tmp[i] = arr->data[i];
    }
    tmp[index] = item;

    for (; i < arr->size; i++)
    {
        tmp[i + 1] = arr->data[i];
    }
    free(arr->data);
    arr->data = tmp;
    arr->size++;
}

void prepend(DINARRAY *arr, int item)
{
    insert(arr, 0, item);
}

int pop(DINARRAY *arr)
{
    if (arr->size == 0)
    {
        printf("cannot pop, array is empty");
        exit(EXIT_FAILURE);
    }
    else
    {
        resize(arr);
        arr->size--;
        return arr->data[arr->size];
    }
}

void delete(DINARRAY *arr, int index)
{

    checkBounds(arr, index);
    int *tmp = malloc((arr->capacity - 1) * sizeof(int));

    // printData(arr);

    if (tmp == NULL)
    {
        free(arr->data);
        exit(EXIT_FAILURE);
    }
    int i = 0;
    for (; i < index; i++)
    {
        tmp[i] = arr->data[i];
    }
    i++;
    for (; i < arr->size; i++)
    {
        tmp[i - 1] = arr->data[i];
    }
    free(arr->data);
    arr->data = tmp;
    arr->size--;
}

void removeItem(DINARRAY *arr, int item)
{
    for (int i = 0; i < arr->size; i++)
    {
        if (arr->data[i] == item)
        {
            delete (arr, i);
            i--;
        }
    }
}

int findItem(DINARRAY *arr, int item)
{
    for (int i = 0; i < arr->size; i++)
    {
        if (arr->data[i] == item)
        {
            return i;
        }
    }
    return -1;
}

void test_jarray()
{
    DINARRAY dinArray = init(3);

    assert(isEmpty(&dinArray) == 0);

    push(1, &dinArray);

    push(2, &dinArray);

    push(3, &dinArray);

    push(4, &dinArray);

    assert(capacity(&dinArray) == 6);
    assert(size(&dinArray) == 4);
    assert(at(&dinArray, 1) == 2);

    insert(&dinArray, 2, 5);

    assert(capacity(&dinArray) == 6);
    assert(size(&dinArray) == 5);
    assert(dinArray.data[0] == 1);
    assert(dinArray.data[1] == 2);
    assert(dinArray.data[2] == 5);
    assert(dinArray.data[3] == 3);
    assert(dinArray.data[4] == 4);
    prepend(&dinArray, 10);
    assert(dinArray.data[0] == 10);
    assert(pop(&dinArray) == 4);
    assert(size(&dinArray) == 5);
    delete (&dinArray, 0);
    assert(dinArray.data[0] == 1);
    assert(size(&dinArray) == 4);
    push(2, &dinArray);
    assert(size(&dinArray) == 5);
    removeItem(&dinArray, 2);
    assert(findItem(&dinArray, 5) == 1);
    assert(findItem(&dinArray, 15) == -1);
    printData(&dinArray);
    pop(&dinArray);
    pop(&dinArray);
    printData(&dinArray);
    printf("capacity: %i\n", dinArray.capacity);
    printf("size: %i\n", dinArray.size);
    free(dinArray.data);

    printf("All tests passed!\n");
}
