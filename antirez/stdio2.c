#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define HEXDUMP_CHARS_PER_LINE 16

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
}

int main(void) {
  int fd = open("stdio3.c", O_RDONLY);
  if (fd == -1) {
    perror("Unable to open file");
    return 1;
  }
  char buf[32];
  ssize_t nread;
  while (1) {
    nread = read(fd, buf, sizeof(buf));
    if (nread == -1) {
      perror("Reading file");
      return 0;
    };
    if (nread == 0)
      break;
    hexdump(buf, nread);
  }
  close(fd);
  return 0;
}
