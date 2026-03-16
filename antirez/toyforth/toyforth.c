// 5 5 +
// 5 dup +
// "Hello World" strlen print
// [dup *] [dup +] [10 20 <] if

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ---------------------  Data Structures --------------------------
#define TF_OK 0
#define TF_ERR 1

typedef enum { INT = 0, STR, BOOL, LIST, SYMBOL, ALL } TfobjType;

typedef struct TfObj {
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

typedef struct Tfparser {
  char *prg; // The program to parse into a list;
  char *p;   // Next token to parse;
} Tfparser;

struct Tfctx;
/* Function table entry: each of this entry represents a symbol name associated
 * with a function implementation. */
typedef struct FunctionTableEntry {
  TfObj *name;
  int (*callback)(struct Tfctx *ctx, char *name);
  TfObj *user_func;
} Tffuncentry;

struct FunctionTable {
  Tffuncentry **func_table;
  size_t func_count;
};

/* Our execution context. */
typedef struct Tfctx {
  TfObj *stack;
  struct FunctionTable functable;
} Tfctx;

/* ----------------------   Function prototypes ---------------------------- */

void retain(TfObj *o);
void release(TfObj *o);

// Standard library prototypes
int basicMathFunctions(Tfctx *ctx, char *name);

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

// ----------------------- Object related functions --------------------------

// Allocate and initialize a new Toy Forth object.

TfObj *createObject(int type) {
  TfObj *o = xmalloc(sizeof(TfObj));
  o->type = type;
  o->refcount = 1;
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

/* Free an object and all the other nested objects. */
void freeObject(TfObj *o) {
  switch (o->type) {
    case LIST:
      for (size_t i = 0; i < o->list.len; i++) {
        TfObj *el = o->list.ele[i];
        release(el);
      }
      break;
    case SYMBOL:
    case STR:
      free(o->str.ptr);
      break;
  }
  free(o);
}

void retain(TfObj *o) {
  o->refcount++;
}

void release(TfObj *o) {
  assert(o->refcount > 0);
  o->refcount--;
  if (o->refcount == 0) freeObject(o);
}

void printObject(TfObj *o) {
  switch (o->type) {
    case INT:
      printf("%d", o->i);
      break;

    case LIST:
      printf("[");
      for (size_t i = 0; i < o->list.len; i++) {
        TfObj *el = o->list.ele[i];
        printObject(el);
        if (i != o->list.len - 1) {
          printf(" ");
        }
      }
      printf("]");
      break;
    case STR:
      printf("\"%s\"", o->str.ptr);
      break;
    case SYMBOL:
      printf("%s", o->str.ptr);
      break;

    default:
      printf("?");
      break;
  }
}
/* ------------------------------ String object --------------------- */

TfObj *createStringObject(char *s, size_t len) {
  TfObj *o = createObject(STR);
  o->str.ptr = xmalloc(len + 1);
  o->str.len = len;
  memcpy(o->str.ptr, s, len);
  o->str.ptr[len] = 0;
  return o;
}

TfObj *createSymbolObject(char *s, size_t len) {
  TfObj *o = createStringObject(s, len);
  o->type = SYMBOL;
  return o;
}
/* Compare the two string objects 'a' and 'b', return 0 if they are the same,
 * '1' if a>b, '-1' if a<b. The comparison is performed using memcmp(). */
int compareStringObject(TfObj *a, TfObj *b) {
  size_t minlen = a->str.len < b->str.len ? a->str.len : b->str.len;
  int cmp = memcmp(a->str.ptr, b->str.ptr, minlen);
  if (cmp == 0) {
    if (a->str.len == b->str.len)
      return 0;
    else if (a->str.len > b->str.len)
      return 1;
    else
      return -1;
  } else {
    if (cmp < 0)
      return -1;
    else
      return 1;
  }
}

/* ------------------------------ List object --------------------- */

TfObj *createListObject(void) {
  TfObj *o = createObject(LIST);
  o->list.ele = NULL;
  o->list.len = 0;
  return o;
}

/* add the new element at the end of the list. It is up to the caller to
 * increment the reference count to the list. */
void listPush(TfObj *l, TfObj *ele) {
  l->list.ele = xrealloc(l->list.ele, sizeof(TfObj *) * (l->list.len + 1));
  l->list.ele[l->list.len] = ele;
  l->list.len++;
}

TfObj *listPopType(Tfctx *ctx, int type){
  TfObj *stack = ctx->stack;
  if(ctx->stack->list.len == 0) return NULL;
  TfObj *to_pop = stack->list.ele[stack->list.len-1];
  if(type != ALL && to_pop->type != type) return NULL;

  stack->list.len--;
  if(stack->list.len == 0){
    free(stack->list.ele);
    stack->list.ele = NULL;

  } else {
    stack->list.ele = xrealloc(stack->list.ele, sizeof(TfObj*) * (stack->list.len));

  }

  return to_pop;
};
TfObj *listPop(Tfctx *ctx){
  return listPopType(ctx, ALL);
}

/* ----------- Turn program into toy forth list      */

TfObj *parseNumber(Tfparser *parser) {
  char buf[128];
  char *start = parser->p;
  char *end;
  if (*parser->p == '-') parser->p++;

  while (*parser->p && isdigit(*parser->p))
    parser->p++;

  end = parser->p;
  size_t len = end - start;
  memcpy(buf, start, len);
  buf[len] = 0;
  return createIntObject(atoi(buf));
}
/* Return true if the character 'c' is one of the characters acceptable for our
 * symbols. */
int isSymbolChar(int c) {
  char symchars[] = "+-*/%";
  return isalpha(c) || strchr(symchars, c) != NULL;
}

TfObj *parseSymbol(Tfparser *parser) {
  char *start = parser->p;
  while (*parser->p && isSymbolChar(*parser->p))
    parser->p++;
  int len = parser->p - start;
  return createSymbolObject(start, len);
}

void parseSpaces(Tfparser *parser) {
  while (isspace(*parser->p)) {
    parser->p++;
  }
}

TfObj *parse(char *prg) {
  Tfparser parser;
  parser.prg = prg;
  parser.p = prg;
  TfObj *o;

  TfObj *parsed = createListObject();

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
      perror("Error parsing\n");
      return NULL;
    } else {
      listPush(parsed, o);
    }
  }
  return parsed;
  // printf("after while: %d\n", *(parser.p));
}
/* --------------------- Execution and context ----------------------- */

