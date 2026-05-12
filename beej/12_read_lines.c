#include <stdio.h>
#include <stdlib.h>

/*
Read a  line of arbitrary size from a file.

Returns a pointer to the line.
Returrn NULL on EOF or error.

It's up to the caller to free() this pointer when done with it.

*/

char *readLine(FILE *fp) {
  char *buf;
  int offset = 0;
  int bufsize = 4;
  int c;

  buf = malloc(bufsize);

  if (buf == NULL) {
    return NULL;
  }
  while (c = fgetc(fp), c != '\n' && c != EOF) {
    if (offset == bufsize - 1) {
      bufsize *= 2;

      char *new_ptr = realloc(buf, bufsize);
      if (new_ptr == NULL) {
        free(buf);
        return NULL;
      }
      buf = new_ptr;
    }
    buf[offset++] = c;
  }
  if (c == EOF && offset == 0) {
    free(buf);
    return NULL;
  }

  if (offset < bufsize - 1) {
    bufsize = offset + 1;
    char *new_ptr = realloc(buf, bufsize);
    if (new_ptr != NULL) {
      buf = new_ptr;
    }
  }

  buf[offset] = 0;

  return buf;
}

int main(void) {
  FILE *fp = fopen("foo.txt", "r");
  char *line;
  while (line = readLine(fp), line != NULL) {
    printf("line: %s\n", line);
    free(line);
  }
  fclose(fp);

  return 0;
}