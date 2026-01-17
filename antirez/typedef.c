#include <stdio.h>

typedef struct {
  int n;
  int d;
} fract;

typedef fract *fractptr;

int main(void) {
  fract f;
  fractptr fp = &f;
  f.n = 10;
  f.d = 20;
  printf("%d/%d my fraction is stored at %p\n", f.n, f.d, fp);
}