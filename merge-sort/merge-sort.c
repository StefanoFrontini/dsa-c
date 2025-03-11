#include <stdio.h>
#include <assert.h>

void test_merge_sort(void);

int main(void)
{
    test_merge_sort();
    return 0;
}

void merge(int *left, int *right, int leftLength, int rightLength, int *arr)
{
    int i = 0;
    int l = 0;
    int r = 0;

    while (l < leftLength && r < rightLength)
    {
        if (left[l] < right[r])
        {
            arr[i] = left[l];
            i++;
            l++;
        }
        else
        {

            arr[i] = right[r];
            i++;
            r++;
        }
    }
    while (l < leftLength)
    {
        arr[i] = left[l];
        l++;
        i++;
    }
    while (r < rightLength)
    {
        arr[i] = right[r];
        r++;
        i++;
    }
}

void merge_sort(int *arr, int length)

{
    if (length <= 1)
    {
        return;
    }
    else
    {
        int middle = length / 2;
        int leftArray[middle];
        int rightArray[length - middle];

        int i = 0;
        int j = 0;

        while (i < length)
        {
            if (i < middle)
            {
                leftArray[i] = arr[i];
                i++;
            }
            else
            {
                rightArray[j] = arr[i];
                j++;
                i++;
            }
        }
        merge_sort(leftArray, middle);
        merge_sort(rightArray, length - middle);
        merge(leftArray, rightArray, middle, length - middle, arr);
    }
}

void test_merge_sort(void)
{
    int arr[7] = {9, 3, 7, 4, 69, 420, 42};
    merge_sort(arr, 7);
    int sorted_arr[7] = {3, 4, 7, 9, 42, 69, 420};
    for (int i = 0; i < 7; i++)
    {
        // printf("%i\n", arr[i]);
        assert(arr[i] == sorted_arr[i]);
    }
}