int ctxCheckStackMinLen(Tfctx *ctx, size_t min){
  return (ctx->stack->list.len < min) ? TF_ERR : TF_OK;
};

/* Pop the top element from the interpreter main stack, assuming it will match 'type', otherwise NULL is returned. Also the function returns NULL if the stack is empty. The reference counting of the popped object is not modified: it is assumed that we just transfer the ownership from the stack to the caller. */
TfObj *ctxStackPop(Tfctx *ctx, int type){
  return listPopType(ctx, type);
};


/* Just push the object on the interpreter main stack. */
void ctxStackPush(Tfctx *ctx, TfObj *obj){
  listPush(ctx->stack, obj);
};

/* Resolve the function scanning the function table looking for a matching name. If a matching function was not found, NULL is returned, otherwise the function returns the function entry object. */
Tffuncentry *getFunctionByName(Tfctx *ctx, TfObj *name) {
  for (size_t i = 0; i < ctx->functable.func_count; i++) {
    Tffuncentry *fe = ctx->functable.func_table[i];
    if (compareStringObject(fe->name, name) == 0) return fe;
  }
  return NULL;
}

/* Push a new function entry in the context. It's up to the caller to set either
 * the C callback or the list representing the user defined function. */
Tffuncentry *registerFunction(Tfctx *ctx, TfObj *name) {
  ctx->functable.func_table =
      xrealloc(ctx->functable.func_table,
               sizeof(Tffuncentry *) * (ctx->functable.func_count + 1));
  Tffuncentry *fe = xmalloc(sizeof(Tffuncentry));
  ctx->functable.func_table[ctx->functable.func_count] = fe;
  ctx->functable.func_count++;
  fe->name = name;
  retain(name);
  fe->callback = NULL;
  fe->user_func = NULL;
  return fe;
}

