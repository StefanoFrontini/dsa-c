
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

/* ----------------   Data structures: Lexer ------------------ */

typedef enum {
  TOKEN_NUMBER = 0,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_DIV,
  TOKEN_MULT,
  TOKEN_SYMBOL,
  TOKEN_LIST,
  TOKEN_ILLEGAL,
  TOKEN_ENDOFFILE
} TokenType;

typedef struct Token {
  int refcount;
  TokenType type;
  union {
    char symbol;
    int i;
    struct {
      struct Token **ele;
      size_t len;
    } list;
  };
} Token;

typedef struct Lexer {
  char *prg;
  char *p;
  Token *curToken;
} Lexer;

struct pCtx;
struct PObj;
typedef struct PObj *(*PrefixFn)(struct pCtx *ctx);
typedef struct PObj *(*InfixFn)(struct pCtx *ctx, struct PObj *left);

typedef struct parsingRulesTableEntry {
  PrefixFn prefixFn;
  InfixFn infixFn;
  char type;
  int i;
} pTableEntry;

typedef struct parsingRulesTable {
  pTableEntry **entry;
  int count;
} pTable;

typedef struct pCtx {
  pTable table;
  Lexer lexer;
} pCtx;

/* ------------------  Token related functions ----------------  */

void retainToken(Token *t) {
  t->refcount++;
}

void releaseToken(Token *t) {
  assert(t->refcount > 0);
  t->refcount--;
  if (t->refcount == 0) {
    switch (t->type) {
      case TOKEN_LIST:
        for (size_t i = 0; i < t->list.len; i++) {
          releaseToken(t->list.ele[i]);
        }
        free(t->list.ele);
        break;
      default:
        break;
    }
    free(t);
  }
}

void readNumber(pCtx *ctx) {
  char buf[128];
  char *start = ctx->lexer.p;
  char *end;

  while (ctx->lexer.p[0] && isdigit(ctx->lexer.p[0]))
    ctx->lexer.p++;

  end = ctx->lexer.p;
  size_t len = end - start;

  if (len >= sizeof(buf)) {
    fprintf(stderr, "Number literal too long\n");
    return;
  }

  memcpy(buf, start, len);
  buf[len] = 0;

  Token *t = xmalloc(sizeof(Token));
  t->refcount = 1;
  t->type = TOKEN_NUMBER;
  t->i = atoi(buf);
  ctx->lexer.curToken = t;
}

/* Return true if the character 'c' is one of the characters acceptable for our
 * symbols. */
int isSymbolChar(int c) {
  char symchars[] = "+-*/";
  return isalpha(c) || strchr(symchars, c) != NULL;
}

void readSymbol(pCtx *ctx) {
  char s = ctx->lexer.p[0];
  ctx->lexer.p++;
  Token *t = xmalloc(sizeof(Token));
  t->refcount = 1;
  t->type = TOKEN_ILLEGAL;
  switch (s) {
    case '+':
      t->type = TOKEN_PLUS;
      break;
    case '-':
      t->type = TOKEN_MINUS;
      break;
    case '*':
      t->type = TOKEN_MULT;
      break;
    case '/':
      t->type = TOKEN_DIV;
      break;
    default:
      printf("???\n");
      break;
  }
  t->symbol = s;
  ctx->lexer.curToken = t;
}

void readIllegalChar(pCtx *ctx, char s) {
  Token *t = xmalloc(sizeof(Token));
  t->refcount = 1;
  t->type = TOKEN_ILLEGAL;
  t->symbol = s;
  ctx->lexer.curToken = t;
  ctx->lexer.p++;
}

void readEndOfFile(pCtx *ctx) {
  Token *t = xmalloc(sizeof(Token));
  t->refcount = 1;
  t->type = TOKEN_ENDOFFILE;
  t->symbol = 0;
  ctx->lexer.curToken = t;

  // l->curToken = t;
}

