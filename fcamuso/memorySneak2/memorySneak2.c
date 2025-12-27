#include <stdio.h>
#include <stdlib.h>

int main()
{
    int i = 0x12345678;
    char *p = (char *)&i;
    int k;
    int value;

    printf("Int value is: %d (0x%X)\n", i, i);
    for (k = 0; k <= 3; k++)
    {
        value = *p;
        if (value < 0)
            value += 256;                                   // COMPLEMENTO A 2 PER OTTENERE IL VALORE UNSIGNED
        printf("Value to address %s is: %.2X\n", p, value); // %.2X --> STAMPA ALMENO 2 CARATTERI ESA
        p++;
    }
    return 0;
}