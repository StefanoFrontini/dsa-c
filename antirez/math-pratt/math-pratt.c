
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
  struct PObj *ast;
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
  return c != 0 && (isalpha(c) || strchr(symchars, c) != NULL);
}

void readSymbol(pCtx *ctx) {
  char s = ctx->lexer.p[0];
  printf("s: %c\n", s);
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
      printf("??? - %d\n", s);
      break;
  }
  t->symbol = s;
  printf("s type: %d\n", t->type);
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
}

void skipSpaces(pCtx *ctx) {
  while (isspace(ctx->lexer.p[0])) {
    ctx->lexer.p++;
  }
}

void advanceLexer(pCtx *ctx) {
  char c = ctx->lexer.p[0];
  if (isspace(c)) {
    skipSpaces(ctx);
    advanceLexer(ctx);
  } else if (isdigit(c)) {
    releaseToken(ctx->lexer.curToken);
    readNumber(ctx);
  } else if (isSymbolChar(c)) {
    releaseToken(ctx->lexer.curToken);
    readSymbol(ctx);
  } else if (c == 0) {
    releaseToken(ctx->lexer.curToken);
    readEndOfFile(ctx);
  } else {
    releaseToken(ctx->lexer.curToken);
    readIllegalChar(ctx, c);
  }
}

void printToken(Token *t) {
  switch (t->type) {
    case TOKEN_NUMBER:
      printf("Current token is a number: %d\n", t->i);
      break;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_MULT:
    case TOKEN_DIV:
    case TOKEN_ILLEGAL:
      printf("Current token is a symbol: %c\n", t->symbol);
    case TOKEN_LIST:
      for (size_t i = 0; i < t->list.len; i++) {
        Token *el = t->list.ele[i];
        printToken(el);
      }
      break;
    case TOKEN_ENDOFFILE:
      break;
    default:
      printf("????? - %d\n", t->type);
      break;
  }
}


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

int getPrecedence(pCtx *ctx) {
  char type = ctx->lexer.curToken->type;
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
      struct PObj *left;
      char operator;
      struct PObj *right;
    } infix;
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
  o->infix.left = left;
  o->infix.operator = operator;
  o->infix.right = right;
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
      freeObject(o->infix.left);
      freeObject(o->infix.right);
      free(o);
      break;
    case NUMBER:
      free(o);
      break;
    case NEGATION:
      freeObject(o->neg.ele);
      free(o);
      break;
    default:
      break;
  }
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
      printf("num: %d ", o->i);
      break;
    case INFIX:
      printf("infix: [ ");
      printPObj(o->infix.left);
      printf("operator: %c ", o->infix.operator);
      printPObj(o->infix.right);
      printf(" ] ");
      break;
    case NEGATION:
      printPObj(o->neg.ele);
      break;
    default:
      printf("?\n");
      break;
  }
}

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
}

PrefixFn getPrefixFn(pCtx *ctx) {
  pTable table = ctx->table;
  char type = ctx->lexer.curToken->type;
  for (int i = 0; i < table.count; i++) {
    if (table.entry != NULL && table.entry[i]->type == type) {
      return table.entry[i]->prefixFn;
    }
  }
  printf("No prefixFn for token type: %c\n", type);
  return NULL;
}

InfixFn getInfixFn(pCtx *ctx) {
  pTable table = ctx->table;
  char type = ctx->lexer.curToken->type;
  for (int i = 0; i < table.count; i++) {
    if (table.entry != NULL && table.entry[i]->type == type) {
      return table.entry[i]->infixFn;
    }
  }
  printf("No infixFn for token type: %d\n", type);
  return NULL;
}

PObj *parseExpression(pCtx *ctx, int precedence) {
  PrefixFn prefix = getPrefixFn(ctx);
  if (prefix == NULL) {
    printf("Error: prefix is NULL");
    return NULL;
  }
  PObj *left = prefix(ctx);
  advanceLexer(ctx);
  while (ctx->lexer.curToken->type != TOKEN_ENDOFFILE &&
         precedence < getPrecedence(ctx)) {
    InfixFn infix = getInfixFn(ctx);

    if (infix == NULL) {
      return left;
    }

    left = infix(ctx, left);

  }
  return left;
}

PObj *parseInfix(pCtx *ctx, PObj *left) {
  int precedence = getPrecedence(ctx);
  if (precedence == -1) {
    printf("Error: precedence cannot be -1 in parseInfix\n");
    exit(1);
  }
  char operator = ctx->lexer.curToken->symbol;
  advanceLexer(ctx);
  PObj *right = parseExpression(ctx, precedence);
  return createInfixObject(left, operator, right);
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
  if (ctx->ast){
    release(ctx->ast);
  }
  free(ctx);
}

pCtx *createContext(char *buf) {
  pCtx *ctx = xmalloc(sizeof(pCtx));
  ctx->table.entry = NULL;
  ctx->table.count = 0;
  ctx->lexer.prg = buf;
  ctx->lexer.p = buf;
  addRule(ctx, TOKEN_NUMBER, -1, parsePrefix, NULL);
  addRule(ctx, TOKEN_PLUS, 2, NULL, parseInfix);
  readEndOfFile(ctx);
  advanceLexer(ctx);
  printToken(ctx->lexer.curToken);
  ctx->ast = parseExpression(ctx, 0);
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

  printf("Result is: \n");
  printPObj(ctx->ast);

  freeContext(ctx);
  free(buf);
  return 0;
}
