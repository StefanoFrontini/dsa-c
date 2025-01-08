#include <stdio.h>
#include <assert.h>

int linear_search(int haystack[], int needle);
void test_linear_search(void);

int main() {
    test_linear_search();
    return 0;
}

int linear_search(int haystack[], int needle) {
    for (int i = 0; i < 7; i++) {
        if (haystack[i] == needle) {
            printf("Found!\n");
            return 0;
        }
    }
    printf("Not found!\n");
    return 1;
}

void test_linear_search(void) {
    int numbers[] = {20, 500, 10, 5, 100, 50};

    // Test cases
    assert(linear_search(numbers, 50) == 0);
    assert(linear_search(numbers, 100) == 0);
    assert(linear_search(numbers, 999) == 1);

    printf("All tests passed!\n");
}

