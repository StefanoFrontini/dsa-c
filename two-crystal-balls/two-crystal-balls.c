#include <math.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

void test_two_crystal_balls(void);

int main(void) {
    test_two_crystal_balls();
    return 0;
}

int two_crystal_balls(bool breaks[], int length) {
    int jmpAmount = (int)floor(sqrt((double)length));
    int i = 0;
    for (; i < length; i += jmpAmount) {
        if (breaks[i]) {
            break;
        }
    }
    if (i < 1) {
        return 0;
    }
    int bound = i;
    i -= jmpAmount;
    for (; i <= bound && i < length; i++) {
        if (breaks[i]) {
            return i;
        }
    }
    return -1;
}


void test_two_crystal_balls(void) {
    // Test 1: Nessuna rottura
    bool breaks1[] = {false, false, false, false, false};
    assert(two_crystal_balls(breaks1, 5) == -1);

    // Test 2: Rottura al primo elemento
    bool breaks2[] = {true, false, false, false, false};
    assert(two_crystal_balls(breaks2, 5) == 0);

    // Test 3: Rottura all'ultimo elemento
    bool breaks3[] = {false, false, false, false, true};
    assert(two_crystal_balls(breaks3, 5) == 4);

    // Test 4: Rottura a metà
    bool breaks4[] = {false, false, true, false, false};
    assert(two_crystal_balls(breaks4, 5) == 2);

    // Test 5: Rotture multiple
    bool breaks5[] = {false, true, true, false, false};
    assert(two_crystal_balls(breaks5, 5) == 1);

    printf("Tutti i test sono passati!\n");
}

