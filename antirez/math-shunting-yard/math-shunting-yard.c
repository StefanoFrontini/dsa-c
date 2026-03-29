#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LIST_CAPACITY 1
#define SY_OK 0
#define SY_ERR 1

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
      struct SyObj **ele;
      size_t len;
      size_t capacity;
    } list;
  };
} SyObj;

/* -------------  Functions prototypes ------------------------- */
void retain(SyObj *o);
void release(SyObj *o);

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

/* Free an object and all the other nested objects. */
void freeObject(SyObj *o) {
  switch (o->type) {
    case LIST:
      for (size_t i = 0; i < o->list.len; i++) {
        SyObj *el = o->list.ele[i];
        release(el);
      }
      free(o->list.ele);
      break;
    default:
      break;
  }
  free(o);
}

void retain(SyObj *o) {
  o->refcount++;
}

void release(SyObj *o) {
  assert(o->refcount > 0);
  o->refcount--;
  if (o->refcount == 0) freeObject(o);
}

void printSyObj(SyObj *o) {
  switch (o->type) {
    case NUMBER:
      printf("%d ", o->num);
      break;
    case SYMBOL:
      printf("%c ", o->symbol);
      break;
    case LIST:
      for (size_t i = 0; i < o->list.len; i++) {
        printSyObj(o->list.ele[i]);
      }
      break;

    default:
      printf("?\n");
      break;
  }
}

/* Add the new element at the end of the list. It is up to the caller to
 * increment the reference count to the list. */
void listPush(SyObj *l, SyObj *o) {
  if (l->list.len >= l->list.capacity) {
    size_t newCap =
        l->list.capacity <= 0 ? l->list.capacity = 1 : l->list.capacity * 2;
    l->list.ele = xrealloc(l->list.ele, newCap * sizeof(SyObj *));
    l->list.capacity = newCap;
  }
  l->list.ele[l->list.len] = o;
  l->list.len++;
}

SyObj *listPop(SyObj *l) {
  if (l->list.len > 0) {
    SyObj *o = l->list.ele[l->list.len - 1];
    l->list.len--;
    return o;
  } else {
    return NULL;
  }
}

typedef struct SyParser {
  char *prg;
  char *p;
} SyParser;

typedef struct precedenceTableEntry {
  char s;
  int i;
} pTableEntry;

typedef struct precedenceTable {
  pTableEntry precedenceTableEntry[3];
} pTable;

typedef struct SyCtx {
  SyObj *stack;
  SyObj *queue;
  pTable precedenceTable;
} SyCtx;

/* ----------- Turn program into shunting yard list ---------- */

SyObj *parseNumber(SyParser *parser) {
  char buf[128];
  char *start = parser->p;
  char *end;
  if (*parser->p == '-') parser->p++;

  while (*parser->p && isdigit(*parser->p))
    parser->p++;

  end = parser->p;
  size_t len = end - start;

  if (len >= sizeof(buf)) {
    fprintf(stderr, "Number literal too long\n");
    return NULL;
  }
  memcpy(buf, start, len);
  buf[len] = 0;
  return createNumberObject(atoi(buf));
}
/* Return true if the character 'c' is one of the characters acceptable for our
 * symbols. */
int isSymbolChar(int c) {
  char symchars[] = "+-*/%";
  return isalpha(c) || strchr(symchars, c) != NULL;
}

SyObj *parseSymbol(SyParser *parser) {
  char s = *parser->p;
  parser->p++;
  return createSymbolObject(s);
}

void parseSpaces(SyParser *parser) {
  while (isspace(*parser->p)) {
    parser->p++;
  }
}

SyObj *parse(char *prg) {
  SyParser parser;
  parser.prg = prg;
  parser.p = prg;
  SyObj *parsed = createListObject();
  SyObj *o;
  while (*parser.p) {
    parseSpaces(&parser);
    if (*parser.p == 0) break;
    if (isdigit(*parser.p) || (*parser.p == '-' && isdigit(parser.p[1]))) {
      o = parseNumber(&parser);
    } else if (isSymbolChar(*parser.p)) {
      o = parseSymbol(&parser);
    } else {
      o = NULL;
    }

    // Check if the current token produced a parsing error.
    if (o == NULL) {
      release(parsed);
      fprintf(stderr, "Error parsing\n");
      return NULL;
    } else {
      listPush(parsed, o);
    }
  }
  return parsed;
}

int getPrecedence(SyCtx *ctx, SyObj *o){
  char s = o->symbol;
  for(int i = 0; i < 3; i++){
    if(ctx->precedenceTable.precedenceTableEntry[i].s == s){
      return ctx->precedenceTable.precedenceTableEntry[i].i;
    }
  }
  printf("Error: %c\n", s);
  return SY_ERR;
}

void ctxEnqueue(SyCtx *ctx, SyObj *obj);

/* Push the object on the context stack. */
void ctxStackPush(SyCtx *ctx, SyObj *o) {
  if (ctx->stack->list.len == 0) {
    listPush(ctx->stack, o);
    return;
  }
  SyObj *peek = ctx->stack->list.ele[ctx->stack->list.len - 1];
  int precPeek = getPrecedence(ctx, peek);
  int precCurrent = getPrecedence(ctx, o);
  if(precPeek > precCurrent){
    SyObj *popped = listPop(ctx->stack);
    ctxEnqueue(ctx, popped);
    listPush(ctx->stack, o);
  } else {
    listPush(ctx->stack, o);
  }
};

/* Enqueue the object on the context queue. */
void ctxEnqueue(SyCtx *ctx, SyObj *obj) {
  listPush(ctx->queue, obj);
};

void eval(SyCtx *ctx, SyObj *o) {
  switch (o->type) {
    case NUMBER:
      ctxEnqueue(ctx, o);
      break;
    case SYMBOL:
      ctxStackPush(ctx, o);
      break;
    case LIST:
      for (size_t i = 0; i < o->list.len; i++) {
        eval(ctx, o->list.ele[i]);
      }
      break;

    default:
      printf("?\n");
      break;
  }
}
SyCtx *createContext(void) {
  SyCtx *ctx = xmalloc(sizeof(SyCtx));
  ctx->stack = createListObject();
  ctx->queue = createListObject();
  ctx->precedenceTable.precedenceTableEntry[0].s = '+';
  ctx->precedenceTable.precedenceTableEntry[0].i = 0;
  ctx->precedenceTable.precedenceTableEntry[1].s = '*';
  ctx->precedenceTable.precedenceTableEntry[1].i = 1;
  ctx->precedenceTable.precedenceTableEntry[2].s = '-';
  ctx->precedenceTable.precedenceTableEntry[2].i = 0;
  return ctx;
}

/* -------------------------  Main ----------------------------- */

int main(int argc, char **argv) {
  if (argc != 2) {
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
  SyObj *parsed = parse(buf);
  printf("\nparsed: \n");
  printSyObj(parsed);
  SyCtx *ctx = createContext();
  eval(ctx, parsed);
  while(ctx->stack->list.len > 0){
    SyObj *popped = listPop(ctx->stack);
    ctxEnqueue(ctx, popped);
  }
  printf("queue: \n");
  printSyObj(ctx->queue);
  release(parsed);
  free(buf);
  return 0;
}
