
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PREFIX_PRECEDENCE 6

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

/* ----------------   Data structures: Lexer & Parser ------------------ */

typedef enum {
  TOKEN_NUMBER = 0,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_DIV,
  TOKEN_MULT,
  TOKEN_SYMBOL,
  TOKEN_OPENPAREN,
  TOKEN_CLOSEPAREN,
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

// AST Object (PObj)

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

typedef struct pCtx {
  Lexer lexer;
  PObj *ast;
} pCtx;

/* Prototypes for the parsing table and related functions */

typedef PObj *(*PrefixFn)(pCtx *ctx);
typedef PObj *(*InfixFn)(pCtx *ctx, PObj *left);

typedef struct {
  PrefixFn prefixFn;
  InfixFn infixFn;
  int precedence;
} pTableEntry;

PObj *parsePrefix(pCtx *ctx);
PObj *parseInfix(pCtx *ctx, PObj *left);
PObj *parseExpression(pCtx *ctx, int precedence);

/* ------------------ Static Parsing Table ---------------- */

static pTableEntry rulesTable[] = {
    [TOKEN_NUMBER] = {parsePrefix, NULL, 0},
    [TOKEN_PLUS] = {NULL, parseInfix, 2},
    [TOKEN_MINUS] = {parsePrefix, parseInfix, 2},
    [TOKEN_MULT] = {NULL, parseInfix, 3},
    [TOKEN_DIV] = {NULL, parseInfix, 3},
    [TOKEN_OPENPAREN] = {parsePrefix, NULL, 0},
    [TOKEN_CLOSEPAREN] = {NULL, NULL, 0},
    [TOKEN_ENDOFFILE] = {NULL, NULL, 0},
};

/* ------------------  Lexer related functions ----------------  */

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
  char symchars[] = "+-*/()";
  return c != 0 && (isalpha(c) || strchr(symchars, c) != NULL);
}

void readSymbol(pCtx *ctx) {
  char s = ctx->lexer.p[0];
  ctx->lexer.p++;

  ctx->lexer.curToken.symbol = s;
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
    case '(':
      ctx->lexer.curToken.type = TOKEN_OPENPAREN;
      break;
    case ')':
      ctx->lexer.curToken.type = TOKEN_CLOSEPAREN;
      break;
    default:
      ctx->lexer.curToken.type = TOKEN_ILLEGAL;
      printf("??? - %d\n", s);
      break;
  }
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

void advanceLexer(pCtx *ctx) {
  while (isspace(ctx->lexer.p[0]))
    ctx->lexer.p++;

  char c = ctx->lexer.p[0];


  printf("c: %c\n", c);

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
/* ------------------  AST Object Management (PObj) ----------------  */

void release(PObj *o);

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
  if (!o) return;
  assert(o->refcount > 0);
  o->refcount--;
  if (o->refcount == 0) freeObject(o);
}

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

void printPObj(PObj *o) {
  switch (o->type) {
    case NUMBER:
      printf("(num: %d)", o->i);
      break;
    case INFIX:
      printf("infix: [ ");
      printPObj(o->infix.left);
      printf("(operator: %c) ", o->infix.operator);
      printPObj(o->infix.right);
      printf(" ] ");
      break;
    case NEGATION:
      printf("(NEGATION ");
      printPObj(o->neg.ele);
      printf(") ");
      break;
    default:
      printf("?\n");
      break;
  }
}
/* ------------------  Pratt Parser logic ----------------  */

int getPrecedence(pCtx *ctx) {
  return rulesTable[ctx->lexer.curToken.type].precedence;
}

PObj *parsePrefix(pCtx *ctx) {
  if (ctx->lexer.curToken.type == TOKEN_NUMBER) {
    PObj *o = createNumberObject(ctx->lexer.curToken.i);
    advanceLexer(ctx);
    return o;
  } else if (ctx->lexer.curToken.type == TOKEN_MINUS) {
    advanceLexer(ctx);
    PObj *ele = parseExpression(ctx, PREFIX_PRECEDENCE);
    return createNegationObject(ele);
  }
  else if(ctx->lexer.curToken.type == TOKEN_OPENPAREN){
    advanceLexer(ctx);
    PObj *innerAST = parseExpression(ctx, 0);

    if(ctx->lexer.curToken.type != TOKEN_CLOSEPAREN){
      printf("Syntax error: close paren expected ')'\n");
      exit(1);
    }
    advanceLexer(ctx);
    return innerAST;
  }
  printf("Error: No prefix function for the token %d\n",
         ctx->lexer.curToken.type);
  return NULL;
}

PObj *parseInfix(pCtx *ctx, PObj *left) {
  int precedence = getPrecedence(ctx);
  char op = ctx->lexer.curToken.symbol;
  advanceLexer(ctx);
  PObj *right = parseExpression(ctx, precedence);
  return createInfixObject(left, op, right);
}

PObj *parseExpression(pCtx *ctx, int precedence) {
  TokenType type = ctx->lexer.curToken.type;
  PrefixFn prefix = rulesTable[type].prefixFn;

  if (prefix == NULL) {
    printf("Error: Token %d cannot be used in prefix notation \n", type);
    return NULL;
  }

  PObj *left = prefix(ctx);

  while (ctx->lexer.curToken.type != TOKEN_ENDOFFILE &&
         precedence < getPrecedence(ctx)) {

    InfixFn infix = rulesTable[ctx->lexer.curToken.type].infixFn;
    printf("infix\n");

    if (infix == NULL) {
      return left;
    }

    left = infix(ctx, left);
  }
  return left;
}

/* ------------------  Context Management  ----------------  */

void initContext(pCtx *ctx, char *buf) {
  memset(ctx, 0, sizeof(pCtx));

  ctx->lexer.prg = buf;
  ctx->lexer.p = buf;
}

void freeContext(pCtx *ctx) {
  if (ctx->ast) {
    release(ctx->ast);
  }
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
  if (file_size == 0) {
    fprintf(stderr, "File is empty\n");
    fclose(fp);
    return 1;
  }
  fseek(fp, 0, SEEK_SET);
  char *buf = xmalloc(file_size + 1);
  fread(buf, file_size, 1, fp);
  buf[file_size] = 0;
  fclose(fp);

  pCtx ctx;
  initContext(&ctx, buf);

  advanceLexer(&ctx);
  ctx.ast = parseExpression(&ctx, 0);

  printf("AST result: \n");
  printPObj(ctx.ast);
  printf("\n");

  freeContext(&ctx);
  free(buf);
  return 0;
}
