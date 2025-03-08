#include <stdio.h>
#include <assert.h>

void test_selection_sort(void);

int main(void)
{
    test_selection_sort();
    return 0;
}

void selection_sort(int *arr, int length)
{
    for (int i = 0; i < length; i++)
    {
        int min = arr[i];
        int index = i;
        for (int j = i; j < length; j++)
        {
            if (arr[j] < min)
            {
                min = arr[j];
                index = j;
            }
        }
        // printf("min: %i\n", min);
        int tmp = arr[index];
        arr[index] = arr[i];
        arr[i] = tmp;
    }
}

void test_selection_sort(void)
{
    int arr[7] = {9, 3, 7, 4, 69, 420, 42};
    selection_sort(arr, 7);
    int sorted_arr[7] = {3, 4, 7, 9, 42, 69, 420};
    for (int i = 0; i < 7; i++)
    {
        // printf("%i\n", arr[i]);
        assert(arr[i] == sorted_arr[i]);
    }
}