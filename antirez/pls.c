#include <stdint.h> // uint32_t uint16_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Initialize a prefixed length string with the specified string 'init' of
length 'len'
*
* The created strings have the following layout:
* +---+-----+------\\\\
* |LLLL|CCCC|My string here
* +---+-----+------\\\\
* Where L is one unsigned byte stating the total length of the string.
* Thus these strings are binary safe: zero bytes are permitted in the middle.
* Warning: this function does not check for buffer overflows.

*/
struct pls {
  uint32_t len;
  uint32_t refcount;
  uint32_t magic;
  char str[];
};

char *ps_create(char *init, int len) {
  struct pls *p = malloc(sizeof(struct pls) + len + 1); // sizeof(uint32_t) = 4
  p->len = len;
  p->refcount = 1;
  p->magic = 0xDEADBEEF;

  memcpy(p->str, init, len);

  // for (int j = 0; j < len; j++) {
  //   p->str[j] = init[j]; // We should use memcpy() here;
  // }
  p->str[len] = 0;
  return p->str;
}
/* Display the string 's' on the screen. */
void ps_print(char *s) {
  struct pls *p = (struct pls *)(s - sizeof(*p));
  // uint32_t *lenptr = (uint32_t *)(s - 4);
  for (int j = 0; j < (int)p->len; j++) {
    putchar(s[j]);
  }
  printf("\n");
}
/* Free a previously created PS string */
void ps_free(char *s) { free(s - sizeof(struct pls)); }

/* Validate a PS string */
void ps_validate(struct pls *p) {
  if (p->magic != 0xDEADBEEF) {
    printf("INVALID STRING: Aborting\n");
    exit(1);
  }
}

/* Drop the reference count of the string object by one and frees the object if
 * the refcount reached 0. */
void ps_release(char *s) {
  struct pls *p = (struct pls *)(s - sizeof(*p));

  ps_validate(p);

  p->refcount--;

  if (p->refcount == 0) {
    p->magic = 0;
    ps_free(s);
  }
}

/* Increase the reference count of the string object. */
void ps_retain(char *s) {
  struct pls *p = (struct pls *)(s - sizeof(*p));
  if (p->refcount == 0) {
    printf("ABORTED ON RETAIN OF ILLEGAL STRING");
    exit(1);
  }
  p->refcount++;
}
/* Return the length of the string in 0(1) time. */
uint32_t ps_len(char *s) {
  struct pls *p = (struct pls *)(s - sizeof(*p));
  return p->len;
}

char *global_string;

int main(void) {
  char *mystr = ps_create("Hello world", 11);
  global_string = mystr;
  ps_retain(mystr);
  ps_print(mystr);
  // printf("%s %d\n", mystr, (int)ps_len(mystr));
  ps_release(mystr);
  // printf("%d\n", (int)sizeof(struct pls));
  printf("%s\n", global_string);
  ps_release(mystr);
  // ps_release(mystr);
  return 0;
}