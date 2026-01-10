#include <stdio.h>
#include <stdlib.h>
/* Initialize a prefixed length string with the specified string 'init' of
length 'len'
*
* The created strings have the following layout:
* +-+------------\\\\
* |L|My string here
* +-+------------\\\\
* Where L is one unsigned byte stating the total length of the string.
* Thus these strings are binary safe: zero bytes are permitted in the middle.
* Warning: this function does not check for buffer overflows.

*/

char *ps_create(char *init, int len) {
  char *s = malloc(1 + len + 1);
  unsigned char *lenptr = (unsigned char *)s;
  *lenptr = len;
  for (int j = 0; j < len; j++) {
    s[j + 1] = init[j];
  }
  s[len + 1] = 0;
  return s;
}
/* Display the string 's' on the screen. */
void ps_print(char *s) {
  unsigned char *lenptr = (unsigned char *)s;
  for (int j = 0; j < *lenptr; j++) {
    putchar(s[j + 1]);
  }
  printf("\n");
}
/* Return the pointer to the null terminated C string embedded inside our PS
 * string 's'. */
char *ps_getc(char *s) { return s + 1; }

int main(void) {
  char *mystr = ps_create("Hello world", 11);
  ps_print(mystr);
  printf("%s\n", ps_getc(mystr));
  free(mystr);
  return 0;
}