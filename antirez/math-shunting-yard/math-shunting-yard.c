#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LIST_CAPACITY 1

/* ------------------------ Allocation wrappers ----------------------------*/

void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    fprintf(stderr, "Out of memory allocation %zu bytes\n", size);
    exit(1);
  }
  return ptr;
}

void *xrealloc(void *old_ptr, size_t size) {
  void *ptr = realloc(old_ptr, size);
  if (ptr == NULL) {
    fprintf(stderr, "Out of memory allocation %zu bytes\n", size);
    exit(1);
  }
  return ptr;
}


/* ------------------    Data structure Shunting-yard object ---------------- */

typedef enum { NUMBER = 0, SYMBOL, LIST } SyObjType;

typedef struct SyObj {
  int refcount;
  SyObjType type;
  union {
    char symbol;
    int num;
    struct {
      SyObj **ele;
      size_t len;
      size_t capacity;
    } list;
  };
} SyObj;


/*---------------     Object related functions ----------------  */

/* Allocate and initialize e new Shunting Yard Object */

SyObj *createObject(SyObjType type) {
  SyObj *o = xmalloc(sizeof(SyObj));
  o->refcount = 1;
  o->type = type;
  return o;
}

SyObj *createNumberObject(int n) {
  SyObj *o = createObject(NUMBER);
  o->refcount = 1;
  o->num = n;
  return o;
}
SyObj *createSymbolObject(char s) {
  SyObj *o = createObject(SYMBOL);
  o->refcount = 1;
  o->symbol = s;
  return o;
}

SyObj *createListObject(void) {
  SyObj *o = createObject(LIST);
  o->list.ele = xmalloc(LIST_CAPACITY * sizeof(SyObj *));
  o->list.capacity = LIST_CAPACITY;
  o->list.len = 0;
  return o;
}

void printSyObj(SyObj *o) {
  switch (o->type) {
    case NUMBER:
      printf("%d ",o->num);
      break;
    case SYMBOL:
      printf("%c ",o->symbol);
      break;
    case LIST:
      for (size_t i = 0; i < o->list.len; i++){
        printSyObj(o->list.ele[i]);
      }
      break;

    default:
      break;


  }
}

void listPush(SyObj *l, SyObj *o){
    if(l->list.len >= l->list.capacity - 1){
        l->list.ele = xrealloc(l->list.ele, l->list.capacity * 2);
        l->list.capacity = l->list.capacity * 2;
    }
    l->list.ele[l->list.len] = o;
    l->list.len++;

}

SyObj *listPop(SyObj *l){
    if(l->list.len > 0){
        SyObj *o = l->list.ele[l->list.len - 1];
        l->list.len--;
        return o;
    }

}

typedef struct SyParser {
  char *prg;
  char *p;
} SyParser;

typedef struct SyCtx {
  SyObj *stack;
  SyObj *queue;

} SyCtx;

/* -------------------------  Main ----------------------------- */

int main(int argc, char **argv){
    if(argc !=2){
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");

  if (fp == NULL) {
    fprintf(stderr, "Unable to open the file\n");
    return 1;
  }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = xmalloc(file_size + 1);
    fread(buf, file_size, 1, fp);
    buf[file_size] = 0;
    printf("text is: %s\n", buf);
    fclose(fp);
    SyParser p;
    p.prg = buf;
    p.p = buf;
    // readEndOfFile(&l);
    // advanceLexer(&l);
    // CsObj *parsed = parseSong(&l);
    // printCsObj(parsed);
    // releaseCsObj(parsed);
    free(buf);
    return 0;
}
