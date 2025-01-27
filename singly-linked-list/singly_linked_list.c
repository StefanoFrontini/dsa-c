#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct node
{
    int number;
    struct node *next;
} node;

node *init(void);
node *createNode(int number, node **list);
void prepend(node **list, int item);
void test_linked_list();
void printList(node *list);
void freeMemory(node **list);
void append(node **list, int item);
node *get(int idx, node **list);
void removeItem(int item, node **list);

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

node *createNode(int number, node **list)
{
    node *n = malloc(sizeof(node));
    if (n == NULL)
    {
        freeMemory(list);
        exit(EXIT_FAILURE);
    }
    n->number = number;
    n->next = NULL;
    return n;
}

void prepend(node **list, int item)
{
    node *n = createNode(item, list);
    n->next = *list;
    *list = n;
}

void append(node **list, int item)
{
    node *n = createNode(item, list);
    if (*list == NULL)
    {
        *list = n;
    }
    else
    {
        for (node *ptr = *list; ptr != NULL; ptr = ptr->next)
        {
            if (ptr->next == NULL)
            {
                ptr->next = n;
                break;
            }
        }
    }
}

void freeMemory(node **list)
{
    node *ptr = *list;
    while (ptr != NULL)
    {
        node *next = ptr->next;
        free(ptr);
        ptr = next;
    }
}

void insertAt(int item, int idx, node **list)
{
    if (idx == 0)
    {
        prepend(list, item);
        printf("Inserted!\n");
    }
    else
    {
        node *n = createNode(item, list);
        int i = 0;
        int inserted = 0;
        for (node *ptr = *list; ptr != NULL; ptr = ptr->next, i++)
        {
            if (i == idx - 1)
            {
                n->next = ptr->next;
                ptr->next = n;
                inserted++;
                break;
            }
        }
        if (inserted == 0)
        {
            printf("Not Inserted!\n");
        }
        else
        {
            printf("Inserted!\n");
        }
    }
}

void deleteAt(int idx, node **list)
{
    if (*list == NULL)
    {
        printf("List is empty, nothing to delete");
        return;
    }
    if (idx == 0)
    {
        printf("Deleting head...\n");
        node *ptr = *list;
        *list = ptr->next;
    }
    else
    {
        int i = 0;
        int deleted = 0;
        for (node *ptr = *list; ptr != NULL; ptr = ptr->next, i++)
        {
            if (i == idx - 1)
            {
                node *next = ptr->next->next;
                free(ptr->next);
                ptr->next = next;
                deleted++;
                break;
            }
        }
        if (deleted == 0)
        {
            printf("Not deleted!\n");
        }
        else
        {
            printf("deleted!\n");
        }
    }
}

node *get(int idx, node **list)
{
    if (*list == NULL)
    {
        printf("List is empty, returning null ptr\n");
        return *list;
    }
    if (idx == 0)
    {
        node *ptr = *list;
        *list = ptr->next;
        printf("Getting head...%i\n", ptr->number);
        return *list;
    }
    int i = 0;
    for (node *ptr = *list; ptr != NULL; ptr = ptr->next, i++)
    {
        if (i == idx)
        {
            printf("Returning node with value %i!\n", ptr->number);
            return ptr;
        }
    }
    printf("Nothing to get, returning list ptr!\n");
    return *list;
}

void removeItem(int item, node **list)
{
    if (*list == NULL)
    {
        printf("List is empty. Cannot remove\n");
        return;
    }
    else if ((**list).number == item && (**list).next != NULL)
    {
        printf("Head %i removed!\n", item);
        *list = (**list).next;
        return;
    }
    else if ((**list).number == item && (**list).next == NULL)
    {
        *list = NULL;
    }

    else
    {
        for (node *ptr = *list; ptr != NULL; ptr = ptr->next)
        {
            if (ptr->next != NULL && ptr->next->number == item)
            {
                ptr->next = ptr->next->next;
                printf("Item %i removed!\n", item);
                return;
            }
        }
        printf("Item not found\n");
    }
}

void test_linked_list()
{
    // list is a variable storing the address of the first node
    node *list = init();
    prepend(&list, 1);
    prepend(&list, 2);
    prepend(&list, 3);
    append(&list, 4);
    insertAt(11, 1, &list);
    deleteAt(0, &list);
    get(1, &list);
    printList(list);
    removeItem(11, &list);
    printList(list);

    freeMemory(&list);
}