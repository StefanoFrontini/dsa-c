#include <stdio.h>

int main(void)
{
    int i = 0;
again:
    printf("%d\n", i);
    i++;
    if (i < 10)
        goto again;
    return 0;
}