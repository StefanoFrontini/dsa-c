
#include <stdio.h>
#include <assert.h>

void test_bubble_sort(void);

int main(void)
{
    test_bubble_sort();
    return 0;
}

void bubble_sort(int *arr, int length)
{
    for (int i = 0; i < length - 1; i++)
    {
        for (int j = 0; j < length - 1 - i; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

void test_bubble_sort(void)
{
    int arr[7] = {9, 3, 7, 4, 69, 420, 42};
    bubble_sort(arr, 7);
    int sorted_arr[7] = {3, 4, 7, 9, 42, 69, 420};
    for (int i = 0; i < 7; i++)
    {
        assert(arr[i] == sorted_arr[i]);
    }
}