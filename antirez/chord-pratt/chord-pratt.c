#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
  TOKEN_LYRIC = 0,
  TOKEN_CHORD,
  TOKEN_WORD_MULT,
  TOKEN_LINE,
  TOKEN_SONG,
  TOKEN_ENDOFLINE,
  TOKEN_ILLEGAL,
  TOKEN_ENDOFFILE
} TokenType;

typedef struct Token {
  TokenType type;
  union {
    char symbol;
    struct {
      char *ptr;
      size_t len;
    } str;
  };
} Token;

typedef enum {
  STATE_EMPTY_CHORD = 0,
  STATE_EMPTY_LYRIC,
  STATE_WORD_MULT,
  STATE_READING,
  STATE_START_SONG,
  STATE_START_ENDOFLINE,
  STATE_EMPTY_LINE,
  STATE_EMPTY_CHORD_ENDING,
  STATE_EMPTY_LYRIC_ENDING,
  STATE_ENDOFFILE
} LexerState;

typedef struct Lexer {
  char *prg;
  char *p;
  Token curToken;
  LexerState state;
} Lexer;

// AST Object (PObj)

typedef enum { LYRIC = 0, CHORD, INFIX, WORD, LINE, SONG } PObjType;

typedef struct PObj {
  int refcount;
  PObjType type;
  union {
    struct {
      char *ptr;
      size_t len;
    } str;
    struct {
      struct PObj *left;
      char operator;
      struct PObj *right;
    } infix;
    struct {
      struct PObj *chord;
      struct PObj *lyric;
    } word;
    struct {
      struct PObj **ele;
      size_t len;
    } list;
  };
} PObj;

typedef struct CpCtx {
  Lexer lexer;
  PObj *ast;
} CpCtx;

/* Prototypes for the parsing table and related functions */

typedef PObj *(*PrefixFn)(CpCtx *ctx);
typedef PObj *(*InfixFn)(CpCtx *ctx, PObj *left);

typedef struct {
  PrefixFn prefixFn;
  InfixFn infixFn;
  int precedence;
} pTableEntry;

// PObj *parsePrefix(CpCtx *ctx);
// PObj *parseInfix(CpCtx *ctx, PObj *left);
// PObj *parseExpression(CpCtx *ctx, int precedence);

/* ------------------ Static Parsing Table ---------------- */

// static pTableEntry rulesTable[] = {
//     [TOKEN_CHORD] = {parsePrefix, NULL, 0},
//     [TOKEN_LYRIC] = {parsePrefix, NULL, 0},
//     [TOKEN_ENDOFLINE] = {NULL, parseInfix, 2},
//     [TOKEN_WORD_MULT] = {NULL, parseInfix, 3},
//     [TOKEN_ENDOFFILE] = {NULL, NULL, 0},
// };

int isStringConstant(CpCtx *ctx) {
  char c = ctx->lexer.p[0];
  char symbols_char[] = "[]\n\r";
  return c != 0 && isascii(c) && strchr(symbols_char, c) == NULL;
}

void readLyric(CpCtx *ctx) {
  if (ctx->lexer.curToken.type != TOKEN_CHORD) {
    ctx->lexer.state = STATE_EMPTY_CHORD;
    // ctx->lexer.curToken.type = TOKEN_CHORD;
    // ctx->lexer.curToken.str.ptr = NULL;
    // ctx->lexer.curToken.str.len = 0;
    return;
  }
  char *start = ctx->lexer.p;
  while (isStringConstant(ctx)) {
    ctx->lexer.p++;
  }
  char *end = ctx->lexer.p;
  size_t len = end - start;
  ctx->lexer.curToken.type = TOKEN_LYRIC;
  char *str = xmalloc(len + 1);
  ctx->lexer.curToken.str.ptr = str;
  ctx->lexer.curToken.str.len = len;
  memcpy(ctx->lexer.curToken.str.ptr, start, len);
  ctx->lexer.curToken.str.ptr[len] = 0;
  // if (ctx->lexer.p[0] == '[') {
  //   ctx->lexer.state = STATE_WORD_MULT;
  // }
}

int isBeginningChord(CpCtx *ctx) {
  return ctx->lexer.p[0] == '[';
}

