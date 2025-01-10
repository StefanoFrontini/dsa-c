#include <stdio.h>
#include <assert.h>

void test_binary_search(void);

int main(void)
{
    test_binary_search();
    return 0;
}

int bs_list(int haystack[], int length, int needle)
{
    int low = 0;
    int high = length;

    do
    {
        int m = low + (high - low) / 2;
        int v = haystack[m];
        if (v == needle)
        {
            return 0;
        }
        else if (v < needle)
        {
            low = m + 1;
        }
        else
        {
            high = m;
        }
    } while (low < high);
    return 1;
}

void test_binary_search(void)
{
    int numbers[] = {5, 10, 20, 50, 100, 500};

    assert(bs_list(numbers, 6, 50) == 0);
    assert(bs_list(numbers, 6, 100) == 0);
    assert(bs_list(numbers, 6, 999) == 1);

    printf("All tests passed!\n");
}