
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
  LYRIC_TOKEN = 0,
  CHORD_TOKEN,
  ENDOFLINE,
  ENDOFFILE,
  WORD_MULT,
  ILLEGAL,
  LIST_TOKEN
} TokenType;

typedef struct Token {
  int refcount;
  TokenType type;
  union {
    char symbol;
    struct {
      char *ptr;
      size_t len;
    } str;
    struct {
      struct Token **ele;
      size_t len;
    } list;
  };
} Token;

typedef struct Lexer {
  char *prg; // Program to parse
  char *p;   // Next token to parse
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
      case LYRIC_TOKEN:
      case CHORD_TOKEN:
        free(t->str.ptr);
        break;
      case LIST_TOKEN:
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

/* Return true if the character 'c' is one of the characters acceptable for the
 * lyric token. */
int isStringConstant(Lexer *l) {
  char c = l->p[0];
  char symbols_char[] = "[]\n\r";
  return isascii(c) && strchr(symbols_char, c) == NULL;
}

void readLyric(Lexer *l) {
  char *start = l->p;
  while (isStringConstant(l)) {
    l->p++;
  }
  char *end = l->p;
  size_t len = end - start;

  Token *t = xmalloc(sizeof(Token));
  char *str = xmalloc(len + 1);
  t->refcount = 1;
  t->type = LYRIC_TOKEN;
  t->str.ptr = str;
  t->str.len = len;
  memcpy(t->str.ptr, start, len);
  t->str.ptr[len] = 0;
  l->curToken = t;
}

int isBeginningChord(Lexer *l) {
  return l->p[0] == '[';
}

void readChord(Lexer *l) {
  l->p++;
  if (!isStringConstant(l)) {
    fprintf(stderr, "No string after open paren\n");
    exit(1);
  }
  char *start = l->p;
  while (isStringConstant(l)) {
    l->p++;
  }
  char *end = l->p;
  size_t len = end - start;
  if (l->p[0] != ']') {
    printf("char: %c\n", l->p[0]);
    fprintf(stderr, "Unmatched close paren\n");
    exit(1);
  }
  l->p++;

  Token *t = xmalloc(sizeof(Token));
  char *str = xmalloc(len + 1);
  t->refcount = 1;
  t->type = CHORD_TOKEN;
  t->str.ptr = str;
  t->str.len = len;
  memcpy(t->str.ptr, start, len);
  t->str.ptr[len] = 0;
  l->curToken = t;
}
Token *createEmptyChordToken(void) {
  Token *t = xmalloc(sizeof(Token));
  t->type = CHORD_TOKEN;
  t->refcount = 1;
  t->str.ptr = xmalloc(1);
  t->str.ptr[0] = 0;
  t->str.len = 0;
  return t;
}

void readEndOfLine(Lexer *l, char s) {
  Token *t = xmalloc(sizeof(Token));
  t->refcount = 1;
  t->type = ENDOFLINE;
  t->symbol = s;
  l->curToken = t;
  l->p++;
}
void readIllegalChar(Lexer *l, char s) {
  Token *t = xmalloc(sizeof(Token));
  t->refcount = 1;
  t->type = ILLEGAL;
  t->symbol = s;
  l->curToken = t;
  l->p++;
}

void readEndOfFile(Lexer *l) {
  Token *t = xmalloc(sizeof(Token));
  t->refcount = 1;
  t->type = ENDOFFILE;
  t->symbol = 0;
  l->curToken = t;
}

Token *createTokenList(void) {
  Token *t = xmalloc(sizeof(Token));
  t->type = LIST_TOKEN;
  t->refcount = 1;
  t->list.ele = NULL;
  t->list.len = 0;
  return t;
}

Token *createTokenWordMult(void) {
  Token *t = xmalloc(sizeof(Token));
  t->type = WORD_MULT;
  t->refcount = 1;
  t->symbol = WORD_MULT;
  return t;
}

Token *createEmptyLyricToken(void) {
  Token *t = xmalloc(sizeof(Token));
  t->type = LYRIC_TOKEN;
  t->refcount = 1;
  t->str.ptr = xmalloc(1);
  t->str.ptr[0] = 0;
  t->str.len = 0;
  return t;
}

void advanceLexer(Lexer *l) {
  releaseToken(l->curToken);
  char c = l->p[0];
  if (isStringConstant(l)) {
    readLyric(l);
  } else if (isBeginningChord(l)) {
    readChord(l);
  } else if (c == '\n' || c == '\r') {
    readEndOfLine(l, c);
  } else if (c == 0) {
    readEndOfFile(l);
  } else {
    readIllegalChar(l, c);
  }
}

void printToken(Token *t) {
  switch (t->type) {
    case LYRIC_TOKEN:
    case CHORD_TOKEN:
      printf("Current token is: %s\n", t->str.ptr);
      break;
    case ILLEGAL:
      printf("Current token is: %c\n", t->symbol);
      break;
    case ENDOFLINE:
      printf("Current token is: 'newLine'\n");
      break;
    case WORD_MULT:
      printf("Current token is: *\n");
      break;
    case LIST_TOKEN:
      for (size_t i = 0; i < t->list.len; i++) {
        Token *el = t->list.ele[i];
        printToken(el);
        // if (i != t->list.len - 1) {
        //   printf(" ");
        // }
      }
      break;

    default:
      printf("? - %d\n", t->type);
      break;
  }
}

/* --------------   Data structures: Chord Sheet Object ------------------- */

typedef enum { SONG = 0, LINE, WORD, CHORD, LYRIC, LIST } CsObjType;

typedef struct CsObj {
  int refcount;
  CsObjType type;
  union {
    struct {
      struct CsObj **ele;
      size_t len;
    } list;
    struct {
      char *ptr;
      size_t len;

    } str;
  };
} CsObj;

/* -----------   CsObj related functions ------------ */

/* Allocate and initialize e new Chord Sheet Object */

CsObj *createObject(CsObjType type) {
  CsObj *o = xmalloc(sizeof(CsObj));
  o->refcount = 1;
  o->type = type;
  return o;
}

CsObj *createSongObject(void) {
  CsObj *o = createObject(SONG);
  o->list.ele = NULL;
  o->list.len = 0;
  return o;
}

CsObj *createLineObject(void) {
  CsObj *o = createObject(LINE);
  o->list.ele = NULL;
  o->list.len = 0;
  return o;
}

CsObj *createWordObject(void) {
  CsObj *o = createObject(WORD);
  o->list.ele = NULL;
  o->list.len = 0;
  return o;
}

CsObj *createChordObject(char *str, size_t len) {
  CsObj *o = createObject(CHORD);
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

CsObj *createLyricObject(char *str, size_t len) {
  CsObj *o = createObject(LYRIC);
  if (len > 0) {
    o->str.ptr = xmalloc(len + 1);
    memcpy(o->str.ptr, str, len);
    o->str.ptr[len] = 0;
  } else {
    o->str.ptr = str;
  }
  o->str.len = len;
  return o;
}

CsObj *createListObject(void) {
  CsObj *o = createObject(LIST);
  o->list.ele = NULL;
  o->list.len = 0;
  return o;
}

/* Add the new element at the end of the list. It is up to the caller to
 * increment the reference count to the list. */
void listPush(Token *l, Token *ele) {
  l->list.ele = xrealloc(l->list.ele, sizeof(Token *) * (l->list.len + 1));
  l->list.ele[l->list.len] = ele;
  l->list.len++;
}

Token *listPop(Token *l) {
  if (l->list.len > 0) {
    Token *t = l->list.ele[l->list.len - 1];
    l->list.len--;
    return t;
  } else {
    return NULL;
  }
}
void listPushObj(CsObj *l, CsObj *ele) {
  l->list.ele = xrealloc(l->list.ele, sizeof(CsObj *) * (l->list.len + 1));
  l->list.ele[l->list.len] = ele;
  l->list.len++;
}

CsObj *listPopObj(CsObj *l) {
  if (l->list.len > 0) {
    CsObj *t = l->list.ele[l->list.len - 1];
    l->list.len--;
    return t;
  } else {
    return NULL;
  }
}

void retainCsObj(CsObj *o) {
  o->refcount++;
}

void releaseCsObj(CsObj *o) {
  assert(o->refcount > 0);
  o->refcount--;
  if (o->refcount == 0) {
    switch (o->type) {
      case SONG:
      case LINE:
      case WORD:
      case LIST:
        for (size_t i = 0; i < o->list.len; i++) {
          releaseCsObj(o->list.ele[i]);
        }
        free(o->list.ele);
        break;
      case CHORD:
      case LYRIC:
        free(o->str.ptr);
        break;
      default:
        break;
    }
    free(o);
  }
}

void printCsObj(CsObj *o) {
  // printf("type: %d\n", o->type);
  switch (o->type) {
    case SONG:
    case LINE:
    case WORD:
    case LIST:
      printf("Printing %d\n", o->type);
      for (size_t i = 0; i < o->list.len; i++) {
        if (o->list.len == 0) {
          printf("");
        } else {
          printCsObj(o->list.ele[i]);
        }
      }
      if (o->type == LINE) {
        printf("\n");
      }
      break;
    case CHORD:
      printf("Printing %d\n", o->type);
      if (o->str.len == 0) {
        printf("");
      } else {
        printf("[%s](type: %d)", o->str.ptr, o->type);
      }
      break;
    case LYRIC:
      printf("Printing %d\n", o->type);
      if (o->str.len == 0) {
        printf("");
      } else {
        printf("%s(type: %d)", o->str.ptr, o->type);
      }
      break;
    default:
      printf("???\n");
      break;
  }
}

typedef struct precedenceTableEntry {
  char s;
  int i;
} pTableEntry;

typedef struct precedenceTable {
  pTableEntry **pTable;
  int count;
} pTable;

struct CsCtx;
typedef int (*MathFn)(struct CsCtx *ctx, char s);

typedef struct symbolTableEntry {
  char s;
  MathFn f;
} sTableEntry;

typedef struct symbolTable {
  sTableEntry **sTable;
  int count;
} sTable;

typedef struct CsCtx {
  CsObj *stack;
  Token *opStack;
  Token *queue;
  pTable precedenceTable;
  sTable symbolTable;
} CsCtx;

void addSymbolPrecedence(CsCtx *ctx, char s, int i) {
  ctx->precedenceTable.pTable =
      xrealloc(ctx->precedenceTable.pTable,
               sizeof(pTableEntry *) * (ctx->precedenceTable.count + 1));
  pTableEntry *entry = xmalloc(sizeof(pTableEntry));
  entry->s = s;
  entry->i = i;
  ctx->precedenceTable.pTable[ctx->precedenceTable.count] = entry;
  ctx->precedenceTable.count++;
}

void addSymbolFn(CsCtx *ctx, char s, MathFn f) {
  ctx->symbolTable.sTable =
      xrealloc(ctx->symbolTable.sTable,
               sizeof(sTableEntry *) * (ctx->symbolTable.count + 1));
  sTableEntry *entry = xmalloc(sizeof(sTableEntry));
  entry->s = s;
  entry->f = f;
  ctx->symbolTable.sTable[ctx->symbolTable.count] = entry;
  ctx->symbolTable.count++;
}

int getPrecedence(CsCtx *ctx, Token *t) {
  // printf("Token type, symbol: %d, %c\n", t->type, t->symbol);
  char s = t->type;
  for (int i = 0; i < ctx->precedenceTable.count; i++) {
    if (ctx->precedenceTable.pTable[i]->s == s) {
      return ctx->precedenceTable.pTable[i]->i;
    }
  }
  printf("Error: %c\n", s);
  return -1;
}

void ctxEnqueue(CsCtx *ctx, Token *t);

/* Push the token on the context operator stack. It's up the caller to increment
 * the refcount. */
void ctxOpStackPush(CsCtx *ctx, Token *t) {

  while (ctx->opStack->list.len > 0) {
    Token *peek = ctx->opStack->list.ele[ctx->opStack->list.len - 1];

    int precPeek = getPrecedence(ctx, peek);
    int precCurrent = getPrecedence(ctx, t);

    /* ASSOCIATIVITY RULE:
      If current operator is right associative ('~'), we use >
      else (left associative), we use >=
    */
    int shouldPop = 0;
    if (t->type == CHORD_TOKEN) {
      shouldPop = (precPeek > precCurrent);
    } else {
      shouldPop = (precPeek >= precCurrent);
    }

    if (shouldPop) {
      Token *popped = listPop(ctx->opStack);
      ctxEnqueue(ctx, popped);
    } else {
      break;
    }
  }

  listPush(ctx->opStack, t);
}

/* Enqueue the token on the context queue. It's up the caller to increment the
 * refcount. */
void ctxEnqueue(CsCtx *ctx, Token *t) {
  listPush(ctx->queue, t);
};

void ctxPostFixStackPush(CsCtx *ctx, CsObj *o) {
  listPushObj(ctx->stack, o);
}

CsObj *ctxPostFixStackPop(CsCtx *ctx) {
  return listPopObj(ctx->stack);
}

sTableEntry *getFunctionByName(CsCtx *ctx, char s) {
  for (int i = 0; i < ctx->symbolTable.count; i++) {
    if (ctx->symbolTable.sTable[i]->s == s) {
      return ctx->symbolTable.sTable[i];
    }
  }
  return NULL;
}

int ctxCheckStackMinLen(CsCtx *ctx, size_t min) {
  return (ctx->stack->list.len < min) ? SY_ERR : SY_OK;
};

int applyOperator(CsCtx *ctx, char s) {
  if (s == CHORD_TOKEN) {
    if (ctxCheckStackMinLen(ctx, 2)) return SY_ERR;
    CsObj *chord = ctxPostFixStackPop(ctx);
    if (chord == NULL || chord->type != CHORD) {
      printf("Expecting chord obj\n");
      return SY_ERR;
    };
    CsObj *lyric = ctxPostFixStackPop(ctx);
    if (lyric == NULL || lyric->type != LYRIC) {
      printf("Expecting lyric obj\n");
      ctxPostFixStackPush(ctx, chord);
      return SY_ERR;
    }
    CsObj *word = createWordObject();
    listPushObj(word, chord);
    listPushObj(word, lyric);
    ctxPostFixStackPush(ctx, word);
    return SY_OK;
  } else if (s == WORD_MULT) {
    if (ctxCheckStackMinLen(ctx, 2)) return SY_ERR;
    CsObj *word_b = ctxPostFixStackPop(ctx);
    if (word_b == NULL) {
      printf("word_b is null\n");
    }
    if (!(word_b->type == WORD || word_b->type == LINE)) {
      printf("Expecting word_b to be a word or line obj, got: %d\n",
             word_b->type);
      return SY_ERR;
    }
    CsObj *word_a = ctxPostFixStackPop(ctx);
    if (word_a == NULL) {
      printf("word_b is null\n");
    }
    if (!(word_b->type == WORD || word_b->type == LINE)) {
      ctxPostFixStackPush(ctx, word_b);
      printf("Expecting word_a to be a word or line obj, got: %d\n",
             word_b->type);
      return SY_ERR;
    }
    switch (word_a->type) {
      case LINE:
        listPushObj(word_a, word_b);
        ctxPostFixStackPush(ctx, word_a);
        break;

      case WORD: {
        CsObj *line = createLineObject();
        listPushObj(line, word_a);
        listPushObj(line, word_b);
        ctxPostFixStackPush(ctx, line);
        break;
      }

      default:
        printf("?\n");
        break;
    }
    return SY_OK;
  } else if (s == ENDOFLINE) {
    CsObj *line_b = ctxPostFixStackPop(ctx);
    if (line_b == NULL) {
      printf("line_b is null\n");
      return SY_ERR;
    }
    if (!(line_b->type == SONG || line_b->type == LINE ||
          line_b->type == WORD)) {
      ctxPostFixStackPush(ctx, line_b);
      printf("Expecting word_b to be a song or line or word obj, got: %d\n",
             line_b->type);
      return SY_ERR;
    }
    CsObj *line_a = ctxPostFixStackPop(ctx);
    if (line_a == NULL) {
      printf("line_a is null\n");
      return SY_ERR;
    }
    if (!(line_a->type == SONG || line_a->type == LINE ||
          line_a->type == WORD)) {
      ctxPostFixStackPush(ctx, line_b);
      printf("Expecting word_b to be a song or line or word obj, got: %d\n",
             line_b->type);
      return SY_ERR;
    }

    if (line_b->type == WORD) {
      CsObj *line = createLineObject();
      listPushObj(line, line_b);
      line_b = line;
    }
    switch (line_a->type) {
      case SONG:
        listPushObj(line_a, line_b);
        ctxPostFixStackPush(ctx, line_a);
        break;
      case LINE: {
        CsObj *song = createSongObject();
        listPushObj(song, line_a);
        listPushObj(song, line_b);
        ctxPostFixStackPush(ctx, song);
        break;
      }
      case WORD: {
        CsObj *song = createSongObject();
        CsObj *line1 = createLineObject();
        listPushObj(line1, line_a);
        listPushObj(song, line1);
        listPushObj(song, line_b);
        ctxPostFixStackPush(ctx, song);
        break;
      }

      default:
        printf("???\n");
        break;
    }
    return SY_OK;

  } else {
    printf("Operator must be chord, word_mult or endofline: %d\n", s);
    return SY_ERR;
  }
}

void freeContext(CsCtx *ctx) {
  if (ctx->stack) {
    releaseCsObj(ctx->stack);
  }
  if (ctx->queue) {
    releaseToken(ctx->queue);
  }
  if (ctx->opStack) {
    releaseToken(ctx->opStack);
  }
  if (ctx->precedenceTable.pTable) {
    for (int i = 0; i < ctx->precedenceTable.count; i++) {
      free(ctx->precedenceTable.pTable[i]);
    }
    free(ctx->precedenceTable.pTable);
  }
  if (ctx->symbolTable.sTable) {
    for (int i = 0; i < ctx->symbolTable.count; i++) {
      free(ctx->symbolTable.sTable[i]);
    }
    free(ctx->symbolTable.sTable);
  }
  free(ctx);
}


CsCtx *createContext(void) {
  CsCtx *ctx = xmalloc(sizeof(CsCtx));
  ctx->stack = createListObject();
  ctx->opStack = createTokenList();
  ctx->queue = createTokenList();
  ctx->precedenceTable.pTable = NULL;
  ctx->precedenceTable.count = 0;
  addSymbolPrecedence(ctx, CHORD_TOKEN, 3);
  addSymbolPrecedence(ctx, WORD_MULT, 2);
  addSymbolPrecedence(ctx, ENDOFLINE, 1);

  ctx->symbolTable.sTable = NULL;
  ctx->symbolTable.count = 0;
  addSymbolFn(ctx, CHORD_TOKEN, applyOperator);
  addSymbolFn(ctx, WORD_MULT, applyOperator);
  addSymbolFn(ctx, ENDOFLINE, applyOperator);

  return ctx;
}

/* Execute the program stored in a postfix notation in the context queue. */

int evalPostfix(CsCtx *ctx, Token *t) {
  switch (t->type) {
    case LYRIC_TOKEN: {
      CsObj *o = createLyricObject(t->str.ptr, t->str.len);
      ctxPostFixStackPush(ctx, o);
      // retain(o);
      break;
    }
    case CHORD_TOKEN: {
      CsObj *o = createChordObject(t->str.ptr, t->str.len);
      ctxPostFixStackPush(ctx, o);
      sTableEntry *entry = getFunctionByName(ctx, CHORD_TOKEN);
      int result = entry->f(ctx, t->type);
      if (result) return SY_ERR;
      break;
    }
    case LIST_TOKEN:
      for (size_t i = 0; i < t->list.len; i++) {
        evalPostfix(ctx, t->list.ele[i]);
      }
      break;
    case WORD_MULT: {
      sTableEntry *entry = getFunctionByName(ctx, WORD_MULT);
      int result = entry->f(ctx, t->type);
      if (result) return SY_ERR;
      break;
    }
    case ENDOFLINE: {
      sTableEntry *entry = getFunctionByName(ctx, ENDOFLINE);
      int result = entry->f(ctx, t->type);
      if (result) return SY_ERR;
      break;
    }

    default:
      printf("????? - %d\n", t->type);
      break;
  }
  return SY_OK;
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
  Lexer l;
  l.prg = buf;
  l.p = buf;
  readEndOfFile(&l);
  advanceLexer(&l);
  CsCtx *ctx = createContext();

  while (l.curToken->type != ENDOFFILE) {
    switch (l.curToken->type) {
      case LYRIC_TOKEN:
        retainToken(l.curToken); // queue is co-owner
        Token *emptyChord = createEmptyChordToken();
        ctxOpStackPush(ctx, emptyChord);
        ctxEnqueue(ctx, l.curToken);
        advanceLexer(&l);
        if (l.curToken->type == CHORD_TOKEN ||
            l.curToken->type == LYRIC_TOKEN) {
          Token *w = createTokenWordMult();
          ctxOpStackPush(ctx, w);
        }
        break;
      case CHORD_TOKEN:
        retainToken(l.curToken); // opStack is co-owner
        ctxOpStackPush(ctx, l.curToken);
        advanceLexer(&l);
        if (l.curToken->type == LYRIC_TOKEN) {
          retainToken(l.curToken); // opStack is co-owner
          ctxEnqueue(ctx, l.curToken);
          advanceLexer(&l);
        } else {
          Token *emptyLyric = createEmptyLyricToken();
          ctxEnqueue(ctx, emptyLyric);
        }
        if (l.curToken->type == CHORD_TOKEN ||
            l.curToken->type == LYRIC_TOKEN) {
          Token *w = createTokenWordMult();
          ctxOpStackPush(ctx, w);
        }
        break;
      case ENDOFLINE:
        retainToken(l.curToken); // opStack is co-owner
        ctxOpStackPush(ctx, l.curToken);
        advanceLexer(&l);
        break;

      default:
        printf("???%c\n", l.curToken->type);
        break;
    }
  }

  while (ctx->opStack->list.len > 0) {
    Token *popped = listPop(ctx->opStack);
    ctxEnqueue(ctx, popped);
  }
  printToken(ctx->queue);
  evalPostfix(ctx, ctx->queue);
  if(ctx->stack->list.len == 1 && ctx->stack->list.ele[0]->type != SONG){
    switch(ctx->stack->list.ele[0]->type){
      case WORD:{
        CsObj *el = ctxPostFixStackPop(ctx);
        CsObj *song = createSongObject();
        CsObj *line = createLineObject();
        listPushObj(line, el);
        listPushObj(song, line);
        ctxPostFixStackPush(ctx, song);
        break;
      }
      case LINE: {
        CsObj *el = ctxPostFixStackPop(ctx);
        CsObj *song = createSongObject();
        listPushObj(song, el);
        ctxPostFixStackPush(ctx, song);
        break;
      }
      default:
      printf("??\n");
      break;
    }
  }
  printf("Result is: \n");
  printCsObj(ctx->stack);
  freeContext(ctx);
  free(buf);
  return 0;
}
