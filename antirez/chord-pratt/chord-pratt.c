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

typedef struct Lexer {
  char *prg;
  char *p;
  Token curToken;
  int isNextTokenEmptyString;
} Lexer;

typedef struct CpCtx {
  Lexer lexer;
} CpCtx;

int isStringConstant(CpCtx *ctx) {
  char c = ctx->lexer.p[0];
  char symbols_char[] = "[]\n\r";
  return c != 0 && isascii(c) && strchr(symbols_char, c) == NULL;
}

void readLyric(CpCtx *ctx) {
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
    ctx->lexer.isNextTokenEmptyString = 1;
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

void advanceLexer(CpCtx *ctx) {
  if (ctx->lexer.isNextTokenEmptyString) {
    ctx->lexer.curToken.type = TOKEN_LYRIC;
    ctx->lexer.curToken.str.ptr = NULL;
    ctx->lexer.curToken.str.len = 0;
    ctx->lexer.isNextTokenEmptyString = 0;
  } else {
    char c = ctx->lexer.p[0];
    if (isStringConstant(ctx)) {
      readLyric(ctx);
    } else if (isBeginningChord(ctx)) {
      readChord(ctx);
    } else if (c == '\n' || c == '\r') {
      readEndOfLine(ctx, c);
    } else if (c == 0) {
      readEndOfFile(ctx);
    } else {
      readIllegalChar(ctx, c);
    }
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
      printf("Current token is Chord: %s\n", t->str.ptr);
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
    default:
      printf("? - %d\n", t->type);
      break;
  }
}

void initContext(CpCtx *ctx, char *buf) {
  memset(ctx, 0, sizeof(CpCtx));

  ctx->lexer.prg = buf;
  ctx->lexer.p = buf;
  ctx->lexer.isNextTokenEmptyString = 0;

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