void readChord(CpCtx *ctx) {
  ctx->lexer.p++;
  if (!isStringConstant(ctx)) {
    fprintf(stderr, "No string after open paren\n");
    exit(1);
  }
  char *start = ctx->lexer.p;
  while (isStringConstant(ctx)) {
    ctx->lexer.p++;
  }

  char *end = ctx->lexer.p;
  size_t len = end - start;
  if (ctx->lexer.p[0] != ']') {
    printf("char: %c\n", ctx->lexer.p[0]);
    fprintf(stderr, "Unmatched close paren\n");
    exit(1);
  }
  ctx->lexer.p++;
  if (!isStringConstant(ctx)) {
    ctx->lexer.state = STATE_EMPTY_LYRIC;
  }

  char *str = xmalloc(len + 1);
  ctx->lexer.curToken.type = TOKEN_CHORD;
  ctx->lexer.curToken.str.ptr = str;
  ctx->lexer.curToken.str.len = len;
  memcpy(ctx->lexer.curToken.str.ptr, start, len);
  ctx->lexer.curToken.str.ptr[len] = 0;
}

void readEndOfLine(CpCtx *ctx, char s) {
  ctx->lexer.curToken.type = TOKEN_ENDOFLINE;
  ctx->lexer.curToken.symbol = s;
  ctx->lexer.p++;
  ctx->lexer.state = STATE_EMPTY_LINE;
}

void readIllegalChar(CpCtx *ctx, char s) {
  ctx->lexer.curToken.type = TOKEN_ILLEGAL;
  ctx->lexer.curToken.symbol = s;
  ctx->lexer.p++;
}

void readEndOfFile(CpCtx *ctx) {
  ctx->lexer.curToken.type = TOKEN_ENDOFFILE;
  ctx->lexer.curToken.symbol = 0;
}

// Token *createEmptyChordToken(void) {
//   Token *t = xmalloc(sizeof(Token));
//   t->type = CHORD_TOKEN;
//   t->refcount = 1;
//   t->str.ptr = xmalloc(1);
//   t->str.ptr[0] = 0;
//   t->str.len = 0;
//   return t;
// }

// Token *createEmptyLyricToken(void) {
//   Token *t = xmalloc(sizeof(Token));
//   t->type = LYRIC_TOKEN;
//   t->refcount = 1;
//   t->str.ptr = xmalloc(1);
//   t->str.ptr[0] = 0;
//   t->str.len = 0;
//   return t;
// }

void advanceLexer(CpCtx *ctx) {
  switch (ctx->lexer.state) {
    case STATE_START_SONG: {
      ctx->lexer.curToken.type = TOKEN_SONG;
      ctx->lexer.curToken.symbol = 'S';
      ctx->lexer.state = STATE_START_ENDOFLINE;
      break;
    }
    case STATE_START_ENDOFLINE: {
      ctx->lexer.curToken.type = TOKEN_ENDOFLINE;
      ctx->lexer.curToken.symbol = '\n';
      ctx->lexer.state = STATE_EMPTY_LINE;
      break;
    }
    case STATE_EMPTY_LINE: {
      ctx->lexer.curToken.type = TOKEN_LINE;
      ctx->lexer.curToken.symbol = 'L';
      ctx->lexer.state = STATE_READING;
      break;
    }
    case STATE_EMPTY_CHORD: {
      ctx->lexer.curToken.type = TOKEN_CHORD;
      ctx->lexer.curToken.str.ptr = NULL;
      ctx->lexer.curToken.str.len = 0;
      ctx->lexer.state = STATE_READING;
      break;
    }
    case STATE_EMPTY_LYRIC: {
      ctx->lexer.curToken.type = TOKEN_LYRIC;
      ctx->lexer.curToken.str.ptr = NULL;
      ctx->lexer.curToken.str.len = 0;
      ctx->lexer.state = STATE_READING;
      break;
    }
    case STATE_EMPTY_CHORD_ENDING: {
      ctx->lexer.curToken.type = TOKEN_CHORD;
      ctx->lexer.curToken.str.ptr = NULL;
      ctx->lexer.curToken.str.len = 0;
      ctx->lexer.state = STATE_EMPTY_LYRIC_ENDING;
      break;
    }
    case STATE_EMPTY_LYRIC_ENDING: {
      ctx->lexer.curToken.type = TOKEN_LYRIC;
      ctx->lexer.curToken.str.ptr = NULL;
      ctx->lexer.curToken.str.len = 0;
      ctx->lexer.state = STATE_ENDOFFILE;
      break;
    }
    case STATE_ENDOFFILE: {
       readEndOfFile(ctx);
       break;
    }
    case STATE_READING: {
      char c = ctx->lexer.p[0];
      if (isStringConstant(ctx)) {
        if (ctx->lexer.curToken.type != TOKEN_CHORD) {
          ctx->lexer.curToken.type = TOKEN_WORD_MULT;
          ctx->lexer.curToken.symbol = '*';
          ctx->lexer.state = STATE_EMPTY_CHORD;
        } else {
          readLyric(ctx);
        }
      } else if (isBeginningChord(ctx)) {
        if (ctx->lexer.curToken.type != TOKEN_WORD_MULT) {
          ctx->lexer.curToken.type = TOKEN_WORD_MULT;
          ctx->lexer.curToken.symbol = '*';
        } else {
          readChord(ctx);
        }
      } else if (c == '\n' || c == '\r') {
        readEndOfLine(ctx, c);
      } else if (c == 0) {
        ctx->lexer.curToken.type = TOKEN_WORD_MULT;
        ctx->lexer.curToken.symbol = '*';
        ctx->lexer.state = STATE_EMPTY_CHORD_ENDING;
      } else {
        readIllegalChar(ctx, c);
      }
      break;
    }
    default:
      printf("???\n");
      break;
  }
}

