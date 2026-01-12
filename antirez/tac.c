#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct line {
  char *s;
  struct line *next;
};
void free_mem(struct line *l) {
  if (l == NULL)
    return;
  free_mem(l->next);
  free(l);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Missing file name\n");
    return 1;
  }
  FILE *fp = fopen(argv[1], "r");
  if (fp == NULL) {
    printf("File does not exist\n");
    return 1;
  }
  char buf[1024];
  struct line *head = NULL;
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    // printf("%s", buf);
    struct line *l = malloc(sizeof(*l));
    size_t linelen = strlen(buf);
    l->s = malloc(linelen + 1);
    for (size_t j = 0; j <= linelen; j++) {
      l->s[j] = buf[j];
    }
    l->next = head;
    head = l;
  }

  fclose(fp);
  while (head != NULL) {
    printf("%s", head->s);
    struct line *next = head->next;
    free(head);
    head = next;
  }
  //   free_mem(head);
  //   printf("%d", (int)sizeof(struct line));
  return 0;
}