// Token *createTokenList(void) {
//   Token *t = xmalloc(sizeof(Token));
//   t->type = TOKEN_LIST;
//   t->refcount = 1;
//   t->list.ele = NULL;
//   t->list.len = 0;
//   return t;
// }

void parseSpaces(pCtx *ctx) {
  while (isspace(ctx->lexer.p[0])) {
    ctx->lexer.p++;
  }
}

void advanceLexer(pCtx *ctx) {
  releaseToken(ctx->lexer.curToken);
  char c = ctx->lexer.p[0];
  if (isdigit(c)) {
    readNumber(ctx);
  } else if (isSymbolChar(c)) {
    readSymbol(ctx);
  } else if (c == 0) {
    readEndOfFile(ctx);
  } else {
    readIllegalChar(ctx, c);
  }
}

void printToken(Token *t) {
  switch (t->type) {
    case TOKEN_NUMBER:
      printf("Current token is: %d\n", t->i);
      break;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_MULT:
    case TOKEN_DIV:
    case TOKEN_ILLEGAL:
      printf("Current token is %c\n", t->symbol);
    case TOKEN_LIST:
      for (size_t i = 0; i < t->list.len; i++) {
        Token *el = t->list.ele[i];
        printToken(el);
      }
      break;
    default:
      printf("? - %d\n", t->type);
      break;
  }
}

// void addSymbolPrecedence(pCtx *ctx, char s, int i) {
//   ctx->table.entry =
//       xrealloc(ctx->table.entry,
//                sizeof(pTableEntry *) * (ctx->table.count + 1));
//   pTableEntry *entry = xmalloc(sizeof(pTableEntry));
//   entry->s = s;
//   entry->i = i;

//   ctx->table.entry[ctx->table.count] = entry;
//   ctx->table.count++;
// }

void addRule(pCtx *ctx, char type, int i, PrefixFn prefix, InfixFn infix) {
  ctx->table.entry = xrealloc(ctx->table.entry,
                              sizeof(pTableEntry *) * (ctx->table.count + 1));
  pTableEntry *new_entry = xmalloc(sizeof(pTableEntry));
  new_entry->i = i;
  new_entry->type = type;
  new_entry->infixFn = infix;
  new_entry->prefixFn = prefix;
  ctx->table.entry[ctx->table.count] = new_entry;
  ctx->table.count++;
}

int getPrecedence(pCtx *ctx, Token *t) {
  char type = t->type;
  for (int i = 0; i < ctx->table.count; i++) {
    if (ctx->table.entry[i]->type == type) {
      return ctx->table.entry[i]->i;
    }
  }
  printf("Error: %c\n", type);
  return -1;
}

/* ------------------    Data structure Pratt object ---------------- */

typedef enum { NUMBER = 0, INFIX, NEGATION } PObjType;

typedef struct PObj {
  int refcount;
  PObjType type;
  union {
    int i;
    struct {
      struct PObj *ele;
    } left;
    char operator;
    struct {
      struct PObj *ele;
    } right;
    struct {
      struct PObj *ele;
    } neg;
  };
} PObj;

/* -------------  Functions prototypes ------------------------- */
void retain(PObj *o);
void release(PObj *o);

/*---------------     Object related functions ----------------  */

/* Allocate and initialize e new Pratt Object */

PObj *createObject(PObjType type) {
  PObj *o = xmalloc(sizeof(PObj));
  o->refcount = 1;
  o->type = type;
  return o;
}

PObj *createNumberObject(int n) {
  PObj *o = createObject(NUMBER);
  o->i = n;
  return o;
}

PObj *createInfixObject(PObj *left, char operator, PObj *right) {
  PObj *o = createObject(INFIX);
  o->left.ele = left;
  o->operator = operator;
  o->right.ele = right;
  return o;
}

