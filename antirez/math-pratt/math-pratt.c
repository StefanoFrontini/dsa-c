
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
  TOKEN_ILLEGAL,
  TOKEN_ENDOFFILE
} TokenType;

typedef struct Token {
  TokenType type;
  union {
    char symbol;
    int i;
  };
} Token;

typedef struct Lexer {
  char *prg;
  char *p;
  Token curToken;
} Lexer;

struct pCtx;
struct PObj;
typedef struct PObj *(*PrefixFn)(struct pCtx *ctx);
typedef struct PObj *(*InfixFn)(struct pCtx *ctx, struct PObj *left);

typedef struct parsingRulesTableEntry {
  PrefixFn prefixFn;
  InfixFn infixFn;
  int i;
} pTableEntry;


typedef struct pCtx {
  Lexer lexer;
  struct PObj *ast;
} pCtx;

struct PObj;
struct PObj *parsePrefix(pCtx *ctx);
struct PObj *parseInfix(pCtx *ctx, struct PObj *left);

static pTableEntry rulesTable[] = {
    [TOKEN_NUMBER] = {parsePrefix, NULL, 0},
    [TOKEN_PLUS] = {NULL, parseInfix, 2},
    [TOKEN_MINUS] = {NULL, parseInfix, 2},
    [TOKEN_MULT] = {NULL, parseInfix, 3},
    [TOKEN_DIV] = {NULL, parseInfix, 3},
    [TOKEN_ENDOFFILE] = {NULL, NULL, 0},
};


/* ------------------  Token related functions ----------------  */

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

  ctx->lexer.curToken.type = TOKEN_NUMBER;
  ctx->lexer.curToken.i = atoi(buf);
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
  ctx->lexer.curToken.type = TOKEN_ILLEGAL;
  switch (s) {
    case '+':
      ctx->lexer.curToken.type = TOKEN_PLUS;
      break;
    case '-':
      ctx->lexer.curToken.type = TOKEN_MINUS;
      break;
    case '*':
      ctx->lexer.curToken.type = TOKEN_MULT;
      break;
    case '/':
      ctx->lexer.curToken.type = TOKEN_DIV;
      break;
    default:
      printf("??? - %d\n", s);
      break;
  }
  ctx->lexer.curToken.symbol = s;
}

void readIllegalChar(pCtx *ctx, char s) {
  ctx->lexer.curToken.type = TOKEN_ILLEGAL;
  ctx->lexer.curToken.symbol = s;
  ctx->lexer.p++;
}

void readEndOfFile(pCtx *ctx) {
  ctx->lexer.curToken.type = TOKEN_ENDOFFILE;
  ctx->lexer.curToken.symbol = 0;
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
      printf("Current token is a number: %d\n", t->i);
      break;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_MULT:
    case TOKEN_DIV:
    case TOKEN_ILLEGAL:
      printf("Current token is a symbol: %c\n", t->symbol);
    case TOKEN_ENDOFFILE:
      break;
    default:
      printf("????? - %d\n", t->type);
      break;
  }
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
      release(o->infix.left);
      release(o->infix.right);
      break;
    case NUMBER:
      break;
    case NEGATION:
      release(o->neg.ele);
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
  switch (ctx->lexer.curToken.type) {
    case TOKEN_NUMBER:
      return createNumberObject(ctx->lexer.curToken.i);
      break;

    default:
      printf("???\n");
      break;
  }
  return NULL;
}

int getPrecedence(pCtx *ctx) {
  TokenType type = ctx->lexer.curToken.type;
  return rulesTable[type].i;
}

PrefixFn getPrefixFn(pCtx *ctx) {
  TokenType type = ctx->lexer.curToken.type;
  PrefixFn fn = rulesTable[type].prefixFn;
  if (fn == NULL) {
    printf("No prefixFn for token type: %c\n", type);
    return NULL;
  }
  return fn;
}

InfixFn getInfixFn(pCtx *ctx) {
  TokenType type = ctx->lexer.curToken.type;
  InfixFn fn = rulesTable[type].infixFn;
  if (fn == NULL) {
    printf("No infixFn for token type: %c\n", type);
    return NULL;
  }
  return fn;
}

PObj *parseExpression(pCtx *ctx, int precedence) {
  PrefixFn prefix = getPrefixFn(ctx);
  if (prefix == NULL) {
    printf("Error: prefix is NULL");
    return NULL;
  }
  PObj *left = prefix(ctx);
  advanceLexer(ctx);
  while (ctx->lexer.curToken.type != TOKEN_ENDOFFILE &&
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
  char operator = ctx->lexer.curToken.symbol;
  advanceLexer(ctx);
  PObj *right = parseExpression(ctx, precedence);
  return createInfixObject(left, operator, right);
}


void freeContext(pCtx *ctx) {
  if (ctx->ast) {
    release(ctx->ast);
  }
  free(ctx);
}

pCtx *createContext(char *buf) {
  pCtx *ctx = xmalloc(sizeof(pCtx));
  ctx->lexer.prg = buf;
  ctx->lexer.p = buf;
  readEndOfFile(ctx);
  advanceLexer(ctx);
  printToken(&ctx->lexer.curToken);
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
