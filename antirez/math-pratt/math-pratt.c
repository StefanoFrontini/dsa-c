
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

void readNumber(Lexer *l) {
  char buf[128];
  char *start = l->p;
  char *end;

  while (*l->p && isdigit(*l->p))
    l->p++;

  end = l->p;
  size_t len = end - start;

  if (len >= sizeof(buf)) {
    fprintf(stderr, "Number literal too long\n");
    return NULL;
  }

  memcpy(buf, start, len);
  buf[len] = 0;

  Token *t = xmalloc(sizeof(Token));
  t->refcount = 1;
  t->type = TOKEN_NUMBER;
  t->i = atoi(buf);
  l->curToken = t;
}

/* Return true if the character 'c' is one of the characters acceptable for our
 * symbols. */
int isSymbolChar(int c) {
  char symchars[] = "+-*/";
  return isalpha(c) || strchr(symchars, c) != NULL;
}

void readSymbol(Lexer *l) {
  char s = *l->p;
  l->p++;
  Token *t = xmalloc(sizeof(Token));
  t->refcount = 1;
  t->type = TOKEN_ILLEGAL;
  switch(s){
    case '+': t->type = TOKEN_PLUS; break;
    case '-': t->type = TOKEN_MINUS; break;
    case '*': t->type = TOKEN_MULT; break;
    case '/': t->type = TOKEN_DIV; break;
    default: printf("???\n"); break;
  }
  t->symbol = s;
  l->curToken = t;
}


void readIllegalChar(Lexer *l, char s) {
  Token *t = xmalloc(sizeof(Token));
  t->refcount = 1;
  t->type = TOKEN_ILLEGAL;
  t->symbol = s;
  l->curToken = t;
  l->p++;
}

void readEndOfFile(Lexer *l) {
  Token *t = xmalloc(sizeof(Token));
  t->refcount = 1;
  t->type = TOKEN_ENDOFFILE;
  t->symbol = 0;
  l->curToken = t;
}


Token *createTokenList(void) {
  Token *t = xmalloc(sizeof(Token));
  t->type = TOKEN_LIST;
  t->refcount = 1;
  t->list.ele = NULL;
  t->list.len = 0;
  return t;
}

void parseSpaces(Lexer *l) {
  while (isspace(*l->p)) {
    l->p++;
  }
}

void advanceLexer(Lexer *l) {
  releaseToken(l->curToken);
  char c = l->p[0];
  if (isdigit(c)) {
    readNumber(l);
  } else if (isSymbolChar(c)) {
    readSymbol(l);
  } else if (c == 0) {
    readEndOfFile(l);
  } else {
    readIllegalChar(l, c);
  }
}

