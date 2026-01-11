#include <stdint.h> // uint32_t uint16_t
#include <stdio.h>
#include <stdlib.h>
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

char *ps_create(char *init, int len) {
  char *s = malloc(4 + len + 1); // sizeof(uint32_t) = 4
  uint32_t *lenptr = (uint32_t *)s;
  *lenptr = len;
  s += 4;
  for (int j = 0; j < len; j++) {
    s[j] = init[j]; // We should use memcpy() here;
  }
  s[len] = 0;
  return s;
}
/* Display the string 's' on the screen. */
void ps_print(char *s) {
  uint32_t *lenptr = (uint32_t *)(s - 4);
  for (int j = 0; j < (int)*lenptr; j++) {
    putchar(s[j]);
  }
  printf("\n");
}
/* Return the length of the string in 0(1) time. */
uint32_t ps_len(char *s) {
  uint32_t *lenptr = (uint32_t *)(s - 4);
  return *lenptr;
}
/* Free a previously created PS string. */
void ps_free(char *s) { free(s - 4); }

int main(void) {
  char *mystr = ps_create("Hello world", 11);
  ps_print(mystr);
  printf("%s %d\n", mystr, (int)ps_len(mystr));
  ps_free(mystr);
  return 0;
}