void printToken(Token *t) {
  switch (t->type) {
    case TOKEN_LYRIC:
      if (t->str.ptr == NULL) {
        printf("Current token is Empty Lyric: \n");
      } else {
        printf("Current token is Lyric: %s\n", t->str.ptr);
      }
      break;
    case TOKEN_CHORD:
      if (t->str.ptr == NULL) {
        printf("Current token is Empty Chord: \n");
      } else {
        printf("Current token is Chord: %s\n", t->str.ptr);
      }
      break;
    case TOKEN_WORD_MULT:
      printf("Current token is WORD_MULT: %c\n", t->symbol);
      break;
    case TOKEN_ILLEGAL:
      printf("Current token is Illegal: %c\n", t->symbol);
      break;
    case TOKEN_ENDOFLINE:
      printf("Current token is: 'newLine'\n");
      break;
    case TOKEN_ENDOFFILE:
      printf("Current token is Endoffile:\n");
      break;
    case TOKEN_LINE:
      printf("Current token is Empty Line\n");
      break;
    case TOKEN_SONG:
      printf("Current token is Empty Song\n");
      break;
    default:
      printf("? - %d\n", t->type);
      break;
  }
}

void initContext(CpCtx *ctx, char *buf) {
  memset(ctx, 0, sizeof(CpCtx));

  ctx->lexer.prg = buf;
  ctx->lexer.p = buf;
  ctx->lexer.state = STATE_START_SONG;
}

/* ------------------  AST Object Management (PObj) ----------------  */

void release(PObj *o);

/* Free an object and all the other nested objects. */
// void freeObject(PObj *o) {
//   switch (o->type) {
//     case INFIX:
//       release(o->infix.left);
//       release(o->infix.right);
//       break;
//     case WORD:
//       break;
//     case NEGATION:
//       release(o->neg.ele);
//       break;
//     default:
//       break;
//   }
//   free(o);
// }

// void retain(PObj *o) {
//   o->refcount++;
// }

// void release(PObj *o) {
//   if (!o) return;
//   assert(o->refcount > 0);
//   o->refcount--;
//   if (o->refcount == 0) freeObject(o);
// }

PObj *createObject(PObjType type) {
  PObj *o = xmalloc(sizeof(PObj));
  o->refcount = 1;
  o->type = type;
  return o;
}

PObj *createSongObject(void) {
  PObj *o = createObject(SONG);
  o->list.ele = NULL;
  o->list.len = 0;
  return o;
}

PObj *createLineObject(void) {
  PObj *o = createObject(LINE);
  o->list.ele = NULL;
  o->list.len = 0;
  return o;
}

