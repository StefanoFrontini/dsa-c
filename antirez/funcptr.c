#include <stdio.h>

void hello(void) { printf("Hello!\n"); }
void baubau(void) { printf("Bau Bau!\n"); }

void call_n_times(int n, void (*x)(void)) {
  while (n--) {
    x();
  }
}

int main(void) {
  //   void (*x)(void);
  call_n_times(10, hello);
  call_n_times(10, baubau);

  //   x = hello;
  //   x();
  //   x = baubau;
  //   x();

  return 0;
}