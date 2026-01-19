#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>

int main(void) {
  int fd = open("stdio3.c", O_RDONLY);
  printf("Open file descriptor: %d\n", fd);
  void *mem = mmap(NULL, 100, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
  printf("File mapped at %p\n", mem);
  char *s = mem;
  for (int j = 0; j < 10; j++) {
    printf("s[%d] = %c\n", j, s[j]);
  }
  return 0;
  // char *mystr = "Hello World";
  // size_t len = strlen(mystr);
  // for (size_t j = 0; j < len; j++) {
  //   // write(STDOUT_FILENO, mystr+j, 1);
  //   putchar(mystr[j]);
  //   // putchar("\n");
  // }
  // fflush(stdout); // file descriptor non di UNIX ma quello ritornato dalla
  // fopen
  //                 // della libc (handle)
  // sleep(5);

  return 0;
}