PObj *createWordObject(void) {
  PObj *o = createObject(WORD);
  o->list.ele = NULL;
  o->list.len = 0;
  return o;
}

PObj *createInfixObject(PObj *left, char operator, PObj *right) {
  PObj *o = createObject(INFIX);
  o->infix.left = left;
  o->infix.operator = operator;
  o->infix.right = right;
  return o;
}

PObj *createChordObject(char *str, size_t len) {
  PObj *o = createObject(CHORD);
  if (len > 0) {
    o->str.ptr = xmalloc(len + 1);
    memcpy(o->str.ptr, str, len);
    o->str.ptr[len] = 0;
  } else {
    o->str.ptr = xmalloc(1);
    o->str.ptr[0] = 0;
  }
  o->str.len = len;
  return o;
}

PObj *createLyricObject(char *str, size_t len) {
  PObj *o = createObject(LYRIC);
  if (len > 0) {
    o->str.ptr = xmalloc(len + 1);
    memcpy(o->str.ptr, str, len);
    o->str.ptr[len] = 0;
  } else {
    o->str.ptr = xmalloc(1);
    o->str.ptr[0] = 0;
  }
  o->str.len = len;
  return o;
}

/* ------------------  Pratt Parser logic ----------------  */

// int getPrecedence(CpCtx *ctx) {
//   return rulesTable[ctx->lexer.curToken.type].precedence;
// }

// PObj *parsePrefix(CpCtx *ctx) {
//   if (ctx->lexer.curToken.type == TOKEN_LYRIC) {
//     PObj *o = createNumberObject(ctx->lexer.curToken.i);
//     advanceLexer(ctx);
//     return o;
//   } else if (ctx->lexer.curToken.type == TOKEN_MINUS) {
//     advanceLexer(ctx);
//     PObj *ele = parseExpression(ctx, PREFIX_PRECEDENCE);
//     return createNegationObject(ele);
//   } else if (ctx->lexer.curToken.type == TOKEN_OPENPAREN) {
//     advanceLexer(ctx);
//     PObj *innerAST = parseExpression(ctx, 0);

//     if (ctx->lexer.curToken.type != TOKEN_CLOSEPAREN) {
//       printf("Syntax error: close paren expected ')'\n");
//       exit(1);
//     }
//     advanceLexer(ctx);
//     return innerAST;
//   }
//   printf("Error: No prefix function for the token %d\n",
//          ctx->lexer.curToken.type);
//   return NULL;
// }

// PObj *parseInfix(CpCtx *ctx, PObj *left) {
//   int precedence = getPrecedence(ctx);
//   char op = ctx->lexer.curToken.symbol;
//   advanceLexer(ctx);
//   PObj *right = parseExpression(ctx, precedence);
//   return createInfixObject(left, op, right);
// }

// PObj *parseExpression(CpCtx *ctx, int precedence) {
//   TokenType type = ctx->lexer.curToken.type;
//   PrefixFn prefix = rulesTable[type].prefixFn;

//   if (prefix == NULL) {
//     printf("Error: Token %d cannot be used in prefix notation \n", type);
//     return NULL;
//   }

//   PObj *left = prefix(ctx);

//   while (ctx->lexer.curToken.type != TOKEN_ENDOFFILE &&
//          precedence < getPrecedence(ctx)) {

//     InfixFn infix = rulesTable[ctx->lexer.curToken.type].infixFn;

//     if (infix == NULL) {
//       return left;
//     }

//     left = infix(ctx, left);
//   }
//   return left;
// }

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
  // if (file_size == 0) {
  //   fprintf(stderr, "File is empty\n");
  //   fclose(fp);
  //   return 1;
  // }
  fseek(fp, 0, SEEK_SET);
  char *buf = xmalloc(file_size + 1);
  fread(buf, file_size, 1, fp);
  buf[file_size] = 0;
  fclose(fp);

  CpCtx ctx;
  initContext(&ctx, buf);

  advanceLexer(&ctx);
  while (ctx.lexer.curToken.type != TOKEN_ENDOFFILE) {
    printToken(&ctx.lexer.curToken);
    advanceLexer(&ctx);
  }
  free(buf);
  return 0;
}