/* Register a new function with the given name in the function table of the
 * context. The function can fail since if a function with the same name already
 * exist, it gets replaced by the new one. */
void registerCfunction(Tfctx *ctx, char *name,
                       int (*callback)(Tfctx *ctx, char *name)) {
  Tffuncentry *fe;
  TfObj *oname = createStringObject(name, strlen(name));
  fe = getFunctionByName(ctx, oname);
  if (fe) {
    if (fe->user_func) {
      release(fe->user_func);
      fe->user_func = NULL;
    }
    fe->callback = callback;

  } else {
    fe = registerFunction(ctx, oname);
    fe->callback = callback;
  }
  release(oname);
}

Tfctx *createContext(void) {
  Tfctx *ctx = xmalloc(sizeof(*ctx));
  ctx->stack = createListObject();
  ctx->functable.func_table = NULL;
  ctx->functable.func_count = 0;
  registerCfunction(ctx, "+", basicMathFunctions);
  return ctx;
}

/* Try to resolve and call the function associated with the symbol name 'word'.
 * Return 0 if the symbol was actually bound to some function and was executed,
 * return 1 otherwise (on error). */
int callSymbol(Tfctx *ctx, TfObj *word) {
  Tffuncentry *fe = getFunctionByName(ctx, word);
  if (fe == NULL) return TF_ERR;
  if(fe->user_func){
    // TODO
    return TF_ERR;
  } else {
    return fe->callback(ctx, fe->name->str.ptr);
  }
  // return TF_OK;
}

/* Execute the Toy Forth program stored into the list 'prg'. */
int exec(Tfctx *ctx, TfObj *prg) {
  assert(prg->type == LIST);
  for (size_t i = 0; i < prg->list.len; i++) {
    TfObj *word = prg->list.ele[i];
    switch (word->type) {
      case SYMBOL:
        if(callSymbol(ctx, word) == TF_ERR){
          printf("Run time error\n");
          return TF_ERR;
        }
        break;
      default:
      ctxStackPush(ctx, word);
        // listPush(ctx->stack, word);
        retain(word);
        break;
    }
  }
  return TF_OK;
}

/* ----------------------Basic standard library -----------------------*/

int basicMathFunctions(Tfctx *ctx, char *name) {
  if(ctxCheckStackMinLen(ctx, 2)) return TF_ERR;
  TfObj *b = ctxStackPop(ctx, INT);
  if(b == NULL) return TF_ERR;
  TfObj *a = ctxStackPop(ctx, INT);
  if(a == NULL){
    ctxStackPush(ctx, b);
    return TF_ERR;
  }

  int result;
  switch(name[0]){
    case '+': result = a->i + b->i; break;
    case '-': result = a->i - b->i; break;
    case '*': result = a->i * b->i; break;
  }
  release(a);
  release(b);
  ctxStackPush(ctx, createIntObject(result));
  return TF_OK;
}

/* -------------------------  Main ----------------------------- */

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
  // printf("file size is: %lu\n", file_size);
  fseek(fp, 0, SEEK_SET);
  char *prgtext = xmalloc(file_size + 1);
  fread(prgtext, file_size, 1, fp);
  prgtext[file_size] = 0;
  printf("\"%s\"\n", prgtext);
  fclose(fp);
  TfObj *parsed = parse(prgtext);
  printObject(parsed);
  printf("\n");

  Tfctx *ctx = createContext();
  exec(ctx, parsed);

  printf("Stack content at end: ");
  printObject(ctx->stack);
  printf("\n");

  return 0;
}