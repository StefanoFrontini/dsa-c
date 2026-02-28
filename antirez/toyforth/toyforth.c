// 5 5 +
// 5 dup +
// "Hello World" strlen print
// [dup *] [dup +] [10 20 <] if

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ---------------------  Data Structures --------------------------
typedef enum { INT = 0, STR, BOOL, LIST, SYMBOL } TfobjType;

typedef struct {
  int refcount;
  int type;
  union {
    int i;
    struct {
      char *ptr;
      size_t len;
    } str;
    struct {
      struct TfObj **ele;
      size_t len;
    } list;
  };
} TfObj;

typedef struct {
  char *prg; // The program to parse into a list;
  char *p;   // Next token to parse;
} Tfparser;

typedef struct {
  TfObj *stack;
} Tfctx;

/* ------------------------ Allocation wrappers ----------------------------*/

void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    fprintf(stderr, "Out of memory allocation %zu bytes\n", size);
    exit(1);
  }
  return ptr;
}

// ----------------------- Object related functions --------------------------

// Allocate and initialize a new Toy Forth object.

TfObj *createObject(int type) {
  TfObj *o = xmalloc(sizeof(TfObj));
  o->type = type;
  o->refcount = 1;
  return o;
}

TfObj *createStringObject(char *s, size_t len) {
  TfObj *o = createObject(STR);
  o->str.ptr = s;
  o->str.len = len;
  return o;
}

TfObj *createSymbolObject(char *s, size_t len) {
  TfObj *o = createStringObject(s, len);
  o->type = SYMBOL;
  return o;
}

TfObj *createIntObject(int i) {
  TfObj *o = createObject(INT);
  o->i = i;
  return o;
}

TfObj *createBoolObject(int i) {
  TfObj *o = createObject(BOOL);
  o->i = i;
  return o;
}

TfObj *createListObject(void) {
  TfObj *o = createObject(LIST);
  o->list.ele = NULL;
  o->list.len = 0;
  return o;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return 1;
  }
  FILE *fp = fopen(argv[1], "r");
  if (fp == NULL) {
    // perror?
    fprintf(stderr, "Unable to open the file\n");
    return 1;
  }
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  printf("file size is: %lu\n", file_size);
  fseek(fp, 0, SEEK_SET);
  char *prgtext = xmalloc(file_size + 1);
  fread(prgtext, 1, sizeof(prgtext), fp);
  prgtext[file_size] = 0;
  printf("\"%s\"\n", prgtext);
  fclose(fp);

  // char buf[10];
  // size_t nread;
  // while (1) {
  //   nread = fread(buf, 1, sizeof(buf), fp);
  //   printf("%zu\n", nread);
  //   if (nread == 0) break;
  // }
  // printf("%c\n", buf[0]);
  //   printf("%s %d\n", argv[0], argc);
  return 0;
}