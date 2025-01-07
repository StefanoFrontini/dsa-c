#include <stdio.h>

int linear_search(int haystack[], int needle);

int main(void) {
  int numbers[] = {20, 500, 10, 5, 100, 50};
  int value = 50;
  int result = linear_search(numbers, value);
  return result;

}

int linear_search(int haystack[], int needle) {
  for (int i = 0; i < 7; i++) {
    if (haystack[i] == needle) {
      printf("Found!\n");
      return 0;
    };

  }
  printf("Not found!\n");
  return 1;
}