#include <stdio.h>
#include <stdlib.h>

int compare_integers(const void *a, const void *b) {
  const int *ap = a;
  const int *bp = b;
  if (*ap < *bp)
    return -1;
  else if (*ap > *bp)
    return 1;
  else
    return 0;
};

int main(void) {
  int a[10];
  for (int j = 0; j < 10; j++) {
    a[j] = rand() & 15;
  }

  qsort(a, 10, sizeof(int), compare_integers);

  for (int j = 0; j < 10; j++) {
    printf("%d ", a[j]);
  }

  printf("\n");
  return 0;
}