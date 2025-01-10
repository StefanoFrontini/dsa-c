
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

void test_binary_search_rec(void);

int main(void)
{
    test_binary_search_rec();
    return false;
}

bool bs_list_rec(int haystack[], int length, int needle)
{
    if (length == 0)
    {
        return false;
    }
    int high = length;
    int m = (high) / 2;
    int v = haystack[m];
    if (v == needle)
    {
        return true;
    }
    else if (v < needle)
    {
        return bs_list_rec(haystack + m + 1, high - m - 1, needle);
    }
    else
    {
        return bs_list_rec(haystack, m, needle);
    }
}

void test_binary_search_rec(void)
{
    int numbers[] = {5, 10, 20, 50, 100, 500};

    assert(bs_list_rec(numbers, 6, 50) == true);
    assert(bs_list_rec(numbers, 6, 100) == true);
    assert(bs_list_rec(numbers, 6, 999) == false);

    printf("All tests passed!\n");
}