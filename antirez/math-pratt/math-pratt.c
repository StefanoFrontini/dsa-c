
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

// Il buffer condiviso passa tramite il puntatore 'sharedBuf'
void printAST_recursive(PObj *node, char *sharedBuf, int isTail) {
  if (node == NULL) return;

  // 1. Stampa usando lo stato attuale del buffer
  printf("%s%s", sharedBuf, isTail ? "└── " : "├── ");

  switch (node->type) {
    case NUMBER:
      printf("NUM: %d\n", node->i);
      break;
    case INFIX:
      printf("OP:  %c\n", node->infix.operator);
      break;
    case NEGATION:
      printf("UNARY: -\n");
      break;
    default:
      break;
  }

  // 2. BACKTRACKING: Salviamo la lunghezza originale
  size_t originalLen = strlen(sharedBuf);

  // Scegliamo la stringa da accodare
  const char *appendStr = isTail ? "    " : "│   ";

  // Dobbiamo calcolare quanti byte copiare.
  // ATTENZIONE: "│   " sembra lungo 4 caratteri, ma '│' in UTF-8
  // occupa 3 byte! Quindi strlen("│   ") restituisce 6.
  size_t appendLen = strlen(appendStr);

  // 3. Mutiamo il buffer (con controllo manuale dei limiti!)
  // Controlliamo che ci sia spazio sufficiente, incluso il byte per '\0'
  if (originalLen + appendLen < 1024) {
    // Usiamo memcpy per copiare i byte esatti, PIÙ 1 per includere
    // il terminatore nullo '\0' che si trova alla fine di appendStr.
    memcpy(sharedBuf + originalLen, appendStr, appendLen + 1);
  } else {
    // Gestione dell'errore (opzionale, ma raccomandata in C puro)
    fprintf(stderr, "Errore: Buffer dell'albero troppo piccolo!\n");
    exit(1);
  }
  // 4. Chiamate ricorsive (passiamo lo STESSO IDENTICO puntatore fisico)
  if (node->type == INFIX) {
    printAST_recursive(node->infix.left, sharedBuf, 0);
    printAST_recursive(node->infix.right, sharedBuf, 1);
  } else if (node->type == NEGATION) {
    printAST_recursive(node->neg.ele, sharedBuf, 1);
  }

  // 5. RIPRISTINO: Troncamo la stringa, cancellando di netto
  // le modifiche fatte al punto 3!
  sharedBuf[originalLen] = '\0';
}

void printTree(PObj *ast) {
  printf("AST Structure:\n");
  if (ast == NULL) return;

  switch (ast->type) {
    case NUMBER:
      printf("NUM: %d\n", ast->i);
      break;
    case INFIX:
      printf("OP:  %c\n", ast->infix.operator);
      break;
    case NEGATION:
      printf("UNARY: -\n");
      break;
    default:
      break;
  }

  // ALLOCAZIONE UNICA SULLO STACK!
  // Solo 1 KB usato, indipendentemente da quanto è profondo l'albero.
  char buffer[1024] = ""; // Inizializzato a stringa vuota

  if (ast->type == INFIX) {
    printAST_recursive(ast->infix.left, buffer, 0);
    printAST_recursive(ast->infix.right, buffer, 1);
  } else if (ast->type == NEGATION) {
    printAST_recursive(ast->neg.ele, buffer, 1);
  }
  printf("\n");
}
// Funzione di supporto per stampare gli spazi di indentazione
void printIndent(int indent) {
  for (int i = 0; i < indent; i++) {
    printf("  "); // 2 spazi per ogni livello di profondità
  }
}

// Funzione ricorsiva DFS per la generazione dell'XML
void printXML(PObj *node, int indent) {
  if (node == NULL) return;

  switch (node->type) {
    case NUMBER:
      printIndent(indent);
      printf("<number>%d</number>\n", node->i);
      break;

    case INFIX:
      printIndent(indent);
      printf("<infix>\n"); // Pre-Order

      // 1. Esplora il ramo sinistro (aumentando l'indentazione)
      printXML(node->infix.left, indent + 1);

      // 2. Stampa l'operatore al centro (In-Order)
      printIndent(indent + 1);
      printf("<op>%c</op>\n", node->infix.operator);

      // 3. Esplora il ramo destro
      printXML(node->infix.right, indent + 1);

      // 4. Chiudi il tag
      printIndent(indent);
      printf("</infix>\n"); // Post-Order
      break;

    case NEGATION:
      printIndent(indent);
      printf("<negation>\n");

      // Esplora l'unico figlio
      printXML(node->neg.ele, indent + 1);

      printIndent(indent);
      printf("</negation>\n");
      break;

    default:
      printIndent(indent);
      printf("<error>Unknown Node</error>\n");
      break;
  }
}

void printJSON(PObj *node, int indent) {
  if (node == NULL) {
    printf("null");
    return;
  }

  switch (node->type) {
    case NUMBER:
      printf("{\n");
      printIndent(indent + 1);
      printf("\"type\": \"Number\",\n");
      printIndent(indent + 1);
      printf("\"value\": %d\n", node->i);
      printIndent(indent);
      printf("}");
      break;

    case INFIX:
      printf("{\n");
      printIndent(indent + 1);
      printf("\"type\": \"Infix\",\n");

      printIndent(indent + 1);
      printf("\"operator\": \"%c\",\n", node->infix.operator);

      printIndent(indent + 1);
      printf("\"left\": ");
      printJSON(node->infix.left, indent + 1); // Ricorsione ramo sinistro
      printf(",\n"); // La virgola separa la chiave "left" dalla chiave "right"

      printIndent(indent + 1);
      printf("\"right\": ");
      printJSON(node->infix.right, indent + 1); // Ricorsione ramo destro
      printf("\n");

      printIndent(indent);
      printf("}");
      break;

    case NEGATION:
      printf("{\n");
      printIndent(indent + 1);
      printf("\"type\": \"Negation\",\n");

      printIndent(indent + 1);
      printf("\"element\": ");
      printJSON(node->neg.ele, indent + 1); // Ricorsione sul figlio unico
      printf("\n");

      printIndent(indent);
      printf("}");
      break;

    default:
      printf("null");
      break;
  }
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
  } else if (ctx->lexer.curToken.type == TOKEN_OPENPAREN) {
    advanceLexer(ctx);
    PObj *innerAST = parseExpression(ctx, 0);

    if (ctx->lexer.curToken.type != TOKEN_CLOSEPAREN) {
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

int evalAST(PObj *ast) {
  switch (ast->type) {
    case NUMBER:
      return ast->i;

    case INFIX: {
      int a = evalAST(ast->infix.left);
      int b = evalAST(ast->infix.right);
      switch (ast->infix.operator) {
        case '+':
          return a + b;
        case '-':
          return a - b;
        case '*':
          return a * b;
        case '/':
          if (b == 0) {
            fprintf(stderr, "Runtime error: Division by zero!\n");
            exit(1);
          }
          return a / b;
        default:
          fprintf(stderr, "Unknown operator: %c\n", ast->infix.operator);
          exit(1);
      }
    }

    case NEGATION: {
      return -evalAST(ast->neg.ele);
    }

    default:
      fprintf(stderr, "Unknown AST type\n");
      exit(1);
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

  printTree(ctx.ast);
  printf("XML Representation:\n");
  printXML(ctx.ast, 0);

  printf("\n");
  printf("JSON Representation:\n");
  printJSON(ctx.ast, 0);
  printf("\n");

  int result = evalAST(ctx.ast);
  printf("Result is: %d\n", result);

  freeContext(&ctx);
  free(buf);
  return 0;
}
