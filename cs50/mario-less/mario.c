#include <cs50.h>
#include <stdio.h>

void print_row(int n);

int main(void)
{
    int answer = get_int("Height? ");
    for (int i = 1; i <= answer; i++)
    {
        for (int j = 0; j < answer - i; j++)
        {
            printf(" ");
        }
        print_row(i);
    }
}

void print_row(int n)
{
    for (int i = 0; i < n; i++)
    {
        printf("#");
    }
    printf("  ");
    for (int i = 0; i < n; i++)
    {
        printf("#");
    }
    printf("\n");
}