PObj *createNegationObject(PObj *ele) {
  PObj *o = createObject(NEGATION);
  o->neg.ele = ele;
  return o;
}
/* Free an object and all the other nested objects. */
void freeObject(PObj *o) {
  switch (o->type) {
    case INFIX:
      freeObject(o->left.ele);
      freeObject(o->right.ele);
      release(o);
      break;
    case NUMBER:
      release(o);
      break;
    case NEGATION:
      freeObject(o->neg.ele);
      release(o);
      break;
    default:
      break;
  }
  free(o);
}

void retain(PObj *o) {
  o->refcount++;
}

void release(PObj *o) {
  assert(o->refcount > 0);
  o->refcount--;
  if (o->refcount == 0) freeObject(o);
}

void printPObj(PObj *o) {
  switch (o->type) {
    case NUMBER:
      printf("%d ", o->i);
      break;
    case INFIX:
      printPObj(o->left.ele);
      printf("operator: %c", o->operator);
      printPObj(o->right.ele);
      break;
    case NEGATION:
      printPObj(o->neg.ele);
      break;
    default:
      printf("?\n");
      break;
  }
}

/* Add the new element at the end of the list. It is up to the caller to
 * increment the reference count to the list. */
// void listPush(SyObj *l, SyObj *o) {
//   if (l->list.len >= l->list.capacity) {
//     size_t newCap = l->list.capacity <= 0 ? 1 : l->list.capacity * 2;
//     l->list.ele = xrealloc(l->list.ele, newCap * sizeof(SyObj *));
//     l->list.capacity = newCap;
//   }
//   l->list.ele[l->list.len] = o;
//   l->list.len++;
// }
PObj *parsePrefix(pCtx *ctx) {
  switch (ctx->lexer.curToken->type) {
    case TOKEN_NUMBER:
      return createNumberObject(ctx->lexer.curToken->i);
      break;

    default:
      printf("???\n");
      break;
  }
  return NULL;
  // if(ctx->lexer.curToken->type == TOKEN_NUMBER){

  // }
}

PrefixFn getPrefixFn(pCtx *ctx) {
  pTable table = ctx->table;
  char type = ctx->lexer.curToken->type;
  for (int i = 0; i < table.count; i++) {
    if (table.entry != NULL && table.entry[i]->type == type) {
    // printf("here\n");
      return table.entry[i]->prefixFn;
    }
    printf("Error: cannot find prefixFn for token type: %c\n", type);
  }
  return NULL;
}

PObj *parseExpression(pCtx *ctx, int precedence) {
  PrefixFn prefix = getPrefixFn(ctx);
  if (prefix == NULL) {
    printf("Error: prefix is NULL");
    return NULL;
  }
  PObj *left = prefix(ctx);

  return left;
}

void freeContext(pCtx *ctx) {
  if (ctx->lexer.curToken) {
    releaseToken(ctx->lexer.curToken);
  }
  if (ctx->table.entry) {
    for (int i = 0; i < ctx->table.count; i++) {
      free(ctx->table.entry[i]);
    }
    free(ctx->table.entry);
  }
  free(ctx);
}


pCtx *createContext(char *buf) {
  pCtx *ctx = xmalloc(sizeof(pCtx));
  ctx->table.entry = NULL;
  ctx->table.count = 0;
  ctx->lexer.prg = buf;
  ctx->lexer.p = buf;
  addRule(ctx, TOKEN_NUMBER, 0, parsePrefix, NULL);
  printf("table entry type: %d\n", ctx->table.entry[0]->type);
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
  fclose(fp);

  pCtx *ctx = createContext(buf);
  readEndOfFile(ctx);
  advanceLexer(ctx);
  printToken(ctx->lexer.curToken);
  PObj *ast = parseExpression(ctx, 0);


  printf("Result is: \n");
  printPObj(ast);
  //   if (ctx->stack->list.len > 0) {
  //     // Garantito matematicamente che l'elemento 0 è una SONG!
  //     printCsObj(ctx->stack->list.ele[0]);
  //   } else {
  //     printf("Empty AST\n");
  //   }

  freeContext(ctx);
  free(buf);
  return 0;
}
