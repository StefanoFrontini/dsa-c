#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct pls {
  long len;
  char str[24];
};

#define HEXDUMP_CHARS_PER_LINE 8

void hexdump(void *p, size_t len) {
  unsigned char *byte = p;
  size_t po = 0;
  for (size_t j = 0; j < len; j++) {
    printf("%02X ", byte[j]);
    if ((j + 1) % HEXDUMP_CHARS_PER_LINE == 0 || j == len - 1) {
      if (j == len - 1) {
        int pad =
            (HEXDUMP_CHARS_PER_LINE - (int)(len % HEXDUMP_CHARS_PER_LINE));
        pad %= HEXDUMP_CHARS_PER_LINE;
        for (int k = 0; k < pad; k++) {
          printf("~~ ");
        }
      }
      printf("\t");
      for (size_t i = po; i <= j; i++) {
        printf("%c", isprint(byte[i]) ? byte[i] : '.');
      }
      po = j + 1;
      printf("\n");
    }
  }
  printf("\n");
}

int main(void) {
  struct pls s;

  s.len = 10;
  // s.str = malloc(10 + 1);
  memcpy(s.str, "1234567890", 11);
  // printf("%p\n", s.str);
  printf("%d\n", (int)sizeof(s));
  hexdump(&s, sizeof(s));
  //   printf("%p\n", "dadadadada");
};