void printToken(Token *t) {
  switch (t->type) {
    case TOKEN_NUMBER:
      printf("Current token is %d\n", t->i);
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

// typedef struct precedenceTableEntry {
//   char s;
//   int i;
// } pTableEntry;

// typedef struct precedenceTable {
//   pTableEntry **pTable;
//   int count;
// } pTable;

struct pCtx;
typedef int (*MathFn)(struct pCtx *ctx, char s);

// typedef struct symbolTableEntry {
//   char s;
//   MathFn f;
// } sTableEntry;

typedef struct parsingRulesTableEntry {
  MathFn prefixFn;
  MathFn infixFn;
  char s;
  int i;
} pTableEntry;

typedef struct parsingRulesTable {
  pTableEntry **entry;
  int count;
} pTable;

// typedef struct symbolTable {
//   sTableEntry **sTable;
//   int count;
// } sTable;

typedef struct pCtx {
  pTable table;
} pCtx;

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

void addRule(pCtx *ctx, char s, int i, MathFn prefix, MathFn infix) {
  ctx->table.entry = xrealloc(ctx->table.entry, sizeof(pTableEntry *) * (ctx->table.count + 1));
  pTableEntry *new_entry = xmalloc(sizeof(pTableEntry));
  new_entry->i = i;
  new_entry->s = s;
  new_entry->infixFn = infix;
  new_entry->prefixFn = prefix;
  ctx->table.count++;
}

int getPrecedence(pCtx *ctx, Token *t) {
  char s = t->type;
  for (int i = 0; i < ctx->table.count; i++) {
    if (ctx->table.entry[i]->s == s) {
      return ctx->table.entry[i]->i;
    }
  }
  printf("Error: %c\n", s);
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

PObj *createInfixObject(PObj *left, char operator, PObj *right ) {
  PObj *o = createObject(INFIX);
  o->left.ele = left;
  o->operator = operator;
  o->right.ele = right;
  return o;
}

PObj *createNegationObject(PObj *ele){
  PObj *o = createObject(NEGATION);
  o->neg.ele = ele;
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

int basicMathFunctions(pCtx *ctx, char s) {
  // if (s == '~') {
  //   if (ctxCheckStackMinLen(ctx, 1)) return SY_ERR;
  //   SyObj *a = ctxPostFixStackPop(ctx);
  //   if (a == NULL) {
  //     return SY_ERR;
  //   };
  //   int result = -(a->num);
  //   printf("result: %d\n", result);
  //   ctxPostFixStackPush(ctx, createNumberObject(result));
  //   release(a);
  //   return SY_OK;
  // }
  // if (ctxCheckStackMinLen(ctx, 2)) return SY_ERR;

  // SyObj *b = ctxPostFixStackPop(ctx);
  // if (b == NULL) return SY_ERR;

  // SyObj *a = ctxPostFixStackPop(ctx);
  // if (a == NULL) {
  //   ctxPostFixStackPush(ctx, b);
  //   return SY_ERR;
  // };
  // int result;
  // switch (s) {
  //   case '+':
  //     result = a->num + b->num;
  //     break;
  //   case '-':
  //     result = a->num - b->num;
  //     break;
  //   case '*':
  //     result = a->num * b->num;
  //     break;
  //   case '/':
  //     result = a->num / b->num;
  //     break;
  // }
  // release(a);
  // release(b);

  // SyObj *resObj = createNumberObject(result);
  // ctxPostFixStackPush(ctx, resObj);
  // return SY_OK;
}


pCtx *createContext(void) {
  pCtx *ctx = xmalloc(sizeof(pCtx));
  ctx->table.entry = NULL;
  ctx->table.count = 0;

  addRule(ctx, '+', 1, NULL, basicMathFunctions);
  // addSymbolPrecedence(ctx, '+', 1);
  // addSymbolPrecedence(ctx, '*', 2);
  // addSymbolPrecedence(ctx, '-', 1);
  // addSymbolPrecedence(ctx, '/', 2);
  // addSymbolPrecedence(ctx, '(', 0);
  // addSymbolPrecedence(ctx, ')', 0);
  // addSymbolPrecedence(ctx, '~', 3);

  // ctx->symbolTable.sTable = NULL;
  // ctx->symbolTable.count = 0;
  // addSymbolFn(ctx, '+', basicMathFunctions);
  // addSymbolFn(ctx, '-', basicMathFunctions);
  // addSymbolFn(ctx, '*', basicMathFunctions);
  // addSymbolFn(ctx, '/', basicMathFunctions);
  // addSymbolFn(ctx, '~', basicMathFunctions);

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

  Lexer l;
  l.prg = buf;
  l.p = buf;
  readEndOfFile(&l);
  advanceLexer(&l);

  pCtx *ctx = createContext();

//   // --- INIEZIONE DELLE SENTINELLE INIZIALI ---
//   ctxEnqueue(ctx, createEmptySongToken());
//   ctxEnqueue(ctx, createEmptyLineToken());

//   while (l.curToken->type != ENDOFFILE) {
//     switch (l.curToken->type) {

//       case LYRIC_TOKEN: {
//         // 1. PRIMA COSA: Iniettiamo il moltiplicatore per legarci alla LINE
//         Token *w = createTokenWordMult();
//         ctxOpStackPush(ctx, w);

//         // 2. Prefisso CHORD (vuoto)
//         Token *emptyChord = createEmptyChordToken();
//         ctxOpStackPush(ctx, emptyChord);

//         // 3. Operando LYRIC
//         retainToken(l.curToken);
//         ctxEnqueue(ctx, l.curToken);

//         advanceLexer(&l);
//         break;
//       }

//       case CHORD_TOKEN: {
//         // 1. PRIMA COSA: Iniettiamo il moltiplicatore per legarci alla LINE
//         Token *w = createTokenWordMult();
//         ctxOpStackPush(ctx, w);

//         // 2. Prefisso CHORD reale
//         retainToken(l.curToken);
//         ctxOpStackPush(ctx, l.curToken);
//         advanceLexer(&l);

//         // 3. Operando LYRIC (o vuoto se c'è solo l'accordo)
//         if (l.curToken->type == LYRIC_TOKEN) {
//           retainToken(l.curToken);
//           ctxEnqueue(ctx, l.curToken);
//           advanceLexer(&l);
//         } else {
//           Token *emptyLyric = createEmptyLyricToken();
//           ctxEnqueue(ctx, emptyLyric);
//         }
//         break;
//       }

//       case ENDOFLINE: {
//         retainToken(l.curToken);
//         ctxOpStackPush(ctx,
//                        l.curToken); // Push \n nello stack (sfratta gli '*')

//         // --- FLUSH IMMEDIATO ---
//         // Chiudiamo la riga mandando il \n subito nella coda
//         Token *popped_nl = listPop(ctx->opStack);
//         ctxEnqueue(ctx, popped_nl);

//         advanceLexer(&l);

//         // Inseriamo la sentinella vuota per accogliere la PROSSIMA riga
//         if (l.curToken->type != ENDOFFILE) {
//           ctxEnqueue(ctx, createEmptyLineToken());
//         }
//         break;
//       }

//       default:
//         printf("???%d\n", l.curToken->type);
//         advanceLexer(&l);
//         break;
//     }
//   }

//   // Chiusura artificiale del file (per essere sicuri di chiudere l'ultima riga)
//   Token *eof_nl = createEndOfLineToken();
//   ctxOpStackPush(ctx, eof_nl);

//   // Svuotiamo lo stack degli operatori
//   while (ctx->opStack->list.len > 0) {
//     Token *popped = listPop(ctx->opStack);
//     ctxEnqueue(ctx, popped);
//   }

  // Evitiamo memory leak dell'ultimo token ENDOFFILE letto dal lexer
  releaseToken(l.curToken);

  // Esecuzione Postfix
//   evalPostfix(ctx, ctx->queue);

  printf("Result is: \n");
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
